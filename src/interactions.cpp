#include "game.h"
#include "broadcast.h"
#include <algorithm>
#include <cmath>
#include <lam/ecs/transform.h>
#include <lam/ecs/world.h>
#include <lam/physics/components.h>
#include <lam/physics/motors.h>
#include <lam/audio/components.h>
namespace prison {
using namespace lam;
static bool service(Game &g){if(curfew(g.model.clock)){notify(g,"Public services are closed during curfew. Return to A-01.");return false;}return true;}
void beginActivity(Ctx &ctx,Activity a,float duration){auto &g=ctx.local<Game>();g.activity=a;g.progress=0;g.duration=duration;g.activityAt=ctx.world().get<Transform>(g.player).position;g.activitySource=g.focusProp;stopActor(ctx.world(),g.player);}
void processCommands(Ctx &ctx){auto &g=ctx.local<Game>();for(auto intent:input_frame(ctx).intent_list()){
    if(intent.kind!=str_id("prison.command"))continue;int cmd=static_cast<int>(intent.a);
    if(cmd==1){openMenu(g,Menu::None);g.grace=std::max(g.grace,1.f);}
    else if(cmd==2)openMenu(g,Menu::Map);
    else if(cmd==3)openMenu(g,Menu::Help);
    else if(cmd==4&&g.phase==Phase::Infirmary)restoreCheckpoint(ctx);
    else if(cmd==5&&(g.phase==Phase::Released||g.phase==Phase::Escaped||g.phase==Phase::Infirmary))newGame(ctx);
    else if(cmd==6&&g.menu==Menu::Pause){if(loadCheckpoint(g))restoreCheckpoint(ctx);else notify(g,"No valid checkpoint found.");}
    else if(cmd==70&&g.menu==Menu::Pause)openMenu(g,Menu::Audio);
    else if(cmd==71&&g.menu==Menu::Audio)startLiveRollCall(ctx);
    else if(cmd==72&&g.menu==Menu::Audio)toggleBroadcast(ctx);
    else if(cmd>=73&&cmd<=77&&g.menu==Menu::Audio){constexpr std::array<std::string_view,5> clips{"demo","phone","verdict","released","escaped"};queueVoice(ctx,clips[static_cast<std::size_t>(cmd-73)],true);}
    else if(cmd>=10&&cmd<=13&&g.menu==Menu::Dealer&&g.phase==Phase::Play){if(service(g))notify(g,purchase(g.model,static_cast<Item>(cmd-9)).message);}
    else if(cmd==20&&(g.menu==Menu::Dialogue||g.menu==Menu::Dealer)){
        std::string cell=std::to_string(g.model.hotspot+1);notify(g,"A quiet tip: look for the cracked tile in cell "+cell+". A shovel might help.",8);
    }
    else if(cmd==21&&(g.menu==Menu::Dialogue||g.menu==Menu::Dealer)){notify(g,helpResident(g.model,g.menu==Menu::Dealer?9:g.dialogueSlot).message);g.uiDirty=true;}
    else if(cmd>=30&&cmd<=33&&g.menu==Menu::Food){if(service(g)){auto result=takeMeal(g.model,cmd-30);notify(g,result.message);if(result.ok)openMenu(g,Menu::None);}}
    else if(cmd==34&&g.menu==Menu::Food){if(mealWindow(g.model.clock)<0){notify(g,"Serving shifts are available during meal windows.");}else if(service(g)&&!g.model.jobs[2]){openMenu(g,Menu::None);beginActivity(ctx,Activity::Serve,5);notify(g,"Help plate meals. Stay beside the counter for five seconds.");}else if(g.model.jobs[2])notify(g,"You already served a shift today.");}
    else if(cmd>=40&&cmd<=42&&g.menu==Menu::Ring){g.difficulty=cmd-40;openMenu(g,Menu::ConfirmRing);}
    else if(cmd==43&&g.menu==Menu::ConfirmRing)startBout(ctx);
    else if(cmd==50&&g.menu==Menu::Drop){
        auto p=ctx.world().get<Transform>(g.player).position;auto &tiles=ctx.local<TileWorld>();Item item=g.model.bag.held();
        auto free=nearest_free_spot(tiles,Rect::from_center(p+g.face*18,{5,5}),3);
        if(item!=Item::None&&free){g.model.drops.push_back({item,*free+Vec2{5,5}});g.model.bag.eraseSelected();notify(g,"Item dropped nearby. E picks it up again.");openMenu(g,Menu::None);}else notify(g,"No safe drop location. Item kept.");
    }
    else if(cmd==60&&g.menu==Menu::Vending){if(service(g)){
        auto &m=g.model;auto slot=std::find(m.bag.slots.begin(),m.bag.slots.end(),Item::None);
        if(m.stats.money<cfg::snackPrice)notify(g,"A snack costs $5. You have insufficient funds.");
        else if(slot==m.bag.slots.end())notify(g,"All three slots are full. Drop an item first. You were not charged.");
        else {
            Item snack=m.rng.chance(.5f)?Item::Soda:Item::Chips;
            *slot=snack;m.bag.selected=static_cast<int>(slot-m.bag.slots.begin());m.stats.money-=cfg::snackPrice;
            notify(g,std::string(itemName(snack))+" delivered to your hand. -$5 / SPACE to consume anywhere (+10 sanity).");
            openMenu(g,Menu::None);
        }
    }}
}}
void updateFocus(Ctx &ctx){
    auto &g=ctx.local<Game>();auto &w=ctx.world();auto p=w.get<Transform>(g.player).position;auto &tiles=ctx.local<TileWorld>();
    g.focusProp=-1;g.focusNPC=-1;g.focusDrop=-1;g.prompt.clear();if(g.phase!=Phase::Play||g.menu!=Menu::None||g.overview>0||g.activity!=Activity::None)return;
    if(g.ringSlot>=0){g.prompt="Empty slot + SPACE / Punch   MOVE / Dodge";return;}
    float best=42*42;
    for(std::size_t i=0;i<g.map.props.size();++i){auto prop=g.map.props[i];if(prop.kind==PropKind::Table||prop.kind==PropKind::Gate)continue;if(prop.kind==PropKind::Spill&&g.model.jobs[0])continue;if(prop.kind==PropKind::LostBook&&(g.model.bookTaken||g.model.jobs[1]))continue;if(prop.kind==PropKind::Bat&&g.model.batTaken)continue;if(prop.kind==PropKind::Ball&&g.model.ballTaken)continue;
        if(prop.kind==PropKind::Seat&&prop.index>=8)continue; // Library furnishings are separate from cafeteria meal seats.
        float d=distance_sq(p,prop.interaction);if(d>=best||!tile_ray_clear(tiles,p,prop.interaction))continue;
        best=d;g.focusProp=static_cast<int>(i);
        switch(prop.kind){
        case PropKind::Bed:g.prompt=prop.index==0&&curfew(g.model.clock)?"E / Rest until morning":"E / Inspect bunk";break;
        case PropKind::Toilet:g.prompt="E / Inspect cell";break;
        case PropKind::Shelf:g.prompt=g.model.bag.has(Item::Book)?"E / Return the lost book (+$15)":"E / Read a book (10 seconds)";break;
        case PropKind::Seat:g.prompt="E / Sit and eat a carried meal";break;
        case PropKind::Counter:g.prompt=prop.index<4?"E / Take "+std::string(cfg::foods[static_cast<std::size_t>(prop.index)]):"E / Help serve (+$20 / +1 conduct)";break;
        case PropKind::Vending:g.prompt="E / Vending machine ($5)";break;
        case PropKind::Phone:g.prompt="E / Call home";break;
        case PropKind::Pool:g.prompt="E / Take a pool shot";break;
        case PropKind::Hoop:g.prompt="E / Shoot a basket";break;
        case PropKind::Ring:g.prompt="E / Choose a sanctioned bout";break;
        case PropKind::Spill:g.prompt="E / Clean spill (+$20 / +1 conduct)";break;
        case PropKind::LostBook:g.prompt="E / Pick up lost library book";break;
        case PropKind::Shed:g.prompt=g.model.loan?"E / Return the groundskeeper's shovel":"E / Groundskeeper task (borrow shovel)";break;
        case PropKind::Patch:g.prompt="E / Tend the marked garden patch";break;
        case PropKind::Bat:g.prompt="E / Pick up bat (2 damage)";break;
        case PropKind::Ball:g.prompt="E / Pick up basketball";break;
        default:break;
        }
    }
    for(int i=0;i<13;++i){if(i<10&&g.model.inmates[static_cast<std::size_t>(i)].hp<=0)continue;auto e=g.actors[static_cast<std::size_t>(i)];auto at=w.get<Transform>(e).position;float d=distance_sq(p,at);if(d<best&&tile_ray_clear(tiles,p,at)){best=d;g.focusNPC=i;g.focusProp=-1;g.prompt=i==9?"E / Vex's discreet supply shop":i>=10?"E / Talk to officer":"E / Talk to "+std::string(residentName(g.model,i));}}
    for(std::size_t i=0;i<g.model.drops.size();++i){auto d=g.model.drops[i];float dist=distance_sq(p,d.at);if(dist<best&&tile_ray_clear(tiles,p,d.at)){best=dist;g.focusDrop=static_cast<int>(i);g.focusProp=-1;g.focusNPC=-1;g.prompt="E / Recover "+std::string(itemName(d.item));}}
    auto hotspot=g.map.cells[static_cast<std::size_t>(g.model.hotspot)].hotspot;float d=distance_sq(p,hotspot);if(d<30*30&&d<=best&&tile_ray_clear(tiles,p,hotspot)){best=d;g.focusProp=-2;g.focusNPC=-1;g.focusDrop=-1;g.prompt=g.model.bag.has(Item::Shovel)||g.model.bag.has(Item::LoanShovel)?"HOLD E / Dig 8 sec / "+std::to_string(g.model.digSteps)+" of 5 complete":"E / Inspect the loose tile";}
    Vec2 door{168,204};d=distance_sq(p,door);if(d<34*34&&d<=best){g.focusProp=-3;g.focusNPC=-1;g.focusDrop=-1;g.prompt=g.doorClosed?"E / Open your cell door":"E / Close your cell door";}
}
void interact(Ctx &ctx){auto &g=ctx.local<Game>();auto &w=ctx.world();auto &m=g.model;if(g.activity!=Activity::None||g.ringSlot>=0)return;updateFocus(ctx);
    if(g.focusDrop>=0){auto i=static_cast<std::size_t>(g.focusDrop);if(m.bag.add(m.drops[i].item)){notify(g,"Recovered "+std::string(itemName(m.drops[i].item))+".");m.drops.erase(m.drops.begin()+g.focusDrop);}else notify(g,"All three carry slots are full.");return;}
    if(g.focusNPC>=0){if(g.focusNPC>=10){notify(g,curfew(m.clock)?"Officer: Curfew is in effect. Return to your assigned cell.":"Officer: Help with jobs. Three conduct points remove a sentence day.",7);return;}
        if(g.brains[static_cast<std::size_t>(g.focusNPC)].hostile){notify(g,"They are fighting. Move away or defend yourself.");return;}
        if(g.focusNPC==9){if(service(g))openMenu(g,Menu::Dealer);}else {g.dialogueSlot=g.focusNPC;openMenu(g,Menu::Dialogue);}return;}
    if(g.focusProp==-3){auto p=w.get<Transform>(g.player).position;if(!g.doorClosed&&std::abs(p.y-204)<12){notify(g,"Step clear of the doorway first.");return;}setDoor(w,g,!g.doorClosed);ctx.emit(PlaySound{.group=str_id("eleven_door"),.volume=.7f});notify(g,g.doorClosed?"Your cell door is closed.":"Your cell door is open.");return;}
    if(g.focusProp==-2){if(!m.bag.has(Item::Shovel)&&!m.bag.has(Item::LoanShovel)){notify(g,"A loose tile. A borrowed or purchased shovel could open a way out.",7);return;}if(g.digNeedsRelease)return;updateShovelThreat(ctx);if(diggingWatched(ctx)){caughtDigging(ctx);return;}beginActivity(ctx,Activity::Dig,cfg::digSeconds);return;}
    if(g.focusProp<0)return;auto prop=g.map.props[static_cast<std::size_t>(g.focusProp)];
    switch(prop.kind){
    case PropKind::Bed:if(prop.index==0&&curfew(m.clock))beginActivity(ctx,Activity::Rest,1.5f);else notify(g,prop.index==0?"Your bunk. Return here after 22:00 to rest until morning.":"Someone else's bunk. The twelve cells each have one bed and toilet.");break;
    case PropKind::Toilet:notify(g,"You inspect the cell. Look for a small orange-marked crack in a floor tile.");break;
    case PropKind::Shelf:if(!service(g))break;if(m.bag.has(Item::Book)){notify(g,finishJob(m,1).message);break;}if(m.books>=3){notify(g,"Three books today. The allowance resets at 07:00.");break;}beginActivity(ctx,Activity::Read,cfg::readSeconds);notify(g,"Reading. Stay still for ten active seconds.");break;
    case PropKind::Counter:
        if(!service(g))break;
        if(prop.index<4)notify(g,takeMeal(m,prop.index).message);
        else if(mealWindow(m.clock)<0)notify(g,"Serving shifts are available during meal windows.");
        else if(m.jobs[2])notify(g,"You already served a shift today.");
        else {beginActivity(ctx,Activity::Serve,5);notify(g,"Help plate meals. Stay beside the counter for five seconds.");}
        break;
    case PropKind::Vending:if(service(g))openMenu(g,Menu::Vending);break;
    case PropKind::Seat:{if(!service(g))break;bool occupied=false;for(int i=0;i<10;++i)if(m.inmates[static_cast<std::size_t>(i)].hp>0&&distance_sq(w.get<Transform>(g.actors[static_cast<std::size_t>(i)]).position,prop.interaction)<20*20)occupied=true;if(occupied){notify(g,"This seat is occupied. Try another round seat.");break;}bool food=false;for(auto item:m.bag.slots)food|=isFood(item);if(!food){notify(g,"Pick up a meal at the far-west counter first.");break;}placeActor(w,g.player,prop.interaction);beginActivity(ctx,Activity::Eat,2);break;}
    case PropKind::Phone:if(service(g)){beginActivity(ctx,Activity::Phone,voiceSeconds(ctx,"phone"));queueVoice(ctx,"phone",true);}break;
    case PropKind::Pool:if(service(g))beginActivity(ctx,Activity::Pool,2);break;
    case PropKind::Hoop:if(service(g))beginActivity(ctx,Activity::Basketball,2.5f);break;
    case PropKind::Ring:if(service(g)){if(m.ringCooldown>0)notify(g,"The ring needs a break. Try again in "+std::to_string(static_cast<int>(std::ceil(m.ringCooldown)))+" seconds.");else openMenu(g,Menu::Ring);}break;
    case PropKind::Spill:if(service(g))beginActivity(ctx,Activity::Clean,4);break;
    case PropKind::LostBook:if(m.bag.add(Item::Book)){g.model.bookTaken=true;notify(g,"Lost book picked up. Return it at any library shelf.");}else notify(g,"You need a free carry slot.");break;
    case PropKind::Shed:if(!service(g))break;if(m.jobs[3])notify(g,"The groundskeeper thanks you. Come back tomorrow.");else if(m.loan){if(g.model.gardenTended)notify(g,finishJob(m,3).message);else notify(g,"Tend the marked garden patch east of the shed, then return the shovel.");}else if(m.bag.add(Item::LoanShovel)){m.loan=true;notify(g,"Shovel on loan. Tend the marked garden patch, then return it for $20.",8);}else notify(g,"Free a carry slot before borrowing the shovel.");break;
    case PropKind::Patch:if(!service(g))break;if(m.bag.has(Item::LoanShovel)&&!g.model.gardenTended)beginActivity(ctx,Activity::Garden,4);else notify(g,g.model.gardenTended?"Patch tended. Return the loaned shovel to the tool shed.":"Borrow a task shovel at the tool shed first.");break;
    case PropKind::Bat:if(m.bag.add(Item::Bat)){g.model.batTaken=true;notify(g,"Bat picked up. 2 damage outside sanctioned bouts.");}else notify(g,"All three slots are full.");break;
    case PropKind::Ball:if(m.bag.add(Item::Ball)){g.model.ballTaken=true;notify(g,"Basketball picked up. Visit the southwest court.");}else notify(g,"All three slots are full.");break;
    default:break;
    }
}
void morning(Ctx &ctx){auto &g=ctx.local<Game>();g.model.bookTaken=g.model.bag.has(Item::Book);for(auto d:g.model.drops)if(d.item==Item::Book)g.model.bookTaken=true;g.model.gardenTended=false;g.warned=false;g.curfewGrace=0;g.incident=false;g.curfewIncident=false;g.brains={};g.grace=5;if(g.model.stats.hp>0){g.checkpoint=g.model;if(!g.scripted)g.saved=saveCheckpoint(g);}notify(g,g.saved?"07:00. A new day. Daily activities reset; checkpoint saved.":"07:00. A new day. Daily activities reset.",7);if(g.model.daysLeft<=0&&g.model.stats.hp>0){g.phase=Phase::Released;g.story=0;g.uiDirty=true;}}
void finishActivity(Ctx &ctx){auto &g=ctx.local<Game>();auto a=g.activity;g.activity=Activity::None;g.progress=0;
    switch(a){
    case Activity::Read:notify(g,finishBook(g.model,cfg::readSeconds).message);break;
    case Activity::Dig:
        ++g.model.digSteps;g.digNeedsRelease=true;
        if(g.model.digSteps>=cfg::digHolds){g.phase=Phase::Escaped;g.story=0;g.uiDirty=true;notify(g,"A way out. The tunnel gives way to daylight.");}
        else notify(g,"Dig "+std::to_string(g.model.digSteps)+" / 5 complete. Release E. Watch the guards before the next hold.",6);
        break;
    case Activity::Clean:notify(g,finishJob(g.model,0).message);break;
    case Activity::Serve:notify(g,finishJob(g.model,2).message);break;
    case Activity::Garden:g.model.gardenTended=true;notify(g,"Patch tended. Return the loaned shovel to the tool shed for payment.");break;
    case Activity::Eat:notify(g,eatMeal(g.model).message);break;
    case Activity::Phone:notify(g,"A familiar voice: Keep your head up. We'll be waiting outside.",7);break;
    case Activity::Pool:notify(g,"A clean shot. The eight ball rolls into the corner pocket.");break;
    case Activity::Basketball:notify(g,"Nothing but net. A small victory in a long week.");break;
    case Activity::Rest:{double target=(std::floor((g.model.clock-420)/1440)+1)*1440+420;if(advanceMinutes(g.model,target-g.model.clock))morning(ctx);break;}
    default:break;
    }
}
void tickActivity(Ctx &ctx){auto &g=ctx.local<Game>();if(g.activity==Activity::None)return;const auto &in=input_frame(ctx);const auto &controls=ctx.res<Controls>();auto p=ctx.world().get<Transform>(g.player).position;
    if(length_sq(in.axis2(controls.move))>.01f||distance_sq(p,g.activityAt)>12*12||(g.activity==Activity::Dig&&!in.is_held(controls.interact))){if(g.activity==Activity::Phone)stopVoice(ctx);notify(g,g.activity==Activity::Read?"Reading interrupted. No progress or book allowance used.":"Action cancelled. Items kept.");g.activity=Activity::None;g.progress=0;return;}
    if(g.activity==Activity::Dig){if(!carryingShovel(g)){g.activity=Activity::None;g.progress=0;return;}if(diggingWatched(ctx)){caughtDigging(ctx);return;}}
    g.progress+=ctx.time().tick_dt;if(g.progress+.0001f>=g.duration)finishActivity(ctx);
}
}
