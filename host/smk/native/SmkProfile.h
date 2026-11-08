// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <algorithm>

// Capability 11 belongs to this project's SMK profile v1; legacy FM-1 peers never set it.
// Faders arrive as absolute CC 20..23 on channel 16 inside MIDI_IN. Conversion
// is intentionally relative takeover: the first reading/bank change is a baseline,
// never an immediate parameter write. No parameter value is guessed from screen pixels.
struct SmkProfile {
    int last[4] = {-1,-1,-1,-1};
    int rem[4] = {};
    bool knobFine = false, faderMain = false;
    bool bankHeld[2] = {};
    int fine[7] = {};
    int scale(int role, int delta) {
        if(!knobFine || role<3 || role>6) return delta;
        fine[role]+=delta;int out=fine[role]/2;fine[role]-=out*2;return out;
    }
    uint64_t outer = 0, touchOuter = 0;
    int bend = 8192, mod = 0, pedal = 0;
    void rearm() { for(int i=0;i<4;i++) {last[i]=-1;rem[i]=0;} }
    // Returns true only for project control messages. Ordinary MIDI remains MIDI.
    template<class Turn, class Master> bool control(const uint8_t* d, unsigned n, int master, Turn turn, Master setMaster) {
        if(n!=3) return false;
        if(d[0]==0xB0 && (d[1]==110 || d[1]==111)) {
            int bank=d[1]-110; bool held=d[2]>=64;
            if(held && !bankHeld[bank]) {
                if(bank==0) knobFine=!knobFine;
                else {faderMain=!faderMain; rearm();}
            }
            bankHeld[bank]=held;
            return true;
        }
        if(d[0]!=0xBF || d[1]<20 || d[1]>23) return false;
        int i=d[1]-20, value=d[2]&127;
        if(last[i]<0) {last[i]=value;return true;}
        rem[i]+=value-last[i];last[i]=value;
        int delta=rem[i]/2;rem[i]-=delta*2;
        if(!delta) return true;
        if(faderMain && i==0) setMaster(std::clamp(master+delta*8,0,1023));
        else turn(faderMain ? i-1 : i+3, delta);
        return true;
    }
    template<class Send> bool outerNote(const uint8_t* d,unsigned n,bool touch,Send send) {
        if(n!=3 || (d[0]!=0x90 && d[0]!=0x80) || !((d[1]>=48 && d[1]<=52)||(d[1]>=80 && d[1]<=84)))return false;
        const uint64_t before=outer|touchOuter,bit=uint64_t(1)<<(d[1]-48);
        uint64_t &held=touch?touchOuter:outer;
        if(d[0]==0x90 && d[2])held|=bit;else held&=~bit;
        if(before!=(outer|touchOuter))send(d,n);
        return true;
    }
    void observe(const uint8_t* d, unsigned n) {
        if(n!=3) return;
        int note=d[1];
        if((d[0]==0x90 || d[0]==0x80) && ((note>=48 && note<=52)||(note>=80 && note<=84))) {
            int k=note-48;
            if(d[0]==0x90 && d[2]) outer|=uint64_t(1)<<k;
            else outer&=~(uint64_t(1)<<k);
        }
        if(d[0]==0xE0) bend=d[1]+128*d[2];
        if(d[0]==0xB0 && d[1]==1) mod=d[2];
        if(d[0]==0xB0 && d[1]==64) pedal=d[2];
    }
    template<class Send> void release(Send send) {
        for(int k=0;k<37;k++) if(((outer|touchOuter)>>k)&1) { uint8_t m[]={0x80,uint8_t(k+48),0};send(m,3); }
        uint8_t sustain[]={0xB0,64,0}; send(sustain,3);
        uint8_t center[]={0xE0,0,64}; send(center,3);
        uint8_t modulation[]={0xB0,1,0}; send(modulation,3);
        *this=SmkProfile{};
    }
};
