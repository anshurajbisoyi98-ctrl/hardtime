#include "game.h"
#include "broadcast.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <sstream>
#include <lam/assets/asset_server.h>
#include <lam/ecs/transform.h>
#include <lam/ecs/world.h>
#include <lam/physics/components.h>
#include <lam/render/font.h>
#include <lam/render/render_queue.h>
#include <lam/render/shapes.h>
namespace prison {
using namespace lam;
namespace {
constexpr auto ink=Color::hex(0x101923ff), panel=Color::hex(0x15222eff), border=Color::hex(0x394c59ff);
constexpr auto cream=Color::hex(0xf2ead6ff), muted=Color::hex(0x99adb2ff), orange=Color::hex(0xe89a54ff);
constexpr auto green=Color::hex(0x9abda1ff), gold=Color::hex(0xe5cb72ff), blue=Color::hex(0x779ebcff), red=Color::hex(0xe27670ff);
void text(Ctx &ctx,std::string_view s,Vec2 p,float size=12,Color color=cream,bool center=false,std::uint8_t layer=220){
    auto *d=ctx.try_res<RenderDefaults>();auto *font=d?ctx.res<AssetServer>().get(d->font):nullptr;if(!font)return;
    if(center)p.x-=measure_text(*font,s,size).x*.5f;
    draw_text(ctx.res<RenderQueue>(),*font,s,p,TextStyle{.size=size,.color=color,.layer=layer});
}
void wrapped(Ctx &ctx,std::string_view value,Vec2 p,float size,float width,Color color=cream){
    auto *defaults=ctx.try_res<RenderDefaults>();auto *font=defaults?ctx.res<AssetServer>().get(defaults->font):nullptr;if(!font)return;
    std::istringstream words{std::string(value)};std::string word,line;
    while(words>>word){std::string next=line.empty()?word:line+" "+word;if(!line.empty()&&measure_text(*font,next,size).x>width){text(ctx,line,p,size,color);p.y+=size+4;line=word;}else line=std::move(next);}
    if(!line.empty())text(ctx,line,p,size,color);
}
void box(RenderQueue &q,Rect r,Color fill=panel,std::uint8_t layer=220){q.rect({r.x+3,r.y+4,r.w,r.h},ink.with_alpha(95),layer);q.rect(r,fill,layer);q.rect_lines(r,1,border,layer);}
void bar(Ctx &ctx,std::string_view label,float value,float maximum,Vec2 p,Color color){auto &q=ctx.res<RenderQueue>();text(ctx,label,p,10,muted);text(ctx,std::to_string(static_cast<int>(std::ceil(value)))+" / "+std::to_string(static_cast<int>(maximum)),{p.x+104,p.y},10,cream);q.rect({p.x,p.y+14,160,4},border,220);q.rect({p.x,p.y+14,160*std::clamp(value/maximum,0.f,1.f),4},color,220);}
void itemIcon(RenderQueue &q,Item i,Vec2 p,float scale=1,std::uint8_t layer=220){
    auto r=[&](float x,float y,float w,float h,Color c){q.rect({p.x+x*scale,p.y+y*scale,w*scale,h*scale},c,layer);};
    if(i==Item::Knife){r(-2,-14,4,18,cream);r(-3,4,6,9,Color::hex(0x84644eff));r(-6,2,12,3,muted);}
    else if(isShovel(i)){r(-1,-14,3,22,Color::hex(0xa77b50ff));r(-6,7,13,10,muted);r(-5,-18,10,3,muted);r(-5,-18,2,7,muted);r(3,-18,2,7,muted);}
    else if(i==Item::Medkit){r(-13,-9,26,20,cream);r(-8,-13,16,4,muted);r(-2,-5,5,12,red);r(-6,-1,13,4,red);}
    else if(i==Item::Adrenaline){q.line(p+Vec2{-7*scale,11*scale},p+Vec2{8*scale,-11*scale},4*scale,blue,layer);q.line(p+Vec2{8*scale,-11*scale},p+Vec2{12*scale,-18*scale},scale,cream,layer);r(-10,9,7,4,muted);}
    else if(i==Item::Bat){q.line(p+Vec2{-6*scale,15*scale},p+Vec2{5*scale,-15*scale},6*scale,orange,layer);r(-8,9,4,7,cream);}
    else if(i==Item::Ball){q.circle(p,10*scale,orange,layer);q.line(p-Vec2{10*scale,0},p+Vec2{10*scale,0},scale,ink,layer);q.line(p-Vec2{0,10*scale},p+Vec2{0,10*scale},scale,ink,layer);}
    else if(i==Item::Book){r(-10,-12,20,25,blue);r(-7,-10,2,21,cream);r(-3,-5,10,2,gold);}
    else if(i==Item::Soda){r(-7,-12,14,24,blue);r(-6,-14,12,3,cream);r(-6,11,12,2,muted);r(-5,-5,10,9,cream);r(-3,-2,6,3,orange);}
    else if(i==Item::Chips){r(-10,-13,20,26,gold);r(-10,-14,20,3,orange);r(-10,11,20,3,orange);r(-8,-5,16,10,ink);r(-4,-2,8,4,gold);}
    else if(i==Item::Rice){q.ellipse(p+Vec2{0,4*scale},{12*scale,7*scale},cream,layer);q.ellipse(p,{11*scale,5*scale},gold,layer);for(int k=0;k<5;++k)r(-8+static_cast<float>(k)*3,-2+static_cast<float>(k%2)*3,3,1,cream);r(-1,-12,1,5,muted);r(4,-14,1,5,muted);}
    else if(i==Item::Sandwich){q.triangle(p+Vec2{-12*scale,9*scale},p+Vec2{12*scale,9*scale},p+Vec2{-12*scale,-12*scale},cream,layer);q.triangle(p+Vec2{-10*scale,6*scale},p+Vec2{8*scale,6*scale},p+Vec2{-10*scale,-8*scale},orange,layer);r(-11,4,21,2,green);r(-12,8,24,3,cream);}
    else if(i==Item::Apple){q.circle(p+Vec2{-3*scale,0},8*scale,red,layer);q.circle(p+Vec2{4*scale,0},8*scale,red,layer);r(0,-13,2,6,Color::hex(0x84644eff));q.ellipse(p+Vec2{5*scale,-10*scale},{5*scale,2*scale},green,layer);r(-6,-4,2,4,cream.with_alpha(150));}
    else if(i==Item::Soup){q.ellipse(p+Vec2{0,4*scale},{12*scale,7*scale},cream,layer);q.ellipse(p,{11*scale,5*scale},Color::hex(0xb36a42ff),layer);r(-6,-2,3,2,green);r(2,1,3,2,gold);r(3,-3,2,2,green);q.line(p+Vec2{8*scale,2*scale},p+Vec2{17*scale,-9*scale},2*scale,muted,layer);}
    else {r(-5,-1,10,2,border);}
}
void person(RenderQueue &q,Vec2 p,const Actor &a,float time,bool moving,bool player,bool dealer,int generation=0,std::uint8_t layer=64,float scale=1){
    float bob=moving?std::sin(time*12)*.7f:0;float stride=moving?std::sin(time*12)*2:0;
    std::array<Color,5> skins{Color::hex(0xc79872ff),Color::hex(0x8b604aff),Color::hex(0xdcb592ff),Color::hex(0xad7956ff),Color::hex(0xe2ba98ff)};
    Color skin=skins[static_cast<std::size_t>((std::max(0,a.slot)+generation)%5)];Color uniform=a.officer?blue:dealer?Color::hex(0x242c36ff):orange;
    if(a.flash>0)uniform=cream;
    float depth=layer<200?p.y:0;
    auto r=[&](float x,float y,float w,float h,Color c){q.rect({p.x+x*scale,p.y+(y+bob)*scale,w*scale,h*scale},c,layer,depth);};
    q.ellipse(p+Vec2{0,2*scale},{8*scale,3*scale},ink.with_alpha(90),layer,depth-.1f);
    r(-5,-3+stride,4,5,ink);r(1,-3-stride,4,5,ink);
    r(-7,-15,14,13,ink);r(-5,-14,10,10,uniform);r(-1,-13,1,9,uniform==orange?Color::hex(0xb16c3fff):muted);
    r(-7,-12,2,7,uniform);r(5,-12,2,7,uniform);r(-7,-5,2,3,skin);r(5,-5,2,3,skin);
    r(-5,-24,10,10,ink);r(-4,-23,8,8,skin);
    Color hair=(a.slot%3==0)?Color::hex(0x6b4938ff):ink;r(-4,-24,8,a.slot%3==1?2:4,hair);
    if(a.facing.y<-.4f)r(-4,-22,8,5,hair);else {float eye=a.facing.x>.3f?2:a.facing.x<-.3f?-3:-2;r(eye,-19,1,2,ink);if(std::abs(a.facing.x)<.3f)r(2,-19,1,2,ink);}
    if(a.officer){r(-5,-25,10,4,Color::hex(0x365675ff));r(-4,-22,10,1,ink);r(2,-12,2,3,gold);r(-5,-5,10,2,ink);}
    if(dealer){r(-3,-13,6,2,gold);r(-4,-20,8,2,ink);}
    if(player){q.triangle(p+Vec2{-3*scale,-33*scale},p+Vec2{3*scale,-33*scale},p+Vec2{0,-29*scale},cream,layer,depth);r(1,-12,3,2,cream);}
}
void prop(Ctx &ctx,const Prop &p,const Game &g){
    auto &q=ctx.res<RenderQueue>();auto b=p.box;float x=b.x,y=b.y,w=b.w,h=b.h;auto c=b.center();constexpr std::uint8_t L=40;
    auto r=[&](Rect rect,Color col){q.rect(rect,col,L);};
    switch(p.kind){
    case PropKind::Bed:r({x+3,y+4,w,h},ink.with_alpha(65));r(b,Color::hex(0x4d626cff));r({x+2,y+2,w-4,h-5},Color::hex(0x82959aff));r({x+4,y+5,w-8,12},cream);r({x+3,y+22,w-6,h-26},p.index==0?blue:Color::hex(0x687d7fff));r({x+3,y+24,w-6,3},muted);r({x+1,y+h-3,3,6},ink);r({x+w-4,y+h-3,3,6},ink);break;
    case PropKind::Toilet:r(b,Color::hex(0xa8b7b5ff));r({x+2,y+1,w-4,7},cream);q.ellipse({x+w/2,y+15},{8,6},cream,L);q.ellipse({x+w/2,y+15},{5,3},Color::hex(0x5c8791ff),L);r({x+3,y+3,3,2},muted);break;
    case PropKind::Shelf:r({x+3,y+5,w,h},ink.with_alpha(90));r(b,Color::hex(0x785d4aff));for(int row=0;row<3;++row){for(int k=0;k<7;++k){std::array<Color,4> colors{blue,green,orange,cream};r({x+4+k*6.f,y+4+row*18.f,4,13},colors[static_cast<std::size_t>((k+row+p.index)%4)]);}r({x,y+18+row*18.f,w,3},Color::hex(0xa28563ff));}r({x,y,w,3},Color::hex(0xb69a73ff));break;
    case PropKind::Table:{float radius=std::min(w,h)*.5f;q.ellipse(c+Vec2{2,4},{radius,radius},ink.with_alpha(90),L);q.circle(c,radius,Color::hex(0x997958ff),L);q.circle(c-Vec2{1,2},radius-3,Color::hex(0xc0a07aff),L);q.circle(c,3,Color::hex(0x997958ff),L);break;}
    case PropKind::Seat:q.circle(c,w*.5f+1,ink,L);q.circle(c-Vec2{0,1},w*.5f-1,Color::hex(0xbda588ff),L);break;
    case PropKind::Counter:
        r(b,Color::hex(0x8b7662ff));r({x+4,y,w-8,h},Color::hex(0xaaa795ff));r({x,y,2,h},cream);
        if(p.index==0)r({x,y,w,4},cream);
        if(p.index<4)itemIcon(q,static_cast<Item>(static_cast<int>(Item::Rice)+p.index),c,.7f,L);
        else {r({x,y+h-4,w,4},cream);text(ctx,"JOB",c-Vec2{0,4},7,ink,true,41);}
        break;
    case PropKind::Vending:r({x+3,y+4,w,h},ink.with_alpha(80));r(b,Color::hex(0x426a70ff));r({x+3,y+3,w-6,h-10},ink);for(int row=0;row<3;++row){r({x+6,y+6+row*10.f,7,7},row%2?orange:green);r({x+16,y+6+row*10.f,6,7},blue);}r({x+25,y+8,3,9},gold);r({x+6,y+h-9,18,5},ink);r({x+3,y+1,w-6,2},blue);break;
    case PropKind::Phone:r(b,Color::hex(0x526673ff));r({x+3,y+3,w-6,h-6},ink);r({x+7,y+7,18,17},blue);r({x+10,y+11,13,9},Color::hex(0xa6c0b9ff));r({x+6,y+10,4,19},cream);for(int row=0;row<3;++row)for(int col=0;col<3;++col)r({x+13+col*4.f,y+28+row*4.f,2,2},muted);break;
    case PropKind::Pool:r({x+4,y+5,w,h},ink.with_alpha(80));r(b,Color::hex(0x83664dff));r({x+7,y+7,w-14,h-14},Color::hex(0x426d62ff));for(auto point:std::array<Vec2,4>{{{x+6,y+6},{x+w-6,y+6},{x+6,y+h-6},{x+w-6,y+h-6}}})q.circle(point,4,ink,L);q.circle(c,3,cream,L);q.circle(c+Vec2{15,7},3,orange,L);q.line({x+12,y+h-8},{x+w-8,y+5},2,gold,L);break;
    case PropKind::Hoop:r(b,Color::hex(0x495f5bff));q.rect_lines({x+5,y+5,w-10,h-10},2,cream.with_alpha(130),L);q.line({x+5,y+h/2},{x+w-5,y+h/2},1,cream.with_alpha(130),L);q.circle(c,28,cream.with_alpha(90),L);q.circle(c,26,Color::hex(0x495f5bff),L);q.rect_lines({x+w/2-34,y+5,68,50},1,cream,L);r({x+w/2-22,y-6,44,5},cream);q.ellipse({c.x,y+14},{10,6},orange,L);q.ellipse({c.x,y+14},{7,3},ink,L);break;
    case PropKind::Ring:r({x-6,y-6,w+12,h+12},Color::hex(0x5d625dff));r(b,Color::hex(0x8b9c99ff));q.rect_lines({x+10,y+10,w-20,h-20},3,cream,L);q.rect_lines({x+16,y+16,w-32,h-32},1,blue,L);for(auto point:std::array<Vec2,4>{{{x+5,y+5},{x+w-5,y+5},{x+5,y+h-5},{x+w-5,y+h-5}}}){q.circle(point,6,ink,L);q.circle(point,4,red,L);}text(ctx,"BLACKWATER",{c.x,c.y-10},12,cream,true,41);text(ctx,"BOXING CLUB",{c.x,c.y+7},8,ink,true,41);break;
    case PropKind::Spill:if(!g.model.jobs[0]){q.ellipse(c,{17,11},blue.with_alpha(140),L);q.ellipse(c+Vec2{10,6},{8,6},blue.with_alpha(130),L);}break;
    case PropKind::LostBook:if(!g.model.bookTaken&&!g.model.jobs[1])itemIcon(q,Item::Book,c,.65f,L);break;
    case PropKind::Shed:r(b,Color::hex(0x625c4dff));r({x-4,y-4,w+8,12},Color::hex(0x7e8270ff));r({x+w-22,y+14,16,h-16},ink);for(int k=0;k<5;++k)r({x+7+k*9.f,y+16,2,h-22},Color::hex(0x8f8c71ff));text(ctx,"TOOLS",{x+30,y+20},8,cream,true,41);break;
    case PropKind::Patch:r(b,Color::hex(0x736451ff));for(int k=0;k<5;++k)for(int j=0;j<3;++j)r({x+10+k*18.f,y+10+j*18.f,6,5},g.model.gardenTended?green:Color::hex(0x9c8a67ff));break;
    case PropKind::Bat:if(!g.model.batTaken)itemIcon(q,Item::Bat,c,.6f,L);break;
    case PropKind::Ball:if(!g.model.ballTaken)itemIcon(q,Item::Ball,c,.7f,L);break;
    case PropKind::Gate:r(b,ink);for(float k=x;k<x+w;k+=8)r({k,y,3,h},muted);break;
    }
}
void overview(Ctx &ctx,const Game &g){auto &q=ctx.res<RenderQueue>();q.rect({0,0,960,600},ink.with_alpha(235),220);box(q,{64,24,832,552});text(ctx,"BLACKWATER",{96,46},24,cream);text(ctx,"COMPOUND DIRECTORY",{96,77},10,orange);text(ctx,"N",{480,48},12,cream,true);
    constexpr float s=.35f;Vec2 origin{256,80};auto rr=[&](Rect r,Color c){q.rect({origin.x+r.x*s,origin.y+r.y*s,r.w*s,r.h*s},c,220);};
    rr({32,32,1216,1216},Color::hex(0x4d5b60ff));rr({64,64,1152,368},Color::hex(0x657277ff));rr({64,480,352,384},Color::hex(0x8b7b62ff));rr({432,480,416,384},Color::hex(0x6a7c7cff));rr({864,480,352,384},Color::hex(0x536b7aff));rr({64,912,1152,304},Color::hex(0x536958ff));
    for(auto c:g.map.cells)rr(c.box,Color::hex(0x8b9897ff));for(auto p:g.map.props)if(p.solid)rr(p.box,p.kind==PropKind::Bed?blue:Color::hex(0xa19b85ff));
    for(auto r:g.map.walls)rr(r,ink);rr(ringBounds,Color::hex(0x9ca9a1ff));rr({128,976,240,192},Color::hex(0x80938aff));
    auto pos=ctx.world().get<Transform>(g.player).position;Vec2 marker=origin+pos*s;q.circle(marker,7,ink,220);q.circle(marker,4,orange,220);text(ctx,"YOU",marker+Vec2{0,-20},10,cream,true);
    text(ctx,"CELL WING",{480,103},10,ink,true);text(ctx,"CAFETERIA",origin+Vec2{240*s,670*s},8,cream,true);text(ctx,"HALL",origin+Vec2{640*s,670*s},10,cream,true);text(ctx,"LIBRARY",origin+Vec2{1040*s,670*s},8,cream,true);text(ctx,"EXERCISE YARD",origin+Vec2{640*s,940*s},10,cream,true);
    text(ctx,"01 / ROUTINE",{96,145},10,orange);text(ctx,"Meals",{96,171},12);text(ctx,"07-09 / 12-14",{96,190},9,muted);text(ctx,"18-20",{96,205},9,muted);text(ctx,"02 / GROW",{96,258},10,blue);text(ctx,"Read in library",{96,284},10);text(ctx,"3 books per day",{96,302},9,muted);text(ctx,"03 / EARN",{96,354},10,green);text(ctx,"Jobs + ring bouts",{96,380},10);text(ctx,"3 conduct = -1 day",{96,398},9,muted);
    text(ctx,"12 CELLS",{736,145},10,muted);text(ctx,"10 INMATES",{736,166},10,muted);text(ctx,"3 OFFICERS",{736,187},10,muted);text(ctx,"YOUR CELL",{736,258},10,orange);text(ctx,"A-01 / NORTHWEST",{736,280},9);text(ctx,"CURFEW",{736,354},10,red);text(ctx,"22:00 - 07:00",{736,376},10);text(ctx,"Return to A-01",{736,397},9,muted);
    text(ctx,g.overview>0?"Entering the common areas...":"M / ESC TO RETURN",{480,548},10,muted,true);
}
void court(Ctx &ctx,const Game &g){auto &q=ctx.res<RenderQueue>();q.rect({0,0,960,600},ink,220);float pan=std::min(g.story,4.f)*3;
    q.rect({150,50+pan,660,430},Color::hex(0x51463fff),220);for(int y=54;y<480;y+=24)q.line({150,static_cast<float>(y)+pan},{810,static_cast<float>(y)+pan},1,Color::hex(0x5d5045ff),220);
    q.rect({380,105+pan,200,68},Color::hex(0x876b50ff),220);q.rect({365,155+pan,230,10},Color::hex(0xb39165ff),220);person(q,{480,124+pan},Actor{.slot=2},0,false,false,true,0,220,2);
    for(float x:{250.f,610.f}){q.rect({x,290+pan,100,60},Color::hex(0x8e7359ff),220);q.rect({x-3,290+pan,106,8},Color::hex(0xbb9b72ff),220);}
    person(q,{300,280+pan},Actor{.slot=0,.officer=true},0,false,false,false,0,220,2);person(q,{660,280+pan},Actor{.slot=-1},0,false,true,false,0,220,2);
    text(ctx,"BLACKWATER COUNTY COURT",{480,45},16,cream,true);text(ctx,"CASE 071 / THE VERDICT",{480,73},10,orange,true);
    if(g.story>4.5f){box(q,{180,416,600,130});text(ctx,"SEVEN DAYS",{480,437},26,orange,true);if(g.story>5.5f){text(ctx,"You are found guilty and sentenced to imprisonment.",{480,480},12,cream,true);text(ctx,"Your choices decide how this sentence ends.",{480,504},10,muted,true);}}
    auto &voice=ctx.local<Broadcast>();if(!voice.subtitle.empty()){box(q,{180,332,600,72});text(ctx,voice.speaker+" / ELEVENLABS",{196,341},9,orange);wrapped(ctx,voice.subtitle,{196,360},10,568);}
    text(ctx,"ENTER / E  SKIP INTRO",{480,572},10,muted,true);
    if(g.story<3)q.rect({0,0,960,600},ink.with_alpha(static_cast<std::uint8_t>(255*(1-g.story/3))),220);
    if(g.story>8)q.rect({0,0,960,600},ink.with_alpha(static_cast<std::uint8_t>(255*std::min(1.f,g.story-8))),220);
}
}
void drawWorld(Ctx &ctx){auto &g=ctx.local<Game>();if(g.phase==Phase::Court)return;auto &q=ctx.res<RenderQueue>();
    q.rect({0,0,1280,1280},ink,8);q.rect({48,48,1184,1184},Color::hex(0x677677ff),8);q.rect({64,64,1152,368},Color::hex(0x69777bff),8);q.rect({64,480,352,384},Color::hex(0x9c8a70ff),8);q.rect({432,480,416,384},Color::hex(0x758685ff),8);q.rect({864,480,352,384},Color::hex(0x617787ff),8);q.rect({48,912,1184,320},Color::hex(0x5f735fff),8);
    for(int y=48;y<1232;y+=32)for(int x=48;x<1232;x+=32){auto col=((x/32+y/32)%2)?ink.with_alpha(13):cream.with_alpha(7);q.rect({static_cast<float>(x),static_cast<float>(y),31,31},col,9);if(y>912&&(x+y)%96==0){q.rect({static_cast<float>(x+5),static_cast<float>(y+14),3,1},green.with_alpha(90),9);q.rect({static_cast<float>(x+6),static_cast<float>(y+12),1,3},green.with_alpha(90),9);}}
    // A painted stripe joins the cell wing, common hall and entrance.
    q.rect({628,448,24,768},Color::hex(0xb8b09aff).with_alpha(100),10);for(int y=460;y<1210;y+=40)q.rect({638,static_cast<float>(y),4,16},cream.with_alpha(80),10);
    for(int i=0;i<12;++i){auto c=g.map.cells[static_cast<std::size_t>(i)];q.rect({c.box.x+8,c.box.y+8,128,112},i==0?Color::hex(0x7c8988ff):Color::hex(0x7a8587ff),12);text(ctx,(i==0?"A-01 / YOUR CELL":std::string("CELL ")+(i<9?"0":"")+std::to_string(i+1)),{c.box.x+72,c.box.y+104},i==0?8:7,i==0?cream:ink.with_alpha(180),true,20);q.rect({c.box.x+64,c.box.y+120,48,4},i==0?orange:muted,20);}
    for(auto p:g.map.props)prop(ctx,p,g);
    for(auto r:g.map.walls){q.rect({r.x+3,r.y+4,r.w,r.h+4},ink.with_alpha(65),28);q.rect(r,Color::hex(0x293c4bff),30);q.rect({r.x,r.y,r.w,std::min(3.f,r.h)},Color::hex(0x51616bff),30);q.rect({r.x,r.y+r.h-2,r.w,2},ink,30);if(r.w>32)for(float x=r.x+16;x<r.x+r.w;x+=32)q.rect({x,r.y+4,1,std::max(1.f,r.h-6)},muted.with_alpha(30),31);}
    if(g.doorClosed){q.rect({144,200,48,6},ink,40);for(float x=144;x<192;x+=7)q.rect({x,194,2,14},muted,41);}
    for(auto d:g.model.drops)itemIcon(q,d.item,d.at,.6f,40);
    auto h=g.map.cells[static_cast<std::size_t>(g.model.hotspot)].hotspot;q.rect_lines({h.x-8,h.y-8,16,16},1,orange.with_alpha(170),22);q.line(h+Vec2{-5,-3},h+Vec2{1,1},1,ink,23);q.line(h+Vec2{1,1},h+Vec2{-2,6},1,ink,23);
    text(ctx,"NORTH WING",{640,395},12,ink.with_alpha(150),true,20);text(ctx,"COMMON HALL",{640,805},12,ink.with_alpha(140),true,20);text(ctx,"CAFETERIA",{240,824},10,cream.with_alpha(150),true,20);text(ctx,"LIBRARY",{1040,515},12,cream.with_alpha(170),true,20);
    const auto player=ctx.world().get<Transform>(g.player).position;
    if(g.escapePatrol){for(int i=10;i<13;++i){auto e=g.actors[static_cast<std::size_t>(i)];auto at=ctx.world().get<Transform>(e).position;auto f=normalize_or(ctx.world().get<Actor>(e).facing,{0,1});float angle=std::atan2(f.y,f.x);Vec2 previous;
        for(int ray=0;ray<=20;++ray){float a=angle-.8632f+1.7264f*static_cast<float>(ray)/20;Vec2 direction{std::cos(a),std::sin(a)},end=at;
            for(float d=4;d<cfg::officerSight;d+=4){auto next=at+direction*d;if(!tile_ray_clear(ctx.local<TileWorld>(),at,next))break;end=next;}
            if(ray>0)q.triangle(at,previous,end,g.activity==Activity::Dig?red.with_alpha(48):gold.with_alpha(38),24);previous=end;
        }
        q.line(at,at+f*22,2,gold,128);
    }}
    for(auto [e,t,a]:ctx.world().view<Transform,Actor>()){
        if(a.slot>=0&&!a.officer&&g.model.inmates[static_cast<std::size_t>(a.slot)].hp<=0)continue;
        auto p=interpolated(ctx.world(),e,ctx.time().alpha).position;auto *v=ctx.world().try_get<Velocity>(e);bool moving=v&&length_sq(v->linear)>2;int gen=a.slot>=0&&!a.officer?g.model.inmates[static_cast<std::size_t>(a.slot)].appearance:0;
        if(e==g.player&&g.invuln>0&&static_cast<int>(g.active*12)%2==0)continue;
        person(q,p,a,g.active,moving,e==g.player,a.slot==9&&!a.officer,gen);
        if(e==g.player){Item held=g.model.bag.held();if(held!=Item::None)itemIcon(q,held,p+Vec2{10,-7},.35f,65);}
        if(e!=g.player&&distance_sq(player,p)<110*110){std::string name=a.officer?std::string(cfg::officers[static_cast<std::size_t>(a.slot)]):std::string(residentName(g.model,a.slot));if(a.slot==9&&!a.officer)name+=" / DEALER";if(!a.officer&&g.model.inmates[static_cast<std::size_t>(a.slot)].helped)name+=" +";text(ctx,name,p+Vec2{0,-37},7,a.officer?blue:a.slot==9?gold:cream,true,128);}
        if(!a.officer&&a.slot>=0){auto slot=static_cast<std::size_t>(a.slot);if(g.brains[slot].hostile||g.model.inmates[slot].hp<5){q.rect({p.x-10,p.y-28,20,2},ink,128);q.rect({p.x-10,p.y-28,4.f*static_cast<float>(g.model.inmates[slot].hp),2},red,128);}if(g.brains[slot].telegraph>0){q.circle(p,21,red.with_alpha(80),63,p.y);q.rect_lines({p.x-18,p.y-18,36,36},1.5f,red,128);text(ctx,"!",p+Vec2{0,-49},12,red,true,128);}}
        if(a.officer&&g.incident){text(ctx,g.incidentTime<1.4f?"STOP!":"!",p+Vec2{0,-48},9,red,true,128);}
    }
    if(g.attackVisual>0){q.circle(player+g.face*20,10,cream.with_alpha(95),128);q.circle(player+g.face*23,4,orange.with_alpha(180),128);}
    if(g.activity==Activity::Basketball){float t=std::clamp(g.progress/g.duration,0.f,1.f);Vec2 goal{248,990};Vec2 ball=player+(goal-player)*t+Vec2{0,-std::sin(t*3.14159265f)*64};itemIcon(q,Item::Ball,ball,.6f,128);}
    if(g.activity==Activity::Pool){auto center=g.map.props[static_cast<std::size_t>(g.activitySource)].box.center();float t=std::clamp(g.progress/g.duration,0.f,1.f);q.circle(center+Vec2{36*t,-20*t},3,cream,128);}
    if(g.activity==Activity::Phone){auto at=g.map.props[static_cast<std::size_t>(g.activitySource)].box.center();q.line(at,player+Vec2{7,-16},1,ink,128);q.rect({player.x+6,player.y-22,3,9},cream,128);}
    if(g.activity==Activity::Dig||g.activity==Activity::Garden)itemIcon(q,Item::Shovel,player+Vec2{12,-7+std::sin(g.progress*8)*3},.5f,128);
    if(g.focusProp>=0){auto p=g.map.props[static_cast<std::size_t>(g.focusProp)].interaction;q.rect_lines({p.x-9,p.y-7,18,14},1,cream.with_alpha(170),128);}
}
void drawHUD(Ctx &ctx){auto &g=ctx.local<Game>();auto &q=ctx.res<RenderQueue>();if(g.phase==Phase::Court){court(ctx,g);return;}
    auto p=ctx.world().get<Transform>(g.player).position;
    box(q,{20,20,200,160});q.rect({20,20,3,160},orange,220);text(ctx,"PRISONER 071",{36,32},11,cream);
    bar(ctx,"HEALTH",static_cast<float>(g.model.stats.hp),10,{36,54},red);bar(ctx,"SANITY",g.model.stats.sanity,100,{36,82},gold);bar(ctx,"INTELLIGENCE",g.model.stats.intelligence,100,{36,110},blue);bar(ctx,"AGILITY",g.model.stats.agility,100,{36,138},green);
    box(q,{304,20,352,54});text(ctx,"BLACKWATER",{480,29},20,cream,true);text(ctx,zoneName(p),{480,55},9,muted,true);
    box(q,{740,20,200,144});text(ctx,"CASH / $500 MAX",{756,32},9,muted);text(ctx,"$"+std::to_string(g.model.stats.money),{756,48},26,gold);
    text(ctx,"REP "+std::to_string(g.model.stats.reputation)+" / 12",{756,82},11,cream);text(ctx,std::to_string(g.model.daysLeft)+" DAYS LEFT",{756,102},11,cream);
    char clock[64];int t=timeOfDay(g.model.clock);std::snprintf(clock,sizeof(clock),"DAY %02d  %02d:%02d",g.model.day,t/60,t%60);text(ctx,clock,{756,124},11,curfew(g.model.clock)?red:green);text(ctx,curfew(g.model.clock)?"CURFEW / RETURN TO A-01":"FREE TIME",{756,144},8,curfew(g.model.clock)?red:muted);
    box(q,{20,500,440,80});text(ctx,"YOUR SENTENCE. YOUR CHOICES.",{36,513},9,orange);
    std::string objective=curfew(g.model.clock)?"Return to A-01. Rest at your bunk.":g.model.bag.has(Item::Book)?"Return the lost book at a library shelf.":g.model.loan?(g.model.gardenTended?"Return the loaned shovel for payment.":"Tend the garden. Or find a way out."):g.model.stats.intelligence>=20?"Loose tile: cell "+std::to_string(g.model.hotspot+1)+". Bring a shovel.":"Earn $20: clean the marked hall spill.";
    if(g.escapePatrol)objective="Escape: "+std::to_string(g.model.digSteps)+" / 5 digs. Stay out of guard sight.";
    else if(g.shovelPressure)objective="Reach loose tile in cell "+std::to_string(g.model.hotspot+1)+". Prisoners are hunting.";
    text(ctx,objective,{36,533},11,cream);text(ctx,"CONDUCT "+std::to_string(g.model.conduct)+"/3   BOOKS "+std::to_string(g.model.books)+"/3    M MAP / ESC PAUSE",{36,559},9,muted);
    for(int i=0;i<3;++i){float x=634+static_cast<float>(i)*104.f;bool selected=g.model.bag.selected==i;box(q,{x,500,98,80},selected?Color::hex(0x283b43ff):panel);if(selected)q.rect({x,500,98,2},orange,220);text(ctx,std::to_string(i+1),{x+7,507},10,muted);itemIcon(q,g.model.bag.slots[static_cast<std::size_t>(i)],{x+49,530},.7f);std::string n(itemName(g.model.bag.slots[static_cast<std::size_t>(i)]));if(n=="Kitchen knife")n="Knife";if(n=="Loaned shovel")n="Loan shovel";text(ctx,n,{x+49,557},9,cream,true);}
    Item held=g.model.bag.held();
    const char *action=held==Item::None?"SPACE PUNCH":held==Item::Knife||held==Item::Bat?"SPACE ATTACK":held==Item::Medkit||held==Item::Adrenaline||held==Item::Soda||held==Item::Chips?"SPACE USE":isFood(held)?"E EAT AT CAFETERIA SEAT":"E INTERACT";
    text(ctx,std::string(action)+"   G DROP",{786,485},9,muted,true);
    if(g.noticeTime>0&&!g.notice.empty()){box(q,{80,436,800,32},Color::hex(0x1d3038ed));text(ctx,g.notice,{480,447},10,cream,true);}
    if(!g.prompt.empty()&&g.menu==Menu::None){box(q,{280,382,400,36});text(ctx,g.prompt,{480,395},10,cream,true);}
    if(g.escapePatrol&&g.phase==Phase::Play&&g.menu==Menu::None){box(q,{300,283,360,36});text(ctx,"DIGS "+std::to_string(g.model.digSteps)+" / 5   "+(diggingWatched(ctx)?"WATCHED / WAIT":"UNSEEN / WATCH THEIR TURNS"),{480,296},10,diggingWatched(ctx)?red:green,true);}
    if(g.activity!=Activity::None){box(q,{300,332,360,40});q.rect({316,357,328,4},border,220);q.rect({316,357,328*std::clamp(g.progress/g.duration,0.f,1.f),4},gold,220);constexpr std::array<std::string_view,11> activities{"","READING","DIGGING / HOLD E","CLEANING","SERVING","GARDENING","CALLING HOME","POOL SHOT","BASKETBALL","EATING","RESTING"};text(ctx,std::string(activities[static_cast<std::size_t>(g.activity)])+" / MOVE TO CANCEL",{480,342},10,cream,true);if(g.activity==Activity::Rest)q.rect({0,0,960,600},ink.with_alpha(static_cast<std::uint8_t>(200*g.progress/g.duration)),220);}
    if(g.phase==Phase::Escort){q.rect({0,232,960,92},ink.with_alpha(220),220);text(ctx,"WELCOME TO BLACKWATER",{480,250},22,cream,true);text(ctx,"Officer Hale is taking you to cell A-01. ENTER to skip.",{480,289},11,muted,true);}
    if(g.menu==Menu::None&&!ctx.local<Broadcast>().subtitle.empty()){
        auto &b=ctx.local<Broadcast>();box(q,{236,90,488,76});text(ctx,b.speaker+" / ELEVENLABS",{252,99},9,orange);wrapped(ctx,b.subtitle,{252,117},10,456);
    }
    text(ctx,"V / ELEVENLABS AUDIO",{480,80},8,muted,true);
    if(g.menu==Menu::Map||g.overview>0){overview(ctx,g);return;}
    if((g.phase==Phase::Escaped||g.phase==Phase::Released)&&g.story<2.5f){q.rect({0,0,960,600},ink,220);float endFade=std::min(1.f,g.story/2.5f);if(g.phase==Phase::Escaped){q.ellipse({480,286},{200,210},Color::hex(0x273b43ff),220);q.ellipse({480,256},{130,160},Color::hex(0x496264ff),220);q.rect({430-endFade*60,120,100+endFade*120,200},cream,220);person(q,{480,390},Actor{.slot=-1},g.story,true,true,false,0,220,2);}else {q.rect({300,120,360,320},Color::hex(0x89a69bff),220);for(int k=0;k<8;++k){float x=310+static_cast<float>(k)*48.f+(k<4?-1.f:1.f)*endFade*190;q.rect({x,120,7,320},muted,220);}person(q,{480,390-endFade*60},Actor{.slot=-1},g.story,true,true,false,0,220,2);}text(ctx,g.phase==Phase::Escaped?"A WAY OUT":"THE GATES OPEN",{480,492},24,cream,true);return;}
    if(g.menu==Menu::None&&g.phase==Phase::Play)return;
    if(g.phase==Phase::Escort)return;
    q.rect({0,0,960,600},ink.with_alpha(220),220);
    if(g.menu==Menu::Audio){
        auto &b=ctx.local<Broadcast>();box(q,{72,64,816,456});text(ctx,"ELEVENLABS AUDIO SHOWCASE",{96,80},10,orange);
        text(ctx,"THE VOICE OF BLACKWATER",{96,101},24,cream);text(ctx,"Voiced story / prison announcements / calls from home / corridor ambience",{96,139},10,muted);
        box(q,{96,180,352,276});box(q,{464,180,400,276});text(ctx,"READY TO PLAY / OFFLINE",{112,195},11,gold);text(ctx,"LIVE API / PERSONALISED ROLL CALL",{480,195},11,gold);
        text(ctx,b.keyFound?"LOCAL API KEY FOUND / KEPT PRIVATE":"LOCAL KEY MISSING / OFFLINE AUDIO AVAILABLE",{480,223},9,b.keyFound?green:muted);
        wrapped(ctx,b.liveStatus,{480,247},9,368,b.liveHttp==200?green:b.liveHttp>0?red:cream);
        std::string receipt="REQUESTS "+std::to_string(b.liveRequests)+"  /  HTTP "+(b.liveHttp?std::to_string(b.liveHttp):"--");
        if(b.liveCredits>=0)receipt+="  /  CREDITS "+std::to_string(static_cast<int>(b.liveCredits));text(ctx,receipt,{480,306},9,muted);
        wrapped(ctx,"Uses your current day and sentence. One API call per click. Your key is never displayed.",{480,380},9,368,muted);
        text(ctx,std::to_string(b.lines.size())+" original voice recordings / ElevenLabs API",{112,433},9,muted);
        box(q,{96,466,768,46});text(ctx,b.subtitle.empty()?"MADE WITH ELEVENLABS":b.speaker,{112,473},9,orange);
        wrapped(ctx,b.subtitle.empty()?"Choose a recording, or generate a real roll call for this playthrough.":b.subtitle,{112,488},9,736);
        return;
    }
    if(g.menu==Menu::Dealer||g.menu==Menu::Food){bool shop=g.menu==Menu::Dealer;box(q,{72,100,816,408});text(ctx,shop?"VEX'S SUPPLY":"THE DAILY MEAL",{96,122},24,cream);text(ctx,shop?"DISCREET GOODS / THREE CARRY SLOTS":"ONE FREE MEAL PER WINDOW / EAT AT A FREE SEAT",{96,154},10,muted);text(ctx,"$"+std::to_string(g.model.stats.money)+"   FREE SLOTS "+std::to_string(3-g.model.bag.count()),{650,133},11,gold);
        for(int i=0;i<4;++i){float x=96+194.f*static_cast<float>(i);box(q,{x,184,176,248},Color::hex(0x1c2d38ff));q.rect({x+8,192,160,74},ink,220);Item item=shop?static_cast<Item>(i+1):static_cast<Item>(static_cast<int>(Item::Rice)+i);itemIcon(q,item,{x+88,228},1.5f);text(ctx,itemName(item),{x+88,284},i==0&&shop?12:14,cream,true);text(ctx,shop?cfg::effects[static_cast<std::size_t>(i)]:"Restore sanity",{x+88,310},9,muted,true);text(ctx,shop?"$"+std::to_string(cfg::prices[static_cast<std::size_t>(i)]):"+"+std::to_string(cfg::foodGains[static_cast<std::size_t>(i)])+" SANITY",{x+88,338},16,gold,true);}
        if(shop){text(ctx,"SPACE uses selected consumables. G drops with confirmation. ESC closes.",{480,490},9,muted,true);}
        else {int meal=mealWindow(g.model.clock);text(ctx,meal<0?"COUNTER CLOSED / 07-09, 12-14, 18-20":g.model.meals[static_cast<std::size_t>(meal)]?"TODAY'S WINDOW MEAL ALREADY COLLECTED":"COUNTER OPEN / SELECT ONE MEAL",{480,450},10,meal<0?red:green,true);}
    }else if(g.menu==Menu::Dialogue){box(q,{220,112,520,384});auto slot=static_cast<std::size_t>(std::max(0,g.dialogueSlot));text(ctx,residentName(g.model,static_cast<int>(slot)),{480,139},26,cream,true);text(ctx,g.model.inmates[slot].helped?"YOU HAVE ALREADY HELPED THIS RESIDENT":"A SMALL CONVERSATION CAN GO A LONG WAY",{480,178},10,muted,true);
        constexpr std::array<std::string_view,9> lines{"First day? Find your rhythm. You'll be okay.","I miss a proper cup of coffee.","Books make these walls feel a little wider.","Keep your promises. People remember.","There's always a fair match in the ring.","You can buy things. Respect takes time.","A little help means more in here.","The garden's quiet. I like that.","Seven days can change a person."};text(ctx,lines[slot%9],{480,222},11,cream,true);
    }else if(g.menu==Menu::Ring||g.menu==Menu::ConfirmRing){box(q,{220,112,520,384});text(ctx,"BLACKWATER BOXING CLUB",{480,142},20,cream,true);text(ctx,"SANCTIONED / PUNCHES ONLY",{480,178},11,orange,true);text(ctx,"Your health: "+std::to_string(g.model.stats.hp)+" / 10   Opponent: 5 HP",{480,218},12,cream,true);
        if(g.menu==Menu::Ring)text(ctx,"Choose a pace. Win a seeded $30-$50 purse.",{480,244},10,muted,true);
        else {constexpr std::array<std::string_view,3> names{"EASY","MEDIUM","HARD"};text(ctx,names[static_cast<std::size_t>(g.difficulty)],{480,256},24,gold,true);text(ctx,"One existing inmate. One bout. No officer penalty.",{480,295},10,muted,true);text(ctx,"Watch the red telegraph and step out of range.",{480,316},10,muted,true);}
    }else if(g.menu==Menu::Vending){box(q,{240,128,480,344});text(ctx,"A SMALL COMFORT",{480,157},22,cream,true);q.rect({436,208,34,52},blue,220);q.rect({440,204,26,6},cream,220);q.rect({440,220,26,22},ink,220);q.rect({490,210,38,48},gold,220);q.rect({490,206,38,5},orange,220);text(ctx,"SODA",{453,227},8,cream,true);text(ctx,"CHIPS",{509,232},8,ink,true);text(ctx,"$5 / +10 SANITY / RANDOM PICKUP",{480,284},11,gold,true);text(ctx,"Needs a free slot. SPACE to consume later.",{480,309},10,muted,true);
    }else if(g.menu==Menu::Drop){box(q,{220,140,520,320});text(ctx,"LEAVE IT BEHIND?",{480,172},24,cream,true);itemIcon(q,g.model.bag.held(),{480,240},1.4f);text(ctx,itemName(g.model.bag.held()),{480,280},14,gold,true);text(ctx,"The item stays nearby and can be recovered with E.",{480,307},10,muted,true);
    }else if(g.menu==Menu::Help){box(q,{140,64,680,464});text(ctx,"FIND YOUR RHYTHM",{480,91},24,cream,true);text(ctx,"WASD / ARROWS",{180,146},12,orange);text(ctx,"Move   /   SHIFT runs using agility",{380,146},12);text(ctx,"E",{180,180},12,orange);text(ctx,"Interact   /   Hold to dig at a loose tile",{380,180},12);text(ctx,"SPACE",{180,214},12,orange);text(ctx,"Attack / use selected consumable",{380,214},12);text(ctx,"1 / 2 / 3",{180,248},12,orange);text(ctx,"Select one of three carried items",{380,248},12);text(ctx,"G",{180,282},12,orange);text(ctx,"Drop selected item with confirmation",{380,282},12);text(ctx,"M / ESC",{180,316},12,orange);text(ctx,"Compound map / pause or close",{380,316},12);text(ctx,"Earn money from jobs and sanctioned bouts. Read to learn.",{480,369},11,green,true);text(ctx,"Three conduct points remove a sentence day, once per day.",{480,394},11,muted,true);text(ctx,"Return to A-01 at curfew. Rest never restores health.",{480,419},11,muted,true);
    }else {box(q,{220,140,520,320});std::string title=g.phase==Phase::Infirmary?"INFIRMARY":g.phase==Phase::Released?"SENTENCE SERVED":g.phase==Phase::Escaped?"BEYOND THE WALLS":"TAKE A BREATH";text(ctx,title,{480,171},24,g.phase==Phase::Escaped?green:cream,true);text(ctx,g.phase==Phase::Infirmary?"Retry rolls back to the last day-start checkpoint.":g.phase==Phase::Released?"The gates open. You earned a lawful release.":g.phase==Phase::Escaped?"The tunnel fades into daylight. Your story continues.":"GAME PAUSED / CLOCK AND ACTIVE TIMERS STOPPED",{480,215},11,muted,true);}
    if(g.noticeTime>0){box(q,{80,540,800,36},panel);text(ctx,g.notice,{480,553},g.notice.size()>110?8:10,cream,true);}
}
void updateUI(Ctx &ctx){auto &g=ctx.local<Game>();auto &ui=ctx.local<UiTree>();if(!g.uiDirty&&g.builtMenu==g.menu&&g.builtPhase==g.phase)return;g.uiDirty=false;g.builtMenu=g.menu;g.builtPhase=g.phase;ui.clear();
    if((g.phase==Phase::Escaped||g.phase==Phase::Released)&&g.story<2.5f)return;
    struct Theme {AssetId<UiTheme> id;};auto *theme=ctx.try_local<Theme>();if(!theme){UiTheme t;t.font_size=12;t.text=cream;t.panel=panel;t.panel_border=border;t.button=Color::hex(0x304751ff);t.button_focus=Color::hex(0x526b72ff);t.button_down=Color::hex(0x21363fff);theme=&ctx.world().resources().insert<Theme>(Theme{ctx.res<AssetServer>().add(t,"prison.theme")});}ui.set_theme(theme->id);
    auto button=[&](std::string_view label,Rect rect,int command){auto id=ui.button(0,str_id(label),label);auto *n=ui.get(id);n->rect=rect;n->intent=str_id("prison.command");n->intent_a=command;return id;};
    auto row=[&](std::string_view label,float y,int command){return button(label,{340,y,280,34},command);};
    if(g.menu==Menu::Pause){ui.focus(row("Resume",250,1));row("Compound map",290,2);row("Controls",330,3);row("Load day-start checkpoint",370,6);row("ElevenLabs audio showcase / V",410,70);}
    if(g.menu==Menu::Audio){
        auto &b=ctx.local<Broadcast>();constexpr std::array<std::string_view,5> labels{"Play prison PA","Play call from home","Play the judge's verdict","Play release message","Play escape ending"};
        for(int i=0;i<5;++i){auto id=button(labels[static_cast<std::size_t>(i)],{112,226+i*40.f,320,32},73+i);if(i==0)ui.focus(id);}
        auto live=button(b.busy?"Generating...":"Generate live roll call",{480,330,368,38},71);ui.get(live)->enabled=!b.busy&&b.keyFound&&b.enabled;
        button(b.enabled?"Audio: ON":"Audio: OFF",{480,416,176,28},72);button("Back to game",{672,416,176,28},1);
    }
    if(g.menu==Menu::Help)ui.focus(row("Back to game",470,1));
    if(g.menu==Menu::Dealer||g.menu==Menu::Food){for(int i=0;i<4;++i){auto id=button(g.menu==Menu::Dealer?"Buy":"Pick up",{112+194.f*static_cast<float>(i),378,144,34},(g.menu==Menu::Dealer?10:30)+i);if(i==0)ui.focus(id);}button("X",{844,116,28,28},1);if(g.menu==Menu::Food)button("Help serve (+$20 / +1 conduct)",{316,468,328,28},34);else {button("Talk / hear a tip",{192,450,250,28},20);auto help=button(g.model.inmates[9].helped?"Already helped Vex":"Offer support (+2 reputation)",{458,450,310,28},21);ui.get(help)->enabled=!g.model.inmates[9].helped;}}
    if(g.menu==Menu::Dialogue){ui.focus(row("Talk / hear a tip",282,20));row("Offer support (+2 reputation)",326,21);row("Leave",370,1);}
    if(g.menu==Menu::Ring){ui.focus(row("Easy / patient opponent",278,40));row("Medium / steady opponent",322,41);row("Hard / quick opponent",366,42);row("Leave",428,1);}
    if(g.menu==Menu::ConfirmRing){ui.focus(row("Confirm / start bout",354,43));row("Cancel",406,1);}
    if(g.menu==Menu::Vending){ui.focus(row("Pick up snack / $5",354,60));row("Leave",400,1);}
    if(g.menu==Menu::Drop){ui.focus(row("Confirm drop",350,50));row("Keep item",396,1);}
    if(g.phase==Phase::Infirmary){ui.focus(row("Retry day-start",280,4));row("New game",326,5);}
    if(g.phase==Phase::Released||g.phase==Phase::Escaped)ui.focus(row("Start a new sentence",300,5));
}
}
