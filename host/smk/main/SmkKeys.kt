// SPDX-License-Identifier: GPL-3.0-only
package com.quixotic7.remotefm1

/** SMK's C3..C6 keybed. Geometry is shared by drawing and multi-touch hit testing. */
object SmkKeys {
    data class Key(val index: Int, val black: Boolean, val x: Float, val w: Float, val h: Float)
    val keys: List<Key> = buildList {
        var white = 0
        for (k in 0..36) {
            val black = k % 12 in setOf(1,3,6,8,10)
            if (black) add(Key(k,true,(white-0.29f)/22f,0.58f/22f,0.62f))
            else {add(Key(k,false,white/22f,1f/22f,1f));white++}
        }
    }
    val pads = listOf(0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26)
    fun hit(x: Float,y: Float): Int? {
        if(x<0 || x>=1 || y<0 || y>=1)return null
        return keys.sortedByDescending { it.black }.firstOrNull { x>=it.x && x<it.x+it.w && y<it.h }?.index
    }
}
