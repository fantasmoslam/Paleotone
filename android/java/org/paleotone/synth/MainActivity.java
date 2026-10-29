/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;

import android.app.*;
import android.os.*;
import android.content.*;
import android.media.AudioManager;
import android.view.*;
import android.widget.Toast;
import android.util.AtomicFile;
import java.io.*;

public final class MainActivity extends Activity {
    private SynthView panel;
    private MidiInput midi;
    private boolean running, resumed, focusHeld;
    private int buffers=3;
    private AudioManager audio;
    private AtomicFile session;
    private final Handler handler=new Handler(Looper.getMainLooper());
    private final AudioManager.OnAudioFocusChangeListener focus=change->{
        if(change<0) {
            if(running){panel.release();Native.stop();running=false;}
            focusHeld=false;
            panel.notice="Audio focus lost — tap START to resume";
        }
    };
    private final Runnable refresh=new Runnable(){public void run(){
        if(!resumed)return;
        panel.update(Native.stats(),Native.peak(),running);
        handler.postDelayed(this,33);
    }};
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN|View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        audio=(AudioManager)getSystemService(AUDIO_SERVICE);
        session=new AtomicFile(new File(getFilesDir(),"session.ptandroid"));
        byte[] saved=null;
        try { saved=session.readFully(); } catch(IOException ignored) {}
        Native.init(saved);
        panel=new SynthView(this);setContentView(panel);
        midi=new MidiInput(this);
    }
    @Override protected void onResume(){super.onResume();resumed=true;startAudio();handler.post(refresh);}
    @Override protected void onPause(){
        resumed=false;handler.removeCallbacks(refresh);
        panel.release();midi.close();Native.stop();running=false;
        writeSession();
        if(focusHeld)audio.abandonAudioFocus(focus);focusHeld=false;
        super.onPause();
    }
    @Override protected void onDestroy(){midi.destroy();super.onDestroy();}
    @Override public void onBackPressed(){
        new AlertDialog.Builder(this).setTitle("Leave Paleotone?")
            .setMessage("Save the session and close audio?")
            .setPositiveButton("Save and exit",(d,w)->finish())
            .setNegativeButton("Keep playing",null).show();
    }
    private void startAudio(){
        if(!resumed)return;
        if(!focusHeld){
            int result=audio.requestAudioFocus(focus,AudioManager.STREAM_MUSIC,AudioManager.AUDIOFOCUS_GAIN);
            if(result!=AudioManager.AUDIOFOCUS_REQUEST_GRANTED){panel.notice="Audio focus unavailable — tap START to retry";return;}
            focusHeld=true;
        }
        String error=Native.start(buffers);
        running=error==null;
        panel.notice=running?"Native Paleotone • hold panel buttons for alternate functions":"Audio error: "+error;
    }
    private boolean writeSession(){
        byte[] data=Native.save();
        if(data==null){panel.notice="Session capture failed";return false;}
        FileOutputStream out=null;
        try{out=session.startWrite();out.write(data);session.finishWrite(out);return true;}
        catch(IOException e){if(out!=null)session.failWrite(out);panel.notice="Save failed: "+e.getMessage();return false;}
    }
    void action(int id){
        if(id==0){
            panel.release();Native.stop();running=false;
            boolean saved=writeSession();startAudio();
            if(saved)Toast.makeText(this,"Session saved",Toast.LENGTH_SHORT).show();
            else new AlertDialog.Builder(this).setMessage("Session could not be saved. Check available storage.").setPositiveButton("OK",null).show();
        }else if(id==1){
            panel.release();
            if(running){Native.stop();running=false;}else startAudio();
        }else if(id==2)midi.choose();
        else if(id==3){
            String[] choices={"Responsive — 2 bursts","Balanced — 3 bursts","Stable — 4 bursts","Extra margin — 6 bursts"};
            new AlertDialog.Builder(this).setTitle("Audio buffer — changing pauses audio")
                .setItems(choices,(d,w)->{buffers=new int[]{2,3,4,6}[w];panel.release();Native.stop();running=false;startAudio();}).show();
        }else if(id==4){Native.panic();panel.release();}
        else if(id==5){
            new AlertDialog.Builder(this).setTitle("Paleotone Android • early alpha")
                .setMessage("Play the multitouch keyboard. Drag encoder circles up/down, or tap their −/+ buttons. Hold any panel button for firmware alternate functions. SELECT moves through fields; ENGINE and PRESET choose sounds.\n\nSAVE SESSION briefly stops audio, writes the current project and flash, then resumes. Leaving the app also saves. Android force-stop, power loss, or a crash can lose unsaved changes. The firmware SAVE button retains its original project menu.\n\nMIDI lists Android controller output ports; connect via USB OTG. MIDI channel messages are supported; MIDI clock, SysEx and sample import are not yet bridged.\n\n48 kHz output uses linear conversion from the 44.1 kHz firmware engine. Diagnostics report actual stream settings, not measured physical latency. Firmware UI drawing still shares engine execution in this parity build.\n\nUse wired audio for testing. If audio disconnects, tap STOP then START. Buffer changes pause audio. Kiosk lockdown is intentionally not enabled yet.\n\nGPL-3.0-only. Paleotone builds on Felucca and GHOULBOX. Third-party notices are included in the APK assets. Full corresponding source accompanies this build.")
                .setPositiveButton("Close",null).show();
        }
    }
}
