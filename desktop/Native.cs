// SPDX-License-Identifier: GPL-3.0-only
using System;using System.Text;using System.Runtime.InteropServices;
internal static class Engine {
 const string D="nightclaw-engine.dll";
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_init();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_scene_count();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] static extern IntPtr pc_scene_name(int i);
 public static string SceneName(int i){return Marshal.PtrToStringAnsi(pc_scene_name(i));}
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_scene_current();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_scene(int i);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_param_count();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] static extern IntPtr pc_param_name(int p);
 public static string ParamName(int p){return Marshal.PtrToStringAnsi(pc_param_name(p));}
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_get(int p);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_min(int p);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_max(int p);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_set(int p,int v);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_track_param(int t,int c);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_voice_param(int t);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] static extern void pc_value_text(int p,int v,StringBuilder s);
 public static string ValueText(int p,int v){var s=new StringBuilder(128);pc_value_text(p,v,s);return s.ToString();}
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern uint pc_envelope_ms10(int value);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_master_get();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_envelope_get(int t,int s);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_envelope_set(int t,int s,int v);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_beat();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] static extern IntPtr pc_beat_name(int i);
 public static string BeatName(int i){return Marshal.PtrToStringAnsi(pc_beat_name(i));}
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_beat_set(int i);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_percussion_decay_id();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_global_id(int i);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_master(int v);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_play(int on);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_playing();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_pad_note(int degree);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_note(int n,int on);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_call(int n);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_panic();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_planner_get(int p);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_planner_set(int p,int v);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_replan();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_plan_count();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_plan_event(int i,int f);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_tick();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_cycle();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern float pc_peak();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern void pc_render([Out]float[] output,int frames);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_save_size();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_save([Out]byte[] data,int n);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_load(byte[] data,int n);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_audio_devices();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl,CharSet=CharSet.Unicode)] public static extern void pc_audio_name(int i,StringBuilder name,int cap);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_audio_start(int index);
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_audio_stop();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern int pc_audio_error();
 [DllImport(D,CallingConvention=CallingConvention.Cdecl)] public static extern float pc_audio_load();
}
