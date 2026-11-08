// SPDX-License-Identifier: GPL-3.0-only
#include "../smk/native/SmkProfile.h"
#include <cassert>
#include <vector>
#include <array>
int main() {
 SmkProfile p;std::vector<std::array<int,2>> turns;int master=500;
 auto feed=[&](int st,int cc,int v){uint8_t m[]={uint8_t(st),uint8_t(cc),uint8_t(v)};return p.control(m,3,master,[&](int r,int d){turns.push_back({r,d});},[&](int v){master=v;});};
 assert(feed(0xBF,20,100)&&turns.empty());assert(feed(0xBF,20,104));assert((turns.back()==std::array<int,2>{3,2}));
 feed(0xBF,21,0);feed(0xBF,21,127);assert((turns.back()==std::array<int,2>{4,63}));
 assert(!feed(0xB0,20,1));assert(!feed(0xBF,24,1));
 feed(0xB0,111,127);assert(p.faderMain);feed(0xB0,111,127);assert(p.faderMain);
 turns.clear();feed(0xBF,20,0);assert(master==500&&turns.empty());feed(0xBF,20,127);assert(master==1004);
 feed(0xBF,20,0);assert(master==500); // remainder retained, no full-scale jump
 feed(0xB0,111,0);feed(0xB0,111,127);assert(!p.faderMain);assert(p.last[0]<0);
 feed(0xB0,110,127);assert(p.knobFine);feed(0xB0,110,127);assert(p.knobFine);
 uint8_t on[]={0x90,84,71},off[]={0x80,84,0},ped[]={0xB0,64,127};p.observe(on,3);assert(p.outer==(uint64_t(1)<<36));p.observe(off,3);assert(!p.outer);p.observe(on,3);p.observe(ped,3);
 std::vector<std::array<int,3>> released;p.release([&](const uint8_t*m,unsigned){released.push_back({m[0],m[1],m[2]});});
 assert(released.size()==4&&released[0][1]==84);assert(!p.outer&&!p.pedal&&!p.faderMain&&!p.knobFine&&p.last[0]<0);
 for(int i=0;i<4;i++){feed(0xBF,20+i,64);assert(p.last[i]==64);}p.rearm();for(int i=0;i<4;i++)assert(p.last[i]<0);
 SmkProfile ownership;int messages=0;uint8_t nOn[]={0x90,48,100},nOff[]={0x80,48,0};
 auto send=[&](const uint8_t*,unsigned){messages++;};
 assert(ownership.outerNote(nOn,3,false,send));assert(messages==1);
 ownership.outerNote(nOn,3,true,send);assert(messages==1);
 ownership.outerNote(nOff,3,false,send);assert(messages==1);
 ownership.outerNote(nOff,3,true,send);assert(messages==2&&!ownership.outer&&!ownership.touchOuter);
 ownership.outerNote(nOn,3,true,send);ownership.release(send);assert(messages==7&&!ownership.touchOuter);
 p.knobFine=true;assert(p.scale(3,1)==0);assert(p.scale(3,1)==1);assert(p.scale(0,1)==1);
}
