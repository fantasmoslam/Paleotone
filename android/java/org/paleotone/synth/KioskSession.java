/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;

/** Policy ordering shared by Android and the failure-injection host tests. */
final class KioskSession {
    interface Platform {
        boolean owner(); boolean ready(); boolean locked();
        void enabled(boolean value);
        void allow(); void home(); void lock(); void unlock();
        void clearAllowlist(); void clearHome();
    }
    static void enter(Platform p){
        if(!p.owner())throw new IllegalStateException("Device-owner setup is required first.");
        if(!p.ready())throw new IllegalStateException("Unlock the tablet and return to Paleotone first.");
        try {
            p.allow();p.home();p.enabled(true);
            if(!p.locked())p.lock();
        }catch(RuntimeException e){
            exit(p);throw e;
        }
    }
    static void exit(Platform p){
        /* Persist escape before releasing policy so onResume/reboot cannot relock. */
        RuntimeException failure=null;
        try{p.enabled(false);}catch(RuntimeException e){failure=e;}
        try{if(p.locked())p.unlock();}catch(RuntimeException e){failure=e;}
        if(p.owner()){
            try{p.clearAllowlist();}catch(RuntimeException e){failure=e;}
            try{p.clearHome();}catch(RuntimeException e){failure=e;}
        }
        if(failure!=null)throw failure;
    }
}
