#include "game.h"
#include "broadcast.h"
#include <algorithm>
#include <cmath>
#include <lam/ecs/transform.h>
#include <lam/ecs/world.h>
#include <lam/physics/components.h>
#include <lam/physics/motors.h>
#include <lam/tilemap/path_follow.h>
#include <lam/audio/components.h>
namespace prison {
using namespace lam;
static Vec2 position(World &w,Entity e){return w.get<Transform>(e).position;}
static void steer(Ctx &ctx,Entity e,Brain &b,Vec2 goal,float speed){
    auto &g=ctx.local<Game>();auto &w=ctx.world();auto &tiles=ctx.local<TileWorld>();auto p=position(w,e);
    auto &motor=w.get<TopDownMotor>(e);motor.max_speed=speed;
    b.repath-=ctx.time().tick_dt;
    if(b.repath<=0||b.route.count==0||b.route.goal!=tiles.tile_at(goal)){
        auto result=g.paths.find(tiles.cost,tiles.tile_at(p),tiles.tile_at(goal),GridDirs::Eight,b.route);
        (void)result;b.repath=.6f;
    }
    motor.move=follow_route(b.route,tiles,p,3);
    if(distance_sq(p,goal)<14*14&&tile_ray_clear(tiles,p,goal))motor.move=distance_sq(p,goal)<3*3?Vec2{}:normalize_or(goal-p,{});
    if(length_sq(motor.move)>.01f)w.get<Actor>(e).facing=motor.move;
}
bool carryingShovel(const Game &g){return g.model.bag.has(Item::Shovel)||g.model.bag.has(Item::LoanShovel);}
bool guardSeesPlayer(Ctx &ctx,int officer){
    auto &g=ctx.local<Game>();auto &w=ctx.world();auto e=g.actors[static_cast<std::size_t>(officer)];
    auto at=position(w,e),p=position(w,g.player),delta=p-at;auto face=normalize_or(w.get<Actor>(e).facing,{0,1});
    return length_sq(delta)<cfg::officerSight*cfg::officerSight&&dot(normalize_or(delta,face),face)>=.65f&&tile_ray_clear(ctx.local<TileWorld>(),at,p);
}
bool diggingWatched(Ctx &ctx){for(int i=10;i<13;++i)if(guardSeesPlayer(ctx,i))return true;return false;}
void caughtDigging(Ctx &ctx){auto &g=ctx.local<Game>();stopVoice(ctx);newGame(ctx);if(!g.scripted)g.saved=saveCheckpoint(g);notify(g,"Caught digging! The entire run has restarted.",9);}
void updateShovelThreat(Ctx &ctx){
    auto &g=ctx.local<Game>();auto &w=ctx.world();bool tool=carryingShovel(g);auto p=position(w,g.player);
    if(!tool){if(g.shovelPressure){for(auto &b:g.brains)if(b.raider){b={};}if(g.escapePatrol)for(int i=10;i<13;++i)g.brains[static_cast<std::size_t>(i)]={};}g.shovelPressure=false;g.escapePatrol=false;g.waveTimer=4;g.waveAge=0;return;}
    if(!g.shovelPressure){g.shovelPressure=true;g.waveTimer=4;for(int i=0;i<9;++i)g.brains[static_cast<std::size_t>(i)]={};notify(g,"Shovel spotted! Prisoners attack in groups of 1-3. Replacements arrive 1.5x faster.",8);}
    const auto &cell=g.map.cells[static_cast<std::size_t>(g.model.hotspot)];
    if(!g.escapePatrol&&cell.box.contains(p)){
        if(g.ringSlot>=0)cancelBout(ctx);
        g.escapePatrol=true;g.escortReturning=false;g.incident=false;g.curfewIncident=false;g.implicated=Item::None;g.patrolStep={};
        for(int i=0;i<13;++i){g.brains[static_cast<std::size_t>(i)]={};stopActor(w,g.actors[static_cast<std::size_t>(i)]);}
        notify(g,"Prisoners retreat. All guards patrol here. Five unseen digs to escape. Caught digging = new run.",10);
    }
    if(g.escapePatrol)return;
    if(g.ringSlot>=0||g.grace>0||g.incident)return;
    bool fighting=false;for(int i=0;i<9;++i)fighting|=g.brains[static_cast<std::size_t>(i)].raider&&g.model.inmates[static_cast<std::size_t>(i)].hp>0;
    if(fighting){g.waveAge+=ctx.time().tick_dt;if(g.waveAge<22)return;for(int i=0;i<9;++i)if(g.brains[static_cast<std::size_t>(i)].raider)g.brains[static_cast<std::size_t>(i)]={};g.waveTimer=cfg::shovelWaveInterval;g.waveAge=0;return;}
    g.waveTimer-=ctx.time().tick_dt;if(g.waveTimer>0)return;
    std::vector<int> slots;for(int i=0;i<9;++i)if(g.model.inmates[static_cast<std::size_t>(i)].hp>0)slots.push_back(i);
    if(slots.empty())return;
    std::stable_sort(slots.begin(),slots.end(),[&](int a,int b){return distance_sq(position(w,g.actors[static_cast<std::size_t>(a)]),p)<distance_sq(position(w,g.actors[static_cast<std::size_t>(b)]),p);});
    int count=std::min(1+g.waveNumber%3,static_cast<int>(slots.size()));++g.waveNumber;
    for(int j=0;j<count;++j){auto &b=g.brains[static_cast<std::size_t>(slots[static_cast<std::size_t>(j)])];b={};b.hostile=true;b.raider=true;b.cooldown=.8f;}
    g.waveAge=0;g.waveTimer=cfg::shovelWaveInterval;notify(g,std::to_string(count)+" prisoner"+(count==1?" is":"s are")+" coming for your shovel!",4);
}
static void patrolDigCell(Ctx &ctx,int i){
    auto &g=ctx.local<Game>();auto &w=ctx.world();auto s=static_cast<std::size_t>(i);auto &b=g.brains[s];auto &step=g.patrolStep[static_cast<std::size_t>(i-10)];
    auto cell=g.map.cells[static_cast<std::size_t>(g.model.hotspot)];auto r=cell.box;
    const std::array<Vec2,5> route{{{r.x+r.w+8,r.y+r.h+16},{cell.door.x,r.y+r.h+16},{r.x-24,r.y+r.h+16},{r.x-24,r.y-16},{r.x+r.w+8,r.y-16}}};
    if(b.wait>0){b.wait-=ctx.time().tick_dt;w.get<Actor>(g.actors[s]).facing=step==1?normalize_or(cell.hotspot-position(w,g.actors[s]),{0,-1}):Vec2{0,1};if(b.wait<=0){step=(step+1)%5;b.route={};}return;}
    auto goal=route[static_cast<std::size_t>(step)];
    if(distance_sq(position(w,g.actors[s]),goal)<7*7){b.wait=step==0?12.f+static_cast<float>(i-10):step==1?2.f:.1f;return;}
    steer(ctx,g.actors[s],b,goal,cfg::officerSpeed);
}
void flagIncident(Ctx &ctx,Item item,std::string message){auto &g=ctx.local<Game>();if(g.incident||g.ringSlot>=0)return;auto &w=ctx.world();Vec2 p=position(w,g.player);bool seen=false;
    for(int i=10;i<13;++i){auto o=position(w,g.actors[static_cast<std::size_t>(i)]);if(distance_sq(p,o)<cfg::officerSight*cfg::officerSight&&tile_ray_clear(ctx.local<TileWorld>(),o,p))seen=true;}
    if(!seen)return;g.incident=true;g.curfewIncident=false;g.incidentTime=0;g.implicated=item;notify(g,std::move(message),7);
}
void cancelBout(Ctx &ctx){auto &g=ctx.local<Game>();if(g.ringSlot<0)return;auto s=static_cast<std::size_t>(g.ringSlot);auto safe=nearest_free_spot(ctx.local<TileWorld>(),Rect::from_center(g.ringReturn,{5,5}),4);if(safe)placeActor(ctx.world(),g.actors[s],*safe+Vec2{5,5});g.brains[s]={};g.ringSlot=-1;g.model.ringCooldown=cfg::ringCooldown;g.grace=5;notify(g,"Bout ended without a reward. Ring rests for 30 seconds.");}
void startBout(Ctx &ctx){auto &g=ctx.local<Game>();if(g.ringSlot>=0||g.model.ringCooldown>0||curfew(g.model.clock)){notify(g,"The ring is unavailable right now.");return;}
    std::vector<int> eligible;for(int i=0;i<9;++i)if(g.model.inmates[static_cast<std::size_t>(i)].hp==5&&!g.brains[static_cast<std::size_t>(i)].hostile)eligible.push_back(i);
    if(eligible.empty()){notify(g,"No rested opponent is available. Try again shortly.");return;}
    g.ringSlot=eligible[static_cast<std::size_t>(g.model.rng.between(0,static_cast<int>(eligible.size())-1))];auto s=static_cast<std::size_t>(g.ringSlot);g.ringReturn=position(ctx.world(),g.actors[s]);g.brains[s]={};g.brains[s].hostile=true;g.brains[s].cooldown=1;
    placeActor(ctx.world(),g.player,ringPlayer);placeActor(ctx.world(),g.actors[s],ringOpponent);g.wasWing=false;g.overview=0;g.face={1,0};ctx.world().get<Actor>(g.player).facing=g.face;g.ringRewarded=false;g.ringReward=0;g.wasInRing=true;openMenu(g,Menu::None);g.grace=5;notify(g,"Sanctioned bout. Punches only. Dodge red telegraphs. SPACE to punch.",7);
}
static void defeated(Ctx &ctx,int slot){auto &g=ctx.local<Game>();auto s=static_cast<std::size_t>(slot);auto &n=g.model.inmates[s];n.hp=0;n.respawn=cfg::respawnSeconds;g.brains[s]={};stopActor(ctx.world(),g.actors[s]);
    if(slot==g.ringSlot&&!g.ringRewarded){g.ringRewarded=true;g.ringReward=g.model.rng.between(cfg::ringRewardMin,cfg::ringRewardMax);int actual=awardMoney(g.model,g.ringReward);g.ringSlot=-1;g.model.ringCooldown=cfg::ringCooldown;g.grace=5;notify(g,"Bout won! Purse $"+std::to_string(g.ringReward)+" / credited $"+std::to_string(actual)+". Ring cooldown: 30 seconds.",8);}
    else notify(g,carryingShovel(g)?"Inmate defeated. Replacement in about 6.7 active seconds. No assault payout.":"Inmate defeated. A replacement arrives in ten active seconds. No assault payout.",6);
}
void playerAttack(Ctx &ctx){auto &g=ctx.local<Game>();if(g.attackCooldown>0)return;Item held=g.model.bag.held();
    if(held!=Item::None&&held!=Item::Knife&&held!=Item::Bat)return;
    if(g.ringSlot>=0&&(held==Item::Knife||held==Item::Bat)){notify(g,"Sanctioned bouts allow punches only. Select an empty slot.");return;}
    g.attackCooldown=cfg::attackCooldown;g.attackVisual=.18f;auto &w=ctx.world();Vec2 p=position(w,g.player);int target=-1;float best=cfg::meleeRange*cfg::meleeRange;
    for(int i=0;i<13;++i){if(g.ringSlot>=0&&i!=g.ringSlot)continue;if(i<10&&g.model.inmates[static_cast<std::size_t>(i)].hp<=0)continue;Vec2 delta=position(w,g.actors[static_cast<std::size_t>(i)])-p;float d=length_sq(delta);if(d<best&&dot(normalize_or(delta,g.face),g.face)>.35f&&tile_ray_clear(ctx.local<TileWorld>(),p,p+delta)){best=d;target=i;}}
    if(target<0)return;Item implicated=held==Item::Knife||held==Item::Bat?held:Item::None;
    if(target>=10){g.incident=true;g.incidentTime=0;g.implicated=implicated;notify(g,"Officer: Assaulting staff is misconduct. Stop immediately.");return;}
    auto s=static_cast<std::size_t>(target);int damage=g.ringSlot>=0?1:itemDamage(held);g.model.inmates[s].hp-=damage;w.get<Actor>(g.actors[s]).flash=.25f;
    ctx.emit(PlaySound{.group=str_id("punch"),.volume=.3f});
    if(g.ringSlot<0){flagIncident(ctx,implicated,"Officer: I witnessed that assault. Stop immediately!");g.brains[s].hostile=true;g.brains[s].cooldown=.5f;}
    if(g.model.inmates[s].hp<=0)defeated(ctx,target);else notify(g,"Hit "+std::string(residentName(g.model,static_cast<int>(s)))+" / -"+std::to_string(damage)+" HP",1.5f);
}
static Vec2 safeSpawn(Ctx &ctx,int slot){auto &g=ctx.local<Game>();auto &w=ctx.world();
    auto clear=[&](Vec2 at){if(box_blocked(ctx.local<TileWorld>(),Rect::from_center(at,{5,5})))return false;float radius=slot==9?24.f:64.f;if(distance_sq(at,position(w,g.player))<radius*radius)return false;for(int j=0;j<13;++j)if(j!=slot&&(j>=10||g.model.inmates[static_cast<std::size_t>(j)].hp>0)&&distance_sq(at,position(w,g.actors[static_cast<std::size_t>(j)]))<24*24)return false;return true;};
    if(slot==9){for(auto offset:std::array<Vec2,5>{{{0,0},{0,32},{0,-64},{0,-96},{-176,-112}}})if(clear(dealerSpot+offset))return dealerSpot+offset;}
    int start=g.model.rng.between(0,static_cast<int>(g.map.visits.size())-1);for(std::size_t i=0;i<g.map.visits.size();++i){auto at=g.map.visits[(static_cast<std::size_t>(start)+i)%g.map.visits.size()];if(clear(at))return at;}
    for(int y=4;y<76;++y)for(int x=4;x<76;++x){auto at=ctx.local<TileWorld>().tile_center({x,y});if(clear(at))return at;}
    return playerHome; // Unreachable in this validated compound with fourteen actors.
}
void tickPeople(Ctx &ctx){auto &g=ctx.local<Game>();auto &w=ctx.world();const float dt=ctx.time().tick_dt;g.paths.begin_tick(16);auto p=position(w,g.player);
    for(auto [e,a]:w.view<Actor>()){a.flash=std::max(0.f,a.flash-dt);a.animation+=dt;}
    for(int i=0;i<13;++i){auto s=static_cast<std::size_t>(i);auto e=g.actors[s];auto &b=g.brains[s];auto at=position(w,e);w.get<TopDownMotor>(e).move={};
        if(i<10){auto &n=g.model.inmates[s];if(n.hp<=0){if(n.respawn<=0){n.hp=5;++n.generation;n.appearance=g.model.rng.between(1,9999);b={};placeActor(w,e,safeSpawn(ctx,i));notify(g,i==9?"Vex's service is available again.":"A new resident has taken the vacant roster slot.",3);}continue;}}
        b.cooldown=std::max(-1.f,b.cooldown-dt);
        if(i<9&&g.escapePatrol){b.hostile=false;b.raider=false;b.telegraph=0;Vec2 retreat{static_cast<float>(480+(i%3)*160),static_cast<float>(928+(i/3)*64)};if(distance_sq(at,retreat)>12*12)steer(ctx,e,b,retreat,cfg::npcSpeed);continue;}
        if(i>=10&&g.escapePatrol){patrolDigCell(ctx,i);continue;}
        if(i<9){bool near=distance_sq(at,p)<cfg::encounterRadius*cfg::encounterRadius;bool eligible=!g.shovelPressure&&g.ringSlot<0&&g.grace<=0&&g.activity==Activity::None&&!g.incident&&!b.hostile;
            if(near&&!b.near&&b.cooldown<=0&&eligible){b.cooldown=cfg::encounterCooldown;if(g.model.rng.chance(aggressionChance(g.model.stats.reputation))){b.hostile=true;b.cooldown=.8f;notify(g,std::string(residentName(g.model,static_cast<int>(s)))+": Think you're better than us?",3);}}
            b.near=near;
        }
        if(i<10&&b.hostile){
            if(g.ringSlot>=0&&i!=g.ringSlot)continue;
            float d=distance_sq(at,p);if(g.ringSlot<0&&!b.raider&&d>260*260){b.wait+=dt;if(b.wait>3){b.hostile=false;b.wait=2;b.goal={};b.cooldown=30;b.telegraph=0;}continue;}
            b.wait=0;
            if(b.telegraph>0){b.telegraph-=dt;if(b.telegraph<=0){
                Vec2 facing=w.get<Actor>(e).facing;
                if(d<36*36&&dot(normalize_or(p-at,facing),facing)>.25f&&g.invuln<=0&&tile_ray_clear(ctx.local<TileWorld>(),at,p)){--g.model.stats.hp;g.invuln=cfg::invulnerability;w.get<Actor>(g.player).flash=.25f;if(g.activity==Activity::Phone)stopVoice(ctx);g.activity=Activity::None;g.progress=0;notify(g,"Hit! -1 HP. Move to dodge the next strike.",2);}
                float cadence=g.ringSlot>=0?cfg::ringCadence[static_cast<std::size_t>(g.difficulty)]:1.4f;b.cooldown=std::max(.1f,cadence-.35f);
            }}else if(b.cooldown<=0&&d<42*42){b.telegraph=.35f;w.get<Actor>(e).facing=normalize_or(p-at,{0,1});}
            else if(d>24*24)steer(ctx,e,b,p,g.ringSlot>=0?54.f:68.f);
            continue;
        }
        if(i==9){if(distance_sq(at,dealerSpot)>6*6&&distance_sq(p,dealerSpot)>24*24)steer(ctx,e,b,dealerSpot,cfg::npcSpeed);continue;}
        if(i>=10&&g.incident){if(g.incidentTime>1.4f)steer(ctx,e,b,p,105);continue;}
        if(i==10&&g.escortReturning){if(distance_sq(at,{640,1176})<14*14)g.escortReturning=false;else {steer(ctx,e,b,{640,1176},100);continue;}}
        if(b.wait>0){b.wait-=dt;continue;}
        if(length_sq(b.goal)<1){
            if(i<9){if(curfew(g.model.clock))b.goal=g.map.cells[static_cast<std::size_t>(i+1)].hotspot;else {int pick=g.model.rng.between(0,static_cast<int>(g.map.visits.size())+1);b.goal=pick>=static_cast<int>(g.map.visits.size())?g.map.seats[static_cast<std::size_t>(pick%8)]:g.map.visits[static_cast<std::size_t>(pick)];}}
            else if(i==10){constexpr std::array<Vec2,3> route{{{600,1176},{680,1176},{640,1128}}};b.goal=route[static_cast<std::size_t>(g.model.rng.between(0,2))];}
            else if(i==11){constexpr std::array<Vec2,5> route{{{640,608},{752,720},{512,784},{640,464},{640,392}}};b.goal=route[static_cast<std::size_t>(g.model.rng.between(0,4))];}
            else {constexpr std::array<Vec2,5> route{{{640,904},{928,672},{320,672},{640,464},{640,1040}}};b.goal=route[static_cast<std::size_t>(g.model.rng.between(0,4))];}
            b.route={};b.repath=0;
        }
        if(distance_sq(at,b.goal)<7*7){b.goal={};b.wait=g.model.rng.range(1,4);continue;}
        if(g.activity==Activity::Eat&&distance_sq(b.goal,g.activityAt)<25*25){b.goal={};b.wait=2;continue;}
        steer(ctx,e,b,b.goal,i<10?cfg::npcSpeed:cfg::officerSpeed);
    }
    if(g.incident){g.incidentTime+=dt;bool confront=false;for(int i=10;i<13;++i)if(distance_sq(p,position(w,g.actors[static_cast<std::size_t>(i)]))<30*30&&tile_ray_clear(ctx.local<TileWorld>(),p,position(w,g.actors[static_cast<std::size_t>(i)])))confront=true;
        if(g.incidentTime>1.4f&&(confront||distance_sq(p,playerHome)<35*35)){
            auto result=confirmMisconduct(g.model);if(g.implicated!=Item::None)g.model.bag.remove(g.implicated);if(g.implicated==Item::LoanShovel)g.model.loan=false;g.implicated=Item::None;g.incident=false;if(g.curfewIncident)g.model.curfewResolved=true;g.curfewIncident=false;g.activity=Activity::None;g.progress=0;g.grace=5;placeActor(w,g.player,playerHome);setDoor(w,g,false);for(auto &b:g.brains){b.hostile=false;b.telegraph=0;b.route={};b.goal={};}notify(g,result.message,8);
        }
    }
    if(!g.escapePatrol&&g.model.loan&&g.model.bag.has(Item::LoanShovel)&&p.y<912)flagIncident(ctx,Item::LoanShovel,"Officer: That loaned tool must stay at the garden task. Stop!");
}
void tickEscort(Ctx &ctx){auto &g=ctx.local<Game>();auto &w=ctx.world();g.paths.begin_tick(4);constexpr std::array<Vec2,5> route{{{640,904},{640,464},{640,224},{168,224},{168,170}}};
    if(g.escortStep<5){auto goal=route[static_cast<std::size_t>(g.escortStep)];auto p=position(w,g.player);auto &m=w.get<TopDownMotor>(g.player);m.max_speed=125;
        auto &tiles=ctx.local<TileWorld>();if(g.escortRoute.count==0||g.escortRoute.goal!=tiles.tile_at(goal)){auto result=g.paths.find(tiles.cost,tiles.tile_at(p),tiles.tile_at(goal),GridDirs::Eight,g.escortRoute);(void)result;}
        m.move=follow_route(g.escortRoute,tiles,p,3);if(distance_sq(p,goal)<12*12)m.move=normalize_or(goal-p,{});w.get<Actor>(g.player).facing=normalize_or(m.move,{0,1});
        steer(ctx,g.actors[10],g.brains[10],goal,130);
        if(distance_sq(p,goal)<6*6){++g.escortStep;g.escortRoute={};}
    }else {stopActor(w,g.player);if(!g.doorClosed)setDoor(w,g,true);g.story+=ctx.time().tick_dt;steer(ctx,g.actors[10],g.brains[10],{640,1176},105);
        if(g.story>1.5f){g.phase=Phase::Play;g.escortReturning=true;g.grace=5;g.uiDirty=true;g.checkpoint=g.model;if(!g.scripted)g.saved=saveCheckpoint(g);notify(g,"Your cell is A-01. Walk to the door and press E to open it.",8);}
    }
}
}
