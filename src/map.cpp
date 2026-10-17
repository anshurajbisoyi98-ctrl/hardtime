#include "map.h"
#include <cmath>
namespace prison {
using namespace lam;
std::string_view zoneName(Vec2 p){if(p.y<448)return "NORTH / CELL WING";if(p.y>896)return "SOUTH / EXERCISE YARD";if(p.x<432)return "WEST / CAFETERIA";if(p.x>848)return "EAST / LIBRARY";return "CENTRAL / COMMON HALL";}
void Map::build(TileWorld &tiles){
    walls.clear();props.clear();visits.clear();tiles.resize(80,80,16,{});
    auto wall=[&](float x,float y,float w,float h){walls.push_back({x,y,w,h});};
    auto prop=[&](PropKind k,Rect r,Vec2 at,int index=0,bool solid=true){props.push_back({k,r,at,index,solid});};
    wall(32,32,1216,16);wall(32,32,16,1216);wall(1232,32,16,1216);wall(32,1232,1216,16);
    wall(48,432,544,16);wall(688,432,544,16);
    int n=0;
    for(int row=0;row<2;++row)for(int side=0;side<2;++side)for(int col=0;col<3;++col){
        float x=static_cast<float>((side==0?80:704)+col*168), y=static_cast<float>(80+row*176);
        cells[static_cast<std::size_t>(n)]={{x,y,144,128},{x+88,y+136},{x+112,y+80},row*2+side};
        wall(x,y,144,8);wall(x,y,8,128);wall(x+136,y,8,128);wall(x,y+120,64,8);wall(x+112,y+120,32,8);
        prop(PropKind::Bed,{x+16,y+32,32,64},{x+72,y+72},n);
        prop(PropKind::Toilet,{x+96,y+16,24,24},{x+108,y+56},n);
        visits.push_back({x+88,y+136});++n;
    }
    // West and east rooms open into the common hall through broad, aligned doorways.
    for(float x:{64.f,864.f}){
        wall(x,480,352,16);wall(x,848,352,16);wall(x,480,16,384);wall(x+336,480,16,160);wall(x+336,704,16,160);
    }
    // Library's west doorway and the cafeteria's east doorway.
    // Replace the library west wall with a door; its east wall stays closed.
    walls.erase(walls.end()-3);
    wall(864,480,16,160);wall(864,704,16,160);wall(1200,640,16,64);
    wall(432,480,160,16);wall(688,480,160,16);wall(432,848,160,16);wall(688,848,160,16);
    wall(432,480,16,160);wall(432,704,16,160);wall(832,480,16,160);wall(832,704,16,160);
    for(int i=0;i<4;++i)prop(PropKind::Counter,{80,static_cast<float>(544+i*64),32,64},{136,static_cast<float>(568+i*64)},i);
    prop(PropKind::Counter,{80,800,32,32},{136,816},4);
    prop(PropKind::Vending,{352,512,32,48},{368,576},0);
    prop(PropKind::Vending,{784,512,32,48},{800,576},1);
    prop(PropKind::Phone,{688,496,32,48},{704,560});
    prop(PropKind::Pool,{464,528,96,56},{512,600});
    for(int table=0;table<2;++table){
        Vec2 center{240,static_cast<float>(608+table*160)};
        prop(PropKind::Table,{center.x-32,center.y-32,64,64},center+Vec2{0,48},table);
        std::array<Vec2,4> offsets{{{0,-48},{48,0},{0,48},{-48,0}}};
        for(int s=0;s<4;++s){int i=table*4+s;auto p=center+offsets[static_cast<std::size_t>(s)];seats[static_cast<std::size_t>(i)]=p;prop(PropKind::Seat,{p.x-8,p.y-8,16,16},p,i,false);}
    }
    for(int row=0;row<2;++row)for(int col=0;col<4;++col){float x=static_cast<float>(896+col*80),y=static_cast<float>(544+row*192);prop(PropKind::Shelf,{x,y,48,64},{x+24,y+80},row*4+col);}
    // Small reading tables between the shelf rows in columns two and four.
    for(int table=0;table<2;++table){
        Vec2 center{static_cast<float>(1000+table*160),672};
        prop(PropKind::Table,{center.x-24,center.y-24,48,48},center+Vec2{0,48},table+2);
        for(int row=0;row<2;++row)for(int chair=0;chair<2;++chair){
            Vec2 at=center+Vec2{chair==0?-14.f:14.f,row==0?-40.f:40.f};
            prop(PropKind::Seat,{at.x-6,at.y-6,12,12},at,8+table*4+row*2+chair,false);
        }
    }
    prop(PropKind::Hoop,{128,976,240,192},{248,1100},0,false);
    prop(PropKind::Ring,ringBounds,{912,1064},0,false);
    prop(PropKind::Spill,{592,704,32,32},{608,720},0,false);
    prop(PropKind::LostBook,{736,784,16,16},{744,792},0,false);
    prop(PropKind::Shed,{464,1136,80,64},{560,1168});
    prop(PropKind::Patch,{704,1120,96,64},{752,1152},0,false);
    prop(PropKind::Bat,{400,976,24,8},{412,980},0,false);
    prop(PropKind::Ball,{384,1088,16,16},{392,1096},0,false);
    prop(PropKind::Gate,{592,1216,96,16},{640,1192});
    auto block=[&](Rect r){for(int y=static_cast<int>(std::floor(r.y/16));y<static_cast<int>(std::ceil((r.y+r.h)/16));++y)for(int x=static_cast<int>(std::floor(r.x/16));x<static_cast<int>(std::ceil((r.x+r.w)/16));++x)tiles.set({x,y},TileShape::Solid,kTileOpaque,0);};
    for(auto r:walls)block(r);for(auto p:props)if(p.solid)block(p.box);
    for(auto p:std::array<Vec2,13>{{{608,512},{640,640},{752,720},{512,784},{640,904},{400,912},{864,912},{928,672},{1120,672},{1120,824},{320,672},{640,1168},{896,1064}}})visits.push_back(p);
}
}
