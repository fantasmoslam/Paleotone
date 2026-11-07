// SPDX-License-Identifier: GPL-3.0-only
package org.paleotone.host.ui

import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Color
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.CompositionLocalProvider
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import com.quixotic7.remotefm1.ui.faceplate.PanelEngine
import com.quixotic7.remotefm1.ui.faceplate.PanelFeed
import com.quixotic7.remotefm1.ui.faceplate.ButtonNames
import java.nio.ByteBuffer

/** Debug-only real-Compose layout fixture. No audio engine or USB session starts. */
class HostLayoutActivity : ComponentActivity() {
    val overflows = mutableListOf<String>()
    val events = mutableListOf<Triple<Int, Int, Int>>()
    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge(SystemBarStyle.dark(Color.TRANSPARENT), SystemBarStyle.dark(Color.TRANSPARENT))
        super.onCreate(savedInstanceState)
        WindowCompat.setDecorFitsSystemWindows(window, false)
        WindowInsetsControllerCompat(window, window.decorView).apply {
            hide(WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }
        val engine = object : PanelEngine {
            override fun event(type: Int, index: Int, value: Int): Boolean { events.add(Triple(type, index, value)); return true }
            override fun matrixButton(label: Int) = label
            override fun matrixEncoder(role: Int) = role
            override fun panel(out: IntArray) = 0
            override fun preview(have: Int, buf: ByteBuffer) = 0
            override val buttonNamesGen get() = 0
            override fun buttonNames() = ButtonNames.STANDARD
        }
        val feed = PanelFeed(engine)
        assets.open("layout-paleotone.png").use { Canvas(feed.bitmap).drawBitmap(BitmapFactory.decodeStream(it), 0f, 0f, null) }
        val state = HostViewState("paleotone", "Paleotone", true, "Playing on Speaker", "speaker",
            listOf(InstrumentItem("paleotone", "Paleotone", "0.1.1-host-prototype", "GPL-3.0-only"), InstrumentItem("felucca", "Felucca", "1.6.1", "GPL-3.0-only")),
            (1..9).map { SnapshotItem("$it", "Session $it") },
            listOf(OutputItem("auto", "Automatic"), OutputItem("speaker", "Tablet speaker"), OutputItem("headphones", "Headphones"), OutputItem("bluetooth", "Bluetooth")))
        setContent {
            CompositionLocalProvider(LocalHostTextAudit provides { overflows.add(it) }) {
                HostSurface(state, feed, HostActions())
            }
        }
    }
}
