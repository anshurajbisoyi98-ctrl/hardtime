#include "game.h"
#include "broadcast.h"
#include <cstdio>
#include <filesystem>
#include <lam/core/app.h>
#include <lam/core/default_plugins.h>
#include <lam/ecs/world.h>
#include <lam/audio/audio.h>
#ifdef PRISON_VERIFICATION
void installVerification(lam::App &app);
#endif
int main(int argc,char **argv){
    lam::App app({.title="Prison Life | Blackwater",.window_size={1200,750},.virtual_size={960,600},.seed=7319,.settings_file=""},argc,argv);
    app.add_plugins(lam::DefaultPlugins{.render={.scale_mode=lam::ScaleMode::Fit}});
    auto folder=std::filesystem::weakly_canonical(argv[0]).parent_path();if(folder.filename()=="MacOS")folder=folder.parent_path().parent_path().parent_path();
    app.insert_resource<prison::Paths>(prison::Paths{(folder/"saves").string()});prison::install(app);
    app.res<lam::Audio>().add_bank(app.res<lam::AssetServer>().load<lam::SoundBank>("sfx/prison.sounds.json"));
    app.add_system(lam::Stage::Update,prison::updateUI,"prison.ui",-700);
    app.add_system(lam::Stage::Render,prison::drawWorld,"prison.world");
    app.add_system(lam::Stage::RenderUI,prison::drawHUD,"prison.hud",-1100);
#ifdef PRISON_VERIFICATION
    installVerification(app);
#endif
    const int result=app.run();
    if(auto *b=app.main_world().resources().try_get<prison::Broadcast>())std::printf("prison_audio: voices=%zu live_requests=%d http=%d completed_lines=%d\n",b->lines.size(),b->liveRequests,b->liveHttp,b->completedLines);
    if(auto *g=app.main_world().resources().try_get<prison::Game>()){int inmates=0,officers=0,players=0;for(auto [e,a]:app.main_world().view<prison::Actor>()){if(a.officer)++officers;else if(a.slot>=0)++inmates;else ++players;}std::printf("prison_life: cells=%zu inmates=%d officers=%d players=%d hp=%d money=%d day=%d sentence=%d phase=%d\n",g->map.cells.size(),inmates,officers,players,g->model.stats.hp,g->model.stats.money,g->model.day,g->model.daysLeft,static_cast<int>(g->phase));}
    return result;
}
