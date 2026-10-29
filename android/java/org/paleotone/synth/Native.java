/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
final class Native {
    static { System.loadLibrary("paleotone"); }
    static native void init(byte[] saved);
    static native String start(int buffers);
    static native void stop();
    static native int error();
    static native void level(int value);
    static native byte[] save();
    static native void keys(int mask);
    static native void buttons(int mask);
    static native void encoder(int role,int steps);
    static native void master(int value);
    static native void panic();
    static native void midi(int status,int d1,int d2);
    static native void pixels(int[] pixels);
    static native String stats();
    static native int peak();
    static native String[] controlLabels();
    static native int[] controls();
    static native String[] controlTexts();
    static native void control(int slot,int value);
    static native boolean scape(int command);
    static native boolean loadSample(float[] data,int rate,int root);
    static native void sampleMode(boolean enabled);
    static native void sampleRoot(int note);
}
