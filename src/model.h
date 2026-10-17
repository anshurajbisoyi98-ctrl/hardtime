#pragma once
#include "config.h"
#include <array>
#include <string>
#include <vector>
#include <lam/math/rng.h>
#include <lam/math/vec2.h>
namespace prison {
enum class Item:int {None,Knife,Shovel,Medkit,Adrenaline,Rice,Sandwich,Apple,Soup,Bat,Ball,Book,LoanShovel,Soda,Chips};
std::string_view itemName(Item i);
bool isFood(Item i);
bool isShovel(Item i);
int itemDamage(Item i);
int price(Item i);
struct Stats {int hp=cfg::maxHP; float sanity=cfg::initialSanity, intelligence=cfg::initialIntelligence, agility=cfg::initialAgility; int money=0, reputation=0;};
struct Inventory {
    std::array<Item,3> slots{}; int selected=0;
    int count() const; bool has(Item i) const; bool add(Item i);
    Item held() const {return slots[static_cast<std::size_t>(selected)];}
    bool remove(Item i); void eraseSelected();
};
struct Resident {
    int hp=cfg::inmateHP, generation=0; bool helped=false; float respawn=0;
    int appearance=0;
};
struct Dropped {Item item=Item::None; lam::Vec2 at;};
struct Model {
    Stats stats; Inventory bag; std::array<Resident,10> inmates{};
    double clock=cfg::morningMinute; int day=1, daysLeft=cfg::sentence, conduct=0, penalties=0, books=0;
    std::array<bool,3> meals{}; std::array<bool,4> jobs{};
    bool bonusToday=false, loan=false, curfewResolved=false;
    int digSteps=0;
    int hotspot=0; std::uint64_t seed=7319; lam::Rng rng{7319};
    double sanityTimer=0, zeroTimer=0;
    float notRunning=1, ringCooldown=0;
    bool exhausted=false;
    bool batTaken=false,ballTaken=false,bookTaken=false,gardenTended=false;
    std::vector<Dropped> drops;
};
struct Result {bool ok=false; std::string message;};
bool curfew(double clock); int timeOfDay(double clock); int mealWindow(double clock);
float aggressionChance(int rep);
int awardMoney(Model &m,int amount);
Result purchase(Model &m,Item i);
Result consume(Model &m);
Result takeMeal(Model &m,int food);
Result eatMeal(Model &m);
Result finishBook(Model &m,float seconds);
Result helpResident(Model &m,int slot);
Result finishJob(Model &m,int job);
bool advanceClock(Model &m,float seconds); // true once when crossing a morning
bool advanceMinutes(Model &m,double minutes);
void advanceVitals(Model &m,float seconds,bool running,float respawnRate=1);
Result confirmMisconduct(Model &m);
void newModel(Model &m,std::uint64_t seed);
std::string_view residentName(const Model &m,int slot);
}
