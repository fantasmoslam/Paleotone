// SPDX-License-Identifier: GPL-3.0-only
package org.paleotone.host.ui

import androidx.compose.runtime.*
import com.quixotic7.remotefm1.Mode
import com.quixotic7.remotefm1.Source
import com.quixotic7.remotefm1.ui.AppUi

/** UI data deliberately independent of the engine, including the layout test fixture. */
data class InstrumentItem(val id: String, val name: String, val version: String, val license: String)
data class SnapshotItem(val id: String, val name: String)
data class OutputItem(val id: String, val name: String)
data class HostViewState(
    val selected: String, val name: String, val soundOn: Boolean, val audioLine: String,
    val route: String, val instruments: List<InstrumentItem>, val snapshots: List<SnapshotItem>,
    val outputs: List<OutputItem>, val message: String = "",
)
data class HostActions(
    val select: (String) -> Unit = {}, val sound: (Boolean) -> Unit = {},
    val output: (String) -> Unit = {}, val save: () -> Unit = {}, val load: (String) -> Unit = {},
)

@Composable
fun InstrumentHostScreen(a: AppUi, askNotifications: () -> Unit) {
    val m = a.model
    LaunchedEffect(m) {
        m.useSource(Source.TABLET)
        if (m.usbAudio) m.useUsbAudio(false)
        if (m.mode != Mode.CORE) m.useMode(Mode.CORE)
    }
    LaunchedEffect(a.feed) {
        try { while (true) withFrameNanos { a.feed.poll() } }
        finally { a.feed.releaseAll() }
    }
    val snapshots = remember(m.selected, m.message, m.dataGen) { m.stateLib.list(m.current?.firmwareID ?: m.selected) }
    val state = HostViewState(
        m.selected, m.current?.name ?: m.selected, m.soundOn, m.audioState.line.ifEmpty { "Audio muted" }, m.outputPick,
        m.catalog.map { InstrumentItem(it.id, it.name, it.version, it.license) },
        snapshots.map { SnapshotItem(it.id, it.name) },
        listOf(OutputItem("auto", "Automatic"), OutputItem("speaker", "Tablet speaker")) +
            m.pickable.filter { !it.usb && it.pickKind != "speaker" }.distinctBy { it.pickKind }.map { OutputItem(it.pickKind, it.title) },
        m.message,
    )
    HostSurface(state, a.feed, HostActions(
        select = { a.feed.releaseAll(); m.select(it); if (m.mode != Mode.CORE) m.useMode(Mode.CORE) },
        sound = { if (it) askNotifications() else a.feed.releaseAll(); m.setSound(it) },
        output = { m.chooseOutput(it) },
        save = { m.saveState("Snapshot ${java.text.SimpleDateFormat("MMM d HH:mm:ss", java.util.Locale.getDefault()).format(java.util.Date())}", a.feed.knobs.master) },
        load = { id -> snapshots.firstOrNull { it.id == id }?.let { a.feed.releaseAll(); m.loadState(it) } },
    ))
}
