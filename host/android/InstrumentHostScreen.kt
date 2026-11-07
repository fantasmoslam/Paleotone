// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Dylan Spaniel and Paleotone contributors
// Project-owned interface. The reusable engine/panel API remains attributed upstream.
package org.paleotone.host.ui

import android.graphics.Paint
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.*
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.FilterQuality
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.pointer.PointerId
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.quixotic7.remotefm1.Mode
import com.quixotic7.remotefm1.Source
import com.quixotic7.remotefm1.ui.AppUi
import com.quixotic7.remotefm1.ui.faceplate.PanelFeed
import com.quixotic7.remotefm1.ui.faceplate.PanelKnob
import com.quixotic7.remotefm1.ui.faceplate.PanelLabel

private val Ink = Color(0xFFF1EBDD)
private val Muted = Color(0xFFA8ADA3)
private val Ground = Color(0xFF131916)
private val Panel = Color(0xFF202923)
private val Line = Color(0xFF39463C)
private val Amber = Color(0xFFE9BA73)
private val Mint = Color(0xFF9FD3BD)
private val Alert = Color(0xFFEAA099)
private val Shape = RoundedCornerShape(10.dp)
private enum class Page(val title: String) { PLAY("Play"), LIBRARY("Instruments"), CONTROLLERS("Controllers"), AUDIO("Audio"), ABOUT("About") }

@Composable
fun InstrumentHostScreen(a: AppUi, askNotifications: () -> Unit) {
    var page by rememberSaveable { mutableStateOf(Page.PLAY.name) }
    val m = a.model
    LaunchedEffect(m) {
        // Hardware integration is a separate, unqualified milestone. This shell
        // makes no assumption that the user has an upstream bridge installed.
        m.useSource(Source.TABLET)
        if (m.usbAudio) m.useUsbAudio(false)
        if (m.mode != Mode.CORE) m.useMode(Mode.CORE)
    }
    LaunchedEffect(a.feed) {
        try { while (true) { withFrameNanos { a.feed.poll() } } }
        finally { a.feed.releaseAll() }
    }
    BackHandler(page != Page.PLAY.name) { page = Page.PLAY.name }
    MaterialTheme(colorScheme = darkColorScheme(primary = Amber, secondary = Mint, background = Ground,
        surface = Panel, onPrimary = Ground, onBackground = Ink, onSurface = Ink)) {
        Column(Modifier.fillMaxSize().background(Ground).safeDrawingPadding().padding(horizontal = 18.dp)) {
            Row(Modifier.fillMaxWidth().padding(vertical = 14.dp), verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(14.dp)) {
                Column(Modifier.weight(1f)) {
                    Eyebrow("TABLET INSTRUMENT WORKBENCH")
                    Text("Instrument Host", color = Ink, fontSize = 25.sp, fontWeight = FontWeight.SemiBold)
                }
                Column(horizontalAlignment = Alignment.End) {
                    Text((m.current?.name ?: m.selected), color = Amber, fontSize = 16.sp, maxLines = 1, overflow = TextOverflow.Ellipsis)
                    Text(m.audioState.line.ifEmpty { "Audio muted" }, color = Muted, fontSize = 12.sp, maxLines = 1, overflow = TextOverflow.Ellipsis, modifier = Modifier.widthIn(max = 180.dp))
                }
                Action(if (m.soundOn) "Mute" else "Enable audio", accent = m.soundOn) {
                    if (m.soundOn) a.feed.releaseAll() else askNotifications()
                    m.setSound(!m.soundOn)
                }
            }
            Row(Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()), horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                Page.entries.forEach { p ->
                    TextButton(onClick = { a.feed.releaseAll(); page = p.name },
                        modifier = Modifier.heightIn(min = 48.dp), shape = Shape,
                        colors = ButtonDefaults.textButtonColors(contentColor = if (page == p.name) Ground else Muted,
                            containerColor = if (page == p.name) Amber else Color.Transparent)) {
                        Text(p.title, fontSize = 14.sp, fontWeight = FontWeight.SemiBold)
                    }
                }
            }
            Box(Modifier.fillMaxWidth().padding(top = 8.dp).height(1.dp).background(Line))
            Box(Modifier.weight(1f).fillMaxWidth().padding(vertical = 12.dp)) {
                when (Page.valueOf(page)) {
                    Page.PLAY -> key(m.selected) { Performance(a) }
                    Page.LIBRARY -> Library(a) { page = Page.PLAY.name }
                    Page.CONTROLLERS -> Controllers(a)
                    Page.AUDIO -> Audio(a)
                    Page.ABOUT -> About(a)
                }
            }
            Row(Modifier.fillMaxWidth().padding(bottom = 10.dp), horizontalArrangement = Arrangement.SpaceBetween) {
                Text("TOUCH PLAY / ${(m.current?.name ?: m.selected)}", color = Muted, fontFamily = FontFamily.Monospace, fontSize = 10.sp)
                Text("HOST 0.1.2 · FM-1 + SMK-37 project", color = Muted, fontFamily = FontFamily.Monospace, fontSize = 10.sp)
            }
        }
    }
}

@Composable
private fun Eyebrow(text: String) {
    Text(text, color = Mint, fontFamily = FontFamily.Monospace, fontSize = 10.sp, letterSpacing = 1.sp)
}

@Composable
private fun Action(text: String, accent: Boolean = false, enabled: Boolean = true, modifier: Modifier = Modifier, onClick: () -> Unit) {
    Button(onClick = onClick, enabled = enabled, modifier = modifier.heightIn(min = 48.dp), shape = Shape,
        contentPadding = PaddingValues(horizontal = 14.dp, vertical = 10.dp),
        colors = ButtonDefaults.buttonColors(containerColor = if (accent) Amber else Line, contentColor = if (accent) Ground else Ink)) {
        Text(text, fontSize = 14.sp, maxLines = 2)
    }
}

@Composable
private fun Performance(a: AppUi) {
    Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(12.dp)) {
        BoxWithConstraints(Modifier.weight(1f).fillMaxWidth()) {
            val displaySide = maxHeight.coerceIn(200.dp, 330.dp)
            if (maxWidth >= 840.dp) {
                Row(Modifier.fillMaxSize(), horizontalArrangement = Arrangement.spacedBy(18.dp)) {
                    Column(Modifier.width(displaySide).fillMaxHeight(), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        OriginalDisplay(a.feed, Modifier.weight(1f).fillMaxWidth())
                        Text("Original instrument display", color = Muted, fontSize = 12.sp)
                    }
                    Controls(a, Modifier.weight(1f).fillMaxHeight().verticalScroll(rememberScrollState()))
                }
            } else {
                Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()), verticalArrangement = Arrangement.spacedBy(16.dp)) {
                    OriginalDisplay(a.feed, Modifier.height(240.dp).fillMaxWidth())
                    Controls(a, Modifier.fillMaxWidth())
                }
            }
        }
        Piano(a.feed, Modifier.fillMaxWidth().height(152.dp))
    }
}

@Composable
private fun OriginalDisplay(feed: PanelFeed, modifier: Modifier) {
    Canvas(modifier.background(Color.Black, Shape).padding(8.dp).semantics { contentDescription = "Original instrument display" }) {
        feed.frames // observe in the draw phase: the same bitmap object changes in place
        val side = minOf(size.width, size.height).toInt().coerceAtLeast(1)
        drawImage(feed.image, dstOffset = IntOffset((size.width.toInt() - side) / 2, (size.height.toInt() - side) / 2),
            dstSize = IntSize(side, side), filterQuality = FilterQuality.None)
    }
}

@Composable
private fun Controls(a: AppUi, modifier: Modifier) {
    val feed = a.feed
    Column(modifier, verticalArrangement = Arrangement.spacedBy(10.dp)) {
        Eyebrow("SOUND & NAVIGATION")
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            listOf(PanelKnob.PRESETS to "Preset", PanelKnob.SELECT to "Select", PanelKnob.ALGORITHM to "Algorithm").forEach { (knob, label) ->
                Stepper(label, Modifier.weight(1f), { feed.turn(knob, -1) }, { feed.turn(knob, 1) })
            }
        }
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            listOf(PanelKnob.K1, PanelKnob.K2, PanelKnob.K3, PanelKnob.K4).forEachIndexed { index, knob ->
                Stepper("Knob ${index + 1}", Modifier.weight(1f), { feed.turn(knob, -1) }, { feed.turn(knob, 1) })
            }
        }
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Text("Master", color = Muted, modifier = Modifier.width(65.dp), fontSize = 13.sp)
            feed.knobs.version
            Slider(value = feed.knobs.master.toFloat(), onValueChange = { feed.master(it.toInt()) }, valueRange = 0f..1023f,
                modifier = Modifier.weight(1f).heightIn(min = 48.dp), colors = SliderDefaults.colors(thumbColor = Amber, activeTrackColor = Amber))
        }
        Eyebrow("INSTRUMENT PANEL · HOLD BUTTONS FOR THEIR SECONDARY ACTIONS")
        PanelLabel.entries.chunked(7).forEach { row ->
            Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                row.forEach { label -> HoldButton(feed, label, Modifier.weight(1f)) }
            }
        }
    }
}

@Composable
private fun Stepper(title: String, modifier: Modifier, back: () -> Unit, next: () -> Unit) {
    Column(modifier.background(Panel, Shape).padding(6.dp), horizontalAlignment = Alignment.CenterHorizontally) {
        Text(title, color = Ink, fontSize = 12.sp, maxLines = 1)
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceEvenly) {
            Action("−", modifier = Modifier.weight(1f), onClick = back)
            Spacer(Modifier.width(4.dp))
            Action("+", modifier = Modifier.weight(1f), onClick = next)
        }
    }
}

@Composable
private fun HoldButton(feed: PanelFeed, label: PanelLabel, modifier: Modifier) {
    feed.lights.version
    val down = feed.lights.labelDown(label.ordinal)
    val title = feed.names.label(label)
    DisposableEffect(feed, label) { onDispose { feed.button(label, false) } }
    Box(modifier.heightIn(min = 52.dp).background(if (down) Mint else Panel, Shape)
        .border(1.dp, if (down) Mint else Line, Shape)
        .semantics { contentDescription = label.spoken }
        .pointerInput(feed, label) {
            detectTapGestures(onPress = {
                feed.button(label, true)
                try { tryAwaitRelease() } finally { feed.button(label, false) }
            })
        }.padding(horizontal = 3.dp, vertical = 12.dp), contentAlignment = Alignment.Center) {
        Text(title, color = if (down) Ground else Ink, fontSize = 11.sp, fontWeight = FontWeight.Medium, maxLines = 2)
    }
}

@Composable
private fun Piano(feed: PanelFeed, modifier: Modifier) {
    feed.lights.version
    val pressed = feed.lights.touchedKeys or feed.lights.heldKeys
    Canvas(modifier.background(Panel, Shape).semantics { contentDescription = "Playable keyboard, F3 to G5" }
        .pointerInput(feed) {
            val held = mutableMapOf<PointerId, Int>()
            try {
                awaitPointerEventScope {
                    while (true) {
                        val event = awaitPointerEvent()
                        for (change in event.changes) {
                            val old = held[change.id]
                            val next = if (change.pressed) HostKeyboard.keyAt(change.position.x, change.position.y, size.width.toFloat(), size.height.toFloat()) else null
                            if (old != next) {
                                if (next != null) { feed.keyDown(next); held[change.id] = next } else held.remove(change.id)
                                if (old != null) feed.keyUp(old)
                            }
                            if (old != null || next != null) change.consume()
                        }
                    }
                }
            } finally { held.values.forEach { feed.keyUp(it) }; held.clear() }
        }) {
        val unit = size.width / HostKeyboard.white.size
        val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Ground.toArgb(); textSize = 10.sp.toPx(); textAlign = Paint.Align.CENTER }
        for ((index, note) in HostKeyboard.white.withIndex()) {
            val down = pressed and (1 shl note) != 0
            drawRoundRect(if (down) Amber else Ink, Offset(index * unit + 1, 0f), Size(unit - 2, size.height), CornerRadius(5.dp.toPx()))
            val midi = note + 53
            val name = arrayOf("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B")[midi % 12]
            drawContext.canvas.nativeCanvas.drawText("$name${midi / 12 - 1}", (index + 0.5f) * unit, size.height - 12.dp.toPx(), paint)
        }
        for (note in HostKeyboard.black) {
            drawRoundRect(if (pressed and (1 shl note) != 0) Mint else Ground, Offset(HostKeyboard.blackLeft(note) * unit, 0f),
                Size(0.62f * unit, size.height * 0.61f), CornerRadius(4.dp.toPx()))
        }
    }
}

@Composable
private fun Library(a: AppUi, play: () -> Unit) {
    val m = a.model
    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()), verticalArrangement = Arrangement.spacedBy(16.dp)) {
        Eyebrow("YOUR OFFLINE COLLECTION")
        Text("Choose an instrument", color = Ink, fontSize = 30.sp, fontWeight = FontWeight.SemiBold)
        Text("Each instrument keeps its own sound engine, screen, sequencer and memory.", color = Muted, fontSize = 15.sp)
        m.catalog.forEachIndexed { index, core ->
            Column(Modifier.fillMaxWidth().background(Panel, Shape).border(1.dp, if (core.id == m.selected) Amber else Line, Shape).padding(20.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Eyebrow("${(index + 1).toString().padStart(2, '0')} / ${if (core.id == m.selected) "SELECTED" else "INSTALLED"}")
                Text(core.name, color = Ink, fontSize = 27.sp, fontWeight = FontWeight.Medium)
                Text(if (core.id == "paleotone") "Dinosaur voices, evolving landscapes and synthesis." else "Multi-engine synthesis, four parts and sequencing.", color = Muted, fontSize = 14.sp)
                Text("${core.version} · ${core.license}", color = Mint, fontSize = 12.sp)
                Action(if (core.id == m.selected) "Return to instrument" else "Play ${core.name}", accent = true) {
                    a.feed.releaseAll(); m.select(core.id); if (m.mode != Mode.CORE) m.useMode(Mode.CORE); play()
                }
            }
        }
        Eyebrow("SESSION SNAPSHOTS")
        Action("Save snapshot of ${(m.current?.name ?: m.selected)}") { m.saveState("Snapshot ${java.text.SimpleDateFormat("MMM d HH:mm:ss", java.util.Locale.getDefault()).format(java.util.Date())}", a.feed.knobs.master) }
        if (m.message.isNotEmpty()) Text(m.message, color = Mint, fontSize = 14.sp)
        val snapshots = remember(m.selected, m.message, m.dataGen) { m.stateLib.list(m.current?.firmwareID ?: m.selected) }
        snapshots.take(12).forEach { saved -> Action("Load ${saved.name}") { a.feed.releaseAll(); m.loadState(saved); play() } }
        Text("Instrument memory also saves automatically. Named snapshots belong to the selected instrument.", color = Muted, fontSize = 13.sp)
    }
}

@Composable
private fun Controllers(a: AppUi) {
    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()), verticalArrangement = Arrangement.spacedBy(16.dp)) {
        Eyebrow("PHYSICAL CONTROLS")
        Text("Tablet now. Hardware next.", color = Ink, fontSize = 28.sp)
        InfoCard("Touchscreen", "Available", "Play both installed instruments using the performance page.")
        InfoCard("FM-1", "Bridge pending", "Our controller integration is still in development. Keep your existing firmware installed.")
        InfoCard("SMK-37", "Bridge pending", "A separate capability profile will preserve the SMK’s additional controls. Hardware operation is not yet qualified.")
        Text("Android MIDI devices detected: ${a.model.midiEntries.size}", color = Muted, fontSize = 14.sp)
        Text("Device detection alone does not establish a working controller connection. No firmware installation is performed by this interface.", color = Muted, fontSize = 14.sp)
    }
}

@Composable
private fun Audio(a: AppUi) {
    val m = a.model
    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()), verticalArrangement = Arrangement.spacedBy(14.dp)) {
        Eyebrow("OUTPUT & RECOVERY")
        Text("Sound from your tablet", color = Ink, fontSize = 28.sp)
        Text(m.audioState.line.ifEmpty { "Audio muted" }, color = Mint, fontSize = 14.sp)
        Text("Selected output: ${m.outputPick}", color = Muted, fontSize = 14.sp)
        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            Action("Automatic", m.outputPick == "auto") { m.chooseOutput("auto") }
            Action("Tablet speaker", m.outputPick == "speaker") { m.chooseOutput("speaker") }
        }
        m.pickable.distinctBy { it.pickKind }.filter { it.pickKind != "speaker" }.forEach { output ->
            Action(output.name, m.outputPick == output.pickKind) { m.chooseOutput(output.pickKind) }
        }
        Text("USB audio remains experimental and is not enabled here. Use the tablet speaker or a connected headphone output for this test.", color = Muted, fontSize = 14.sp)
        if (m.perfNote.isNotEmpty()) Text(m.perfNote, color = Alert, fontSize = 14.sp)
        if (m.message.isNotEmpty()) Text(m.message, color = Muted, fontSize = 14.sp)
    }
}

@Composable
private fun InfoCard(title: String, state: String, detail: String) {
    Column(Modifier.fillMaxWidth().background(Panel, Shape).padding(20.dp), verticalArrangement = Arrangement.spacedBy(7.dp)) {
        Text(title, color = Ink, fontSize = 21.sp)
        Text(state.uppercase(), color = Mint, fontFamily = FontFamily.Monospace, fontSize = 11.sp)
        Text(detail, color = Muted, fontSize = 14.sp)
    }
}

@Composable
private fun About(a: AppUi) {
    Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState()), verticalArrangement = Arrangement.spacedBy(14.dp)) {
        Eyebrow("PROJECT & ATTRIBUTION")
        Text("One tablet. Distinct instruments.", color = Ink, fontSize = 28.sp)
        Text("An Android instrument host for the DIY more powerful FM-1 project. Interface by the Paleotone project; each instrument keeps its own identity.", color = Muted, fontSize = 15.sp)
        InfoCard("Host internals", "Open-source foundation", "Reuses RemoteFM1Android, FM1VST and their contributors’ runtime, audio and persistence code. This interface is independently authored. GPL-3.0 source and notices are retained.")
        a.model.catalog.forEach { core -> InfoCard(core.name, core.license, "${core.version}\n${core.sourceURL}") }
        Text("This build is a development preview. Tablet playback has been checked on SM-T860; physical FM-1 and SMK-37 control remains unqualified.", color = Muted, fontSize = 14.sp)
    }
}
