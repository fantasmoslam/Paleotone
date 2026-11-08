// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Dylan Spaniel and Paleotone contributors
package org.paleotone.host.ui

/** Original FM-1 key indices, F3 through G5. Geometry belongs to this UI. */
object HostKeyboard {
    val white = listOf(0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26)
    val black = (0..26).filterNot { it in white }
    fun blackLeft(note: Int): Float = white.count { it < note } - 0.31f
    fun keyAt(x: Float, y: Float, width: Float, height: Float): Int? {
        if (width <= 0 || height <= 0 || x < 0 || x >= width || y < 0 || y >= height) return null
        val unit = width / white.size
        if (y < height * 0.61f) {
            black.firstOrNull { x / unit >= blackLeft(it) && x / unit < blackLeft(it) + 0.62f }?.let { return it }
        }
        return white[(x / unit).toInt().coerceIn(0, white.lastIndex)]
    }
}
