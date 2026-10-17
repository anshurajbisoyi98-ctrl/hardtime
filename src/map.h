#pragma once
#include "config.h"
#include <array>
#include <vector>
#include <string_view>
#include <lam/math/rect.h>
#include <lam/physics/tile_world.h>
namespace prison {
enum class PropKind {Bed,Toilet,Shelf,Table,Seat,Counter,Vending,Phone,Pool,Hoop,Ring,Spill,LostBook,Shed,Patch,Bat,Ball,Gate};
struct Prop {PropKind kind;lam::Rect box;lam::Vec2 interaction;int index=0;bool solid=true;};
struct Cell {lam::Rect box;lam::Vec2 door,hotspot;int block=0;};
struct Map {
    std::vector<lam::Rect> walls;std::vector<Prop> props;std::array<Cell,12> cells;
    std::array<lam::Vec2,8> seats{};
    std::vector<lam::Vec2> visits;
    void build(lam::TileWorld &tiles);
};
inline constexpr lam::Vec2 playerHome{168,170},dealerSpot{1120,784},ringPlayer{1000,1064},ringOpponent{1120,1064};
inline constexpr lam::Rect ringBounds{944,976,240,176};
std::string_view zoneName(lam::Vec2 p);
}
