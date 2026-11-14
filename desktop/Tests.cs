// SPDX-License-Identifier: GPL-3.0-only
using System;using System.IO;using System.Text;using System.Threading;
static class Tests {
 static void Check(bool ok,string message){if(!ok)throw new Exception(message);}
 static string Plan(){var s=new StringBuilder();for(int i=0;i<Engine.pc_plan_count();i++)for(int f=0;f<4;f++)s.Append(Engine.pc_plan_event(i,f)).Append(',');return s.ToString();}
 static float Render(int frames){float peak=0;var b=new float[256];while(frames>0){int n=Math.Min(128,frames);Engine.pc_render(b,n);for(int i=0;i<n*2;i++){Check(!float.IsNaN(b[i])&&!float.IsInfinity(b[i])&&Math.Abs(b[i])<=1.05f,"finite bounded audio");peak=Math.Max(peak,Math.Abs(b[i]));}frames-=n;}return peak;}
 public static int Run(string dir){Directory.CreateDirectory(dir);var log=new StringBuilder();try{
  Engine.pc_init();Check(Engine.pc_scene_count()==20,"20 scenes");Check(Engine.pc_playing()==0,"silent transport on startup");
  for(int i=0;i<20;i++){Engine.pc_scene(i);Engine.pc_play(1);float p=Render(44100*3);Check(p>.0001,"scene audible "+i);Engine.pc_play(0);log.AppendLine("scene "+i+" "+Engine.SceneName(i)+" peak "+p);}
  Engine.pc_scene(2);Engine.pc_planner_set(0,1);Engine.pc_planner_set(5,12345);string a=Plan();Engine.pc_planner_set(5,12345);Check(a==Plan(),"reproducible lookahead");
  Check(Engine.pc_plan_count()>30,"nonempty developed plan");int prior=-1;
  for(int i=0;i<Engine.pc_plan_count();i++){int tick=Engine.pc_plan_event(i,0);Check(tick>prior&&tick<512,"ordered plan");prior=tick;Check(Engine.pc_plan_event(i,2)>=0&&Engine.pc_plan_event(i,2)<=6,"diatonic notes");}
  int last=Engine.pc_plan_count()-1;Check(Engine.pc_plan_event(last,0)==500&&Engine.pc_plan_event(last,2)==0,"tonic resolution");
  Engine.pc_play(1);Check(Render(44100*15)>.001,"expanded composer audible");
  Engine.pc_set(Engine.pc_global_id(0),160);int cycle=Engine.pc_cycle();Render(44100*52);Check(Engine.pc_cycle()>cycle,"full 32-bar form completes and develops another plan");
  Engine.pc_note(60,1);Render(4410);Engine.pc_note(60,0);Engine.pc_play(0);Render(4410);
  Engine.pc_envelope_set(2,0,100);Engine.pc_envelope_set(2,3,95);
  var data=new byte[Engine.pc_save_size()];Check(Engine.pc_save(data,data.Length)==data.Length,"save snapshot");
  File.WriteAllBytes(Path.Combine(dir,"test-project.nightclaw"),data);
  Engine.pc_init();Check(Engine.pc_load(data,data.Length)==0,"load project");Check(Engine.pc_envelope_get(2,0)==100,"ADSR persisted");Check(Engine.pc_planner_get(5)==12345,"seed persisted");Check(Engine.pc_planner_get(0)==1,"planner mode persisted");
  byte[] bad=(byte[])data.Clone();bad[bad.Length-4]^=1;Check(Engine.pc_load(bad,bad.Length)!=0,"corrupt file rejected");Check(Engine.pc_envelope_get(2,0)==100,"failed load leaves current project intact");
  Engine.pc_master(0);int hr=Engine.pc_audio_start(-1);log.AppendLine("WASAPI default output "+hr.ToString("X8"));
  if(hr==0){Engine.pc_play(1);Thread.Sleep(350);Check(Engine.pc_audio_error()==0,"WASAPI streaming");Check(Engine.pc_audio_stop()==0,"WASAPI stop");Engine.pc_play(0);}
  else{Engine.pc_audio_stop();log.AppendLine("No output device available: live audio test skipped.");}
  log.AppendLine("PASS: native Windows engine, twenty scenes, lookahead, manual notes, ADSR, save/load/corruption, audio lifecycle.");
  File.WriteAllText(Path.Combine(dir,"self-test.log"),log.ToString());return 0;
 }catch(Exception e){log.AppendLine("FAIL "+e);File.WriteAllText(Path.Combine(dir,"self-test.log"),log.ToString());return 1;}}
}
