/* SPDX-License-Identifier: GPL-3.0-only */
#include <aaudio/AAudio.h>
#include <android/log.h>
#include <jni.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "engine.h"
#include "display.h"
#include "output_stage.h"
#include "sample_layer.h"

#define JNI(name) Java_org_paleotone_synth_Native_##name
#define PIXELS (240*240)
#define TAG "PaleotoneAudio"
static AAudioStream *stream;
static uint64_t nanos(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;
}
static int booted, prepared_rate, sample_rate, burst, buffer_frames, sharing, performance;
static _Atomic unsigned key_mask, button_mask, master_value=700, panic_requested;
static _Atomic int encoders[7], stream_error;
static _Atomic unsigned callbacks, max_us, latest_us, peak_q15;
/* Diagnostics, written by the callback only (adb: logcat -s PaleotoneAudio). */
static _Atomic unsigned late_callbacks, max_gap_us, snapshots, snapshot_max_us;
static _Atomic uint64_t render_total_us;
static uint64_t started_ns, last_callback_ns;
static double latency_max_ms;
static _Atomic unsigned output_level=1, limited_frames;
static pt_output_t output_stage;
static float output_gain(void){return (float)(1u<<atomic_load(&output_level));}
#define CONTROL_CAP 64
static pt_sample_layer sampler;
static float *sample_data;
static _Atomic unsigned sample_mode, sample_root=60;
static unsigned sample_keys, previous_mode;
static _Atomic int control_pending[CONTROL_CAP+1];
static _Atomic unsigned control_dirty[CONTROL_CAP+1];
/* The screen leaves the audio thread as the firmware's display snapshot, through
 * a lock-free triple buffer: the callback captures one (about 30 us) only when the
 * UI has taken the last one; the UI thread draws it with the display copy. */
typedef struct { uint32_t size; uint8_t data[PT_SNAPSHOT_MAX]; } snapshot_t;
static snapshot_t slots[3];
#define SLOT_FRESH 4u
static _Atomic unsigned shared_slot=1, snapshot_wanted=1;
static unsigned audio_slot=0, ui_slot=2, snapshot_frame;
/* UI thread only: what the display copy last drew. */
static uint32_t screen_pixels[PIXELS];
static int control_values[CONTROL_CAP*3+2];
static char control_text[CONTROL_CAP][64];
static unsigned display_stale=1;
static void publish_snapshot(void){
    uint64_t start=nanos();
    snapshot_t *slot=&slots[audio_slot];
    slot->size=pt_snapshot(slot->data,sizeof slot->data);
    if(!slot->size)return;
    snapshot_frame=pt_ui_frames();
    audio_slot=atomic_exchange_explicit(&shared_slot,audio_slot|SLOT_FRESH,memory_order_acq_rel)&3u;
    atomic_store_explicit(&snapshot_wanted,0,memory_order_relaxed);
    unsigned us=(unsigned)((nanos()-start)/1000);
    atomic_fetch_add_explicit(&snapshots,1,memory_order_relaxed);
    if(us>atomic_load_explicit(&snapshot_max_us,memory_order_relaxed))atomic_store_explicit(&snapshot_max_us,us,memory_order_relaxed);
}
static void draw_snapshot(const snapshot_t *slot){
    if(!pt_display_apply(slot->data,slot->size))return;
    pt_display_screen(screen_pixels);
    unsigned n=pt_control_count();
    for(unsigned i=0;i<n;++i)pt_display_control_read(i,&control_values[i*3],&control_values[i*3+1],&control_values[i*3+2],control_text[i]);
    control_values[n*3]=(int)pt_display_playing();
    control_values[n*3+1]=(int)pt_display_scape_busy();
}
static void refresh_display(void){
    if(!stream){
        /* Stopped: this thread owns the engine and may capture directly. */
        if(!display_stale)return;
        snapshot_t *slot=&slots[ui_slot];
        slot->size=pt_snapshot(slot->data,sizeof slot->data);
        if(slot->size)draw_snapshot(slot);
        display_stale=0;return;
    }
    if(!(atomic_load_explicit(&shared_slot,memory_order_acquire)&SLOT_FRESH))return;
    ui_slot=atomic_exchange_explicit(&shared_slot,ui_slot,memory_order_acq_rel)&3u;
    draw_snapshot(&slots[ui_slot]);
    atomic_store_explicit(&snapshot_wanted,1,memory_order_relaxed);
}
static uint8_t save_data[1100000];
/* Multiple non-audio producers serialize here. The consumer never locks. */
static pthread_mutex_t midi_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t midi_queue[256];
static _Atomic unsigned midi_w, midi_r;
static void inputs(void) {
    if (atomic_exchange(&panic_requested, 0)) {
        pt_panic(); sample_panic(&sampler); sample_keys=0;
        atomic_store(&midi_r, atomic_load(&midi_w));
    }
    unsigned mode=atomic_load(&sample_mode), keys=atomic_load(&key_mask);
    if(mode!=previous_mode){pt_panic();sample_panic(&sampler);sample_keys=0;previous_mode=mode;}
    sampler.root=atomic_load(&sample_root);
    if(mode){
        unsigned changed=keys^sample_keys;
        for(unsigned k=0;k<27;++k)if(changed&(1u<<k))sample_note(&sampler,53+k,keys&(1u<<k)?100:0,(unsigned)sample_rate);
        sample_keys=keys;pt_keys(0);
    }else pt_keys(keys);
    pt_buttons(atomic_load(&button_mask));
    for(unsigned i=0;i<=pt_control_count();++i)if(atomic_exchange_explicit(&control_dirty[i],0,memory_order_acquire))
        pt_control_set(i,atomic_load(&control_pending[i]));
    pt_master(atomic_load(&master_value));
    for (unsigned i=0; i<7; ++i) {
        int n=atomic_exchange(&encoders[i], 0);
        if (n) pt_encoder(i, n < -32 ? -32 : n > 32 ? 32 : n);
    }
    unsigned r=atomic_load_explicit(&midi_r, memory_order_relaxed);
    unsigned w=atomic_load_explicit(&midi_w, memory_order_acquire);
    /* Bound work even if a controller floods the port. */
    for (unsigned i=0; r!=w && i<64; ++i, ++r) {
        uint32_t m=midi_queue[r&255];
        if((m&255)==0xf0){pt_scape((m>>8)&255);continue;}
        unsigned status=m&240,note=(m>>8)&127,vel=(m>>16)&127;
        if(mode){
            if(status==0x90||status==0x80)sample_note(&sampler,note,status==0x90?vel:0,(unsigned)sample_rate);
            if(status==0xb0&&(note==120||note==123))sample_panic(&sampler);
        }else pt_midi(m&255,note,vel);
    }
    atomic_store_explicit(&midi_r, r, memory_order_release);
}
static aaudio_data_callback_result_t audio_callback(AAudioStream *s, void *user,
                                                    void *data, int32_t frames) {
    (void)s; (void)user;
    uint64_t start=nanos();
    float *out=data;
    inputs();
    pt_render(out, (unsigned)frames, (unsigned)sample_rate);
    if(atomic_load(&sample_mode))sample_mix(&sampler,out,(unsigned)frames,(unsigned)sample_rate,atomic_load(&master_value)/1023.0f);
    float peak=pt_output_process(&output_stage,out,(unsigned)frames,output_gain());
    atomic_store(&limited_frames,output_stage.limited);
    atomic_store(&peak_q15, (unsigned)(peak*32767));
    if(atomic_load_explicit(&snapshot_wanted,memory_order_relaxed)&&pt_ui_frames()!=snapshot_frame)publish_snapshot();
    uint64_t end=nanos();
    unsigned us=(unsigned)((end-start)/1000);
    atomic_store(&latest_us, us);
    if (us>atomic_load(&max_us)) atomic_store(&max_us, us);
    atomic_fetch_add_explicit(&render_total_us,us,memory_order_relaxed);
    /* Late: the callback took longer than the audio it produced. */
    if ((uint64_t)us*(unsigned)sample_rate > (uint64_t)frames*1000000u) atomic_fetch_add(&late_callbacks,1);
    if (last_callback_ns) {
        unsigned gap=(unsigned)((start-last_callback_ns)/1000);
        if (gap>atomic_load(&max_gap_us)) atomic_store(&max_gap_us,gap);
    }
    last_callback_ns=start;
    atomic_fetch_add(&callbacks, 1);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
static void audio_error(AAudioStream *s, void *u, aaudio_result_t error) {
    (void)s; (void)u;
    atomic_store(&stream_error, error); /* UI thread closes/reopens the stream. */
}
static double output_latency_ms(void) {
    /* When the next frame written will be heard, from the stream's own clock:
     * the HAL/DSP path, not the acoustic path. Negative when unavailable. */
    int64_t position,time;
    if (AAudioStream_getTimestamp(stream,CLOCK_MONOTONIC,&position,&time)!=AAUDIO_OK) return -1;
    int64_t written=AAudioStream_getFramesWritten(stream);
    double ms=((double)time+(double)(written-position)*1e9/sample_rate-(double)nanos())/1e6;
    if (ms>latency_max_ms) latency_max_ms=ms;
    return ms;
}
static void describe(char *text,size_t cap,int full) {
    unsigned n=atomic_load(&callbacks);
    unsigned average=n?(unsigned)(atomic_load(&render_total_us)/n):0;
    double latency=output_latency_ms();
    char heard[48];
    if (latency>=0) snprintf(heard,sizeof heard,full?"output latency %.1f ms (max %.1f)":"latency %.0f ms",latency,latency_max_ms);
    else snprintf(heard,sizeof heard,"latency n/a");
    if (!full) {
        snprintf(text,cap,"%d Hz | burst %d | buffer %d | %s / %s | underruns %d | late %u | render %u us (max %u) | %s | +%u dB | limits %u",
            sample_rate,burst,buffer_frames,sharing==AAUDIO_SHARING_MODE_EXCLUSIVE?"exclusive":"shared",
            performance==AAUDIO_PERFORMANCE_MODE_LOW_LATENCY?"low latency":"normal",
            AAudioStream_getXRunCount(stream),atomic_load(&late_callbacks),average,atomic_load(&max_us),heard,
            atomic_load(&output_level)*6,atomic_load(&limited_frames));
        return;
    }
    snprintf(text,cap,"%s | %d Hz, burst %d (%.1f ms), buffer %d of %d | %s / %s | %.0f s | callbacks %u | "
        "underruns %d | late callbacks %u | longest gap %.1f ms | render avg %u us, max %u us | %s | "
        "screen snapshots %u, max %u us | limits %u | error %d",
        sizeof(void*)==8?"64-bit":"32-bit",sample_rate,burst,burst*1000.0/sample_rate,buffer_frames,
        AAudioStream_getBufferCapacityInFrames(stream),
        sharing==AAUDIO_SHARING_MODE_EXCLUSIVE?"exclusive":"shared",
        performance==AAUDIO_PERFORMANCE_MODE_LOW_LATENCY?"low latency":"normal",(nanos()-started_ns)/1e9,n,
        AAudioStream_getXRunCount(stream),atomic_load(&late_callbacks),atomic_load(&max_gap_us)/1000.0,
        average,atomic_load(&max_us),heard,atomic_load(&snapshots),atomic_load(&snapshot_max_us),
        atomic_load(&limited_frames),atomic_load(&stream_error));
}
static void stop(void) {
    if (stream) {
        char text[512]; describe(text,sizeof text,1);
        __android_log_print(ANDROID_LOG_INFO,TAG,"stopped: %s",text);
        AAudioStream_requestStop(stream);
        /* close waits for callbacks; engine ownership transfers only afterward. */
        AAudioStream_close(stream); stream=NULL;
    }
    atomic_store(&key_mask,0); atomic_store(&button_mask,0);
    if (booted) {
        for(unsigned i=0;i<=pt_control_count();++i)if(atomic_exchange(&control_dirty[i],0))pt_control_set(i,atomic_load(&control_pending[i]));
        unsigned r=atomic_load(&midi_r),w=atomic_load(&midi_w);
        for(;r!=w;++r){uint32_t m=midi_queue[r&255];if((m&255)==0xf0)pt_scape((m>>8)&255);}
        pt_panic();display_stale=1;
    }
    sample_panic(&sampler);sample_keys=0;
    atomic_store(&midi_r, atomic_load(&midi_w));
    for (unsigned i=0;i<7;++i) atomic_store(&encoders[i],0);
}
JNIEXPORT void JNICALL JNI(init)(JNIEnv *env,jclass cls,jbyteArray saved) {
    (void)cls;
    if (booted) return;
    size_t n=saved ? (size_t)(*env)->GetArrayLength(env,saved) : 0;
    if (n>sizeof save_data) n=0;
    if (n) (*env)->GetByteArrayRegion(env,saved,0,(jsize)n,(jbyte*)save_data);
    pt_boot(n ? save_data : NULL,n); booted=1;
    pt_display_boot(); display_stale=1;
}
JNIEXPORT jstring JNICALL JNI(start)(JNIEnv *env,jclass cls,jint buffers) {
    (void)cls;
    if (!booted) return (*env)->NewStringUTF(env,"Engine is not initialized");
    if (stream && !atomic_load(&stream_error)) return NULL;
    stop(); atomic_store(&stream_error,0);
    atomic_store(&callbacks,0); atomic_store(&max_us,0); atomic_store(&latest_us,0);
    atomic_store(&late_callbacks,0); atomic_store(&max_gap_us,0); atomic_store(&render_total_us,0);
    atomic_store(&snapshots,0); atomic_store(&snapshot_max_us,0); latency_max_ms=0; last_callback_ns=0;
    /* No callback runs yet: reset the triple buffer so no stale picture returns. */
    atomic_store(&shared_slot,1); atomic_store(&snapshot_wanted,1); audio_slot=0; ui_slot=2; snapshot_frame=pt_ui_frames()-1;
    AAudioStreamBuilder *builder=NULL;
    aaudio_result_t result=AAudio_createStreamBuilder(&builder);
    if (result!=AAUDIO_OK) return (*env)->NewStringUTF(env,AAudio_convertResultToText(result));
    AAudioStreamBuilder_setDirection(builder,AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setChannelCount(builder,2);
    AAudioStreamBuilder_setFormat(builder,AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode(builder,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setSharingMode(builder,AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setUsage(builder,AAUDIO_USAGE_GAME);
    AAudioStreamBuilder_setDataCallback(builder,audio_callback,NULL);
    AAudioStreamBuilder_setErrorCallback(builder,audio_error,NULL);
    /* Let the driver choose its native sample rate. */
    result=AAudioStreamBuilder_openStream(builder,&stream);
    if (result!=AAUDIO_OK) {
        AAudioStreamBuilder_setSharingMode(builder,AAUDIO_SHARING_MODE_SHARED);
        result=AAudioStreamBuilder_openStream(builder,&stream);
    }
    AAudioStreamBuilder_delete(builder);
    if (result!=AAUDIO_OK) { stream=NULL; return (*env)->NewStringUTF(env,AAudio_convertResultToText(result)); }
    sample_rate=AAudioStream_getSampleRate(stream);
    burst=AAudioStream_getFramesPerBurst(stream);
    sharing=AAudioStream_getSharingMode(stream);
    performance=AAudioStream_getPerformanceMode(stream);
    if (sample_rate<8000 || sample_rate>192000 || burst<1 ||
        AAudioStream_getChannelCount(stream)!=2 ||
        AAudioStream_getFormat(stream)!=AAUDIO_FORMAT_PCM_FLOAT) {
        stop(); return (*env)->NewStringUTF(env,"Unsupported audio configuration");
    }
    if (buffers<2) buffers=2;
    if (buffers>8) buffers=8;
    AAudioStream_setBufferSizeInFrames(stream,burst*buffers);
    buffer_frames=AAudioStream_getBufferSizeInFrames(stream);
    pt_output_init(&output_stage,(unsigned)sample_rate,output_gain());
    if (sample_rate!=prepared_rate) { pt_prepare((unsigned)sample_rate); prepared_rate=sample_rate; }
    started_ns=nanos();
    atomic_store(&limited_frames,0);
    result=AAudioStream_requestStart(stream);
    if (result!=AAUDIO_OK) { stop(); return (*env)->NewStringUTF(env,AAudio_convertResultToText(result)); }
    __android_log_print(ANDROID_LOG_INFO,TAG,"started: %d Hz, burst %d, buffer %d, %s",sample_rate,burst,buffer_frames,
        sharing==AAUDIO_SHARING_MODE_EXCLUSIVE?"exclusive":"shared");
    return NULL;
}
JNIEXPORT void JNICALL JNI(stop)(JNIEnv *env,jclass cls) {(void)env;(void)cls;stop();}
JNIEXPORT jint JNICALL JNI(error)(JNIEnv *e,jclass c){(void)e;(void)c;return atomic_load(&stream_error);}
JNIEXPORT void JNICALL JNI(level)(JNIEnv *e,jclass c,jint value){
    (void)e;(void)c;if(value>=0&&value<=2)atomic_store(&output_level,(unsigned)value);
}
JNIEXPORT jbyteArray JNICALL JNI(save)(JNIEnv *env,jclass cls) {
    (void)cls;
    if (stream || !booted) return NULL;
    size_t n=pt_save(save_data,sizeof save_data);
    if (!n) return NULL;
    jbyteArray out=(*env)->NewByteArray(env,(jsize)n);
    if (out) (*env)->SetByteArrayRegion(env,out,0,(jsize)n,(jbyte*)save_data);
    return out;
}
JNIEXPORT void JNICALL JNI(keys)(JNIEnv *e,jclass c,jint mask) {(void)e;(void)c;atomic_store(&key_mask,(unsigned)mask);}
JNIEXPORT void JNICALL JNI(buttons)(JNIEnv *e,jclass c,jint mask) {(void)e;(void)c;atomic_store(&button_mask,(unsigned)mask);}
JNIEXPORT void JNICALL JNI(master)(JNIEnv *e,jclass c,jint value) {(void)e;(void)c;atomic_store(&master_value,value<0?0:value>1023?1023:(unsigned)value);}
JNIEXPORT void JNICALL JNI(encoder)(JNIEnv *e,jclass c,jint role,jint steps) {
    (void)e;(void)c;
    if (role>=0 && role<7 && steps>=-32 && steps<=32) atomic_fetch_add(&encoders[role],steps);
}
JNIEXPORT void JNICALL JNI(panic)(JNIEnv *e,jclass c) {
    (void)e;(void)c;atomic_store(&key_mask,0);atomic_store(&button_mask,0);atomic_store(&panic_requested,1);
}
JNIEXPORT void JNICALL JNI(midi)(JNIEnv *e,jclass c,jint status,jint d1,jint d2) {
    (void)e;(void)c;
    if (status<0x80 || status>=0xf0) return;
    pthread_mutex_lock(&midi_mutex);
    unsigned w=atomic_load(&midi_w), r=atomic_load_explicit(&midi_r,memory_order_acquire);
    if (w-r>=256) atomic_store(&panic_requested,1);
    else {midi_queue[w&255]=(unsigned)status|((d1&127)<<8)|((d2&127)<<16);atomic_store_explicit(&midi_w,w+1,memory_order_release);}
    pthread_mutex_unlock(&midi_mutex);
}
JNIEXPORT void JNICALL JNI(pixels)(JNIEnv *env,jclass cls,jintArray out) {
    (void)cls;
    if ((*env)->GetArrayLength(env,out)<PIXELS) return;
    refresh_display();
    (*env)->SetIntArrayRegion(env,out,0,PIXELS,(jint*)screen_pixels);
}
JNIEXPORT jstring JNICALL JNI(stats)(JNIEnv *env,jclass cls) {
    (void)cls;
    char text[256];
    if (stream) describe(text,sizeof text,0);
    else snprintf(text,sizeof text,"Audio paused — project held in memory");
    return (*env)->NewStringUTF(env,text);
}
JNIEXPORT jstring JNICALL JNI(report)(JNIEnv *env,jclass cls) {
    (void)cls;
    if (!stream) return NULL;
    char text[512]; describe(text,sizeof text,1);
    return (*env)->NewStringUTF(env,text);
}
JNIEXPORT jint JNICALL JNI(peak)(JNIEnv *e,jclass c) {(void)e;(void)c;return (jint)atomic_load(&peak_q15);}

JNIEXPORT jobjectArray JNICALL JNI(controlLabels)(JNIEnv *e,jclass c){
    (void)c;unsigned n=pt_control_count();jclass string=(*e)->FindClass(e,"java/lang/String");
    jobjectArray a=(*e)->NewObjectArray(e,n,string,NULL);if(!a)return NULL;
    for(unsigned i=0;i<n;++i){jstring t=(*e)->NewStringUTF(e,pt_control_label(i));(*e)->SetObjectArrayElement(e,a,i,t);(*e)->DeleteLocalRef(e,t);}return a;
}
JNIEXPORT jintArray JNICALL JNI(controls)(JNIEnv *e,jclass c){
    (void)c;unsigned n=pt_control_count()*3+2;jint values[CONTROL_CAP*3+2];
    refresh_display();memcpy(values,control_values,n*sizeof(jint));
    jintArray a=(*e)->NewIntArray(e,n);if(a)(*e)->SetIntArrayRegion(e,a,0,n,values);return a;
}
JNIEXPORT jobjectArray JNICALL JNI(controlTexts)(JNIEnv *e,jclass c){
    (void)c;unsigned n=pt_control_count();char values[CONTROL_CAP][64];
    refresh_display();memcpy(values,control_text,sizeof values);
    jobjectArray a=(*e)->NewObjectArray(e,n,(*e)->FindClass(e,"java/lang/String"),NULL);if(!a)return NULL;
    for(unsigned i=0;i<n;++i){jstring t=(*e)->NewStringUTF(e,values[i]);(*e)->SetObjectArrayElement(e,a,i,t);(*e)->DeleteLocalRef(e,t);}return a;
}
JNIEXPORT void JNICALL JNI(control)(JNIEnv *e,jclass c,jint slot,jint value){
    (void)e;(void)c;if(slot<0||(unsigned)slot>pt_control_count())return;
    if(!stream){pt_control_set((unsigned)slot,value);display_stale=1;return;}
    atomic_store(&control_pending[slot],value);atomic_store_explicit(&control_dirty[slot],1,memory_order_release);
}
/* Called only on the activity thread after Native.stop: allocation never touches the callback. */
JNIEXPORT jboolean JNICALL JNI(loadSample)(JNIEnv *e,jclass c,jfloatArray data,jint rate,jint root){
    (void)c;if(stream)return JNI_FALSE;
    unsigned n=data?(unsigned)(*e)->GetArrayLength(e,data):0;
    if(data&&(n<2||n>4000000||rate<8000||rate>192000||root<0||root>127))return JNI_FALSE;
    float *next=n?malloc(n*sizeof(float)):NULL;if(n&&!next)return JNI_FALSE;
    if(n){(*e)->GetFloatArrayRegion(e,data,0,n,next);if((*e)->ExceptionCheck(e)){free(next);return JNI_FALSE;}}
    sample_panic(&sampler);free(sample_data);sample_data=next;
    sampler.data=next;sampler.frames=n;sampler.rate=rate;sample_keys=0;
    atomic_store(&sample_root,root);if(!n)atomic_store(&sample_mode,0);return JNI_TRUE;
}
JNIEXPORT void JNICALL JNI(sampleMode)(JNIEnv *e,jclass c,jboolean enabled){(void)e;(void)c;atomic_store(&sample_mode,enabled?1:0);}
JNIEXPORT void JNICALL JNI(sampleRoot)(JNIEnv *e,jclass c,jint note){(void)e;(void)c;if(note>=0&&note<=127)atomic_store(&sample_root,(unsigned)note);}

JNIEXPORT jboolean JNICALL JNI(scape)(JNIEnv *e,jclass c,jint command){
    (void)e;(void)c;if(command<PT_ENV_START||command>PT_SCAPE_NEXT)return JNI_FALSE;
    if(!stream){pt_scape((unsigned)command);display_stale=1;return JNI_TRUE;}
    /* Reuse the serialized producer queue; 0xf0 is internal, never accepted by MIDI JNI. */
    pthread_mutex_lock(&midi_mutex);
    unsigned w=atomic_load(&midi_w),r=atomic_load_explicit(&midi_r,memory_order_acquire);
    int ok=w-r<256;
    if(ok){midi_queue[w&255]=0xf0|((unsigned)command<<8);atomic_store_explicit(&midi_w,w+1,memory_order_release);}
    pthread_mutex_unlock(&midi_mutex);return ok?JNI_TRUE:JNI_FALSE;
}
