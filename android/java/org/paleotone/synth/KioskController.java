/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
import android.app.*;
import android.app.admin.DevicePolicyManager;
import android.content.*;
import android.provider.Settings;
import android.view.View;

final class KioskController {
    private static boolean maintenanceRequested;
    static boolean enabled(Context c){return c.getSharedPreferences("kiosk",Context.MODE_PRIVATE).getBoolean("enabled",false);}
    static final class AndroidPlatform implements KioskSession.Platform {
        final Context context;final Activity activity;final DevicePolicyManager dpm;
        final ComponentName admin;
        AndroidPlatform(Context c,Activity a){
            context=c;activity=a;dpm=(DevicePolicyManager)c.getSystemService(Context.DEVICE_POLICY_SERVICE);
            admin=new ComponentName(c,KioskAdminReceiver.class);
        }
        public boolean owner(){return dpm!=null&&dpm.isDeviceOwnerApp(context.getPackageName());}
        public boolean ready(){
            KeyguardManager k=(KeyguardManager)context.getSystemService(Context.KEYGUARD_SERVICE);
            return activity!=null&&activity.hasWindowFocus()&&(k==null||!k.isKeyguardLocked());
        }
        public boolean locked(){
            return ((ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE)).getLockTaskModeState()!=ActivityManager.LOCK_TASK_MODE_NONE;
        }
        public void enabled(boolean value){
            if(!context.getSharedPreferences("kiosk",Context.MODE_PRIVATE).edit().putBoolean("enabled",value).commit())
                throw new IllegalStateException("Could not save kiosk state.");
        }
        public void allow(){
            dpm.setLockTaskPackages(admin,new String[]{context.getPackageName()});
            /* Keep power menu usable for ordinary reboot/power-off. */
            dpm.setLockTaskFeatures(admin,DevicePolicyManager.LOCK_TASK_FEATURE_GLOBAL_ACTIONS);
        }
        public void home(){
            IntentFilter filter=new IntentFilter(Intent.ACTION_MAIN);
            filter.addCategory(Intent.CATEGORY_HOME);filter.addCategory(Intent.CATEGORY_DEFAULT);
            dpm.addPersistentPreferredActivity(admin,filter,new ComponentName(context,MainActivity.class));
        }
        public void lock(){if(!dpm.isLockTaskPermitted(context.getPackageName()))throw new IllegalStateException("App is not allowlisted");activity.startLockTask();}
        public void unlock(){if(activity!=null)activity.stopLockTask();}
        public void clearAllowlist(){dpm.setLockTaskPackages(admin,new String[0]);dpm.setLockTaskFeatures(admin,DevicePolicyManager.LOCK_TASK_FEATURE_NONE);}
        public void clearHome(){dpm.clearPackagePersistentPreferredActivities(admin,context.getPackageName());}
    }
    static void restore(Activity activity){
        if(maintenanceRequested||!enabled(activity))return;
        AndroidPlatform p=new AndroidPlatform(activity,activity);
        if(!p.owner()||!p.ready()||p.locked())return;
        try{KioskSession.enter(p);}catch(RuntimeException e){maintenanceRequested=true;message(activity,e.getMessage());}
    }
    static void exit(Context context,Activity activity){
        maintenanceRequested=true;
        KioskSession.exit(new AndroidPlatform(context,activity));
        if(activity!=null)activity.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
    }
    static void menu(MainActivity activity){
        boolean owner=new AndroidPlatform(activity,activity).owner();
        String title=enabled(activity)?"Kiosk maintenance":"Paleotone kiosk setup";
        String[] choices=owner?
            new String[]{"Enable kiosk / return to instrument","Exit kiosk and open Android settings","Keep kiosk disabled","Remove device management"}:
            new String[]{"Device-owner setup instructions","Open Android settings"};
        new AlertDialog.Builder(activity).setTitle(title).setItems(choices,(dialog,index)->{
            try{
                if(!owner){
                    if(index==0)message(activity,"Connect to your PC and complete device-owner setup using the supplied instructions. Then return here to enable kiosk mode. No reset is performed by Paleotone.");
                    else activity.startActivity(new Intent(Settings.ACTION_SETTINGS));
                }else if(index==0){
                    /* Wait until this dialog releases window focus. */
                    activity.getWindow().getDecorView().postDelayed(()->{
                        try{KioskSession.enter(new AndroidPlatform(activity,activity));maintenanceRequested=false;
                            activity.notice("Kiosk enabled • hold PALEOTONE for 3 seconds to exit");
                        }catch(RuntimeException e){maintenanceRequested=true;message(activity,e.getMessage());}
                    },250);
                }else if(index==3){
                    new AlertDialog.Builder(activity).setTitle("Remove Paleotone device management?")
                        .setMessage("This disables kiosk mode and removes Paleotone as device owner. Your saved session stays on the tablet.")
                        .setPositiveButton("Remove",(d,w)->{
                            try{
                                exit(activity,activity);
                                new AndroidPlatform(activity,activity).dpm.clearDeviceOwnerApp(activity.getPackageName());
                                activity.notice("Device management removed • ordinary app mode");
                            }catch(RuntimeException e){message(activity,e.getMessage());}
                        }).setNegativeButton("Cancel",null).show();
                }else{
                    exit(activity,activity);activity.notice("Maintenance mode • kiosk disabled");
                    if(index==1)activity.startActivity(new Intent(Settings.ACTION_SETTINGS));
                }
            }catch(RuntimeException e){message(activity,e.getMessage());}
        }).setNegativeButton("Cancel",null).show();
    }
    private static void message(Activity a,String text){new AlertDialog.Builder(a).setMessage(text).setPositiveButton("OK",null).show();}
}
