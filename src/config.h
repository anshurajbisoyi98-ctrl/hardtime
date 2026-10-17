#pragma once
#include <array>
#include <string_view>
namespace prison::cfg {
inline constexpr int mapSize=1280, tile=16, grid=80;
inline constexpr float walk=90, run=150, npcSpeed=44, officerSpeed=60;
inline constexpr int maxHP=10, inmateHP=5, maxMoney=500, maxRep=12, sentence=7;
inline constexpr float attackCooldown=.6f, invulnerability=.5f, meleeRange=33;
inline constexpr float sanityInterval=10, zeroDamageInterval=10, respawnSeconds=10;
inline constexpr float staminaDrain=20, staminaRefill=15, staminaDelay=1;
inline constexpr float readSeconds=10, digSeconds=8, encounterCooldown=30, spawnGrace=5;
inline constexpr int digHolds=5;
inline constexpr float shovelRespawnRate=1.5f, shovelWaveInterval=12;
inline constexpr int booksPerDay=3;
inline constexpr int statMax=100, initialSanity=80, initialIntelligence=0, initialAgility=100;
inline constexpr int minutePerDay=1440, morningMinute=420, curfewMinute=1320, curfewWarningMinute=1290;
inline constexpr double realSecondsPerDay=5*60;
inline constexpr double daytimeMinutesPerSecond=minutePerDay/realSecondsPerDay, nightMinutesPerSecond=daytimeMinutesPerSecond;
inline constexpr int snackPrice=5, snackGain=10, readGain=10, conductNeeded=3, maxDailyPenalties=2;
inline constexpr int ringRewardMin=30, ringRewardMax=50;
inline constexpr int medkitHeal=5, adrenalineHeal=2, adrenalineSanity=2;
inline constexpr float ringCooldown=30, ambientProbability=.05f, encounterRadius=48, officerSight=220;
inline constexpr std::array<int,4> jobRewards{20,15,20,20};
inline constexpr std::array<std::array<int,2>,3> mealWindows{{{420,540},{720,840},{1080,1200}}};
inline constexpr std::array<int,4> prices{100,400,50,50};
inline constexpr std::array<int,4> foodGains{20,15,10,20};
inline constexpr std::array<std::string_view,10> names{"Milo","Jules","Rafi","Theo","Bram","Niko","Otis","Leon","Ash","Vex"};
inline constexpr std::array<std::string_view,3> officers{"Hale","Reed","Quinn"};
inline constexpr std::array<std::string_view,10> replacements{"Rowe","Kit","Pax","Indi","Drew","Luca","Ren","Jae","Noel","Finn"};
inline constexpr std::array<std::string_view,4> foods{"Rice bowl","Sandwich","Apple","Soup"};
inline constexpr std::array<std::string_view,4> effects{"Melee / 3 damage","A tool. A possibility.","Restore up to 5 HP","+2 HP and +2 sanity"};
inline constexpr std::array<float,3> ringCadence{1.4f,1.0f,.75f};
}
