// Separate verification executable. No fixture grants or autoplay are linked into prison_life.
#include "game.h"
#include "broadcast.h"
#include <algorithm>
#include <cstdio>
#include <lam/ecs/world.h>
#include <lam/ecs/transform.h>
#include <lam/input/driver.h>
#include <lam/physics/components.h>
#include <lam/tilemap/path_follow.h>
using namespace lam;
using namespace prison;
namespace {
class Verification:public InputDriver {
    std::string scenario;bool initialized=false,reported=false;int tick=0,stage=0,wait=0,visits=0;PathFinder finder;PathRoute route;Vec2 lastGoal;int routeTick=-100;
    void command(InputFrame &f,int c){f.push_intent({str_id("prison.command"),c,0});}
    bool go(const Ctx &ctx,InputFrame &f,Vec2 goal){auto &g=ctx.local<Game>();auto &tiles=ctx.local<TileWorld>();auto p=ctx.world().get<Transform>(g.player).position;
        if(distance_sq(p,goal)<9*9){f.set_axis(ctx.res<Controls>().move,{});return true;}
        if(tick-routeTick>45||distance_sq(lastGoal,goal)>4*4||route.count==0){finder.begin_tick(1);auto result=finder.find(tiles.cost,tiles.tile_at(p),tiles.tile_at(goal),GridDirs::Eight,route);(void)result;lastGoal=goal;routeTick=tick;}
        auto move=follow_route(route,tiles,p,3);if(distance_sq(p,goal)<22*22&&tile_ray_clear(tiles,p,goal))move=normalize_or(goal-p,{});f.set_axis(ctx.res<Controls>().move,move);return false;
    }
    bool safeDig(const Ctx &ctx){auto &g=ctx.local<Game>();if(!g.escapePatrol)return false;auto p=ctx.world().get<Transform>(g.player).position;
        for(int i=10;i<13;++i){auto s=static_cast<std::size_t>(i);auto at=ctx.world().get<Transform>(g.actors[s]).position;auto face=ctx.world().get<Actor>(g.actors[s]).facing;
            if(distance_sq(at,p)<cfg::officerSight*cfg::officerSight&&dot(normalize_or(p-at,face),face)>=.65f&&tile_ray_clear(ctx.local<TileWorld>(),at,p))return false;

        }return true;
    }
    void next(){++stage;route={};wait=0;std::printf("verification stage=%d tick=%d\n",stage,tick);}
public:
    explicit Verification(std::string s):scenario(std::move(s)){finder.resize(6400);}
    void fill(const Ctx &ctx,std::span<InputFrame,kMaxPlayers> frames) override {
        auto &f=frames[0];f.clear_devices();auto *ptr=ctx.try_local<Game>();if(!ptr)return;auto &g=*ptr;const auto &c=ctx.res<Controls>();++tick;
        if(!initialized){initialized=true;g.scripted=true;
            if(scenario=="dealer"){g.phase=Phase::Play;g.model.stats.money=180;placeActor(ctx.world(),g.player,{1120,816});g.wasWing=false;}
            if(scenario=="food"){g.phase=Phase::Play;placeActor(ctx.world(),g.player,{136,568});g.wasWing=false;}
            if(scenario=="library"){g.phase=Phase::Play;placeActor(ctx.world(),g.player,{920,624});g.wasWing=false;}
            if(scenario=="library-furniture"){g.phase=Phase::Play;placeActor(ctx.world(),g.player,{1040,680});g.wasWing=false;}
            if(scenario=="elevenlabs"||scenario=="elevenlabs-live"){g.phase=Phase::Play;g.menu=Menu::Audio;g.uiDirty=true;}
            if(scenario=="yard"){g.phase=Phase::Play;placeActor(ctx.world(),g.player,{640,1040});g.wasWing=false;}
            if(scenario=="combat"){g.phase=Phase::Play;placeActor(ctx.world(),g.player,{912,1064});g.wasWing=false;g.menu=Menu::ConfirmRing;command(f,43);}
            if(scenario=="release"){g.phase=Phase::Play;g.model.clock=1321;g.model.daysLeft=1;g.model.stats.hp=6;placeActor(ctx.world(),g.player,{152,152});}
            if(scenario=="escape"){g.phase=Phase::Play;g.model.bag.add(Item::LoanShovel);g.model.loan=true;placeActor(ctx.world(),g.player,g.map.cells[static_cast<std::size_t>(g.model.hotspot)].hotspot);}
            if(scenario=="daily"||scenario=="roam")g.model.stats.reputation=12;
        }
        if(g.phase==Phase::Released||g.phase==Phase::Escaped){if(!reported){std::printf("verification ending=%d tick=%d hp=%d sentence=%d\n",int(g.phase),tick,g.model.stats.hp,g.model.daysLeft);reported=true;}return;}
        if(scenario=="court"||scenario=="escort")return;
        if(g.phase!=Phase::Play){if(g.phase==Phase::Court||g.phase==Phase::Escort)f.press(c.confirm);return;}
        if(scenario=="elevenlabs"||scenario=="elevenlabs-live"){
            if(tick==3)command(f,scenario=="elevenlabs-live"?71:73);
            auto &audio=ctx.local<Broadcast>();if(scenario=="elevenlabs-live"&&audio.liveHttp&&!reported){std::printf("verification elevenlabs http=%d busy=%d requests=%d\n",audio.liveHttp,audio.busy,audio.liveRequests);reported=true;}
            return;
        }
        if(scenario=="dealer"||scenario=="food"){if(tick==3)f.press(c.interact);return;}
        if(scenario=="library"){if(tick==3)f.press(c.interact);return;}
        if(scenario=="yard")return;
        if(scenario=="release"){if(tick==3)f.press(c.interact);return;}
        if(scenario=="escape"){
            if(tick%1800==0)std::printf("shovel progress=%d activity=%d release=%d menu=%d steps=%d/%d/%d waits=%.1f/%.1f/%.1f\n",g.model.digSteps,int(g.activity),g.digNeedsRelease,int(g.menu),g.patrolStep[0],g.patrolStep[1],g.patrolStep[2],g.brains[10].wait,g.brains[11].wait,g.brains[12].wait);
            // Reactive verification bot: release immediately when watched; partial holds are lost.
            if(g.digNeedsRelease)return;
            if(g.activity==Activity::Dig){if(safeDig(ctx))f.hold(c.interact);return;}
            bool safe=safeDig(ctx);
            if(tick==3||safe){f.press(c.interact);f.hold(c.interact);}return;
        }
        if(scenario=="combat"){
            if(g.ringSlot>=0){auto at=ctx.world().get<Transform>(g.actors[static_cast<std::size_t>(g.ringSlot)]).position;auto p=ctx.world().get<Transform>(g.player).position;if(distance_sq(p,at)>27*27)go(ctx,f,at);else {g.face=normalize_or(at-p,{1,0});if(g.attackCooldown<=0)f.press(c.attack);}}
            if(g.ringRewarded&&!reported){std::printf("verification bout reward=%d money=%d hp=%d tick=%d\n",g.ringReward,g.model.stats.money,g.model.stats.hp,tick);reported=true;}return;
        }
        if(scenario=="escape-route"){
            if(g.digNeedsRelease)return;
            if(g.activity==Activity::Dig){if(safeDig(ctx))f.hold(c.interact);return;}
            if(g.activity!=Activity::None||g.overview>0)return;
            if(g.menu!=Menu::None){command(f,1);return;}
            if(stage>0&&!g.model.bag.has(Item::LoanShovel)){stage=0;route={};}
            if(stage==0){if(go(ctx,f,{560,1168})){if(g.model.bag.has(Item::LoanShovel))next();else if(tick%15==0)f.press(c.interact);}}
            else if(stage==1){if(go(ctx,f,{56,928}))next();}
            else if(stage==2){if(go(ctx,f,{56,464}))next();}
            else if(stage==3){if(go(ctx,f,{520,464}))next();}
            else if(stage==4){bool clear=true;for(int i=10;i<13;++i){auto e=g.actors[static_cast<std::size_t>(i)];auto at=ctx.world().get<Transform>(e).position;auto velocity=ctx.world().get<Velocity>(e).linear;for(auto point:std::array<Vec2,2>{{{640,464},{640,400}}})for(float future:{0.f,2.f})if(distance_sq(at+velocity*future,point)<240*240&&tile_ray_clear(ctx.local<TileWorld>(),at+velocity*future,point))clear=false;}if(clear)next();}
            else if(stage==5){if(go(ctx,f,{640,400}))next();}
            else if(stage==6){auto hotspot=g.map.cells[static_cast<std::size_t>(g.model.hotspot)].hotspot;if(go(ctx,f,hotspot)){bool safe=safeDig(ctx);if(safe){f.press(c.interact);f.hold(c.interact);}}}
            return;
        }
        if(g.menu!=Menu::None){command(f,1);return;}
        if(g.activity!=Activity::None||g.overview>0)return;
        if(scenario=="roam"){
            constexpr std::array<Vec2,8> goals{{{640,392},{640,672},{136,568},{920,624},{1120,816},{640,1040},{912,1064},{640,464}}};
            if(stage<8&&go(ctx,f,goals[static_cast<std::size_t>(stage)])){visits|=1<<stage;next();}if(stage==8&&!reported){std::printf("verification roam visited=%d tick=%d\n",visits,tick);reported=true;}return;
        }
        if(scenario=="daily"){
            auto action=[&](Vec2 goal){if(go(ctx,f,goal)&&tick%15==0)f.press(c.interact);};
            if(stage==0){if(go(ctx,f,{136,568})){next();f.press(c.interact);}}
            else if(stage==1){if(g.model.bag.has(Item::Rice))next();else action({136,568});}
            else if(stage==2){if(go(ctx,f,{288,608})){f.press(c.interact);next();}}
            else if(stage==3){if(go(ctx,f,{608,720})){f.press(c.interact);next();}}
            else if(stage==4){if(go(ctx,f,{744,792})){f.press(c.interact);next();}}
            else if(stage==5){if(go(ctx,f,{920,624})){f.press(c.interact);next();}}
            else if(stage==6){if(tick%15==0){f.press(c.interact);next();}}
            else if(stage==7){if(g.model.books>0)next();else action({920,624});}
            else if(stage==8){if(go(ctx,f,{136,816})){f.press(c.interact);next();}}
            else if(stage==9){if(g.model.jobs[2])next();else action({136,816});}
            else if(stage==10){if(go(ctx,f,{560,1168})){f.press(c.interact);next();}}
            else if(stage==11){if(go(ctx,f,{752,1152})){f.press(c.interact);next();}}
            else if(stage==12){if(go(ctx,f,{560,1168})){f.press(c.interact);next();}}
            else if(stage==13&&!reported){std::printf("verification daily hp=%d money=%d sanity=%f intelligence=%f books=%d sentence=%d jobs=%d%d%d%d tick=%d\n",g.model.stats.hp,g.model.stats.money,g.model.stats.sanity,g.model.stats.intelligence,g.model.books,g.model.daysLeft,g.model.jobs[0],g.model.jobs[1],g.model.jobs[2],g.model.jobs[3],tick);reported=true;}
        }
    }
};
}
void installVerification(App &app){auto scenario=std::string(app.options().value("--scenario").value_or("roam"));static Verification driver(scenario);app.res<InputSystem>().set_driver(&driver);}
