#include "game.h"
#include <lam/scene/save.h>
namespace prison {
std::string encodeModel(const Model &m){lam::DataWriter w;w.begin_object();
    w.key("stats").begin_object().key("hp").value(m.stats.hp).key("sanity").value(m.stats.sanity).key("intelligence").value(m.stats.intelligence).key("agility").value(m.stats.agility).key("money").value(m.stats.money).key("reputation").value(m.stats.reputation).end_object();
    w.key("inventory").begin_array();for(auto item:m.bag.slots)w.value(static_cast<int>(item));w.end_array().key("selected").value(m.bag.selected);
    w.key("inmates").begin_array();for(auto n:m.inmates)w.begin_object().key("hp").value(n.hp).key("generation").value(n.generation).key("helped").value(n.helped).key("respawn").value(n.respawn).key("appearance").value(n.appearance).end_object();w.end_array();
    w.key("digSteps").value(m.digSteps);
    w.key("clock").value(m.clock).key("day").value(m.day).key("daysLeft").value(m.daysLeft).key("conduct").value(m.conduct).key("penalties").value(m.penalties).key("books").value(m.books);
    w.key("meals").begin_array();for(bool flag:m.meals)w.value(flag);w.end_array();w.key("jobs").begin_array();for(bool flag:m.jobs)w.value(flag);w.end_array();
    w.key("bonusToday").value(m.bonusToday).key("loan").value(m.loan).key("curfewResolved").value(m.curfewResolved).key("hotspot").value(m.hotspot).key("seed").value(m.seed).key("rng").value(m.rng.state());
    w.key("sanityTimer").value(m.sanityTimer).key("zeroTimer").value(m.zeroTimer).key("notRunning").value(m.notRunning).key("ringCooldown").value(m.ringCooldown).key("exhausted").value(m.exhausted);
    w.key("batTaken").value(m.batTaken).key("ballTaken").value(m.ballTaken).key("bookTaken").value(m.bookTaken).key("gardenTended").value(m.gardenTended);
    w.key("drops").begin_array();for(auto drop:m.drops)w.begin_object().key("item").value(static_cast<int>(drop.item)).key("at").value(drop.at).end_object();w.end_array().end_object();auto s=w.finish();return s?*s:"{}";
}
lam::Result<Model> decodeModel(std::string_view s){return lam::parse_data<Model>(s,"checkpoint",[](lam::DataReader &r){Model m;auto stats=r.object("stats");
    m.stats.hp=stats.integer("hp",0,10);m.stats.sanity=stats.number("sanity",0,100);m.stats.intelligence=stats.number("intelligence",0,100);m.stats.agility=stats.number("agility",0,100);m.stats.money=stats.integer("money",0,500);m.stats.reputation=stats.integer("reputation",0,12);
    auto bag=r.array("inventory");if(bag.size()!=3)r.fail("inventory must contain three slots");else for(int i=0;i<3;++i)m.bag.slots[static_cast<std::size_t>(i)]=static_cast<Item>(bag.integer(static_cast<std::size_t>(i),0,static_cast<int>(Item::Chips)));m.bag.selected=r.integer("selected",0,2);
    auto residents=r.array("inmates");if(residents.size()!=10)r.fail("exactly ten resident slots required");else for(std::size_t i=0;i<10;++i){auto n=residents.object(i);auto &to=m.inmates[i];to.hp=n.integer("hp",0,5);to.generation=n.integer("generation",0,1000000);to.helped=n.boolean("helped");to.respawn=n.number("respawn",0,10);to.appearance=n.integer_or("appearance",0,0,9999);}
    m.digSteps=r.integer_or("digSteps",0,0,cfg::digHolds);
    m.clock=r.number64("clock",420,1e12);m.day=r.integer("day",1,1000000);m.daysLeft=r.integer("daysLeft",0,10000);m.conduct=r.integer("conduct",0,10);m.penalties=r.integer("penalties",0,2);m.books=r.integer("books",0,3);
    auto meals=r.array("meals");if(meals.size()!=3)r.fail("three meal flags required");else for(std::size_t i=0;i<3;++i)m.meals[i]=meals.boolean(i);
    auto jobs=r.array("jobs");if(jobs.size()!=4)r.fail("four job flags required");else for(std::size_t i=0;i<4;++i)m.jobs[i]=jobs.boolean(i);
    m.bonusToday=r.boolean("bonusToday");m.loan=r.boolean("loan");m.curfewResolved=r.boolean("curfewResolved");m.hotspot=r.integer("hotspot",0,11);m.seed=r.u64("seed");m.rng.set_state(r.u64("rng"));
    m.sanityTimer=r.number64("sanityTimer",-.001,10);m.zeroTimer=r.number64("zeroTimer",-.001,10);m.notRunning=r.number("notRunning",0,1e8f);m.ringCooldown=r.number("ringCooldown",0,30);m.exhausted=r.boolean("exhausted");
    m.batTaken=r.boolean("batTaken");m.ballTaken=r.boolean("ballTaken");m.bookTaken=r.boolean("bookTaken");m.gardenTended=r.boolean("gardenTended");
    auto drops=r.array("drops");if(drops.size()>1024)r.fail("too many dropped objects");else for(std::size_t i=0;i<drops.size();++i){auto d=drops.object(i);auto at=d.vec2("at");int item=d.integer("item",1,static_cast<int>(Item::Chips));if(at.x<32||at.x>1248||at.y<32||at.y>1248)r.fail("drop outside compound");m.drops.push_back({static_cast<Item>(item),at});}
    return m;
});}
bool saveCheckpoint(Game &g){lam::SaveSlots slots(g.saveDir,"prison-life",1,1);auto result=slots.save(1,{.game="prison-life",.version=1,.summary_json="{\"day\":"+std::to_string(g.model.day)+"}",.data_json=encodeModel(g.model)});if(!result){notify(g,"Checkpoint write failed: "+result.error().message,8);return false;}return true;}
bool loadCheckpoint(Game &g){lam::SaveSlots slots(g.saveDir,"prison-life",1,1);auto e=slots.load(1);if(!e)return false;auto m=decodeModel(e->data_json);if(!m)return false;g.model=*m;g.checkpoint=*m;return true;}
}
