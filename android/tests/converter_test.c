/* SPDX-License-Identifier: GPL-3.0-only */
/* Sine sweeps through the converter: passband gain and everything that is not
 * the sine (images folded back into the audio band), against the old linear one. */
#include "../native/converter.h"
#include <assert.h>
#include <stdio.h>
static pt_converter conv;
static double freq, t;
static void sine(float *v){v[0]=v[1]=(float)(0.5*sin(2*CONV_PI*freq*t/44100));t+=1;}
static void measure(unsigned rate,unsigned linear,double f,double *gain_db,double *rest_db){
    static float out[2*24000];
    conv_prepare(&conv,44100,rate);if(linear)conv.linear=1;
    freq=f;t=0;conv_run(&conv,out,4000,sine);conv_run(&conv,out,24000,sine);  /* settle, then measure */
    /* Least-squares fit of the sine at f; the remainder is images and error. */
    double ss=0,cc=0,sc=0,sy=0,cy=0,yy=0;
    for(unsigned i=0;i<24000;++i){double w=2*CONV_PI*f*i/rate,s=sin(w),c=cos(w),y=out[2*i];
        ss+=s*s;cc+=c*c;sc+=s*c;sy+=s*y;cy+=c*y;yy+=y*y;assert(out[2*i]==out[2*i+1]);}
    double det=ss*cc-sc*sc,a=(sy*cc-cy*sc)/det,b=(cy*ss-sy*sc)/det;
    double fit=a*a*ss+b*b*cc+2*a*b*sc,amp=sqrt(a*a+b*b);
    *gain_db=20*log10(amp/0.5);*rest_db=10*log10((yy-fit)/fit+1e-30);
}
int main(void){
    const double tones[]={100,1000,5000,10000,15000,18000,20000};
    printf("44.1 -> 48 kHz    linear: gain / other     new: gain / other\n");
    for(unsigned i=0;i<sizeof tones/sizeof *tones;++i){
        double lg,lr,g,r;measure(48000,1,tones[i],&lg,&lr);measure(48000,0,tones[i],&g,&r);
        printf("%6.0f Hz      %6.2f dB / %6.1f dB      %6.2f dB / %6.1f dB\n",tones[i],lg,lr,g,r);
        if(tones[i]<=18000){assert(fabs(g)<0.1);assert(r<-65);}
        if(tones[i]<=20000)assert(fabs(g)<0.5);
    }
    assert(conv.up==160&&conv.down==147&&!conv.linear);
    double g,r;
    measure(44100,0,1000,&g,&r);assert(fabs(g)<0.01&&r<-90);               /* same rate: a pure delay */
    measure(96000,0,15000,&g,&r);assert(fabs(g)<0.1&&r<-65);
    measure(32000,0,10000,&g,&r);assert(fabs(g)<0.1&&r<-65);               /* down-conversion */
    conv_prepare(&conv,44100,47999);assert(conv.linear);                     /* an odd ratio: linear fallback */
    measure(47999,0,1000,&g,&r);assert(fabs(g)<0.1);
    puts("PASS: converter flat to 18 kHz within 0.1 dB, images below -65 dB; 44.1/96/32 kHz; odd-rate fallback");
}
