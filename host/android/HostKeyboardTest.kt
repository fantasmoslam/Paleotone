// SPDX-License-Identifier: GPL-3.0-only
package org.paleotone.host.ui

import org.junit.Assert.*
import org.junit.Test

class HostKeyboardTest {
    @Test fun everyKeyMapsToItsFirmwareIndexAtTabletSizes() {
        for (width in listOf(560f, 800f, 1240f)) {
            val unit = width / 16f
            HostKeyboard.white.forEachIndexed { index, note ->
                assertEquals(note, HostKeyboard.keyAt((index + 0.5f) * unit, 140f, width, 152f))
            }
            HostKeyboard.black.forEach { note ->
                assertEquals(note, HostKeyboard.keyAt((HostKeyboard.blackLeft(note) + 0.31f) * unit, 40f, width, 152f))
            }
        }
        assertEquals((0..26).toList(), (HostKeyboard.white + HostKeyboard.black).sorted())
    }
    @Test fun LeavingTheKeyboardReleasesInsteadOfClampingToAnEdgeKey() {
        for ((x, y) in listOf(-1f to 40f, 800f to 40f, 40f to -1f, 40f to 152f))
            assertNull(HostKeyboard.keyAt(x, y, 800f, 152f))
        assertNull(HostKeyboard.keyAt(0f, 0f, 0f, 152f))
    }
    @Test fun BlackKeysOnlyOwnTheirShorterTopRegion() {
        val x = (HostKeyboard.blackLeft(1) + 0.31f) * 50f
        assertEquals(1, HostKeyboard.keyAt(x, 10f, 800f, 152f))
        assertTrue(HostKeyboard.keyAt(x, 140f, 800f, 152f) in HostKeyboard.white)
    }
}
