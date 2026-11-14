/* SPDX-License-Identifier: GPL-3.0-only */
#define COBJMACROS
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdio.h>
extern void pc_render(float *,int);
static HANDLE thread,ready;static LONG stopping;static HRESULT result;static float load_percent;static int output_index=-1;
static IMMDevice *get_device(IMMDeviceEnumerator *e,int index){
 IMMDevice *d=0;IMMDeviceCollection *c=0;
 if(index<0)IMMDeviceEnumerator_GetDefaultAudioEndpoint(e,eRender,eConsole,&d);
 else if(SUCCEEDED(IMMDeviceEnumerator_EnumAudioEndpoints(e,eRender,DEVICE_STATE_ACTIVE,&c))){IMMDeviceCollection_Item(c,(UINT)index,&d);IMMDeviceCollection_Release(c);}return d;
}
__declspec(dllexport) int pc_audio_devices(void){
 IMMDeviceEnumerator *e=0;IMMDeviceCollection *c=0;UINT n=0;HRESULT hr=CoInitializeEx(0,COINIT_MULTITHREADED);
 if(SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator,0,CLSCTX_ALL,&IID_IMMDeviceEnumerator,(void**)&e))){
  if(SUCCEEDED(IMMDeviceEnumerator_EnumAudioEndpoints(e,eRender,DEVICE_STATE_ACTIVE,&c))){IMMDeviceCollection_GetCount(c,&n);IMMDeviceCollection_Release(c);}IMMDeviceEnumerator_Release(e);}
 if(SUCCEEDED(hr))CoUninitialize();return (int)n;
}
__declspec(dllexport) void pc_audio_name(int i,wchar_t *out,int cap){
 IMMDeviceEnumerator *e=0;IMMDevice *d=0;IPropertyStore *p=0;PROPVARIANT v;
 HRESULT hr=CoInitializeEx(0,COINIT_MULTITHREADED);if(cap<1)return;out[0]=0;memset(&v,0,sizeof v);
 if(SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator,0,CLSCTX_ALL,&IID_IMMDeviceEnumerator,(void**)&e))){
  d=get_device(e,i);if(d&&SUCCEEDED(IMMDevice_OpenPropertyStore(d,STGM_READ,&p))){
   if(SUCCEEDED(IPropertyStore_GetValue(p,&PKEY_Device_FriendlyName,&v))&&v.vt==VT_LPWSTR){wcsncpy(out,v.pwszVal,(size_t)cap-1);out[cap-1]=0;}PropVariantClear(&v);IPropertyStore_Release(p);}
  if(d)IMMDevice_Release(d);IMMDeviceEnumerator_Release(e);}if(SUCCEEDED(hr))CoUninitialize();
}
static DWORD WINAPI audio_thread(void *unused){
 IMMDeviceEnumerator *e=0;IMMDevice *d=0;IAudioClient *a=0;IAudioRenderClient *r=0;
 WAVEFORMATEX format={WAVE_FORMAT_IEEE_FLOAT,2,44100,44100*8,8,32,0};
 UINT32 capacity=0,padding=0,n;BYTE *data;HRESULT hr;LARGE_INTEGER freq,start,end;
 (void)unused;QueryPerformanceFrequency(&freq);CoInitializeEx(0,COINIT_MULTITHREADED);
 hr=CoCreateInstance(&CLSID_MMDeviceEnumerator,0,CLSCTX_ALL,&IID_IMMDeviceEnumerator,(void**)&e);if(FAILED(hr))goto done;
 d=get_device(e,output_index);if(!d){hr=E_FAIL;goto done;}
 hr=IMMDevice_Activate(d,&IID_IAudioClient,CLSCTX_ALL,0,(void**)&a);if(FAILED(hr))goto done;
 hr=IAudioClient_Initialize(a,AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM|AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,400000,0,&format,0);if(FAILED(hr))goto done;
 hr=IAudioClient_GetBufferSize(a,&capacity);if(FAILED(hr))goto done;
 hr=IAudioClient_GetService(a,&IID_IAudioRenderClient,(void**)&r);if(FAILED(hr))goto done;
 hr=IAudioRenderClient_GetBuffer(r,capacity,&data);if(FAILED(hr))goto done;
 memset(data,0,capacity*8);IAudioRenderClient_ReleaseBuffer(r,capacity,0);hr=IAudioClient_Start(a);if(FAILED(hr))goto done;
 result=S_OK;SetEvent(ready);
 while(!InterlockedCompareExchange(&stopping,0,0)){
  hr=IAudioClient_GetCurrentPadding(a,&padding);if(FAILED(hr))break;n=capacity-padding;
  if(n){hr=IAudioRenderClient_GetBuffer(r,n,&data);if(FAILED(hr))break;
   QueryPerformanceCounter(&start);pc_render((float*)data,(int)n);QueryPerformanceCounter(&end);
   load_percent=.95f*load_percent+.05f*(float)((end.QuadPart-start.QuadPart)*44100.0*100.0/(freq.QuadPart*n));
   hr=IAudioRenderClient_ReleaseBuffer(r,n,0);if(FAILED(hr))break;}Sleep(2);}
 IAudioClient_Stop(a);
done:
 result=hr;SetEvent(ready);if(r)IAudioRenderClient_Release(r);if(a)IAudioClient_Release(a);if(d)IMMDevice_Release(d);if(e)IMMDeviceEnumerator_Release(e);CoUninitialize();return 0;
}
__declspec(dllexport) int pc_audio_stop(void){
 if(thread){InterlockedExchange(&stopping,1);if(WaitForSingleObject(thread,5000)!=WAIT_OBJECT_0)return -1;CloseHandle(thread);thread=0;}if(ready){CloseHandle(ready);ready=0;}return 0;
}
__declspec(dllexport) int pc_audio_start(int index){
 if(pc_audio_stop())return -1;output_index=index;InterlockedExchange(&stopping,0);result=E_PENDING;load_percent=0;
 ready=CreateEventW(0,TRUE,FALSE,0);if(!ready)return -1;thread=CreateThread(0,0,audio_thread,0,0,0);
 if(!thread){CloseHandle(ready);ready=0;return -1;}if(WaitForSingleObject(ready,5000)!=WAIT_OBJECT_0)return -1;return (int)result;
}
__declspec(dllexport) int pc_audio_error(void){return (int)result;}
__declspec(dllexport) float pc_audio_load(void){return load_percent;}
