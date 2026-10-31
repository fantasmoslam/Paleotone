/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
import android.app.AlertDialog;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.view.*;
import android.widget.*;
import java.util.*;

/** A performance workspace: transport remains visible above independently browsable panels. */
final class ScapeView extends LinearLayout {
    private static final int TEXT=0xffeee8cf,TEAL=0xff77c6aa,AMBER=0xffedb75b,MUTED=0xffa8b5a9;
    private static final String[] BIOMES={"Water","Jungle","Tundra","Forest","Plains","Mountains","Canyons","Caldera"};
    private static final String[] MOODS={"Minor","Dorian","Phrygian","Harmonic minor","Major","Mixolydian"};
    private static final String[] ROLES={"Automatic","Bed","Melody","Bass","Pulse","Beast"};
    final Workspace w;
    private final TextView environmentState,musicState,droneState,audioState;
    private final Button environment,music,drone,next,generate;
    private final Switch hold;
    private final Button[] sections=new Button[3];
    private final View[] panels=new View[3];
    private final ArrayList<Runnable> bindings=new ArrayList<>();
    private boolean syncing;
    private final android.content.SharedPreferences settings;
    private int rememberedEnvironment,rememberedDrone;
    ScapeView(Workspace workspace){
        super(workspace.activity);w=workspace;
        settings=w.activity.getSharedPreferences("scape",0);rememberedEnvironment=settings.getInt("environmentLevel",80);rememberedDrone=settings.getInt("droneLevel",40);
        setOrientation(VERTICAL);setPadding(dp(16),dp(8),dp(16),dp(8));
        LinearLayout title=row();title.setGravity(Gravity.CENTER_VERTICAL);addView(title);
        TextView name=text("SCAPE",28,TEXT);name.setTypeface(Typeface.create("sans-serif-medium",Typeface.NORMAL));name.setLetterSpacing(.12f);
        title.addView(name,new LayoutParams(0,-2,1));audioState=text("",14,MUTED);title.addView(audioState);
        LinearLayout meters=row();addView(meters);environmentState=badge(meters);musicState=badge(meters);droneState=badge(meters);
        LinearLayout actions=row();addView(actions);
        environment=action("START ENVIRONMENT",()->command(value(13)>0?2:1));actions.addView(environment,weight());
        music=action("PLAY MUSIC",()->command(playing()?6:5));actions.addView(music,weight());
        Button stop=action("STOP ALL",()->{rememberLevels();w.activity.stopAllScape();});stop.setTextColor(0xffffc5a4);actions.addView(stop,weight());
        LinearLayout nav=row();addView(nav);
        String[] names={"ENVIRONMENT","MUSIC","DETAIL"};for(int i=0;i<3;i++){
            final int tab=i;sections[i]=action(names[i],()->select(tab));nav.addView(sections[i],weight());
        }
        FrameLayout body=new FrameLayout(w.activity);addView(body,new LayoutParams(-1,0,1));

        LinearLayout env=row(),left=column(),right=column();env.addView(left,new LayoutParams(0,-2,1));env.addView(right,new LayoutParams(0,-2,1.2f));
        LinearLayout habitat=card(left,"Habitat","Start the environment above. Its master also controls the environment drone.");
        selector(habitat,39,"Biome",BIOMES,"Choosing a biome resets its layer balance.");
        habitat.addView(w.control(13));pair(habitat,15,16);
        LinearLayout droneCard=card(left,"Environment drone","A sustained tone under the habitat. Separate from the four musical tracks.");
        drone=action("DRONE ON",()->command(value(14)>0?4:3));droneCard.addView(drone);droneCard.addView(w.control(14));
        LinearLayout layers=card(right,"Layer balance","Each layer keeps its own level. The environment master controls them together.");
        pair(layers,27,28);pair(layers,29,30);pair(layers,31,32);pair(layers,33,34);
        panels[0]=scroll(env);body.addView(panels[0],new FrameLayout.LayoutParams(-1,-1));

        LinearLayout score=row(),musicLeft=column(),musicRight=column();score.addView(musicLeft,new LayoutParams(0,-2,1));score.addView(musicRight,new LayoutParams(0,-2,1.2f));
        LinearLayout phrases=card(musicLeft,"Evolving music","Play evolves the current four tracks. Stop stops their sequence; the environment can keep playing. Transport is shared with Mixer.");
        selector(phrases,40,"Mood",MOODS,"Mood is applied when you generate new phrases.");
        generate=action("GENERATE NEW PHRASES…",this::generate);phrases.addView(generate);
        LinearLayout direction=card(musicLeft,"Direction","Hold keeps harmony and evolution steady while the notes continue.");
        hold=new Switch(w.activity);hold.setText("Hold current harmony");hold.setTextColor(TEXT);hold.setTextSize(17);hold.setMinHeight(dp(56));direction.addView(hold);
        hold.setOnCheckedChangeListener((b,on)->{if(!syncing)Native.control(24,on?1:0);});
        next=action("NEXT HARMONY",()->command(9));direction.addView(next);pair(direction,17,18);direction.addView(w.control(19));
        LinearLayout roles=card(musicRight,"Track roles","Assign the current instrument sounds to musical parts.");
        for(int i=0;i<4;i++)selector(roles,35+i,"Track "+(i+1),ROLES,null);
        LinearLayout balance=card(musicRight,"Musical balance","Shape the parts without changing your track mixer faders.");
        pair(balance,43,44);pair(balance,45,46);
        panels[1]=scroll(score);body.addView(panels[1],new FrameLayout.LayoutParams(-1,-1));

        LinearLayout detail=row(),detailsLeft=column(),detailsRight=column();detail.addView(detailsLeft,new LayoutParams(0,-2,1));detail.addView(detailsRight,new LayoutParams(0,-2,1.2f));
        LinearLayout creatures=card(detailsLeft,"Creature behaviour","Calls use the chosen track’s instrument. Choose Off to keep calls out of that track.");
        selector(creatures,41,"Call instrument",new String[]{"Off","Track 1","Track 2","Track 3","Track 4"},null);
        creatures.addView(w.control(20));creatures.addView(w.control(42));
        LinearLayout presence=card(detailsRight,"Presence & danger","Control the character of the passing creatures and their effect on the habitat.");
        presence.addView(w.control(21));presence.addView(w.control(22));
        LinearLayout guide=card(detailsRight,"Playing Scape","Environment and Music have independent transports. Drone belongs to the environment. Stop All silences the instrument and closes audio; starting Environment or Music opens audio again.");
        guide.addView(text("Save keeps your current project and settings. Generating phrases replaces track patterns only after confirmation.",15,MUTED));
        panels[2]=scroll(detail);body.addView(panels[2],new FrameLayout.LayoutParams(-1,-1));
        select(0);refresh(false);
    }
    private int dp(int n){return w.dp(n);}
    private LayoutParams weight(){LayoutParams p=new LayoutParams(0,dp(54),1);p.setMargins(dp(3),dp(3),dp(3),dp(3));return p;}
    private LinearLayout row(){LinearLayout l=new LinearLayout(w.activity);l.setOrientation(HORIZONTAL);return l;}
    private LinearLayout column(){LinearLayout l=new LinearLayout(w.activity);l.setOrientation(VERTICAL);return l;}
    private TextView text(String s,int size,int color){TextView t=new TextView(w.activity);t.setText(s);t.setTextSize(size);t.setTextColor(color);t.setPadding(dp(8),dp(6),dp(8),dp(6));return t;}
    private GradientDrawable surface(int color,int edge){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(10));d.setStroke(dp(1),edge);return d;}
    private TextView badge(LinearLayout parent){TextView t=text("",13,MUTED);t.setTypeface(Typeface.create("sans-serif-medium",Typeface.NORMAL));parent.addView(t,new LayoutParams(0,dp(36),1));return t;}
    private Button action(String title,Runnable r){Button b=new Button(w.activity);b.setText(title);b.setTextColor(TEAL);b.setTextSize(14);b.setMinHeight(dp(54));b.setOnClickListener(v->r.run());return b;}
    private LinearLayout card(LinearLayout parent,String title,String description){
        LinearLayout c=column();c.setPadding(dp(12),dp(10),dp(12),dp(12));c.setBackground(surface(0xff1b2927,0xff344b42));
        LayoutParams p=new LayoutParams(-1,-2);p.setMargins(dp(4),dp(8),dp(4),dp(4));parent.addView(c,p);
        TextView heading=text(title,22,TEXT);heading.setTypeface(Typeface.create("sans-serif-medium",Typeface.NORMAL));c.addView(heading);
        if(description!=null)c.addView(text(description,15,MUTED));return c;
    }
    private ScrollView scroll(View v){ScrollView s=new ScrollView(w.activity);s.setFillViewport(true);s.addView(v);return s;}
    private void pair(LinearLayout parent,int a,int b){LinearLayout r=row();parent.addView(r);r.addView(w.control(a),new LayoutParams(0,-2,1));r.addView(w.control(b),new LayoutParams(0,-2,1));}
    private void selector(LinearLayout parent,int id,String label,String[] options,String hint){
        Button choice=action("",()->new AlertDialog.Builder(w.activity).setTitle(label)
            .setSingleChoiceItems(options,Math.max(0,Math.min(options.length-1,value(id))),(d,index)->{Native.control(id,index);d.dismiss();})
            .setNegativeButton("Cancel",null).show());
        choice.setAllCaps(false);choice.setGravity(Gravity.CENTER_VERTICAL|Gravity.LEFT);choice.setPadding(dp(16),0,dp(16),0);parent.addView(choice);
        bindings.add(()->choice.setText(label+"  ·  "+options[Math.max(0,Math.min(options.length-1,value(id)))]+"  ▾"));
        if(hint!=null)parent.addView(text(hint,13,MUTED));
    }
    private int value(int id){return w.state[id*3];}
    private boolean playing(){return w.state[w.labels.length*3]!=0;}
    private boolean busy(){return w.state[w.labels.length*3+1]!=0;}
    private void rememberLevels(){
        int env=w.pending[13]!=Integer.MIN_VALUE?w.pending[13]:value(13);
        int drn=w.pending[14]!=Integer.MIN_VALUE?w.pending[14]:value(14);
        if(env>0&&env!=rememberedEnvironment){rememberedEnvironment=env;settings.edit().putInt("environmentLevel",env).apply();}
        if(drn>0&&drn!=rememberedDrone){rememberedDrone=drn;settings.edit().putInt("droneLevel",drn).apply();}
    }
    private void command(int id){
        rememberLevels();
        if(id==1&&value(13)==0)Native.control(13,rememberedEnvironment);
        if(id==3&&value(14)==0)Native.control(14,rememberedDrone);
        if(id==1||id==5)w.activity.ensureAudio();
        if(!Native.scape(id))w.activity.notice("Control queue busy — try again");
    }
    private void generate(){
        if(busy())return;
        new AlertDialog.Builder(w.activity).setTitle("Replace all four track patterns?")
            .setMessage("Generate phrases using the selected mood and track roles. Save your session first to keep the current patterns. Playback stays as it is.")
            .setNegativeButton("Keep current patterns",null).setPositiveButton("Generate phrases",(d,v)->command(8)).show();
    }
    private void select(int page){for(int i=0;i<3;i++){panels[i].setVisibility(i==page?VISIBLE:GONE);sections[i].setTextColor(i==page?AMBER:MUTED);sections[i].setBackground(surface(i==page?0xff2c3a30:0xff16211f,i==page?0xff786849:0xff344b42));}}
    void refresh(boolean audio){
        rememberLevels();
        boolean env=value(13)>0,run=playing(),evolve=value(23)!=0,drn=value(14)>0,locked=busy();
        environmentState.setText("ENVIRONMENT  "+(env&&audio?"LIVE":"OFF"));environmentState.setTextColor(env&&audio?TEAL:MUTED);
        musicState.setText((evolve?"MUSIC  ":"SEQUENCE  ")+(run&&audio?"PLAYING":"STOPPED"));musicState.setTextColor(run&&audio?TEAL:MUTED);
        droneState.setText("DRONE  "+(drn&&env&&audio?"LIVE":drn?"MUTED":"OFF"));droneState.setTextColor(drn&&env&&audio?TEAL:MUTED);
        audioState.setText(locked?"SONG CHAIN ACTIVE • stop music to edit":audio?"AUDIO ON":"AUDIO OFF • choose Start or Play");
        environment.setText(env&&audio?"PAUSE ENVIRONMENT":"START ENVIRONMENT");
        environment.setOnClickListener(v->command(env&&audio?2:1));
        music.setText(run&&audio?"STOP MUSIC":"PLAY MUSIC");music.setOnClickListener(v->command(run&&audio?6:5));music.setEnabled(!locked||run);
        drone.setText(drn?"TURN DRONE OFF":"TURN DRONE ON");drone.setEnabled(env&&audio);
        syncing=true;hold.setChecked(value(24)!=0);syncing=false;
        next.setEnabled(run&&evolve&&audio&&!locked);generate.setEnabled(!locked);
        for(Runnable r:bindings)r.run();
    }
}
