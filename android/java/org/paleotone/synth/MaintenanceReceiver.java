/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
import android.content.*;
import android.util.Log;
/** Shell-only emergency escape; manifest requires the platform DUMP permission. */
public final class MaintenanceReceiver extends BroadcastReceiver {
    @Override public void onReceive(Context context,Intent intent){
        try{KioskController.exit(context,null);setResultCode(0);setResultData("Kiosk disabled; Home policy cleared");}
        catch(RuntimeException e){Log.e("PaleotoneKiosk","Maintenance escape incomplete",e);setResultCode(1);setResultData(e.toString());}
    }
}
