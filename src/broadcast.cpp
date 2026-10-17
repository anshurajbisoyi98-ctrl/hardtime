#include "broadcast.h"
#include "game.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <lam/core/data.h>
#include <lam/core/fs.h>
#include <lam/ecs/world.h>
#if defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif
namespace prison {
using namespace lam;
namespace fs=std::filesystem;
Broadcast::~Broadcast(){
#if defined(__APPLE__) || defined(__linux__)
    if(child>0){kill(child,SIGTERM);waitpid(child,nullptr,0);}
#endif
}
void stopVoice(Ctx &ctx){auto &b=ctx.local<Broadcast>();ctx.res<Audio>().stop(b.voice);b.voice={};b.remaining=0;b.subtitle.clear();b.speaker.clear();b.pending.clear();}
void queueVoice(Ctx &ctx,std::string_view id,bool interrupt){auto &b=ctx.local<Broadcast>();if(!b.enabled)return;
    auto it=std::find_if(b.lines.begin(),b.lines.end(),[&](const auto &line){return line.id==id;});if(it==b.lines.end())return;
    if(interrupt)stopVoice(ctx);
    if(b.pending.size()<4)b.pending.push_back(*it);
}
float voiceSeconds(Ctx &ctx,std::string_view id){for(auto &line:ctx.local<Broadcast>().lines)if(line.id==id)return line.seconds;return 3;}
void setupBroadcast(Ctx &ctx){
    auto &b=ctx.world().resources().insert<Broadcast>();auto &g=ctx.local<Game>();auto &assets=ctx.res<AssetServer>();
    b.bus=ctx.res<Audio>().bus("elevenlabs");
    auto folder=fs::absolute(g.saveDir).parent_path();auto key=folder/".env.local";
    if(!fs::is_regular_file(key))key=folder.parent_path()/".env.local";b.keyFile=key.string();
    b.keyFound=fs::is_regular_file(b.keyFile)||(std::getenv("ELEVENLABS_API_KEY")&&*std::getenv("ELEVENLABS_API_KEY"));
    b.helper=assets.resolve_path("elevenlabs/client.py");b.cacheDir=(fs::absolute(g.saveDir)/"elevenlabs-cache").string();
    auto text=read_file(assets.resolve_path("elevenlabs/lines.json"));
    if(text){auto doc=DataDocument::parse(*text,"ElevenLabs lines");if(doc){auto r=doc->root();auto lines=r.array("lines");
        for(std::size_t i=0;i<lines.size();++i){auto row=lines.object(i);SpokenLine line;line.id=row.string("id");line.speaker=row.string("speaker");line.text=row.string("text");
            auto clip=assets.load_now<SoundClip>("sfx/elevenlabs/"+line.id+".wav");
            if(clip){line.clip=*clip;line.seconds=assets.get(*clip)->seconds;b.lines.push_back(std::move(line));}
        }r.ignore_unknown();auto valid=doc->finish();if(!valid)b.lines.clear();
    }}
    b.ambience=assets.load<MusicTrack>("sfx/elevenlabs/corridor.wav");
    if(ctx.app().options().has("--audio-showcase")){if(g.phase==Phase::Court)enterPlay(ctx,false);openMenu(g,Menu::Audio);}
}
void toggleBroadcast(Ctx &ctx){auto &b=ctx.local<Broadcast>();b.enabled=!b.enabled;stopVoice(ctx);ctx.res<Audio>().set_bus_volume(b.bus,b.enabled?1.f:0.f);ctx.local<Game>().uiDirty=true;}
void startLiveRollCall(Ctx &ctx){
    auto &b=ctx.local<Broadcast>();auto &g=ctx.local<Game>();if(b.busy)return;
    if(ctx.app().options().scripted()&&!ctx.app().options().has("--allow-live-audio")){b.liveStatus="Live API calls are disabled in automated tests.";return;}
    if(!b.keyFound){b.liveStatus="Add the local API key to enable live generation.";return;}
    if(!b.enabled){b.liveStatus="Turn audio on before generating a live roll call.";return;}
#if defined(__APPLE__) || defined(__linux__)
    std::error_code ec;fs::create_directories(b.cacheDir,ec);if(ec){b.liveStatus="Cannot write the audio cache. Offline audio is available.";return;}
    auto nonce=std::chrono::steady_clock::now().time_since_epoch().count();b.resultFile=(fs::path(b.cacheDir)/("live-"+std::to_string(nonce)+".json")).string();
    std::vector<std::string> args{"/usr/bin/python3",b.helper,"--key-file",b.keyFile,"--result",b.resultFile,"--day",std::to_string(g.model.day),"--sentence",std::to_string(g.model.daysLeft)};
    std::vector<char*> argv;for(auto &s:args)argv.push_back(s.data());argv.push_back(nullptr);
    posix_spawn_file_actions_t actions;posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions,STDOUT_FILENO,"/dev/null",O_WRONLY,0600);
    posix_spawn_file_actions_addopen(&actions,STDERR_FILENO,"/dev/null",O_WRONLY,0600);
    pid_t child=-1;int result=posix_spawn(&child,args[0].c_str(),&actions,nullptr,argv.data(),environ);posix_spawn_file_actions_destroy(&actions);
    if(result!=0){b.liveStatus="Python audio helper unavailable. Offline recordings work.";return;}
    b.child=child;b.busy=true;b.liveHttp=0;b.liveCredits=-1;++b.liveRequests;b.liveStarted=std::chrono::steady_clock::now();
    b.liveStatus="Generating a personalised roll call with ElevenLabs...";g.uiDirty=true;
#else
    b.liveStatus="Live generation is available on this macOS showcase build.";
#endif
}
static void finishLive(Ctx &ctx){auto &b=ctx.local<Broadcast>();auto &assets=ctx.res<AssetServer>();
    b.busy=false;ctx.local<Game>().uiDirty=true;
    auto text=read_file(b.resultFile);if(!text){b.liveStatus="Live helper did not complete. Offline recordings work.";return;}
    auto doc=DataDocument::parse(*text,"ElevenLabs response");if(!doc){b.liveStatus="Invalid live response. Offline recordings work.";return;}
    auto r=doc->root();bool ok=r.boolean("ok");b.liveHttp=r.integer_or("http_status",0,0,599);b.liveStatus=r.text_or("message","Live response received.");
    b.liveCredits=r.number_or("credits",-1,-1,1000000);
    if(ok){
        auto file=r.string("file");auto subtitle=r.string("text");
        std::error_code ec;auto parent=fs::weakly_canonical(fs::path(file).parent_path(),ec);
        if(!ec&&parent==fs::weakly_canonical(b.cacheDir,ec)){
            auto clip=assets.load_now<SoundClip>(file);
            if(clip){stopVoice(ctx);b.pending.push_back({"live","LIVE ELEVENLABS ROLL CALL",subtitle,*clip,assets.get(*clip)->seconds});}
            else b.liveStatus="API succeeded; playback failed. Offline recordings work.";
        }else b.liveStatus="Rejected an unexpected audio file location.";
    }
    r.ignore_unknown();auto valid=doc->finish();if(!valid)b.liveStatus="Invalid live response. Offline recordings work.";
}
void updateBroadcast(Ctx &ctx){auto *ptr=ctx.try_local<Broadcast>();if(!ptr)return;auto &b=*ptr;auto &g=ctx.local<Game>();auto &audio=ctx.res<Audio>();
#if defined(__APPLE__) || defined(__linux__)
    if(b.child>0){int status=0;pid_t result=waitpid(b.child,&status,WNOHANG);
        if(result==b.child){b.child=-1;finishLive(ctx);}
        else if(result<0){b.child=-1;b.busy=false;b.liveStatus="Live request ended unexpectedly. Offline audio works.";g.uiDirty=true;}
        else if(std::chrono::steady_clock::now()-b.liveStarted>std::chrono::seconds(60)){kill(b.child,SIGTERM);waitpid(b.child,nullptr,0);b.child=-1;b.busy=false;b.liveStatus="Live request timed out. Offline recordings work.";g.uiDirty=true;}
    }
#endif
    if(b.lastPhase!=static_cast<int>(g.phase)){
        b.lastPhase=static_cast<int>(g.phase);
        if(g.phase==Phase::Court){stopVoice(ctx);b.verdictSpoken=false;b.lastMeal=-1;b.curfewDay=-1;}
        else if(g.phase==Phase::Play)queueVoice(ctx,"welcome");
        else if(g.phase==Phase::Released)queueVoice(ctx,"released",true);
        else if(g.phase==Phase::Escaped)queueVoice(ctx,"escaped",true);
        else if(g.phase==Phase::Infirmary)stopVoice(ctx);
    }
    if(g.phase==Phase::Court&&g.story>=4.5f&&!b.verdictSpoken){queueVoice(ctx,"verdict");b.verdictSpoken=true;}
    if(g.phase==Phase::Play){
        if(g.warned&&b.curfewDay!=g.model.day){queueVoice(ctx,"curfew");b.curfewDay=g.model.day;}
        int meal=mealWindow(g.model.clock),stamp=g.model.day*4+meal;
        if(g.menu==Menu::None&&meal>=0&&stamp!=b.lastMeal){queueVoice(ctx,"meal");b.lastMeal=stamp;}
    }
    b.remaining=std::max(0.f,b.remaining-ctx.time().frame_dt);
    if(b.remaining<=0&&!b.subtitle.empty()){audio.stop(b.voice);b.voice={};b.subtitle.clear();b.speaker.clear();++b.completedLines;}
    if(b.enabled&&b.remaining<=0&&!b.pending.empty()){
        auto line=b.pending.front();b.pending.erase(b.pending.begin());
        b.voice=audio.play(line.clip,PlayParams{.volume=.9f,.bus=b.bus});
        if(b.voice){b.subtitle=line.text;b.speaker=line.speaker;b.remaining=line.seconds;}
    }
    float ambient=b.enabled&&g.phase==Phase::Play?(g.menu==Menu::None?.10f:.035f):0;
    if(b.remaining>0)ambient*=.35f;
    audio.set_ambience(b.ambience,ambient,.4f);
}
}
