/* SPDX-License-Identifier: GPL-3.0-only */
#include <aaudio/AAudio.h>
#include <jni.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "engine.h"
#include "output_stage.h"

#define JNI(name) Java_org_paleotone_synth_Native_##name
#define PIXELS (240*240)
static AAudioStream *stream;
static int booted, sample_rate, burst, buffer_frames, sharing, performance;
static _Atomic unsigned key_mask, button_mask, master_value=700, panic_requested;
static _Atomic int encoders[7], stream_error;
static _Atomic unsigned callbacks, max_us, latest_us, peak_q15;
static _Atomic unsigned output_level=1, limited_frames;
static pt_output_t output_stage;
static float output_gain(void){return (float)(1u<<atomic_load(&output_level));}
static uint32_t screen_pixels[PIXELS];
static pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned snapshot_due;
static uint8_t save_data[1100000];
/* Multiple non-audio producers serialize here. The consumer never locks. */
static pthread_mutex_t midi_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t midi_queue[256];
static _Atomic unsigned midi_w, midi_r;
static uint64_t nanos(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;
}
static void inputs(void) {
    if (atomic_exchange(&panic_requested, 0)) {
        pt_panic();
        atomic_store(&midi_r, atomic_load(&midi_w));
    }
    pt_keys(atomic_load(&key_mask)); pt_buttons(atomic_load(&button_mask));
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
        pt_midi(m&255, (m>>8)&127, (m>>16)&127);
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
    float peak=pt_output_process(&output_stage,out,(unsigned)frames,output_gain());
    atomic_store(&limited_frames,output_stage.limited);
    atomic_store(&peak_q15, (unsigned)(peak*32767));
    snapshot_due+=(unsigned)frames;
    if (snapshot_due >= (unsigned)sample_rate/30) {
        snapshot_due=0;
        /* UI may hold this mutex. Never wait for it on the audio thread. */
        if (!pthread_mutex_trylock(&screen_mutex)) {
            pt_screen(screen_pixels);
            pthread_mutex_unlock(&screen_mutex);
        }
    }
    unsigned us=(unsigned)((nanos()-start)/1000);
    atomic_store(&latest_us, us);
    unsigned previous=atomic_load(&max_us);
    if (us>previous) atomic_store(&max_us, us);
    atomic_fetch_add(&callbacks, 1);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
static void audio_error(AAudioStream *s, void *u, aaudio_result_t error) {
    (void)s; (void)u;
    atomic_store(&stream_error, error); /* UI thread closes/reopens the stream. */
}
static void stop(void) {
    if (stream) {
        AAudioStream_requestStop(stream);
        /* close waits for callbacks; engine ownership transfers only afterward. */
        AAudioStream_close(stream); stream=NULL;
    }
    atomic_store(&key_mask,0); atomic_store(&button_mask,0);
    if (booted) pt_panic();
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
    pt_screen(screen_pixels);
}
JNIEXPORT jstring JNICALL JNI(start)(JNIEnv *env,jclass cls,jint buffers) {
    (void)cls;
    if (!booted) return (*env)->NewStringUTF(env,"Engine is not initialized");
    if (stream && !atomic_load(&stream_error)) return NULL;
    stop(); atomic_store(&stream_error,0);
    atomic_store(&callbacks,0); atomic_store(&max_us,0); atomic_store(&latest_us,0);
    snapshot_due=0;
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
    atomic_store(&limited_frames,0);
    result=AAudioStream_requestStart(stream);
    if (result!=AAUDIO_OK) { stop(); return (*env)->NewStringUTF(env,AAudio_convertResultToText(result)); }
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
    pthread_mutex_lock(&screen_mutex);
    (*env)->SetIntArrayRegion(env,out,0,PIXELS,(jint*)screen_pixels);
    pthread_mutex_unlock(&screen_mutex);
}
JNIEXPORT jstring JNICALL JNI(stats)(JNIEnv *env,jclass cls) {
    (void)cls;
    char text[256];
    if (stream) snprintf(text,sizeof text,"%d Hz | burst %d | buffer %d | %s / %s | xruns %d | render %u us (max %u) | +%u dB | limits %u | error %d",
        sample_rate,burst,buffer_frames,sharing==AAUDIO_SHARING_MODE_EXCLUSIVE?"exclusive":"shared",
        performance==AAUDIO_PERFORMANCE_MODE_LOW_LATENCY?"low latency":"normal",
        AAudioStream_getXRunCount(stream),atomic_load(&latest_us),atomic_load(&max_us),atomic_load(&output_level)*6,
        atomic_load(&limited_frames),atomic_load(&stream_error));
    else snprintf(text,sizeof text,"Audio paused — project held in memory");
    return (*env)->NewStringUTF(env,text);
}
JNIEXPORT jint JNICALL JNI(peak)(JNIEnv *e,jclass c) {(void)e;(void)c;return (jint)atomic_load(&peak_q15);}
