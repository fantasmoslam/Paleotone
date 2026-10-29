/* SPDX-License-Identifier: GPL-3.0-only */
package org.paleotone.synth;
final class Native {
    static { System.loadLibrary("paleotone"); }
    static native void init(byte[] saved);
    static native String start(int buffers);
    static native void stop();
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
}
