#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "game.h"
#include "broadcast.h"
#include <algorithm>
#include <lam/physics/tile_world.h>
#include <lam/tilemap/grid_path.h>
#include <lam/core/default_plugins.h>
#include <lam/ecs/world.h>
#include <lam/ecs/transform.h>
#include <lam/input/driver.h>
#include <lam/physics/components.h>
#include <lam/render/virtual_screen.h>
#include <lam/audio/audio.h>
using namespace prison;
using namespace lam;
TEST_CASE("Map has exact furniture counts and connected essential interactions"){
    TileWorld tiles;Map map;map.build(tiles);
    auto count=[&](PropKind k){return std::count_if(map.props.begin(),map.props.end(),[&](auto p){return p.kind==k;});};
    CHECK(map.cells.size()==12);CHECK(count(PropKind::Bed)==12);CHECK(count(PropKind::Toilet)==12);CHECK(count(PropKind::Shelf)==8);CHECK(count(PropKind::Table)==4);CHECK(count(PropKind::Seat)==16);CHECK(count(PropKind::Vending)==2);
    PathFinder paths;paths.resize(6400);paths.begin_tick(1000);
    auto reachable=[&](Vec2 p){PathRoute r;auto result=paths.find(tiles.cost,tiles.tile_at(playerHome),tiles.tile_at(p),GridDirs::Four,r);INFO("destination "<<p.x<<","<<p.y);REQUIRE(result);CHECK(*result==PathStatus::Found);CHECK(r.complete);CHECK_FALSE(box_blocked(tiles,Rect::from_center(p,{5,5})));};
    for(auto c:map.cells){reachable(c.door);reachable(c.hotspot);CHECK(c.hotspot.y>c.box.y);}
    for(auto p:map.props)reachable(p.interaction);
    for(auto p:map.visits)reachable(p);
    reachable(dealerSpot);reachable(ringPlayer);reachable(ringOpponent);
}
TEST_CASE("Transactions respect capacity and money"){
    Model m;CHECK_FALSE(purchase(m,Item::Knife).ok);CHECK(m.stats.money==0);CHECK(m.bag.count()==0);
    m.stats.money=500;REQUIRE(purchase(m,Item::Knife).ok);CHECK(m.stats.money==400);CHECK(m.bag.count()==1);
    m.bag.add(Item::Bat);m.bag.add(Item::Ball);CHECK_FALSE(purchase(m,Item::Medkit).ok);CHECK(m.stats.money==400);CHECK(m.bag.count()==3);
    CHECK(awardMoney(m,200)==100);CHECK(m.stats.money==500);
}
TEST_CASE("Combat and consumable values match specification"){
    CHECK(itemDamage(Item::None)==1);CHECK(itemDamage(Item::Knife)==3);CHECK(itemDamage(Item::Bat)==2);
    Model m;m.bag.add(Item::Medkit);CHECK_FALSE(consume(m).ok);CHECK(m.bag.count()==1);
    m.stats.hp=3;CHECK(consume(m).ok);CHECK(m.stats.hp==8);CHECK(m.bag.count()==0);
    m.bag.add(Item::Adrenaline);m.stats.hp=9;m.stats.sanity=99;CHECK(consume(m).ok);CHECK(m.stats.hp==10);CHECK(m.stats.sanity==100);
    m.bag.add(Item::Adrenaline);CHECK_FALSE(consume(m).ok);CHECK(m.bag.count()==1);m.bag.remove(Item::Adrenaline);
    m.bag.add(Item::Rice);m.stats.hp=4;m.stats.sanity=50;CHECK(eatMeal(m).ok);CHECK(m.stats.hp==4);CHECK(m.stats.sanity==70);
}
TEST_CASE("Books and reputation cannot be farmed"){
    Model m;CHECK_FALSE(finishBook(m,9.9f).ok);CHECK(m.books==0);CHECK(m.stats.intelligence==0);
    for(int i=0;i<3;++i)CHECK(finishBook(m,10).ok);CHECK_FALSE(finishBook(m,10).ok);CHECK(m.stats.intelligence==30);
    CHECK(helpResident(m,0).ok);CHECK_FALSE(helpResident(m,0).ok);CHECK(m.stats.reputation==2);
    for(int i=1;i<10;++i)helpResident(m,i);CHECK(m.stats.reputation==12);CHECK(aggressionChance(12)==0);CHECK(aggressionChance(0)==doctest::Approx(.05));
}
TEST_CASE("Active timers are independent from game clock"){
    for(int rate:{30,60,120}){Model m;m.stats.sanity=0;for(int i=0;i<rate*20;++i)advanceVitals(m,1.f/rate,false);CHECK(m.stats.hp==8);}
    Model m;m.clock=1319.99;CHECK_FALSE(curfew(m.clock));advanceClock(m,.02f);CHECK(curfew(m.clock));
    m.clock=1859.99;CHECK(curfew(m.clock));CHECK(advanceClock(m,.02f));CHECK_FALSE(curfew(m.clock));CHECK(m.day==2);CHECK(m.daysLeft==6);CHECK_FALSE(advanceClock(m,.01f));CHECK(m.daysLeft==6);
    m.clock=1319;advanceClock(m,2);CHECK(m.clock==doctest::Approx(1328.6));
}
TEST_CASE("A full game day takes five active minutes at day and night across frame rates"){
    for(int rate:{30,60,120}){
        Model m;int mornings=0;
        for(int i=0;i<rate*300-1;++i)mornings+=advanceClock(m,1.f/rate);
        CHECK(mornings==0);CHECK(m.day==1);CHECK(m.daysLeft==7);
        REQUIRE(advanceClock(m,1.f/rate));CHECK(m.day==2);CHECK(m.daysLeft==6);CHECK(m.clock==doctest::Approx(1860));
        CHECK_FALSE(advanceClock(m,1.f/rate));CHECK(m.day==2);CHECK(m.daysLeft==6);
    }
    Model day,night;day.clock=420;night.clock=1320;
    advanceClock(day,12.5f);advanceClock(night,12.5f);
    CHECK(day.clock==doctest::Approx(480));CHECK(night.clock==doctest::Approx(1380));
    Model largeStep;largeStep.jobs={true,true,true,true};largeStep.meals={true,true,true};largeStep.books=3;
    REQUIRE(advanceClock(largeStep,300));CHECK(largeStep.clock==doctest::Approx(1860));CHECK(largeStep.day==2);CHECK(largeStep.daysLeft==6);
    CHECK(largeStep.books==0);CHECK(largeStep.jobs==std::array<bool,4>{});CHECK(largeStep.meals==std::array<bool,3>{});
}
namespace {
struct Driver:InputDriver {
    Vec2 move;bool running=false,holdE=false;ButtonAction pressed;int command=0;
    void fill(const Ctx &ctx,std::span<InputFrame,kMaxPlayers> frames) override {auto &f=frames[0];f.clear_devices();const auto &c=ctx.res<Controls>();f.set_axis(c.move,move);if(running)f.hold(c.run);if(holdE)f.hold(c.interact);if(pressed){f.press(pressed);pressed={};}if(command){f.push_intent({str_id("prison.command"),command,0});command=0;}}
};
const char *arguments[]{"prison_tests","--headless","--skip-intro","--step"};
struct Fixture {
    Driver driver;App app{AppConfig{.title="Prison tests",.virtual_size={960,600},.audio=false,.settings_file=""},4,arguments};
    Fixture(){app.add_plugins(DefaultPlugins{.headless=true,.scenes=false});install(app);app.res<Audio>().add_bank(app.res<AssetServer>().load<SoundBank>("sfx/prison.sounds.json"));app.add_system(Stage::Update,updateUI,"test.ui",-700);app.res<InputSystem>().set_driver(&driver);app.step_ticks(1);g().grace=1000;for(auto &b:g().brains)b.wait=1000;}
    ~Fixture(){app.res<InputSystem>().set_driver(nullptr);}
    Game &g(){return app.main_world().resources().get<Game>();}World &w(){return app.main_world();}
    void ticks(int n){app.step_ticks(n);}void at(Vec2 p){placeActor(w(),g().player,p);g().wasWing=p.y<448;}
    void press(ButtonAction a){driver.pressed=a;ticks(1);}void command(int c){driver.command=c;ticks(1);}
    const Controls &controls(){return app.res<Controls>();}
};
}
TEST_CASE("Roster remains exact through defeat and ten-second replacement"){
    Fixture f;auto &g=f.g();CHECK(f.w().count<Actor>()==14);f.at({800,1000});placeActor(f.w(),g.actors[0],{825,1000});g.face={1,0};
    auto punch=[&](){f.press(f.controls().attack);f.ticks(40);};
    f.press(f.controls().attack);CHECK(g.model.inmates[0].hp==4);CHECK(g.model.stats.money==0);f.ticks(40);
    for(int i=0;i<4;++i)punch();CHECK(g.model.inmates[0].hp==0);CHECK(g.model.stats.money==0);CHECK(f.w().count<Actor>()==14);
    auto generation=g.model.inmates[0].generation;float remaining=g.model.inmates[0].respawn;REQUIRE(remaining>0);f.ticks(static_cast<int>(remaining*60)-1);CHECK(g.model.inmates[0].hp==0);f.ticks(3);CHECK(g.model.inmates[0].hp==5);CHECK(g.model.inmates[0].generation==generation+1);CHECK(f.w().count<Actor>()==14);
}
TEST_CASE("Directional knife damage is single-hit and menus capture combat"){
    Fixture f;auto &g=f.g();f.at({800,1000});placeActor(f.w(),g.actors[0],{825,1000});g.model.bag.add(Item::Knife);g.face={-1,0};f.press(f.controls().attack);CHECK(g.model.inmates[0].hp==5);f.ticks(40);g.face={1,0};f.press(f.controls().attack);CHECK(g.model.inmates[0].hp==2);f.ticks(5);CHECK(g.model.inmates[0].hp==2);
    g.menu=Menu::Dealer;f.press(f.controls().attack);CHECK(g.model.inmates[0].hp==2);f.press(f.controls().slots[1]);CHECK(g.model.bag.selected==0);
}
TEST_CASE("Space attacks only with empty hands a knife or a bat"){
    for(Item item:{Item::None,Item::Knife,Item::Bat}){
        Fixture f;auto &g=f.g();f.at({800,1000});placeActor(f.w(),g.actors[0],{825,1000});g.face={1,0};
        if(item!=Item::None)g.model.bag.add(item);g.model.stats.hp=3;g.model.stats.sanity=50;
        f.press(f.controls().attack);CHECK(g.model.inmates[0].hp==5-itemDamage(item));CHECK(g.attackCooldown>0);CHECK(g.attackVisual>0);
        CHECK(g.model.stats.hp==3);CHECK(g.model.stats.sanity==50);CHECK(g.model.bag.held()==item);
    }
}
TEST_CASE("Space uses utilities without damage punch animation or attack cooldown"){
    for(Item item:{Item::Shovel,Item::Medkit,Item::Adrenaline,Item::Rice,Item::Sandwich,Item::Apple,Item::Soup,Item::Ball,Item::Book,Item::LoanShovel,Item::Soda,Item::Chips}){
        Fixture f;auto &g=f.g();f.at({800,1000});placeActor(f.w(),g.actors[0],{825,1000});g.face={1,0};g.model.bag.add(item);g.model.stats.hp=3;g.model.stats.sanity=50;
        f.press(f.controls().attack);CHECK(g.model.inmates[0].hp==5);CHECK(g.attackCooldown==0);CHECK(g.attackVisual==0);CHECK_FALSE(g.incident);
        int expectedHP=item==Item::Medkit?8:item==Item::Adrenaline?5:3;
        int expectedSanity=item==Item::Adrenaline?52:item==Item::Soda||item==Item::Chips?60:50;
        CHECK(g.model.stats.hp==expectedHP);CHECK(g.model.stats.sanity==expectedSanity);
        bool used=item==Item::Medkit||item==Item::Adrenaline||item==Item::Soda||item==Item::Chips;
        CHECK(g.model.bag.held()==(used?Item::None:item));
    }
}
TEST_CASE("A failed full-stat consumable use never falls through to punching"){
    for(Item item:{Item::Medkit,Item::Adrenaline}){
        Fixture f;auto &g=f.g();f.at({800,1000});placeActor(f.w(),g.actors[0],{825,1000});g.face={1,0};g.model.bag.add(item);g.model.stats.sanity=100;
        f.press(f.controls().attack);CHECK(g.model.bag.held()==item);CHECK(g.model.stats.hp==10);CHECK(g.model.stats.sanity==100);
        CHECK(g.model.inmates[0].hp==5);CHECK(g.attackCooldown==0);CHECK(g.attackVisual==0);
    }
}
TEST_CASE("Raw Space consumes once while held and Q no longer uses items"){
    Fixture f;auto &g=f.g();f.at({800,1000});placeActor(f.w(),g.actors[0],{825,1000});g.face={1,0};g.model.bag.add(Item::Medkit);g.model.stats.hp=3;
    f.app.res<InputSystem>().set_driver(nullptr);RawInput raw;raw.set_key(Key::Q,true);f.app.res<InputSystem>().feed_raw(raw);f.ticks(2);
    CHECK(g.model.stats.hp==3);CHECK(g.model.bag.held()==Item::Medkit);CHECK(g.model.inmates[0].hp==5);
    raw.set_key(Key::Q,false);raw.set_key(Key::Space,true);f.app.res<InputSystem>().feed_raw(raw);f.ticks(2);
    CHECK(g.model.stats.hp==8);CHECK(g.model.bag.held()==Item::None);CHECK(g.model.inmates[0].hp==5);CHECK(g.attackCooldown==0);
    f.ticks(60);CHECK(g.model.inmates[0].hp==5);CHECK(g.attackCooldown==0);
    raw.set_key(Key::Space,false);f.app.res<InputSystem>().feed_raw(raw);f.ticks(1);
    raw.set_key(Key::Space,true);f.app.res<InputSystem>().feed_raw(raw);f.ticks(1);CHECK(g.model.inmates[0].hp==4);
}
TEST_CASE("Space cancels a timed activity without consuming on the same press"){
    Fixture f;auto &g=f.g();g.model.bag.add(Item::Medkit);g.model.stats.hp=3;f.at({920,624});f.press(f.controls().interact);REQUIRE(g.activity==Activity::Read);
    f.press(f.controls().attack);CHECK(g.activity==Activity::None);CHECK(g.model.stats.hp==3);CHECK(g.model.bag.held()==Item::Medkit);CHECK(g.attackCooldown==0);
    f.press(f.controls().attack);CHECK(g.model.stats.hp==8);CHECK(g.model.bag.held()==Item::None);CHECK(g.attackCooldown==0);
}
TEST_CASE("Simultaneous E and Space start only the seat interaction"){
    Fixture f;auto &g=f.g();g.model.bag.add(Item::Rice);f.at({288,608});
    f.app.res<InputSystem>().set_driver(nullptr);RawInput raw;raw.set_key(Key::E,true);raw.set_key(Key::Space,true);f.app.res<InputSystem>().feed_raw(raw);f.ticks(1);
    CHECK(g.activity==Activity::Eat);CHECK(g.model.bag.has(Item::Rice));CHECK(g.attackCooldown==0);CHECK(g.attackVisual==0);
}
TEST_CASE("Modal map pauses both clocks, drains and respawn"){
    Fixture f;auto &g=f.g();g.model.inmates[0].hp=0;g.model.inmates[0].respawn=10;g.menu=Menu::Map;double clock=g.model.clock;float sanity=g.model.stats.sanity;f.ticks(600);CHECK(g.model.clock==clock);CHECK(g.model.stats.sanity==sanity);CHECK(g.model.inmates[0].respawn==10);CHECK(g.model.stats.hp==10);
}
TEST_CASE("Continuous reading cancels without reward and caps at three"){
    Fixture f;auto &g=f.g();f.at({920,624});f.press(f.controls().interact);REQUIRE(g.activity==Activity::Read);f.ticks(100);f.driver.move={1,0};f.ticks(1);CHECK(g.activity==Activity::None);CHECK(g.model.books==0);CHECK(g.model.stats.intelligence==0);f.driver.move={};
    for(int i=0;i<3;++i){f.at({920,624});f.press(f.controls().interact);REQUIRE(g.activity==Activity::Read);f.ticks(598);CHECK(g.model.books==i);f.ticks(2);CHECK(g.model.books==i+1);}
    f.press(f.controls().interact);CHECK(g.activity==Activity::None);CHECK(g.model.books==3);g.model.clock=1859.99;f.ticks(1);CHECK(g.model.books==0);CHECK(g.model.day==2);f.ticks(1);CHECK(g.model.day==2);
}
TEST_CASE("Library chair pairs leave shelf reading accessible without eating cafeteria food"){
    Fixture f;auto &g=f.g();g.model.bag.add(Item::Rice);
    for(Vec2 shelf:std::array<Vec2,2>{{{1000,624},{1160,624}}}){
        f.at(shelf);f.press(f.controls().interact);REQUIRE(g.activity==Activity::Read);CHECK(g.menu==Menu::None);
        int books=g.model.books;f.ticks(601);CHECK(g.model.books==books+1);CHECK(g.model.bag.has(Item::Rice));CHECK(g.model.stats.hp==10);
    }
    CHECK(g.model.stats.intelligence==20);
}
TEST_CASE("Sanctioned bout uses an existing inmate and pays once"){
    Fixture f;auto &g=f.g();g.menu=Menu::ConfirmRing;f.command(43);REQUIRE(g.ringSlot>=0);auto slot=static_cast<std::size_t>(g.ringSlot);CHECK(f.w().count<Actor>()==14);g.model.bag.add(Item::Knife);placeActor(f.w(),g.actors[slot],{1025,1064});f.press(f.controls().attack);CHECK(g.model.inmates[slot].hp==5);g.model.bag.remove(Item::Knife);g.face={1,0};
    for(int i=0;i<5;++i){placeActor(f.w(),g.actors[slot],{1025,1064});f.press(f.controls().attack);f.ticks(40);}
    CHECK(g.ringSlot==-1);CHECK(g.ringReward>=30);CHECK(g.ringReward<=50);CHECK(g.model.stats.money==g.ringReward);CHECK(g.model.daysLeft==7);int money=g.model.stats.money;f.ticks(120);CHECK(g.model.stats.money==money);CHECK(g.model.ringCooldown>0);
}
TEST_CASE("Daily jobs reward actual completion once and sentence bonus is bounded"){
    Model m;CHECK_FALSE(finishJob(m,1).ok);CHECK(m.stats.money==0);CHECK(finishJob(m,0).ok);CHECK_FALSE(finishJob(m,0).ok);m.bag.add(Item::Book);CHECK(finishJob(m,1).ok);CHECK(finishJob(m,2).ok);CHECK(m.stats.money==55);CHECK(m.daysLeft==6);CHECK(m.bonusToday);m.bag.add(Item::LoanShovel);m.loan=true;CHECK(finishJob(m,3).ok);CHECK(m.stats.money==75);CHECK(m.daysLeft==6);CHECK_FALSE(m.bag.has(Item::LoanShovel));
    confirmMisconduct(m);confirmMisconduct(m);confirmMisconduct(m);CHECK(m.daysLeft==8);CHECK(m.penalties==2);
}
TEST_CASE("Versioned checkpoint round trip preserves all model data and retry rolls back"){
    Model m;newModel(m,42);m.stats.hp=7;m.stats.money=174;m.stats.intelligence=30;m.stats.reputation=8;m.bag.add(Item::Knife);m.bag.add(Item::LoanShovel);m.bag.selected=1;m.inmates[2]={0,3,true,4};m.clock=1860;m.day=2;m.daysLeft=5;m.meals[1]=true;m.jobs[0]=true;m.penalties=1;m.books=2;m.loan=true;m.bonusToday=true;m.batTaken=true;m.bookTaken=true;m.gardenTended=true;m.drops.push_back({Item::Medkit,{640,904}});m.sanityTimer=4.3;m.zeroTimer=2.1;m.rng.next_u32();
    auto encoded=encodeModel(m);auto decoded=decodeModel(encoded);REQUIRE(decoded);CHECK(encodeModel(*decoded)==encoded);CHECK_FALSE(decodeModel("{\"stats\":{}}").ok());
    Game disk;disk.model=m;disk.saveDir="/tmp/prison-life-test-save";REQUIRE(saveCheckpoint(disk));disk.model=Model{};REQUIRE(loadCheckpoint(disk));CHECK(encodeModel(disk.model)==encoded);
    Fixture f;auto &g=f.g();g.checkpoint=m;g.model.stats.hp=0;f.ticks(1);REQUIRE(g.phase==Phase::Infirmary);f.command(4);CHECK(g.phase==Phase::Play);CHECK(g.model.stats.money==174);CHECK(g.model.stats.hp==7);CHECK(g.model.hotspot==m.hotspot);CHECK(g.model.bag.count()==2);CHECK(f.w().count<Actor>()==14);
}
TEST_CASE("Rest crosses morning once without healing and release ending fires"){
    Fixture f;auto &g=f.g();g.model.clock=1321;g.model.daysLeft=1;g.model.stats.hp=6;f.at({152,152});f.press(f.controls().interact);REQUIRE(g.activity==Activity::Rest);f.ticks(92);CHECK(g.phase==Phase::Released);CHECK(g.model.daysLeft==0);CHECK(g.model.day==2);CHECK(g.model.stats.hp==6);f.ticks(60);CHECK(g.model.day==2);
}
TEST_CASE("Escape requires five separate uninterrupted eight-second holds"){
    Fixture f;auto &g=f.g();auto at=g.map.cells[static_cast<std::size_t>(g.model.hotspot)].hotspot;f.at(at);f.press(f.controls().interact);CHECK(g.activity==Activity::None);
    g.model.bag.add(Item::Shovel);f.driver.holdE=true;f.press(f.controls().interact);REQUIRE(g.activity==Activity::Dig);
    // Isolate hold timing; patrol geometry and visibility are covered independently below.
    for(int i=10;i<13;++i){placeActor(f.w(),g.actors[static_cast<std::size_t>(i)],{640,1100});g.brains[static_cast<std::size_t>(i)].wait=10000;}
    f.ticks(200);f.driver.holdE=false;f.ticks(1);CHECK(g.phase==Phase::Play);CHECK(g.activity==Activity::None);CHECK(g.model.digSteps==0);
    for(int step=1;step<=5;++step){f.driver.holdE=true;f.press(f.controls().interact);REQUIRE(g.activity==Activity::Dig);f.ticks(478);CHECK(g.model.digSteps==step-1);f.ticks(3);CHECK(g.model.digSteps==step);
        if(step<5){CHECK(g.phase==Phase::Play);f.ticks(500);CHECK(g.model.digSteps==step);f.press(f.controls().interact);CHECK(g.activity==Activity::None);f.driver.holdE=false;f.ticks(1);}
    }CHECK(g.phase==Phase::Escaped);
}
TEST_CASE("Real UiTree pointer click buys once and keyboard confirmation cannot punch through menu"){
    Fixture f;auto &g=f.g();g.model.stats.money=180;g.menu=Menu::Dealer;f.ticks(2);auto &ui=f.w().resources().get<UiTree>();auto buy=ui.find(str_id("Buy"));REQUIRE(buy!=0);auto rect=ui.screen_rect(buy);CHECK(rect.w>100);CHECK(rect.h>20);
    RawInput raw;raw.mouse_valid=true;raw.mouse_fb=virtual_to_framebuffer(f.app.res<VirtualLayout>(),rect.center());raw.mouse_down=1;f.app.res<InputSystem>().feed_raw(raw);f.ticks(1);raw.mouse_down=0;f.app.res<InputSystem>().feed_raw(raw);f.ticks(2);CHECK(g.model.stats.money==80);CHECK(g.model.bag.has(Item::Knife));CHECK(g.model.bag.count()==1);f.ticks(20);CHECK(g.model.stats.money==80);
    f.app.res<InputSystem>().set_driver(nullptr);raw.set_key(Key::Space,true);f.app.res<InputSystem>().feed_raw(raw);f.ticks(2);raw.set_key(Key::Space,false);f.app.res<InputSystem>().feed_raw(raw);f.ticks(2);CHECK(g.attackCooldown==0);CHECK(g.model.stats.money==80);CHECK(g.menu==Menu::Dealer);
}
TEST_CASE("Movement normalizes diagonals and collisions stop wall running drain"){
    Fixture f;auto &g=f.g();f.at({640,1000});auto before=f.w().get<Transform>(g.player).position;f.driver.move={1,1};f.ticks(60);float traveled=distance(before,f.w().get<Transform>(g.player).position);CHECK(traveled>86);CHECK(traveled<92);
    f.at({168,110});f.driver.move={0,-1};f.driver.running=true;f.ticks(120);auto pos=f.w().get<Transform>(g.player).position;CHECK_FALSE(box_blocked(f.w().resources().get<TileWorld>(),Rect::from_center(pos,{5,5})));float stamina=g.model.stats.agility;f.ticks(120);CHECK(g.model.stats.agility>=stamina);CHECK(pos.y==doctest::Approx(f.w().get<Transform>(g.player).position.y));
}
TEST_CASE("Witnessed misconduct confiscates implicated item and applies only one incident"){
    Fixture f;auto &g=f.g();f.at({640,704});placeActor(f.w(),g.actors[0],{665,704});placeActor(f.w(),g.actors[10],{640,750});g.model.bag.add(Item::Knife);g.face={1,0};f.press(f.controls().attack);REQUIRE(g.incident);f.ticks(210);CHECK_FALSE(g.incident);CHECK(g.model.daysLeft==8);CHECK(g.model.penalties==1);CHECK_FALSE(g.model.bag.has(Item::Knife));CHECK_FALSE(g.model.curfewResolved);CHECK(distance_sq(f.w().get<Transform>(g.player).position,playerHome)<40*40);f.ticks(120);CHECK(g.model.penalties==1);
}
TEST_CASE("Curfew warns then confronts once while routes remain open"){
    Fixture f;auto &g=f.g();g.model.clock=1320;f.at({640,904});placeActor(f.w(),g.actors[10],{640,944});f.ticks(45);CHECK_FALSE(g.incident);f.ticks(200);CHECK(g.model.curfewResolved);CHECK(g.model.penalties==1);CHECK(g.model.daysLeft==8);CHECK(g.phase==Phase::Play);
}
TEST_CASE("One-second wing overview triggers once per outward crossing"){
    Fixture f;auto &g=f.g();f.at({640,440});g.wasWing=true;f.driver.move={0,1};f.ticks(10);CHECK(g.overview>0);f.driver.move={};f.ticks(65);CHECK(g.overview==0);f.ticks(60);CHECK(g.overview==0);
}
TEST_CASE("Every roaming actor stays outside solid tiles"){
    Fixture f;auto &g=f.g();for(auto &b:g.brains)b.wait=0;g.model.stats.reputation=12;std::array<Vec2,13> initial;for(std::size_t i=0;i<13;++i)initial[i]=f.w().get<Transform>(g.actors[i]).position;f.ticks(1800);int moved=0;auto &tiles=f.w().resources().get<TileWorld>();for(std::size_t i=0;i<13;++i){auto p=f.w().get<Transform>(g.actors[i]).position;CHECK_FALSE(box_blocked(tiles,Rect::from_center(p,{5,5})));if(distance_sq(p,initial[i])>10*10)++moved;}CHECK(moved>=10);CHECK(f.w().get<Transform>(g.actors[9]).position==dealerSpot);
}
TEST_CASE("Meals reserve allowance once and interruptions preserve carried food"){
    Fixture f;auto &g=f.g();f.at({136,568});f.press(f.controls().interact);REQUIRE(g.menu==Menu::None);CHECK(g.model.bag.has(Item::Rice));CHECK(g.model.meals[0]);CHECK_FALSE(takeMeal(g.model,1).ok);int hp=g.model.stats.hp;f.at({288,608});f.press(f.controls().interact);REQUIRE(g.activity==Activity::Eat);f.driver.move={1,0};f.ticks(1);CHECK(g.activity==Activity::None);CHECK(g.model.bag.has(Item::Rice));f.driver.move={};f.at({288,608});f.press(f.controls().interact);f.ticks(122);CHECK_FALSE(g.model.bag.has(Item::Rice));CHECK(g.model.stats.hp==hp);
}
TEST_CASE("The continuous cafeteria counter gives exactly the food beside the player"){
    for(int food=0;food<4;++food){
        Fixture f;auto &g=f.g();f.at({136,static_cast<float>(568+food*64)});f.ticks(1);
        REQUIRE(g.focusProp>=0);CHECK(g.map.props[static_cast<std::size_t>(g.focusProp)].index==food);
        CHECK(g.prompt=="E / Take "+std::string(cfg::foods[static_cast<std::size_t>(food)]));
        f.press(f.controls().interact);CHECK(g.menu==Menu::None);CHECK(g.model.bag.count()==1);
        CHECK(g.model.bag.held()==static_cast<Item>(static_cast<int>(Item::Rice)+food));CHECK(g.model.meals[0]);CHECK(g.model.stats.money==0);
        f.at({136,static_cast<float>(568+((food+1)%4)*64)});f.press(f.controls().interact);CHECK(g.model.bag.count()==1);CHECK(g.menu==Menu::None);
        std::vector<Prop> counter;for(auto p:g.map.props)if(p.kind==PropKind::Counter)counter.push_back(p);
        REQUIRE(counter.size()==5);for(std::size_t i=1;i<counter.size();++i){CHECK(counter[i].box.x==counter[0].box.x);CHECK(counter[i].box.w==counter[0].box.w);CHECK(counter[i-1].box.y+counter[i-1].box.h==counter[i].box.y);}
    }
}
TEST_CASE("Direct food pickup preserves meal hours capacity and allowance"){
    Fixture f;auto &g=f.g();f.at({136,696});g.model.bag.add(Item::Knife);g.model.bag.add(Item::Bat);g.model.bag.add(Item::Soda);
    f.press(f.controls().interact);CHECK(g.model.bag.count()==3);CHECK_FALSE(g.model.meals[0]);CHECK(g.menu==Menu::None);
    g.model.bag.eraseSelected();g.model.clock=600;f.press(f.controls().interact);CHECK(g.model.bag.count()==2);CHECK(g.model.meals==std::array<bool,3>{});CHECK(g.menu==Menu::None);
    g.model.clock=720;f.press(f.controls().interact);CHECK(g.model.bag.held()==Item::Apple);CHECK(g.model.meals[1]);CHECK_FALSE(g.model.meals[0]);CHECK(g.model.bag.has(Item::Soda));CHECK(g.menu==Menu::None);
}
TEST_CASE("Serving work stays available at the end of the cafeteria counter"){
    Fixture f;auto &g=f.g();f.at({136,816});f.press(f.controls().interact);REQUIRE(g.activity==Activity::Serve);CHECK(g.menu==Menu::None);CHECK(g.model.bag.count()==0);CHECK_FALSE(g.model.meals[0]);
    f.ticks(301);CHECK(g.model.jobs[2]);CHECK(g.model.stats.money==20);CHECK(g.model.conduct==1);CHECK_FALSE(g.model.meals[0]);
    f.press(f.controls().interact);CHECK(g.activity==Activity::None);CHECK(g.model.stats.money==20);
    g.model.jobs[2]=false;g.model.clock=600;f.press(f.controls().interact);CHECK(g.activity==Activity::None);CHECK_FALSE(g.model.jobs[2]);
}
TEST_CASE("Dealer recovery avoids a player camping the service spot"){
    Fixture f;auto &g=f.g();f.at(dealerSpot-Vec2{0,25});placeActor(f.w(),g.actors[9],dealerSpot);g.model.bag.add(Item::Knife);g.face={0,1};f.press(f.controls().attack);f.ticks(40);f.press(f.controls().attack);REQUIRE(g.model.inmates[9].hp==0);f.at(dealerSpot);f.ticks(602);CHECK(g.model.inmates[9].hp==5);CHECK(g.model.inmates[9].generation==1);CHECK(g.model.inmates[9].appearance>0);CHECK(distance_sq(f.w().get<Transform>(g.actors[9]).position,f.w().get<Transform>(g.player).position)>=24*24);CHECK(f.w().count<Actor>()==14);
}
TEST_CASE("Vending purchases put one snack in the hand without consuming it"){
    for(Item expected:{Item::Soda,Item::Chips}){
        Fixture f;auto &g=f.g();g.menu=Menu::Vending;g.model.stats.money=10;g.model.stats.sanity=80;
        g.model.bag.add(Item::Knife);g.model.bag.add(Item::Medkit);
        for(std::uint64_t seed=1;;++seed){g.model.rng.reseed(seed);auto probe=g.model.rng;if((probe.chance(.5f)?Item::Soda:Item::Chips)==expected)break;}
        f.ticks(2);auto &ui=f.w().resources().get<UiTree>();auto buy=ui.find(str_id("Pick up snack / $5"));REQUIRE(buy!=0);
        RawInput raw;raw.mouse_valid=true;raw.mouse_fb=virtual_to_framebuffer(f.app.res<VirtualLayout>(),ui.screen_rect(buy).center());raw.mouse_down=1;f.app.res<InputSystem>().feed_raw(raw);f.ticks(1);raw.mouse_down=0;f.app.res<InputSystem>().feed_raw(raw);f.ticks(2);
        CHECK(g.model.stats.money==5);CHECK(g.model.stats.sanity==80);CHECK(g.model.stats.hp==10);
        CHECK(g.model.bag.count()==3);CHECK(g.model.bag.selected==2);CHECK(g.model.bag.held()==expected);CHECK(g.model.bag.has(Item::Knife));CHECK(g.model.bag.has(Item::Medkit));CHECK(g.menu==Menu::None);
        f.ticks(20);CHECK(g.model.stats.money==5);CHECK(g.model.bag.held()==expected);
        f.at({800,1000});g.model.clock=1321;f.press(f.controls().attack);
        CHECK(g.model.stats.sanity==90);CHECK(g.model.stats.hp==10);CHECK(g.model.bag.held()==Item::None);CHECK(g.model.bag.count()==2);CHECK(g.model.stats.money==5);
    }
}
TEST_CASE("Failed vending purchases preserve funds inventory and RNG"){
    Fixture f;auto &g=f.g();g.menu=Menu::Vending;g.model.stats.money=4;g.model.stats.sanity=90;
    auto unchanged=encodeModel(g.model);f.command(60);CHECK(encodeModel(g.model)==unchanged);CHECK(g.menu==Menu::Vending);
    g.model.stats.money=5;g.model.bag.add(Item::Knife);g.model.bag.add(Item::Bat);g.model.bag.add(Item::Medkit);
    unchanged=encodeModel(g.model);f.command(60);CHECK(encodeModel(g.model)==unchanged);CHECK(g.menu==Menu::Vending);
    g.model.bag.eraseSelected();g.model.clock=1321;unchanged=encodeModel(g.model);f.command(60);CHECK(encodeModel(g.model)==unchanged);
}
TEST_CASE("Portable snacks remain separate from cafeteria meals and cap sanity"){
    for(Item snack:{Item::Soda,Item::Chips}){
        Model m;m.clock=1321;m.stats.hp=4;m.stats.sanity=95;m.bag.add(snack);m.bag.add(Item::Rice);
        CHECK_FALSE(isFood(snack));
        REQUIRE(consume(m).ok);CHECK(m.stats.sanity==100);CHECK(m.stats.hp==4);CHECK(m.bag.has(Item::Rice));CHECK_FALSE(m.bag.has(snack));
        m.bag.selected=1;CHECK_FALSE(consume(m).ok);CHECK(m.bag.has(Item::Rice));
        m.bag.selected=0;m.bag.add(snack);CHECK(consume(m).ok);CHECK(m.stats.sanity==100);CHECK_FALSE(m.bag.has(snack));
        Model onlySnack;onlySnack.bag.add(snack);CHECK_FALSE(eatMeal(onlySnack).ok);CHECK(onlySnack.bag.has(snack));
        Model mealWindow;mealWindow.bag.add(snack);CHECK(takeMeal(mealWindow,0).ok);CHECK(mealWindow.bag.has(snack));CHECK(mealWindow.bag.has(Item::Rice));
    }
}
TEST_CASE("Portable vending snacks survive checkpoints and dropping and recovery"){
    Model m;m.bag.add(Item::Soda);m.bag.add(Item::Chips);m.bag.selected=1;m.drops.push_back({Item::Soda,{800,1018}});m.drops.push_back({Item::Chips,{816,1018}});
    auto encoded=encodeModel(m);auto decoded=decodeModel(encoded);REQUIRE(decoded);CHECK(encodeModel(*decoded)==encoded);
    Fixture f;auto &g=f.g();f.at({800,1000});g.model.bag.add(Item::Chips);f.press(f.controls().drop);REQUIRE(g.menu==Menu::Drop);f.command(50);REQUIRE(g.model.drops.size()==1);CHECK(g.model.drops[0].item==Item::Chips);CHECK_FALSE(g.model.bag.has(Item::Chips));f.press(f.controls().interact);CHECK(g.model.bag.has(Item::Chips));CHECK(g.model.drops.empty());
}
TEST_CASE("Telegraphed strikes are directional and simultaneous hits respect invulnerability"){
    Fixture f;auto &g=f.g();f.at({800,1000});placeActor(f.w(),g.actors[0],{825,1000});g.brains[0].hostile=true;g.brains[0].telegraph=.01f;f.w().get<Actor>(g.actors[0]).facing={1,0};g.invuln=0;f.ticks(1);CHECK(g.model.stats.hp==10);
    for(int i=0;i<2;++i){placeActor(f.w(),g.actors[static_cast<std::size_t>(i)],{825,1000});g.brains[static_cast<std::size_t>(i)].hostile=true;g.brains[static_cast<std::size_t>(i)].telegraph=.01f;f.w().get<Actor>(g.actors[static_cast<std::size_t>(i)]).facing={-1,0};}f.ticks(1);CHECK(g.model.stats.hp==9);f.ticks(20);CHECK(g.model.stats.hp==9);
}
TEST_CASE("An officer seeing an ongoing dig restarts the entire run and checkpoint"){
    Fixture f;auto &g=f.g();g.model.hotspot=0;f.at(g.map.cells[0].hotspot);g.model.bag.add(Item::Shovel);g.model.stats.money=200;g.model.digSteps=3;g.checkpoint=g.model;
    f.driver.holdE=true;f.press(f.controls().interact);REQUIRE(g.activity==Activity::Dig);auto at=g.map.cells[0].hotspot;placeActor(f.w(),g.actors[10],at+Vec2{0,24});f.w().get<Actor>(g.actors[10]).facing={0,-1};f.ticks(1);
    CHECK(g.phase==Phase::Court);CHECK(g.model.digSteps==0);CHECK(g.model.stats.money==0);CHECK(g.model.bag.count()==0);CHECK(g.model.day==1);CHECK(g.checkpoint.bag.count()==0);CHECK_FALSE(g.escapePatrol);CHECK(g.activity==Activity::None);
}
TEST_CASE("Confirmed drop is recoverable and a full bag never deletes a ground item"){
    Fixture f;auto &g=f.g();f.at({800,1000});g.model.bag.add(Item::Knife);f.press(f.controls().drop);CHECK(g.menu==Menu::Drop);CHECK(g.model.bag.has(Item::Knife));f.command(50);REQUIRE(g.model.drops.size()==1);CHECK_FALSE(g.model.bag.has(Item::Knife));f.press(f.controls().interact);CHECK(g.model.bag.has(Item::Knife));CHECK(g.model.drops.empty());g.model.bag.add(Item::Bat);g.model.bag.add(Item::Medkit);g.model.drops.push_back({Item::Shovel,{800,1018}});f.press(f.controls().interact);CHECK(g.model.drops.size()==1);CHECK(g.model.bag.count()==3);
}
TEST_CASE("The dealer is an inmate and also grants reputation once through his menu"){
    Fixture f;auto &g=f.g();g.menu=Menu::Dealer;f.command(21);CHECK(g.model.inmates[9].helped);CHECK(g.model.stats.reputation==2);f.command(21);CHECK(g.model.stats.reputation==2);CHECK(g.menu==Menu::Dealer);
}
TEST_CASE("ElevenLabs recordings are real loaded audio and its panel pauses gameplay"){
    Fixture f;auto &b=f.w().resources().get<Broadcast>();REQUIRE(b.lines.size()==8);
    for(auto &line:b.lines){CHECK(line.seconds>1);CHECK(line.seconds<30);auto *clip=f.app.res<AssetServer>().get(line.clip);REQUIRE(clip);CHECK(clip->device_id>0);}
    CHECK(f.app.res<Audio>().has_group(str_id("eleven_door")));
    f.press(f.controls().audio);REQUIRE(f.g().menu==Menu::Audio);auto clock=f.g().model.clock;auto sanity=f.g().model.stats.sanity;f.ticks(600);
    CHECK(f.g().model.clock==clock);CHECK(f.g().model.stats.sanity==sanity);CHECK(f.g().model.daysLeft==7);
    f.press(f.controls().audio);CHECK(f.g().menu==Menu::None);
}
TEST_CASE("Audio showcase pointer click plays a voice without spending game money or API credits"){
    Fixture f;auto &g=f.g();auto &b=f.w().resources().get<Broadcast>();f.press(f.controls().audio);f.ticks(1);
    auto &ui=f.w().resources().get<UiTree>();auto call=ui.find(str_id("Play call from home"));REQUIRE(call);
    RawInput raw;raw.mouse_valid=true;raw.mouse_fb=virtual_to_framebuffer(f.app.res<VirtualLayout>(),ui.screen_rect(call).center());raw.mouse_down=1;f.app.res<InputSystem>().feed_raw(raw);f.ticks(1);raw.mouse_down=0;f.app.res<InputSystem>().feed_raw(raw);f.ticks(2);
    CHECK(b.speaker=="A CALL FROM HOME");CHECK(b.remaining>0);CHECK(b.voice);CHECK(g.model.stats.money==0);CHECK(b.liveRequests==0);CHECK_FALSE(b.busy);
    f.command(72);CHECK_FALSE(b.enabled);CHECK(b.subtitle.empty());CHECK(b.pending.empty());CHECK(f.app.res<Audio>().bus_volume(b.bus)==0);
    f.command(72);CHECK(b.enabled);CHECK(f.app.res<Audio>().bus_volume(b.bus)==1);
}
TEST_CASE("Automated runs cannot accidentally spend ElevenLabs credits"){
    Fixture f;auto &b=f.w().resources().get<Broadcast>();f.press(f.controls().audio);f.command(71);
    CHECK(b.liveRequests==0);CHECK_FALSE(b.busy);CHECK(b.child==-1);CHECK(b.liveStatus=="Live API calls are disabled in automated tests.");
    f.command(73);CHECK(b.speaker=="BLACKWATER PA");CHECK(b.liveRequests==0);
}
TEST_CASE("Voiced phone calls use the phone and movement cancels narration"){
    Fixture f;auto &g=f.g();auto &b=f.w().resources().get<Broadcast>();f.at({704,560});f.press(f.controls().interact);
    REQUIRE(g.activity==Activity::Phone);CHECK(b.speaker=="A CALL FROM HOME");CHECK(g.duration==doctest::Approx(b.remaining).epsilon(.01));
    f.driver.move={1,0};f.ticks(1);CHECK(g.activity==Activity::None);CHECK(b.subtitle.empty());CHECK(b.pending.empty());CHECK(g.model.stats.money==0);
}
TEST_CASE("Meal and curfew voices trigger once and endings have priority"){
    Fixture f;auto &g=f.g();auto &b=f.w().resources().get<Broadcast>();b.pending.clear();b.remaining=0;b.subtitle.clear();
    f.ticks(120);CHECK(b.pending.empty());CHECK(b.subtitle.empty());
    g.model.clock=720;f.ticks(1);CHECK(b.speaker=="BLACKWATER PA");auto mealStamp=b.lastMeal;
    f.ticks(600);CHECK(b.lastMeal==mealStamp);CHECK(b.pending.empty());CHECK(b.subtitle.empty());
    g.model.clock=1290;f.ticks(1);CHECK(b.speaker=="OFFICER HALE");CHECK(b.curfewDay==g.model.day);
    g.model.daysLeft=0;f.ticks(1);CHECK(g.phase==Phase::Released);CHECK(b.speaker=="OFFICER HALE");CHECK(b.subtitle.find("sentence is served")!=std::string::npos);CHECK(b.pending.empty());
}

TEST_CASE("Shovel ambushes cycle through solo pairs and trios even at maximum reputation"){
    Fixture f;auto &g=f.g();f.at({800,1000});g.grace=0;g.model.stats.reputation=12;g.model.bag.add(Item::Shovel);
    for(int count=1;count<=3;++count){g.waveTimer=0;if(count==1){f.ticks(1);g.waveTimer=0;}f.ticks(1);int attacking=0;for(int i=0;i<9;++i)attacking+=g.brains[static_cast<std::size_t>(i)].hostile&&g.brains[static_cast<std::size_t>(i)].raider;CHECK(attacking==count);CHECK_FALSE(g.brains[9].raider);
        for(int i=0;i<9;++i)g.brains[static_cast<std::size_t>(i)]={};
    }
    g.model.bag.remove(Item::Shovel);f.ticks(1);CHECK_FALSE(g.shovelPressure);CHECK_FALSE(g.escapePatrol);
}
TEST_CASE("Replacement inmates respawn one and a half times faster during shovel attacks"){
    Fixture f;auto &g=f.g();g.model.bag.add(Item::Shovel);f.at({800,1000});g.model.inmates[0].hp=0;g.model.inmates[0].respawn=10;f.ticks(399);CHECK(g.model.inmates[0].hp==0);CHECK(g.model.inmates[0].respawn==doctest::Approx(.025).epsilon(.01));f.ticks(2);CHECK(g.model.inmates[0].hp==5);CHECK(g.model.inmates[0].generation==1);
    g.model.inmates[1].hp=0;g.model.inmates[1].respawn=10;f.at(g.map.cells[static_cast<std::size_t>(g.model.hotspot)].hotspot);f.ticks(60);CHECK(g.escapePatrol);CHECK(g.model.inmates[1].respawn==doctest::Approx(9));
}
TEST_CASE("Arrival at target cell clears attackers and brings every guard onto patrol"){
    Fixture f;auto &g=f.g();g.model.bag.add(Item::LoanShovel);g.model.loan=true;g.grace=0;f.at({800,1000});f.ticks(1);g.waveTimer=0;f.ticks(1);REQUIRE(g.waveNumber==1);
    f.at(g.map.cells[static_cast<std::size_t>(g.model.hotspot)].hotspot);f.ticks(1);REQUIRE(g.escapePatrol);CHECK_FALSE(g.incident);
    for(int i=0;i<9;++i){CHECK_FALSE(g.brains[static_cast<std::size_t>(i)].hostile);CHECK_FALSE(g.brains[static_cast<std::size_t>(i)].raider);}
    for(int i=10;i<13;++i)CHECK(g.brains[static_cast<std::size_t>(i)].route.count>0);
    f.ticks(180);CHECK_FALSE(g.incident);CHECK(g.model.bag.has(Item::LoanShovel));
}
TEST_CASE("Guard digging sight needs facing range and an unobstructed ray"){
    Fixture f;auto &g=f.g();f.at({640,700});auto e=g.actors[10];placeActor(f.w(),e,{640,750});f.w().get<Actor>(e).facing={0,1};
    f.app.add_system(Stage::Simulate,[](Ctx &ctx){auto &g=ctx.local<Game>();auto &w=ctx.world();auto e=g.actors[10];
        CHECK_FALSE(guardSeesPlayer(ctx,10));w.get<Actor>(e).facing={0,-1};CHECK(guardSeesPlayer(ctx,10));
        placeActor(w,e,{640,1000});CHECK_FALSE(guardSeesPlayer(ctx,10));
        placeActor(w,g.player,{192,160});placeActor(w,e,{232,160});w.get<Actor>(e).facing={-1,0};CHECK_FALSE(guardSeesPlayer(ctx,10));
    },"test.sight",500);f.ticks(1);
}
TEST_CASE("Dig progress survives checkpoint round trips and old checkpoints default to zero"){
    Model m;m.digSteps=4;auto encoded=encodeModel(m);auto result=decodeModel(encoded);REQUIRE(result);CHECK(result->digSteps==4);
    auto old=encoded;auto start=old.find("\"digSteps\":4,");REQUIRE(start!=std::string::npos);old.erase(start,std::string("\"digSteps\":4,").size());result=decodeModel(old);REQUIRE(result);CHECK(result->digSteps==0);
}
TEST_CASE("Every seeded cell has reachable guard rounds and an eight-second unseen window"){
    for(int cell=0;cell<12;++cell){Fixture f;auto &g=f.g();g.model.hotspot=cell;g.model.bag.add(Item::Shovel);g.model.clock=420;f.at(g.map.cells[static_cast<std::size_t>(cell)].hotspot);f.ticks(1);REQUIRE(g.escapePatrol);
        float unseen=0,best=0;bool clear=true,everWatched=false;std::array<bool,3> arrived{};auto hotspot=g.map.cells[static_cast<std::size_t>(cell)].hotspot;
        for(int t=0;t<7200;++t){f.ticks(1);bool watched=false;for(int i=10;i<13;++i){auto e=g.actors[static_cast<std::size_t>(i)];auto at=f.w().get<Transform>(e).position;auto facing=f.w().get<Actor>(e).facing;
                clear&=!box_blocked(f.w().resources().get<TileWorld>(),Rect::from_center(at,{5,5}));
                if(distance_sq(at,hotspot)<160*160)arrived[static_cast<std::size_t>(i-10)]=true;
                watched|=distance_sq(at,hotspot)<cfg::officerSight*cfg::officerSight&&dot(normalize_or(hotspot-at,facing),facing)>=.65f&&tile_ray_clear(f.w().resources().get<TileWorld>(),at,hotspot);
            }
            everWatched|=watched;
            if(t>3600){unseen=watched?0:unseen+1.f/60;best=std::max(best,unseen);}
        }
        INFO("cell "<<cell<<" safe window "<<best);CHECK(clear);CHECK(everWatched);CHECK(best>=cfg::digSeconds);CHECK(arrived==std::array<bool,3>{true,true,true});CHECK(g.phase==Phase::Play);
    }
}

TEST_CASE("Caught digging overwrites the old disk checkpoint with a fresh run"){
    Fixture f;auto &g=f.g();g.saveDir="/tmp/prison-life-caught-save";g.scripted=false;g.model.stats.money=250;g.model.bag.add(Item::Shovel);g.model.digSteps=4;g.model.hotspot=0;g.checkpoint=g.model;REQUIRE(saveCheckpoint(g));
    auto at=g.map.cells[0].hotspot;f.at(at);placeActor(f.w(),g.actors[10],at+Vec2{0,24});f.w().get<Actor>(g.actors[10]).facing={0,-1};f.driver.holdE=true;f.press(f.controls().interact);
    REQUIRE(g.phase==Phase::Court);CHECK(g.saved);Game disk;disk.saveDir=g.saveDir;REQUIRE(loadCheckpoint(disk));CHECK(disk.model.bag.count()==0);CHECK(disk.model.digSteps==0);CHECK(disk.model.stats.money==0);CHECK(disk.model.daysLeft==7);
}
