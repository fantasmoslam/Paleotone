// SPDX-License-Identifier: GPL-3.0-only
using System;using System.IO;using System.Text;using System.Drawing;using System.Windows.Forms;
using System.Collections.Generic;using System.Runtime.InteropServices;
class NightClaw : Form {
 static readonly Color Bone=Color.FromArgb(230,220,194),Red=Color.FromArgb(255,40,78),Yellow=Color.FromArgb(234,255,0),Purple=Color.FromArgb(166,58,255),Dark=Color.FromArgb(5,5,8),PanelColor=Color.FromArgb(22,22,29);
 readonly string[] tracks={"Drone","Sequence","Lead","Sequence 2","Atmosphere","Skin drums","Creature calls"};
 ComboBox scene,voice,device;Label state,trackName,voiceLabel;Button play;TableLayoutPanel mixer,editor,compose;
 Panel planView;TabControl tabs;SliderRow masterSlider;NumericUpDown seed;CheckBox expanded;Timer timer;
 bool refreshing,dirty,audio;int selected=2,seenCycle;string path="";
 readonly List<SliderRow> soundRows=new List<SliderRow>(),globalRows=new List<SliderRow>(),composerRows=new List<SliderRow>();
 readonly List<TrackBar> levels=new List<TrackBar>();
 readonly List<Button> trackButtons=new List<Button>();
 sealed class SliderRow {
  public TrackBar Bar;public Label Value,Name;public Func<int,string> Format;
  public void Set(int lo,int hi,int v){Bar.Minimum=lo;Bar.Maximum=hi;Bar.Value=Math.Max(lo,Math.Min(hi,v));Value.Text=Format(Bar.Value);}
 }
 static Label TextLabel(string text,int size,Color color){return new Label{Text=text,AutoSize=false,Font=new Font("Segoe UI",size),ForeColor=color,Dock=DockStyle.Fill,TextAlign=ContentAlignment.MiddleLeft};}
 static Button Btn(string text){return new Button{Text=text,AutoSize=true,MinimumSize=new Size(95,44),BackColor=PanelColor,ForeColor=Bone,FlatStyle=FlatStyle.Flat,Margin=new Padding(5),Font=new Font("Segoe UI",12)};}
 static ComboBox Combo(){return new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList,BackColor=PanelColor,ForeColor=Bone,Font=new Font("Segoe UI",13),Dock=DockStyle.Fill,Margin=new Padding(5)};}
 static TableLayoutPanel Table(int cols){var t=new TableLayoutPanel{ColumnCount=cols,Dock=DockStyle.Fill,BackColor=Dark,Padding=new Padding(8)};return t;}
 void Changed(){if(!refreshing){dirty=true;Text="NightClaw PC — "+(path==""?"Untitled":Path.GetFileName(path))+" *";}}
 SliderRow AddSlider(TableLayoutPanel table,int row,string name,int lo,int hi,int v,Color color,Action<int> change,Func<int,string> format){
  var label=TextLabel(name,13,Bone);var bar=new TrackBar{Minimum=lo,Maximum=hi,Value=Math.Max(lo,Math.Min(hi,v)),TickStyle=TickStyle.None,SmallChange=1,LargeChange=1,Dock=DockStyle.Fill,AutoSize=false,Height=42,BackColor=Dark,Margin=new Padding(5,10,5,0),AccessibleName=name};
  var value=TextLabel("",12,color);var sr=new SliderRow{Bar=bar,Value=value,Name=label,Format=format};value.Text=format(bar.Value);
  bar.ValueChanged+=(s,e)=>{value.Text=sr.Format(bar.Value);if(!refreshing){change(bar.Value);Changed();}};
  table.RowStyles.Add(new RowStyle(SizeType.Absolute,53));table.Controls.Add(label,0,row);table.Controls.Add(bar,1,row);table.Controls.Add(value,2,row);return sr;
 }
 TableLayoutPanel Sliders(){var t=Table(3);t.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,145));t.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));t.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,112));t.AutoScroll=true;return t;}
 public NightClaw(){
  Engine.pc_init();Text="NightClaw PC — Untitled";Font=new Font("Segoe UI",12);ForeColor=Bone;BackColor=Dark;
  AutoScaleMode=AutoScaleMode.Dpi;ClientSize=new Size(1280,900);MinimumSize=new Size(1080,760);StartPosition=FormStartPosition.CenterScreen;
  var root=Table(1);root.RowCount=5;root.RowStyles.Add(new RowStyle(SizeType.Absolute,58));root.RowStyles.Add(new RowStyle(SizeType.Absolute,66));root.RowStyles.Add(new RowStyle(SizeType.Percent,100));root.RowStyles.Add(new RowStyle(SizeType.Absolute,112));root.RowStyles.Add(new RowStyle(SizeType.Absolute,40));Controls.Add(root);
  var title=Table(2);title.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,50));title.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,50));title.Controls.Add(TextLabel("NIGHTCLAW  /  PC",26,Bone),0,0);title.Controls.Add(TextLabel("Western shadows. Living themes.",14,Purple),1,0);root.Controls.Add(title,0,0);
  var top=Table(6);top.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));for(int i=0;i<5;i++)top.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,i==0?110:105));
  scene=Combo();for(int i=0;i<Engine.pc_scene_count();i++)scene.Items.Add(Engine.SceneName(i));scene.SelectedIndex=Engine.pc_scene_current();top.Controls.Add(scene,0,0);
  play=Btn("Play");play.Click+=(s,e)=>TogglePlay();top.Controls.Add(play,1,0);
  var stop=Btn("Silence");stop.Click+=(s,e)=>{Engine.pc_panic();UpdateStatus();};top.Controls.Add(stop,2,0);
  var save=Btn("Save");save.Click+=(s,e)=>SaveProject(false);top.Controls.Add(save,3,0);
  var saveAs=Btn("Save as");saveAs.Click+=(s,e)=>SaveProject(true);top.Controls.Add(saveAs,4,0);
  var open=Btn("Open");open.Click+=(s,e)=>OpenProject();top.Controls.Add(open,5,0);root.Controls.Add(top,0,1);
  tabs=new TabControl{Dock=DockStyle.Fill,Font=new Font("Segoe UI",13),Padding=new Point(18,8)};
  var instrument=new TabPage("Instrument"){BackColor=Dark};var composer=new TabPage("Composer"){BackColor=Dark};var controls=new TabPage("Scene & effects"){BackColor=Dark};tabs.TabPages.AddRange(new[]{instrument,composer,controls});root.Controls.Add(tabs,0,2);
  var instrumentLayout=Table(2);instrumentLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,345));instrumentLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));instrument.Controls.Add(instrumentLayout);
  mixer=Table(2);mixer.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,160));mixer.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
  mixer.RowStyles.Add(new RowStyle(SizeType.Absolute,45));mixer.Controls.Add(TextLabel("TRACK",12,Purple),0,0);mixer.Controls.Add(TextLabel("LEVEL",12,Yellow),1,0);
  for(int i=0;i<7;i++){int tr=i;var b=Btn(tracks[i]);b.Dock=DockStyle.Fill;b.Click+=(s,e)=>{selected=tr;RefreshEditor();};trackButtons.Add(b);var level=new TrackBar{Minimum=0,Maximum=100,Value=Engine.pc_get(Engine.pc_track_param(i,0)),TickStyle=TickStyle.None,SmallChange=1,LargeChange=1,Dock=DockStyle.Fill,BackColor=Dark,AccessibleName=tracks[i]+" level"};level.ValueChanged+=(s,e)=>{if(!refreshing){Engine.pc_set(Engine.pc_track_param(tr,0),level.Value);if(selected==tr&&soundRows.Count>0){refreshing=true;soundRows[0].Set(0,100,level.Value);refreshing=false;}Changed();}};levels.Add(level);mixer.RowStyles.Add(new RowStyle(SizeType.Absolute,57));mixer.Controls.Add(b,0,i+1);mixer.Controls.Add(level,1,i+1);}
  mixer.RowStyles.Add(new RowStyle(SizeType.Percent,100));mixer.RowCount=9;
  instrumentLayout.Controls.Add(mixer,0,0);
  var right=Table(1);right.RowStyles.Add(new RowStyle(SizeType.Absolute,42));right.RowStyles.Add(new RowStyle(SizeType.Absolute,48));right.RowStyles.Add(new RowStyle(SizeType.Percent,100));trackName=TextLabel("Lead",21,Red);right.Controls.Add(trackName,0,0);
  var voiceRow=Table(2);voiceRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,145));voiceRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));voiceLabel=TextLabel("Instrument",13,Bone);voiceRow.Controls.Add(voiceLabel,0,0);voice=Combo();voiceRow.Controls.Add(voice,1,0);right.Controls.Add(voiceRow,0,1);
  editor=Sliders();var colors=new[]{Bone,Red,Yellow,Purple};var labels=new[]{"Level","Filter","LFO","Reverb","Attack","Decay","Sustain","Release"};
  for(int i=0;i<8;i++){int c=i;soundRows.Add(AddSlider(editor,i,labels[i],0,127,0,colors[i%4],v=>SetSound(c,v),v=>SoundText(c,v)));}
  right.Controls.Add(editor,0,2);instrumentLayout.Controls.Add(right,1,0);
  voice.SelectedIndexChanged+=(s,e)=>{if(!refreshing&&voice.SelectedIndex>=0){if(selected==5)Engine.pc_beat_set(voice.SelectedIndex);else Engine.pc_set(Engine.pc_voice_param(selected),voice.SelectedIndex);Changed();}};
  var compRoot=Table(1);compRoot.RowStyles.Add(new RowStyle(SizeType.Absolute,64));compRoot.RowStyles.Add(new RowStyle(SizeType.Absolute,90));compRoot.RowStyles.Add(new RowStyle(SizeType.Absolute,60));compRoot.RowStyles.Add(new RowStyle(SizeType.Percent,100));compRoot.RowStyles.Add(new RowStyle(SizeType.Absolute,55));composer.Controls.Add(compRoot);
  expanded=new CheckBox{Text="Expanded composer — plan a 32-bar Lead passage",Font=new Font("Segoe UI",17),ForeColor=Yellow,Dock=DockStyle.Fill,Checked=false,AutoSize=false};expanded.CheckedChanged+=(s,e)=>{if(!refreshing){Engine.pc_planner_set(0,expanded.Checked?1:0);Changed();planView.Invalidate();}};compRoot.Controls.Add(expanded,0,0);
  planView=new Panel{Dock=DockStyle.Fill,BackColor=PanelColor};planView.Paint+=PaintPlan;compRoot.Controls.Add(planView,0,1);
  var seedRow=Table(4);seedRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,100));seedRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,180));seedRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,190));seedRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
  seedRow.Controls.Add(TextLabel("Seed",13,Bone),0,0);seed=new NumericUpDown{Minimum=1,Maximum=int.MaxValue,Value=7391,Font=new Font("Segoe UI",15),BackColor=PanelColor,ForeColor=Bone,Dock=DockStyle.Fill};seed.ValueChanged+=(s,e)=>{if(!refreshing){Engine.pc_planner_set(5,(int)seed.Value);planView.Invalidate();Changed();}};seedRow.Controls.Add(seed,1,0);
  var fresh=Btn("Develop next plan");fresh.Click+=(s,e)=>{Engine.pc_replan();planView.Invalidate();Changed();};seedRow.Controls.Add(fresh,2,0);compRoot.Controls.Add(seedRow,0,2);
  compose=Sliders();string[] names={"Variation","Space","Theme reuse","Lead register"};
  for(int i=0;i<4;i++){int c=i;composerRows.Add(AddSlider(compose,i,names[i],i==3?-2:0,i==3?2:100,Engine.pc_planner_get(i+1),colors[i],v=>Engine.pc_planner_set(c+1,v),v=>c==3?v+" oct":v+"%"));}
  compose.RowStyles.Add(new RowStyle(SizeType.Percent,100));compose.RowCount=5;
  compRoot.Controls.Add(compose,0,3);compRoot.Controls.Add(TextLabel("Changes shape the next plan. Mouse-played Lead takes priority over generated notes.",12,Purple),0,4);
  var settings=Table(2);settings.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,60));settings.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,40));controls.Controls.Add(settings);
  var all=Sliders();for(int i=0;i<11;i++){int id=Engine.pc_global_id(i);globalRows.Add(AddSlider(all,i,Engine.ParamName(id),Engine.pc_min(id),Engine.pc_max(id),Engine.pc_get(id),colors[i%4],v=>Engine.pc_set(id,v),v=>Engine.ValueText(id,v)));}
  settings.Controls.Add(all,0,0);
  var output=Table(1);output.RowStyles.Add(new RowStyle(SizeType.Absolute,45));output.RowStyles.Add(new RowStyle(SizeType.Absolute,50));output.RowStyles.Add(new RowStyle(SizeType.Absolute,50));output.RowStyles.Add(new RowStyle(SizeType.Absolute,80));output.RowStyles.Add(new RowStyle(SizeType.Percent,100));
  output.Controls.Add(TextLabel("Audio output",19,Bone),0,0);device=Combo();ReloadDevices();output.Controls.Add(device,0,1);var refreshDevices=Btn("Refresh outputs");refreshDevices.Click+=(s,e)=>{StopAudio();ReloadDevices();};output.Controls.Add(refreshDevices,0,2);
  var masterRow=Sliders();masterSlider=AddSlider(masterRow,0,"Master",0,4096,2400,Yellow,v=>Engine.pc_master(v),v=>(v*100/4096)+"%");output.Controls.Add(masterRow,0,3);
  output.Controls.Add(TextLabel("Shared-mode Windows audio. MIDI and remote control come after PC composition work. Projects save scene settings, ADSR overrides, seeds and the composer’s theme bank.",13,Purple),0,4);settings.Controls.Add(output,1,0);
  var pads=Table(1);pads.RowStyles.Add(new RowStyle(SizeType.Absolute,27));pads.RowStyles.Add(new RowStyle(SizeType.Percent,100));pads.Controls.Add(TextLabel("PLAY LEAD  ·  hold a pad     /     CREATURES  ·  click to call",12,Purple),0,0);
  var padRow=Table(16);for(int i=0;i<16;i++)padRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,6.25f));
  string[] creatures={"Herd","Raptor","Hoot","Moan","Nest","Croak","Hadro","Snort"};
  for(int i=0;i<16;i++){int k=i;var pad=Btn(i<8?(i+1).ToString():creatures[i-8]);pad.MinimumSize=new Size(0,42);pad.Dock=DockStyle.Fill;pad.Font=new Font("Segoe UI",i<8?15:10);if(i<8){int pressed=-1;pad.MouseDown+=(s,e)=>{if(e.Button!=MouseButtons.Left)return;if(!StartAudio())return;pressed=Engine.pc_pad_note(k);Engine.pc_note(pressed,1);pad.Capture=true;};pad.MouseUp+=(s,e)=>{if(pressed>=0)Engine.pc_note(pressed,0);pressed=-1;};pad.MouseCaptureChanged+=(s,e)=>{if(!pad.Capture&&pressed>=0){Engine.pc_note(pressed,0);pressed=-1;}};}else pad.Click+=(s,e)=>{if(StartAudio())Engine.pc_call(k-8);};padRow.Controls.Add(pad,i,0);}
  pads.Controls.Add(padRow,0,1);root.Controls.Add(pads,0,3);
  state=TextLabel("Stopped — choose a scene, then Play.",12,Bone);root.Controls.Add(state,0,4);
  scene.SelectedIndexChanged+=(s,e)=>{if(!refreshing&&scene.SelectedIndex>=0){Engine.pc_scene(scene.SelectedIndex);RefreshAll();Changed();}};
  timer=new Timer{Interval=100};timer.Tick+=(s,e)=>{UpdateStatus();if(expanded.Checked)planView.Invalidate();};timer.Start();
  Deactivate+=(s,e)=>{Engine.pc_note(-1,0);};FormClosing+=OnClosing;RefreshAll();
 }
 void ReloadDevices(){device.Items.Clear();device.Items.Add("Windows default output");for(int i=0;i<Engine.pc_audio_devices();i++){var n=new StringBuilder(512);Engine.pc_audio_name(i,n,512);device.Items.Add(n.ToString());}device.SelectedIndex=0;device.SelectedIndexChanged+=(s,e)=>{if(audio)StopAudio();};}
 bool StartAudio(){if(audio)return true;int hr=Engine.pc_audio_start(device.SelectedIndex-1);if(hr!=0){Engine.pc_audio_stop();MessageBox.Show(this,"Could not open this audio output (0x"+hr.ToString("X8")+"). Choose another output and try again.","Audio output");return false;}audio=true;return true;}
 void StopAudio(){Engine.pc_play(0);Engine.pc_panic();Engine.pc_audio_stop();audio=false;UpdateStatus();}
 void TogglePlay(){if(Engine.pc_playing()!=0)Engine.pc_play(0);else if(StartAudio())Engine.pc_play(1);UpdateStatus();}
 void UpdateStatus(){int currentCycle=Engine.pc_cycle();if(currentCycle!=seenCycle){seenCycle=currentCycle;if(expanded.Checked)Changed();}play.Text=Engine.pc_playing()!=0?"Rest":"Play";string[] sections={"Call","Answer","Develop","Resolve"};int tick=Engine.pc_tick();state.Text=(Engine.pc_playing()!=0?"Playing":"Stopped")+"  ·  "+Engine.SceneName(Engine.pc_scene_current())+(expanded.Checked?"  ·  "+sections[Math.Min(3,tick/128)]+" / bar "+(tick/16+1)+" of 32":"  ·  Original generator")+"  ·  audio render "+Engine.pc_audio_load().ToString("0.0")+"%";if(audio&&Engine.pc_audio_error()!=0){StopAudio();state.Text="Audio output disconnected. Select an output and press Play.";}}
 void SetSound(int c,int v){if(c<4){Engine.pc_set(Engine.pc_track_param(selected,c),v);if(c==0){refreshing=true;levels[selected].Value=v;refreshing=false;}}else if(selected==5&&c==5)Engine.pc_set(Engine.pc_percussion_decay_id(),v);else Engine.pc_envelope_set(selected,c-4,v);}
 string SoundText(int c,int v){if(c<4)return Engine.ValueText(Engine.pc_track_param(selected,c),v);if(selected==5)return c==5?Engine.ValueText(Engine.pc_percussion_decay_id(),v):"—";if(v<0)return "Voice";if(c==6)return (v*100/127)+"%";return EnvelopeTime(v)+" ms";}
 static int EnvelopeTime(int v){return (int)(Engine.pc_envelope_ms10(v)/10);}
 void RefreshEditor(){
  refreshing=true;trackName.Text=tracks[selected];for(int i=0;i<7;i++)trackButtons[i].ForeColor=i==selected?Red:Bone;
  voice.Items.Clear();voiceLabel.Text=selected==5?"Beat":"Instrument";
  if(selected==5){for(int i=0;i<14;i++)voice.Items.Add(Engine.BeatName(i));voice.SelectedIndex=Engine.pc_beat();}
  else{int p=Engine.pc_voice_param(selected);for(int i=0;i<=Engine.pc_max(p);i++)voice.Items.Add(Engine.ValueText(p,i));voice.SelectedIndex=Engine.pc_get(p);}
  for(int i=0;i<8;i++){if(i<4){int p=Engine.pc_track_param(selected,i);soundRows[i].Set(Engine.pc_min(p),Engine.pc_max(p),Engine.pc_get(p));}else if(selected==5&&i==5){int id=Engine.pc_percussion_decay_id();soundRows[i].Set(Engine.pc_min(id),Engine.pc_max(id),Engine.pc_get(id));}else soundRows[i].Set(-1,127,Engine.pc_envelope_get(selected,i-4));soundRows[i].Bar.Enabled=selected!=5||i<4||i==5;}
  soundRows[2].Name.Text=selected==5?"Ghosts":selected==6?"Auto chance":"LFO";
  refreshing=false;
 }
 void RefreshAll(){seenCycle=Engine.pc_cycle();refreshing=true;masterSlider.Set(0,4096,Engine.pc_master_get());scene.SelectedIndex=Engine.pc_scene_current();for(int i=0;i<7;i++)levels[i].Value=Engine.pc_get(Engine.pc_track_param(i,0));for(int i=0;i<11;i++){int p=Engine.pc_global_id(i);globalRows[i].Set(Engine.pc_min(p),Engine.pc_max(p),Engine.pc_get(p));}expanded.Checked=Engine.pc_planner_get(0)!=0;seed.Value=Math.Max(1,Engine.pc_planner_get(5));for(int i=0;i<4;i++)composerRows[i].Set(i==3?-2:0,i==3?2:100,Engine.pc_planner_get(i+1));refreshing=false;RefreshEditor();planView.Invalidate();}
 void PaintPlan(object sender,PaintEventArgs e){var g=e.Graphics;int w=planView.Width,h=planView.Height;string[] names={"CALL","ANSWER","DEVELOP","RESOLVE"};Color[] colors={Bone,Red,Yellow,Purple};
  for(int i=0;i<4;i++){int x=i*w/4;using(var pen=new Pen(colors[i]))g.DrawLine(pen,x,0,x,h);using(var brush=new SolidBrush(colors[i]))g.DrawString(names[i],Font,brush,x+8,3);}
  for(int i=0;i<Engine.pc_plan_count();i++){int tick=Engine.pc_plan_event(i,0),dur=Engine.pc_plan_event(i,1),degree=Engine.pc_plan_event(i,2),s=Engine.pc_plan_event(i,3);using(var b=new SolidBrush(colors[s]))g.FillRectangle(b,tick*w/512f,34+(6-degree)*(h-45)/7f,Math.Max(2,dur*w/512f),4);}
  if(expanded.Checked){using(var pen=new Pen(Red,2))g.DrawLine(pen,Engine.pc_tick()*w/512f,0,Engine.pc_tick()*w/512f,h);}
 }
 bool ConfirmDiscard(){return !dirty||MessageBox.Show(this,"Discard unsaved changes?","NightClaw",MessageBoxButtons.YesNo)==DialogResult.Yes;}
 void SaveProject(bool choose){if(choose||path==""){using(var d=new SaveFileDialog{Filter="NightClaw project|*.nightclaw",DefaultExt="nightclaw",FileName="NightClaw"}){if(d.ShowDialog(this)!=DialogResult.OK)return;path=d.FileName;}}try{var data=new byte[Engine.pc_save_size()];if(Engine.pc_save(data,data.Length)!=data.Length)throw new IOException("Could not capture project.");string tmp=path+".tmp";File.WriteAllBytes(tmp,data);if(File.Exists(path))File.Replace(tmp,path,null);else File.Move(tmp,path);dirty=false;Text="NightClaw PC — "+Path.GetFileName(path);}catch(Exception ex){MessageBox.Show(this,ex.Message,"Could not save");}}
 void OpenProject(){if(!ConfirmDiscard())return;using(var d=new OpenFileDialog{Filter="NightClaw project|*.nightclaw"}){if(d.ShowDialog(this)!=DialogResult.OK)return;try{byte[] data=File.ReadAllBytes(d.FileName);if(Engine.pc_load(data,data.Length)!=0)throw new IOException("This file is damaged or is not a compatible NightClaw PC project.");path=d.FileName;dirty=false;RefreshAll();Text="NightClaw PC — "+Path.GetFileName(path);}catch(Exception ex){MessageBox.Show(this,ex.Message,"Could not open");}}}
 void OnClosing(object sender,FormClosingEventArgs e){if(!ConfirmDiscard()){e.Cancel=true;return;}timer.Stop();Engine.pc_audio_stop();}
 public void Smoke(string file){ShowInTaskbar=false;Opacity=0;Show();Application.DoEvents();using(var bmp=new Bitmap(Width,Height)){CreateControl();PerformLayout();DrawToBitmap(bmp,new Rectangle(Point.Empty,bmp.Size));bmp.Save(file);}Console.WriteLine("UI rendered "+ClientSize);tabs.SelectedIndex=1;Application.DoEvents();using(var b=new Bitmap(Width,Height)){DrawToBitmap(b,new Rectangle(Point.Empty,b.Size));b.Save(Path.Combine(Path.GetDirectoryName(file),"composer.png"));}timer.Stop();Dispose();}
 [STAThread] static int Main(string[] args){try{Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);if(args.Length>0&&args[0]=="--self-test")return Tests.Run(args.Length>1?args[1]:".");using(var f=new NightClaw()){if(args.Length>0&&args[0]=="--smoke"){f.Smoke(args[1]);return 0;}Application.Run(f);}return 0;}catch(Exception ex){File.WriteAllText(Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"nightclaw-error.txt"),ex.ToString());if(args.Length==0)MessageBox.Show(ex.Message,"NightClaw could not start");return 1;}}
}
