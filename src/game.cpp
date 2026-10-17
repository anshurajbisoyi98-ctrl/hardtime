#include "game.h"
#include "broadcast.h"
#include <algorithm>
#include <cmath>
#include <lam/ecs/world.h>
#include <lam/ecs/transform.h>
#include <lam/input/actions.h>
#include <lam/input/keys.h>
#include <lam/physics/components.h>
#include <lam/physics/motors.h>
#include <lam/render/camera.h>
#include <lam/tilemap/path_follow.h>
#include <lam/audio/components.h>
namespace prison {
using namespace lam;
void notify(Game &g,std::string s,float seconds){g.notice=std::move(s);g.noticeTime=seconds;}
void openMenu(Game &g,Menu m){g.menu=m;g.uiDirty=true;g.activity=Activity::None;g.progress=0;}
void setDoor(World &w,Game &g,bool closed){g.doorClosed=closed;auto &t=w.resources().get<TileWorld>();for(int x=9;x<12;++x)t.set({x,12},closed?TileShape::Solid:TileShape::Empty,closed?kTileOpaque:0,closed?0:10);}
static Entity actor(World &w,Vec2 p,int slot,bool officer){
    auto e=w.create();w.add<Transform>(e,Transform{.position=p});w.add<Actor>(e,Actor{.slot=slot,.officer=officer});
    w.add<Collider>(e,Collider{.half={5,5},.layer=2,.mask=1});
    w.add<TopDownMotor>(e,TopDownMotor{.max_speed=slot<0?cfg::walk:officer?cfg::officerSpeed:cfg::npcSpeed,.accel=1800,.decel=2200,.corner_assist=2,.solid_mask=1});
    return e;
}
void placeActor(World &w,Entity e,Vec2 p){w.get<Transform>(e).position=p;if(auto *prev=w.try_get<PrevTransform>(e))prev->position=p;w.get<Velocity>(e).linear={};w.get<TopDownMotor>(e).move={};}
void setup(Ctx &ctx){
    auto &w=ctx.world();auto &g=w.resources().insert<Game>();
    g.scripted=ctx.app().options().scripted();auto *paths=ctx.try_res<Paths>();g.saveDir=std::string(ctx.app().options().value("--save-dir").value_or(paths?paths->saveDir:"saves"));
    newModel(g.model,ctx.app().options().seed?ctx.app().options().seed:7319);
    g.map.build(w.resources().get<TileWorld>());g.paths.resize(6400);
    g.player=actor(w,playerHome,-1,false);
    for(int i=0;i<10;++i){Vec2 at=i==9?dealerSpot:g.map.cells[static_cast<std::size_t>(i+1)].door;g.actors[static_cast<std::size_t>(i)]=actor(w,at,i,false);}
    g.actors[10]=actor(w,{640,1176},0,true);g.actors[11]=actor(w,{640,608},1,true);g.actors[12]=actor(w,{640,904},2,true);
    g.camera=w.create();w.add<Camera2D>(g.camera,Camera2D{.position=playerHome,.zoom=2});
    w.add<CameraFollow>(g.camera,CameraFollow{.target=g.player,.half_life=.1f});
    w.add<CameraBounds>(g.camera,CameraBounds{.world={32,32,1216,1216},.enabled=true});
    g.checkpoint=g.model;
    if(ctx.app().options().has("--continue")&&loadCheckpoint(g)){restoreCheckpoint(ctx);notify(g,"Day-start checkpoint restored.");}
    if(ctx.app().options().has("--skip-intro"))enterPlay(ctx,!g.scripted);
    if(ctx.app().options().has("--overview"))openMenu(g,Menu::Map);
    setupBroadcast(ctx);
}
void enterPlay(Ctx &ctx,bool save){auto &g=ctx.local<Game>();g.phase=Phase::Play;g.story=0;g.grace=5;g.menu=Menu::None;g.uiDirty=true;placeActor(ctx.world(),g.player,playerHome);setDoor(ctx.world(),g,false);placeActor(ctx.world(),g.actors[10],{640,1176});g.wasWing=true;if(save){g.checkpoint=g.model;g.saved=saveCheckpoint(g);}notify(g,"Cell A-01. E opens doors and interactions. M shows the compound.",7);}
void restoreCheckpoint(Ctx &ctx){auto &g=ctx.local<Game>();g.model=g.checkpoint;g.phase=Phase::Play;g.menu=Menu::None;g.activity=Activity::None;g.uiDirty=true;g.ringSlot=-1;g.ringReward=0;g.ringRewarded=false;g.incident=false;g.curfewIncident=false;g.curfewGrace=0;g.warned=false;g.invuln=1;g.grace=5;g.attackCooldown=0;g.attackVisual=0;g.story=0;g.escortReturning=false;g.escortRoute={};g.brains={};g.shovelPressure=false;g.escapePatrol=false;g.digNeedsRelease=false;g.waveTimer=4;g.waveAge=0;g.waveNumber=0;g.patrolStep={};g.progress=0;g.wasWing=true;g.wasInRing=false;g.overview=0;setDoor(ctx.world(),g,false);placeActor(ctx.world(),g.player,playerHome);for(int i=0;i<10;++i)placeActor(ctx.world(),g.actors[static_cast<std::size_t>(i)],i==9?dealerSpot:g.map.cells[static_cast<std::size_t>(i+1)].door);placeActor(ctx.world(),g.actors[10],{640,1176});placeActor(ctx.world(),g.actors[11],{640,608});placeActor(ctx.world(),g.actors[12],{640,904});for(auto [e,a]:ctx.world().view<Actor>()){a.flash=0;a.facing={0,1};stopActor(ctx.world(),e);}notify(g,"Day-start restored. Rewards and progress rolled back.");}
void newGame(Ctx &ctx){auto &g=ctx.local<Game>();newModel(g.model,g.model.seed);g.checkpoint=g.model;restoreCheckpoint(ctx);g.phase=Phase::Court;g.story=0;g.saved=false;g.uiDirty=true;}
void stopActor(World &w,Entity e){w.get<TopDownMotor>(e).move={};w.get<Velocity>(e).linear={};}
void drive(Ctx &ctx){
    auto &g=ctx.local<Game>();auto &w=ctx.world();const auto &c=ctx.res<Controls>();const auto &in=input_frame(ctx);
    processCommands(ctx);
    if(!in.is_held(c.interact))g.digNeedsRelease=false;
    if(g.phase==Phase::Court||g.phase==Phase::Escort){if(in.is_pressed(c.confirm)||in.is_pressed(c.interact)||in.is_pressed(c.pause))enterPlay(ctx,!g.scripted);if(g.phase==Phase::Court)stopActor(w,g.player);return;}
    if(g.phase!=Phase::Play){stopActor(w,g.player);return;}
    if(in.is_pressed(c.pause))openMenu(g,g.menu==Menu::None?Menu::Pause:Menu::None);
    if(in.is_pressed(c.map))openMenu(g,g.menu==Menu::Map?Menu::None:Menu::Map);
    if(in.is_pressed(c.audio))openMenu(g,g.menu==Menu::Audio?Menu::None:Menu::Audio);
    if(g.menu!=Menu::None||g.overview>0){stopActor(w,g.player);for(auto e:g.actors)stopActor(w,e);return;}
    Vec2 move=clamp_length(in.axis2(c.move),1.f);g.running=length_sq(move)>.01f&&in.is_held(c.run)&&!g.model.exhausted&&g.model.stats.agility>0;
    auto &motor=w.get<TopDownMotor>(g.player);motor.move=move;motor.max_speed=g.running?cfg::run:cfg::walk;
    if(length_sq(move)>.01f){g.face=normalize_or(move,{0,1});w.get<Actor>(g.player).facing=g.face;}
    for(int i=0;i<3;++i)if(in.is_pressed(c.slots[static_cast<std::size_t>(i)]))g.model.bag.selected=i;
    if(in.is_pressed(c.drop)&&g.model.bag.held()!=Item::None)openMenu(g,Menu::Drop);
    else if(in.is_pressed(c.interact)&&g.menu==Menu::None)interact(ctx);
    else if(in.is_pressed(c.attack)&&g.menu==Menu::None){
        if(g.activity!=Activity::None){if(g.activity==Activity::Phone)stopVoice(ctx);g.activity=Activity::None;g.progress=0;notify(g,"Action interrupted.");}
        else {
            Item held=g.model.bag.held();
            if(held==Item::None||held==Item::Knife||held==Item::Bat)playerAttack(ctx);
            else notify(g,consume(g.model).message);
        }
    }
    if(g.menu!=Menu::None){stopActor(w,g.player);for(auto e:g.actors)stopActor(w,e);}
}
void simulate(Ctx &ctx){auto &g=ctx.local<Game>();const float dt=ctx.time().tick_dt;
    if(g.menu!=Menu::None)return;
    if(g.overview>0){g.overview=std::max(0.f,g.overview-dt);return;}
    if(g.phase==Phase::Court){if(g.story<4.5f&&g.story+dt>=4.5f)ctx.emit(PlaySound{.group=str_id("gavel"),.volume=.6f});g.story+=dt;if(g.story>=9){g.phase=Phase::Escort;g.story=0;g.escortStep=0;g.escortRoute={};g.uiDirty=true;placeActor(ctx.world(),g.player,{640,1176});placeActor(ctx.world(),g.actors[10],{640,1152});}return;}
    if(g.phase==Phase::Escort){tickEscort(ctx);return;}
    if(g.phase!=Phase::Play){if(g.phase==Phase::Escaped||g.phase==Phase::Released){if(g.story<2.5f&&g.story+dt>=2.5f)g.uiDirty=true;g.story+=dt;}stopActor(ctx.world(),g.player);for(auto e:g.actors)stopActor(ctx.world(),e);return;}
    g.active+=dt;g.grace=std::max(0.f,g.grace-dt);g.noticeTime=std::max(0.f,g.noticeTime-dt);
    g.invuln=std::max(0.f,g.invuln-dt);g.attackCooldown=std::max(0.f,g.attackCooldown-dt);g.attackVisual=std::max(0.f,g.attackVisual-dt);
    bool movingRun=g.running&&length_sq(ctx.world().get<Velocity>(g.player).linear)>1;
    updateShovelThreat(ctx);
    advanceVitals(g.model,dt,movingRun,g.shovelPressure&&!g.escapePatrol?cfg::shovelRespawnRate:1.f);
    if(advanceClock(g.model,dt))morning(ctx);
    tickActivity(ctx);if(g.phase!=Phase::Play)return;tickPeople(ctx);
    int t=timeOfDay(g.model.clock);
    if(t>=1290&&t<1320&&!g.warned){g.warned=true;notify(g,"21:30. Curfew in thirty minutes. Head back to your cell.",8);}
    if(curfew(g.model.clock)){
        auto p=ctx.world().get<Transform>(g.player).position;
        bool home=g.map.cells[0].box.contains(p);
        if(!home&&!g.model.curfewResolved&&!g.escapePatrol){g.curfewGrace+=dt*20;if(g.curfewGrace>=20){bool before=g.incident;flagIncident(ctx,Item::None,"Officer: Curfew. Return to your assigned cell immediately!");if(!before&&g.incident)g.curfewIncident=true;}}
        else if(home&&!g.incident)g.curfewGrace=0;
    }
    if(g.model.daysLeft<=0){g.phase=Phase::Released;g.story=0;g.uiDirty=true;}
    if(g.model.stats.hp<=0){g.phase=Phase::Infirmary;g.uiDirty=true;}
}
void afterPhysics(Ctx &ctx){auto &g=ctx.local<Game>();if(g.phase!=Phase::Play||g.menu!=Menu::None)return;auto p=ctx.world().get<Transform>(g.player).position;if(g.wasWing&&p.y>=448){g.overview=1;g.grace=5;notify(g,"BLACKWATER / Compound overview",3);}g.wasWing=p.y<448;
    bool inRing=ringBounds.contains(p);if(g.ringSlot>=0&&!inRing)cancelBout(ctx);
    if(inRing&&!g.wasInRing&&g.ringSlot<0&&g.model.ringCooldown<=0&&!curfew(g.model.clock))openMenu(g,Menu::Ring);
    g.wasInRing=inRing;updateFocus(ctx);
}
void install(App &app){
    app.register_component<Actor>();auto &m=app.res<ActionMap>();auto key=[](Key k){return ActionMap::key(k);};
    Controls c;c.move=m.axis2("move",{{key(Key::W),key(Key::S),key(Key::A),key(Key::D)},{key(Key::Up),key(Key::Down),key(Key::Left),key(Key::Right)}});
    c.run=m.button("run",{key(Key::LShift),key(Key::RShift)});c.interact=m.button("interact",{key(Key::E)});c.attack=m.button("attack",{key(Key::Space)});c.map=m.button("map",{key(Key::M)});c.pause=m.button("pause",{key(Key::Escape)});c.drop=m.button("drop",{key(Key::G)});c.confirm=m.button("confirm",{key(Key::Enter)});c.slots={m.button("slot1",{key(Key::Num1)}),m.button("slot2",{key(Key::Num2)}),m.button("slot3",{key(Key::Num3)})};
    c.audio=m.button("audio",{key(Key::V)});app.res<Audio>().add_bus("elevenlabs");
    app.insert_resource<Controls>(c);app.add_startup(setup,"prison.setup");app.add_system(Stage::Input,drive,"prison.input");app.add_system(Stage::Simulate,simulate,"prison.simulate");app.add_system(Stage::PostPhysics,afterPhysics,"prison.after_physics");
    app.add_system(Stage::Update,updateBroadcast,"prison.broadcast",-750);
}
}
