// SPDX-License-Identifier: GPL-3.0-only
import("paleo.lib");
// bone rasp: a notched bone scraped with a stick over a gourd; f = how fast the notches go by (teeth per second)
env = en.asr(0.01,1,0.04,t)*g;
rate = f*(1+0.15*no.lfnoise(4));
teeth = os.lf_imptrain(rate)*env;
click = teeth : fi.resonbp(1800,3,1) + (teeth : fi.resonbp(3400,4,0.6)) + (no.noise*env*0.05 : fi.highpass(2,2000));
gourd = click : fi.resonbp(420,4,1) : *(0.6);
process = (click*0.8 + gourd)*1.5 : fi.lowpass(2,5000) : ma.tanh;
