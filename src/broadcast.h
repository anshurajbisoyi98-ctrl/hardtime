#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <lam/assets/asset_server.h>
#include <lam/audio/audio.h>
#include <lam/core/app.h>
namespace prison {
struct SpokenLine {
    std::string id,speaker,text;
    lam::AssetId<lam::SoundClip> clip;
    float seconds=0;
};
struct Broadcast {
    std::vector<SpokenLine> lines,pending;
    std::string subtitle,speaker,keyFile,helper,cacheDir,resultFile;
    std::string liveStatus="No live request this session. Offline recordings are ready.";
    lam::VoiceHandle voice;
    lam::AssetId<lam::MusicTrack> ambience;
    lam::BusId bus=0;
    float remaining=0;
    bool enabled=true,keyFound=false,busy=false,verdictSpoken=false;
    int child=-1,lastPhase=-1,lastMeal=-1,curfewDay=-1,liveHttp=0,liveRequests=0,completedLines=0;
    double liveCredits=-1;
    std::chrono::steady_clock::time_point liveStarted;
    ~Broadcast();
};
void setupBroadcast(lam::Ctx &ctx);
void updateBroadcast(lam::Ctx &ctx);
void queueVoice(lam::Ctx &ctx,std::string_view id,bool interrupt=false);
void stopVoice(lam::Ctx &ctx);
float voiceSeconds(lam::Ctx &ctx,std::string_view id);
void startLiveRollCall(lam::Ctx &ctx);
void toggleBroadcast(lam::Ctx &ctx);
}
