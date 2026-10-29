/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
import java.util.*;
public final class KioskSessionTest {
    static class Fake implements KioskSession.Platform {
        boolean owner=true,ready=true,locked,enabled,allowed,home;
        String fail="";List<String> log=new ArrayList<>();
        void step(String name){log.add(name);if(name.equals(fail)){fail="";throw new IllegalStateException(name);}}
        public boolean owner(){return owner;}public boolean ready(){return ready;}public boolean locked(){return locked;}
        public void enabled(boolean x){step("enabled="+x);enabled=x;}
        public void allow(){step("allow");allowed=true;}
        public void home(){step("home");home=true;}
        public void lock(){step("lock");assert allowed&&home&&enabled;locked=true;}
        public void unlock(){step("unlock");locked=false;}
        public void clearAllowlist(){step("clearAllowlist");allowed=false;locked=false;}
        public void clearHome(){step("clearHome");home=false;}
    }
    public static void main(String[] args){
        Fake f=new Fake();KioskSession.enter(f);assert f.enabled&&f.locked&&f.home;
        assert f.log.equals(Arrays.asList("allow","home","enabled=true","lock"));
        f.log.clear();KioskSession.exit(f);assert !f.enabled&&!f.locked&&!f.home&&!f.allowed;
        assert f.log.get(0).equals("enabled=false");
        for(String step:new String[]{"allow","home","enabled=true","lock"}){
            f=new Fake();f.fail=step;
            try{KioskSession.enter(f);throw new AssertionError("missing failure");}catch(IllegalStateException expected){}
            assert !f.enabled&&!f.locked&&!f.home&&!f.allowed;
        }
        f=new Fake();f.owner=false;
        try{KioskSession.enter(f);throw new AssertionError();}catch(IllegalStateException expected){}
        assert f.log.isEmpty();
        f=new Fake();f.ready=false;
        try{KioskSession.enter(f);throw new AssertionError();}catch(IllegalStateException expected){}
        assert f.log.isEmpty();
        f=new Fake();KioskSession.enter(f);f.fail="unlock";
        try{KioskSession.exit(f);throw new AssertionError();}catch(IllegalStateException expected){}
        assert !f.locked&&!f.enabled&&!f.home&&!f.allowed;
        f=new Fake();KioskSession.enter(f);f.fail="enabled=false";
        try{KioskSession.exit(f);throw new AssertionError();}catch(IllegalStateException expected){}
        assert !f.locked&&!f.home&&!f.allowed; // Policy release continues even if disk persistence fails.
        System.out.println("PASS: kiosk policy order, no-owner/keyguard gate, entry rollback, durable exit and failure cleanup");
    }
}
