// SPDX-License-Identifier: GPL-3.0-only
package org.paleotone.host.ui

import android.graphics.Bitmap
import androidx.compose.ui.graphics.asAndroidBitmap
import androidx.compose.ui.test.*
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.semantics.SemanticsProperties
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.*
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

@RunWith(AndroidJUnit4::class)
class HostLayoutTest {
    @get:Rule val rule = createAndroidComposeRule<HostLayoutActivity>()
    private fun fit(tag: String) {
        rule.onNodeWithTag(tag).assertIsDisplayed()
        val root = rule.onNodeWithTag("host-root").fetchSemanticsNode().boundsInRoot
        val item = rule.onNodeWithTag(tag).fetchSemanticsNode().boundsInRoot
        assertTrue("$tag outside root: $item vs $root", item.left >= root.left - 1 && item.top >= root.top - 1 && item.right <= root.right + 1 && item.bottom <= root.bottom + 1)
    }
    private fun shot(name: String) {
        rule.waitForIdle()
        val c = rule.activity.resources.configuration
        val root = File(rule.activity.getExternalFilesDir(null), "layout").apply { mkdirs() }
        val filename = "${c.screenWidthDp}x${c.screenHeightDp}-font${c.fontScale}-$name.png"
        File(root, filename).outputStream().use { rule.onRoot().captureToImage().asAndroidBitmap().compress(Bitmap.CompressFormat.PNG, 100, it) }
        rule.onAllNodes(SemanticsMatcher.keyIsDefined(SemanticsProperties.VerticalScrollAxisRange), useUnmergedTree = true).assertCountEquals(0)
        rule.onAllNodes(SemanticsMatcher.keyIsDefined(SemanticsProperties.HorizontalScrollAxisRange), useUnmergedTree = true).assertCountEquals(0)
        assertTrue("Clipped text: ${rule.activity.overflows}", rule.activity.overflows.isEmpty())
        rule.activity.overflows.clear()
    }
    @Test fun everyPageAndControlBankFitsWithoutScrolling() {
        fit("keyboard"); fit("instrument-display"); fit("control-bank"); fit("audio-toggle")
        fit("dial-PRESETS"); fit("dial-SELECT"); fit("dial-ALGORITHM"); fit("dial-MASTER")
        shot("play-sound")
        rule.onNodeWithTag("bank-1").performClick()
        fit("dial-K1"); fit("dial-K4"); shot("play-edit")
        rule.onNodeWithTag("bank-2").performClick()
        fit("panel-FX"); fit("panel-OCT_UP"); shot("play-panel")
        rule.onNodeWithTag("instruments").performClick()
        fit("instrument-paleotone"); fit("instrument-felucca"); shot("instruments")
        rule.onNodeWithTag("menu").performClick(); shot("menu")
        for (page in listOf("SNAPSHOTS", "AUDIO", "CONTROLLERS", "ABOUT")) {
            rule.onNodeWithTag("menu-$page").performClick(); shot(page.lowercase())
            rule.onNodeWithTag("menu").performClick()
        }
    }
    @Test fun keyboardAndDialGesturesReachPanel() {
        rule.onNodeWithTag("keyboard").performTouchInput { down(center); advanceEventTime(100) }
        rule.onNodeWithTag("bank-1").performClick()
        assertFalse("Changing control banks must preserve held notes", rule.activity.events.any { it.first == 1 && it.third == 0 })
        rule.onNodeWithTag("keyboard").performTouchInput { up() }
        rule.onNodeWithTag("bank-0").performClick()
        assertTrue(rule.activity.events.any { it.first == 1 && it.third == 1 })
        assertTrue(rule.activity.events.any { it.first == 1 && it.third == 0 })
        rule.onNodeWithTag("dial-PRESETS").performTouchInput { swipe(center, center.copy(y = center.y - 50)) }
        assertTrue(rule.activity.events.any { it.first == 3 && it.third != 0 })
    }
}
