#include "model.h"
#include <algorithm>
#include <cmath>
namespace prison {
std::string_view residentName(const Model &m,int slot){auto s=static_cast<std::size_t>(slot);return slot==9||m.inmates[s].generation==0?cfg::names[s]:cfg::replacements[static_cast<std::size_t>(m.inmates[s].appearance%10)];}
std::string_view itemName(Item i) {
    constexpr std::array<std::string_view,15> n{"Empty","Kitchen knife","Shovel","Medkit","Adrenaline","Rice bowl","Sandwich","Apple","Soup","Bat","Ball","Lost book","Loaned shovel","Soda","Chips"};
    int k=static_cast<int>(i); return k>=0&&k<15?n[static_cast<std::size_t>(k)]:"Unknown";
}
bool isFood(Item i){return i>=Item::Rice&&i<=Item::Soup;}
bool isShovel(Item i){return i==Item::Shovel||i==Item::LoanShovel;}
int itemDamage(Item i){return i==Item::Knife?3:i==Item::Bat?2:1;}
int price(Item i){int k=static_cast<int>(i)-1;return k>=0&&k<4?cfg::prices[static_cast<std::size_t>(k)]:0;}
int Inventory::count() const {return static_cast<int>(std::count_if(slots.begin(),slots.end(),[](Item i){return i!=Item::None;}));}
bool Inventory::has(Item i) const {return std::find(slots.begin(),slots.end(),i)!=slots.end();}
bool Inventory::add(Item i){if(i==Item::None)return false;for(auto &s:slots)if(s==Item::None){s=i;return true;}return false;}
bool Inventory::remove(Item i){for(auto &s:slots)if(s==i){s=Item::None;return true;}return false;}
void Inventory::eraseSelected(){slots[static_cast<std::size_t>(selected)]=Item::None;}
int timeOfDay(double c){return static_cast<int>(std::fmod(c,static_cast<double>(cfg::minutePerDay)));}
bool curfew(double c){int t=timeOfDay(c);return t>=cfg::curfewMinute||t<cfg::morningMinute;}
int mealWindow(double c){int t=timeOfDay(c);for(std::size_t i=0;i<cfg::mealWindows.size();++i)if(t>=cfg::mealWindows[i][0]&&t<cfg::mealWindows[i][1])return static_cast<int>(i);return -1;}
float aggressionChance(int r){return cfg::ambientProbability*(1-static_cast<float>(std::clamp(r,0,cfg::maxRep))/cfg::maxRep);}
int awardMoney(Model &m,int a){int before=m.stats.money;m.stats.money=std::clamp(before+a,0,cfg::maxMoney);return m.stats.money-before;}
Result purchase(Model &m,Item i){
    if(price(i)<=0)return {false,"That item is not for sale."};
    if(m.bag.count()>=3)return {false,"All three slots are full. Drop an item first."};
    if(m.stats.money<price(i))return {false,"Not enough money. Jobs and ring bouts pay."};
    if(!m.bag.add(i))return {false,"Delivery failed. You were not charged."};
    m.stats.money-=price(i);return {true,std::string(itemName(i))+" purchased."};
}
Result consume(Model &m){
    const auto i=m.bag.held();
    if(i==Item::Medkit){if(m.stats.hp==cfg::maxHP)return {false,"Health is full. Medkit kept."};m.stats.hp=std::min(cfg::maxHP,m.stats.hp+cfg::medkitHeal);}
    else if(i==Item::Adrenaline){if(m.stats.hp==cfg::maxHP&&m.stats.sanity>=100)return {false,"Health and sanity are full. Shot kept."};m.stats.hp=std::min(cfg::maxHP,m.stats.hp+cfg::adrenalineHeal);m.stats.sanity=std::min(100.f,m.stats.sanity+cfg::adrenalineSanity);}
    else if(i==Item::Soda||i==Item::Chips)m.stats.sanity=std::min(100.f,m.stats.sanity+cfg::snackGain);
    else return {false,isFood(i)?"Take your meal to a free cafeteria seat.":"Use E at the matching interaction to use this item."};
    m.bag.eraseSelected();return {true,"Used "+std::string(itemName(i))+"."};
}
Result takeMeal(Model &m,int food){
    int window=mealWindow(m.clock);
    if(curfew(m.clock)||window<0)return {false,"Counter closed. Meals: 07-09, 12-14, 18-20."};
    if(m.meals[static_cast<std::size_t>(window)])return {false,"You have collected this window's free meal."};
    for(auto i:m.bag.slots)if(isFood(i))return {false,"Finish the meal you are carrying first."};
    if(food<0||food>3||!m.bag.add(static_cast<Item>(static_cast<int>(Item::Rice)+food)))return {false,"You need a free carry slot."};
    m.meals[static_cast<std::size_t>(window)]=true;
    for(int k=0;k<3;++k)if(isFood(m.bag.slots[static_cast<std::size_t>(k)]))m.bag.selected=k;
    return {true,"Meal collected. Find a free round seat and press E."};
}
Result eatMeal(Model &m){for(auto &i:m.bag.slots)if(isFood(i)){m.stats.sanity=std::min(100.f,m.stats.sanity+static_cast<float>(cfg::foodGains[static_cast<std::size_t>(static_cast<int>(i)-static_cast<int>(Item::Rice))]));i=Item::None;return {true,"A quiet meal. Sanity restored; health unchanged."};}return {false,"Pick up a meal at the west counter first."};}
Result finishBook(Model &m,float seconds){if(m.books>=cfg::booksPerDay)return {false,"Three books today. Come back after 07:00."};if(seconds+0.001f<cfg::readSeconds)return {false,"Reading interrupted. No book allowance used."};++m.books;m.stats.intelligence=std::min(100.f,m.stats.intelligence+cfg::readGain);return {true,m.stats.intelligence>=20?"+10 intelligence. Clue: inspect the marked cell's loose tile.":"+10 intelligence. A note mentions a loose tile in a cell."};}
Result helpResident(Model &m,int s){if(s<0||s>=10)return {false,"No one nearby."};auto &n=m.inmates[static_cast<std::size_t>(s)];if(n.helped)return {false,"You already helped this resident. Thank you."};n.helped=true;m.stats.reputation=std::min(12,m.stats.reputation+2);return {true,"You listened and offered support. +2 reputation (max 12)."};}
Result finishJob(Model &m,int j){if(j<0||j>3)return {false,"Unknown task."};if(m.jobs[static_cast<std::size_t>(j)])return {false,"This job is complete for today."};
    if(j==1&&!m.bag.remove(Item::Book))return {false,"Find the dropped book in the hall first."};
    if(j==3&&!m.bag.remove(Item::LoanShovel))return {false,"Return the loaned shovel to the tool shed."};
    m.jobs[static_cast<std::size_t>(j)]=true;if(j==3)m.loan=false;
    int gain=awardMoney(m,cfg::jobRewards[static_cast<std::size_t>(j)]);++m.conduct;
    std::string message="Task complete. +$"+std::to_string(gain)+", +1 conduct.";
    if(m.conduct>=cfg::conductNeeded&&!m.bonusToday){m.conduct-=cfg::conductNeeded;m.bonusToday=true;m.daysLeft=std::max(0,m.daysLeft-1);message+=" Good conduct: one sentence day removed.";}
    return {true,message};
}
bool advanceClock(Model &m,float dt){
    double remaining=std::max(0.0,static_cast<double>(dt));bool morning=false;
    // Split at schedule boundaries so crossing 22:00 or 07:00 never applies the old rate
    // to the remainder of a step. Real-time gameplay durations never use this clock.
    while(remaining>1e-12){
        double base=std::floor(m.clock/cfg::minutePerDay)*cfg::minutePerDay;
        bool night=curfew(m.clock);double rate=night?cfg::nightMinutesPerSecond:cfg::daytimeMinutesPerSecond;
        double boundary=base+(night?(timeOfDay(m.clock)>=cfg::curfewMinute?cfg::minutePerDay+cfg::morningMinute:cfg::morningMinute):cfg::curfewMinute);
        double seconds=(boundary-m.clock)/rate;
        if(remaining>=seconds){morning=advanceMinutes(m,boundary-m.clock)||morning;remaining-=seconds;}
        else {morning=advanceMinutes(m,remaining*rate)||morning;remaining=0;}
    }
    return morning;
}
bool advanceMinutes(Model &m,double minutes){
    const int prior=static_cast<int>(std::floor((m.clock-cfg::morningMinute)/cfg::minutePerDay));
    m.clock+=minutes;
    const int now=static_cast<int>(std::floor((m.clock-cfg::morningMinute)/cfg::minutePerDay));
    if(now<=prior)return false;
    int mornings=now-prior; m.day+=mornings;m.daysLeft=std::max(0,m.daysLeft-mornings);
    m.meals={};m.jobs={};m.books=0;m.penalties=0;m.bonusToday=false;m.curfewResolved=false;return true;
}
void advanceVitals(Model &m,float dt,bool running,float respawnRate){
    m.sanityTimer+=dt;while(m.sanityTimer+0.00001f>=cfg::sanityInterval){m.sanityTimer-=cfg::sanityInterval;m.stats.sanity=std::max(0.f,m.stats.sanity-1);}
    if(m.stats.sanity<=0){m.zeroTimer+=dt;while(m.zeroTimer+0.00001f>=cfg::zeroDamageInterval){m.zeroTimer-=cfg::zeroDamageInterval;m.stats.hp=std::max(0,m.stats.hp-1);}}else m.zeroTimer=0;
    if(running&&m.stats.agility>0){m.stats.agility=std::max(0.f,m.stats.agility-cfg::staminaDrain*dt);m.notRunning=0;if(m.stats.agility<=0)m.exhausted=true;}
    else {m.notRunning+=dt;if(m.notRunning>=cfg::staminaDelay)m.stats.agility=std::min(100.f,m.stats.agility+cfg::staminaRefill*dt);if(m.stats.agility>20)m.exhausted=false;}
    m.ringCooldown=std::max(0.f,m.ringCooldown-dt);
    for(auto &n:m.inmates)if(n.hp<=0)n.respawn=std::max(0.f,n.respawn-dt*respawnRate);
}
Result confirmMisconduct(Model &m){bool add=m.penalties<cfg::maxDailyPenalties;if(add){++m.penalties;++m.daysLeft;}return {true,add?"Officer confrontation: +1 sentence day. Returned to your cell.":"Officer confrontation. Daily sentence penalty cap reached."};}
void newModel(Model &m,std::uint64_t seed){m=Model{};m.seed=seed;m.rng.reseed(seed);m.hotspot=m.rng.between(0,11);}
}
