/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
import android.app.*;
import android.content.*;
import android.graphics.Color;
import android.net.Uri;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.util.*;
import java.util.concurrent.*;

/** Tablet controls are native widgets; the original firmware panel remains one tab. */
final class Workspace extends LinearLayout {
    private static final int TEXT=0xffeee8cf,TEAL=0xff77c6aa,MUTED=0xffa8b5a9;
    final MainActivity activity;final SynthView panel;final SampleLibrary library;
    final SharedPreferences prefs;
    final ExecutorService worker=Executors.newSingleThreadExecutor();
    final FrameLayout pages;final TextView status;final Button[] tabs=new Button[4];
    final SeekBar[] sliders;final TextView[] values;final String[] labels;
    final ArrayList<View> pageViews=new ArrayList<>();
    final LinearLayout libraryRows;final TextView sampleInfo,rootText;
    final Button mode,transport;final SeekBar root;
    SampleLibrary.Entry selected;boolean sampleMode,busy,closed;int rootNote=60,tick;
    int[] state;String[] texts;int activeTab;
    Workspace(MainActivity a,SynthView p){
        super(a);activity=a;panel=p;library=new SampleLibrary(a);prefs=a.getSharedPreferences("samples",0);
        setOrientation(VERTICAL);setBackgroundColor(0xff111919);
        labels=Native.controlLabels();state=Native.controls();texts=Native.controlTexts();
        sliders=new SeekBar[labels.length];values=new TextView[labels.length];
        LinearLayout bar=row();addView(bar,new LayoutParams(-1,dp(50)));
        String[] names={"INSTRUMENT","MIXER","SCAPE","LIBRARY"};
        for(int i=0;i<4;i++){final int n=i;tabs[i]=button(names[i],()->show(n));bar.addView(tabs[i],new LayoutParams(0,-1,1));}
        bar.addView(button("SAVE",()->activity.action(0)));bar.addView(button("⋮",()->activity.maintenance()));
        status=text("INSTRUMENT • native synth",14,TEAL);status.setPadding(dp(12),dp(4),dp(12),dp(4));addView(status);
        pages=new FrameLayout(a);addView(pages,new LayoutParams(-1,0,1));addPage(panel);
        LinearLayout mixer=column();heading(mixer,"Four-track mixer");
        TextView hint=text("Level, mute and pan control each track directly. Changes are included in SAVE SESSION.",15,MUTED);mixer.addView(hint);
        LinearLayout top=row();mixer.addView(top);transport=button("PLAY SEQUENCE",()->Native.control(labels.length,state[labels.length*3]==0?1:0));top.addView(transport);
        top.addView(button("PANIC",()->activity.action(4)));top.addView(button("MIDI",()->activity.action(2)));top.addView(button("AUDIO",()->activity.action(3)));
        addControl(mixer,12);
        for(int tr=0;tr<4;tr++){heading(mixer,"TRACK "+(tr+1));addPair(mixer,tr,8+tr);addControl(mixer,4+tr);}
        addPage(scroll(mixer));
        LinearLayout scape=column();heading(scape,"Scape · a living environment");
        scape.addView(text("Scape uses the existing sound engine. Enable evolving music and press PLAY to hear generated phrases. New phrases replaces current patterns after confirmation.",15,MUTED));
        addPair(scape,39,40);addPair(scape,13,14);addPair(scape,15,16);addPair(scape,23,24);
        LinearLayout commands=row();scape.addView(commands);commands.addView(button("NEXT HARMONY",()->Native.control(25,1)));
        commands.addView(button("NEW PHRASES…",()->new AlertDialog.Builder(a).setTitle("Replace current patterns?")
            .setMessage("Scape will generate new phrases in the current project. Save your session first if you want to keep these patterns. Evolving music must be enabled.")
            .setNegativeButton("Cancel",null).setPositiveButton("Generate",(d,w)->Native.control(26,1)).show()));
        heading(scape,"Behaviour");for(int i=17;i<=21;i+=2)addPair(scape,i,i+1);
        heading(scape,"Environment layers");for(int i=27;i<35;i+=2)addPair(scape,i,i+1);
        heading(scape,"Musical roles");addPair(scape,35,36);addPair(scape,37,38);addPair(scape,41,42);addPair(scape,43,44);addPair(scape,45,46);
        addPage(scroll(scape));
        LinearLayout lib=column();heading(lib,"Your sample library");
        lib.addView(text("Load a WAV, then play it on the INSTRUMENT keyboard or via MIDI. One sample at a time, eight voices. The firmware sequencer keeps its own sounds. Stereo files are mixed to mono.",16,MUTED));
        LinearLayout actions=row();lib.addView(actions);actions.addView(button("IMPORT WAV…",this::pick));
        mode=button("USE SAMPLE",this::toggleMode);actions.addView(mode);
        actions.addView(button("PLAY ON KEYBOARD",()->show(0)));
        sampleInfo=text("Preparing starter samples…",18,TEXT);lib.addView(sampleInfo);
        rootText=text("Root note · C4 (60)",16,TEAL);lib.addView(rootText);
        root=new SeekBar(a);root.setMax(127);root.setProgress(rootNote);lib.addView(root,new LayoutParams(-1,dp(48)));
        root.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar s,int v,boolean user){if(user){rootNote=v;Native.sampleRoot(v);saveSelection();}rootText.setText("Root note · "+note(rootNote)+" ("+rootNote+")");}
            public void onStartTrackingTouch(SeekBar s){}public void onStopTrackingTouch(SeekBar s){}
        });
        lib.addView(text("PCM 8/16/24/32-bit or float32 WAV • 8–192 kHz • 32 MiB / 4 million frames maximum. Imports are copied into Paleotone; deleting the original will not remove them.",14,MUTED));
        libraryRows=column();lib.addView(libraryRows);addPage(scroll(lib));show(0);
        worker.execute(()->{
            try{library.starters();final SampleLibrary.Entry saved=library.find(prefs.getString("selected",""));
                activity.runOnUiThread(()->{if(closed)return;refreshLibrary();sampleInfo.setText("Choose a sample below, or import your own WAV.");if(saved!=null)load(saved,prefs.getBoolean("mode",false));});
            }catch(Exception e){report(e);}
        });
    }
    private int dp(int n){return (int)(n*getResources().getDisplayMetrics().density+.5f);}
    private LinearLayout row(){LinearLayout l=new LinearLayout(activity);l.setOrientation(HORIZONTAL);return l;}
    private LinearLayout column(){LinearLayout l=new LinearLayout(activity);l.setOrientation(VERTICAL);l.setPadding(dp(12),dp(6),dp(12),dp(12));return l;}
    private TextView text(String t,int size,int color){TextView v=new TextView(activity);v.setText(t);v.setTextSize(size);v.setTextColor(color);v.setPadding(0,dp(5),0,dp(5));return v;}
    private Button button(String t,Runnable r){Button b=new Button(activity);b.setText(t);b.setTextSize(13);b.setTextColor(TEXT);b.setMinHeight(dp(48));b.setOnClickListener(v->r.run());return b;}
    private void heading(LinearLayout c,String t){c.addView(text(t,22,TEAL));}
    private ScrollView scroll(View v){ScrollView s=new ScrollView(activity);s.setFillViewport(true);s.addView(v);return s;}
    private void addPage(View v){pages.addView(v,new FrameLayout.LayoutParams(-1,-1));pageViews.add(v);v.setVisibility(GONE);}
    void show(int index){panel.release();activeTab=index;for(int i=0;i<4;i++){pageViews.get(i).setVisibility(i==index?VISIBLE:GONE);tabs[i].setTextColor(i==index?TEAL:MUTED);}updateStatus();}
    private void updateStatus(){status.setText(sampleMode&&selected!=null?"SAMPLE • "+selected.name+" • root "+note(rootNote):"INSTRUMENT • firmware keyboard & MIDI");mode.setText(sampleMode?"USE INSTRUMENT":"USE SAMPLE");root.setEnabled(selected!=null&&!busy);}
    private void addPair(LinearLayout parent,int a,int b){LinearLayout r=row();parent.addView(r);r.addView(control(a),new LayoutParams(0,-2,1));r.addView(control(b),new LayoutParams(0,-2,1));}
    private void addControl(LinearLayout parent,int id){parent.addView(control(id));}
    private View control(int id){
        LinearLayout box=column();values[id]=text(labels[id]+" · "+texts[id],17,TEXT);box.addView(values[id]);
        SeekBar s=new SeekBar(activity);sliders[id]=s;s.setMax(state[id*3+2]-state[id*3+1]);s.setProgress(state[id*3]-state[id*3+1]);box.addView(s,new LayoutParams(-1,dp(48)));
        s.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar bar,int value,boolean user){if(user)Native.control(id,value+state[id*3+1]);}
            public void onStartTrackingTouch(SeekBar bar){}public void onStopTrackingTouch(SeekBar bar){}
        });return box;
    }
    void update(){
        if(++tick%6!=0||(activeTab!=1&&activeTab!=2))return;
        state=Native.controls();texts=Native.controlTexts();
        for(int i=0;i<labels.length;i++)if(sliders[i]!=null&&!sliders[i].isPressed()){
            sliders[i].setProgress(state[i*3]-state[i*3+1]);values[i].setText(labels[i]+" · "+texts[i]);
        }
        transport.setText(state[labels.length*3]==0?"PLAY SEQUENCE":"STOP SEQUENCE");
    }
    private String note(int n){return new String[]{"C","C♯","D","D♯","E","F","F♯","G","G♯","A","A♯","B"}[n%12]+(n/12-1);}
    private void saveSelection(){SharedPreferences.Editor e=prefs.edit().putBoolean("mode",sampleMode);if(selected!=null)e.putString("selected",selected.id()).putInt("root."+selected.id(),rootNote);else e.remove("selected");e.apply();}
    private void toggleMode(){if(busy)return;if(selected==null){activity.notice("Load a sample in LIBRARY first");return;}panel.release();sampleMode=!sampleMode;Native.sampleMode(sampleMode);saveSelection();updateStatus();}
    private void load(SampleLibrary.Entry entry,boolean enable){
        if(busy)return;busy=true;updateStatus();sampleInfo.setText("Loading "+entry.name+"…");
        worker.execute(()->{
            try{WavSample decoded=WavSample.read(entry.file);activity.runOnUiThread(()->{
                if(closed)return;busy=false;int key=prefs.getInt("root."+entry.id(),entry.name.contains("C3")?48:60);
                if(!activity.installSample(decoded,key)){sampleInfo.setText("Cannot load sample into audio engine");updateStatus();return;}
                selected=entry;rootNote=key;root.setProgress(key);sampleMode=enable;Native.sampleMode(enable);saveSelection();
                sampleInfo.setText(String.format(Locale.ROOT,"%s  ·  %.2f s  ·  %d Hz",entry.name,(double)decoded.data.length/decoded.rate,decoded.rate));
                updateStatus();refreshLibrary();
            });}catch(Exception e){report(e);}catch(OutOfMemoryError e){report(new IOException("Not enough memory to load this sample. Try a shorter WAV."));}
        });
    }
    private void refreshLibrary(){
        libraryRows.removeAllViews();for(SampleLibrary.Entry entry:library.list()){
            LinearLayout r=row();libraryRows.addView(r);r.addView(text(entry.name+(selected!=null&&selected.id().equals(entry.id())?"  • loaded":""),17,TEXT),new LayoutParams(0,-2,1));
            r.addView(button("LOAD",()->load(entry,true)));
            r.addView(button("DELETE",()->{
                if(busy)return;new AlertDialog.Builder(activity).setTitle("Delete "+entry.name+"?").setMessage("Removes this copy from Paleotone's library.")
                    .setNegativeButton("Cancel",null).setPositiveButton("Delete",(d,w)->{
                        try{if(selected!=null&&selected.id().equals(entry.id())){
                            activity.installSample(null,60);selected=null;sampleMode=false;sampleInfo.setText("Sample removed. Firmware instrument selected.");saveSelection();updateStatus();
                        }library.delete(entry);refreshLibrary();}catch(IOException e){report(e);}
                    }).show();
            }));
        }
    }
    private void pick(){
        if(busy)return;
        try{KioskController.suspendForPicker(activity);
            Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("*/*");activity.pickSample(i);
        }catch(RuntimeException e){KioskController.resumeAfterPicker(activity);report(e);}
    }
    void picked(Uri uri){
        KioskController.resumeAfterPicker(activity);if(uri==null)return;
        busy=true;sampleInfo.setText("Importing WAV…");updateStatus();
        worker.execute(()->{try{SampleLibrary.Entry entry=library.importUri(uri);activity.runOnUiThread(()->{if(closed)return;busy=false;refreshLibrary();load(entry,true);});}catch(Exception e){report(e);}catch(OutOfMemoryError e){report(new IOException("Not enough memory. Try a shorter WAV."));}});
    }
    private void report(Throwable e){activity.runOnUiThread(()->{if(closed)return;busy=false;updateStatus();sampleInfo.setText("Library: "+e.getMessage());});}
    void close(){closed=true;worker.shutdown();}
}
