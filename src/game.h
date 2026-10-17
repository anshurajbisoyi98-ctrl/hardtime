#pragma once
#include "model.h"
#include "map.h"
#include <lam/core/app.h>
#include <lam/ecs/component.h>
#include <lam/ecs/entity.h>
#include <lam/input/input_frame.h>
#include <lam/tilemap/grid_path.h>
#include <lam/ui/ui.h>
namespace prison {
struct Actor {int slot=-1;bool officer=false;lam::Vec2 facing{0,1};float animation=0,flash=0;};
enum class Phase {Court,Escort,Play,Infirmary,Released,Escaped};
enum class Menu {None,Pause,Map,Dealer,Dialogue,Food,Ring,ConfirmRing,Drop,Help,Vending,Audio};
enum class Activity {None,Read,Dig,Clean,Serve,Garden,Phone,Pool,Basketball,Eat,Rest};
struct Brain {lam::PathRoute route;lam::Vec2 goal;float wait=0,repath=0,cooldown=0,telegraph=0;bool near=false,hostile=false,raider=false;};
struct Controls {
    lam::AxisAction move;lam::ButtonAction run,interact,attack,map,pause,drop,confirm,audio;
    std::array<lam::ButtonAction,3> slots;
};
struct Paths {std::string saveDir;};
struct Game {
    Model model,checkpoint;Map map;lam::Entity player,camera;std::array<lam::Entity,13> actors;
    std::array<Brain,13> brains;lam::PathFinder paths;
    Phase phase=Phase::Court;Menu menu=Menu::None;Activity activity=Activity::None;
    float story=0,active=0,grace=5,invuln=0,attackCooldown=0,attackVisual=0,overview=0;
    float progress=0,duration=0;lam::Vec2 activityAt,face{0,1};
    std::string notice="Seven days. Two ways out.",prompt;float noticeTime=5;
    int focusProp=-1,focusNPC=-1,focusDrop=-1,dialogueSlot=-1,ringSlot=-1,difficulty=0;
    int escortStep=0;bool doorClosed=false,running=false,wasWing=true,incident=false,curfewIncident=false;
    float incidentTime=0,curfewGrace=0;Item implicated=Item::None;
    int lastMinute=-1;bool warned=false;
    float ringTimer=0;int ringReward=0;bool ringRewarded=false,wasInRing=false;
    lam::Vec2 ringReturn;bool checkpointPending=false;int activitySource=-1;
    lam::PathRoute escortRoute;bool escortReturning=false;
    bool shovelPressure=false,escapePatrol=false,digNeedsRelease=false;
    float waveTimer=4,waveAge=0;int waveNumber=0;
    std::array<int,3> patrolStep{};
    Menu builtMenu=Menu::None;Phase builtPhase=Phase::Court;bool uiDirty=true;
    lam::UiId modalRoot=0;std::string saveDir="saves";bool scripted=false,saved=false;
};
void setup(lam::Ctx &ctx);void drive(lam::Ctx &ctx);void simulate(lam::Ctx &ctx);void afterPhysics(lam::Ctx &ctx);
void updateUI(lam::Ctx &ctx);void drawWorld(lam::Ctx &ctx);void drawHUD(lam::Ctx &ctx);
void notify(Game &g,std::string message,float seconds=5);void openMenu(Game &g,Menu menu);
void setDoor(lam::World &w,Game &g,bool closed);void enterPlay(lam::Ctx &ctx,bool save=true);
void restoreCheckpoint(lam::Ctx &ctx);void newGame(lam::Ctx &ctx);
bool saveCheckpoint(Game &g);bool loadCheckpoint(Game &g);
std::string encodeModel(const Model &m);lam::Result<Model> decodeModel(std::string_view s);
void install(lam::App &app);
void placeActor(lam::World &w,lam::Entity e,lam::Vec2 p);
void stopActor(lam::World &w,lam::Entity e);
void processCommands(lam::Ctx &ctx);void interact(lam::Ctx &ctx);void updateFocus(lam::Ctx &ctx);
void tickActivity(lam::Ctx &ctx);void beginActivity(lam::Ctx &ctx,Activity a,float duration);
bool carryingShovel(const Game &g);
void updateShovelThreat(lam::Ctx &ctx);
bool guardSeesPlayer(lam::Ctx &ctx,int officer);
bool diggingWatched(lam::Ctx &ctx);
void caughtDigging(lam::Ctx &ctx);
void tickPeople(lam::Ctx &ctx);void playerAttack(lam::Ctx &ctx);void startBout(lam::Ctx &ctx);
void cancelBout(lam::Ctx &ctx);void flagIncident(lam::Ctx &ctx,Item item,std::string message);
void tickEscort(lam::Ctx &ctx);void morning(lam::Ctx &ctx);void finishActivity(lam::Ctx &ctx);
}
LAM_REFLECT(prison::Actor,slot,officer,facing,animation,flash)
