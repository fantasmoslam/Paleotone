// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Dylan Spaniel and Paleotone contributors
package org.paleotone.host.ui

import android.graphics.Paint
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectDragGestures
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
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.pointer.PointerId
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.semantics.*
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.*
import com.quixotic7.remotefm1.ui.faceplate.PanelFeed
import com.quixotic7.remotefm1.ui.faceplate.PanelKnob
import com.quixotic7.remotefm1.ui.faceplate.PanelLabel
import kotlin.math.*

private val Background = Color(0xFF111416)
private val Surface = Color(0xFF1D2225)
private val Edge = Color(0xFF343B3F)
private val Ink = Color(0xFFECE8E0)
private val Quiet = Color(0xFF9CA5A9)
private val Accent = Color(0xFFD7B17D)
private val Shape = RoundedCornerShape(12.dp)
private enum class Page { PLAY, INSTRUMENTS, MENU, SNAPSHOTS, CONTROLLERS, AUDIO, ABOUT }

@Composable
fun HostSurface(state: HostViewState, feed: PanelFeed, actions: HostActions,
                initialPage: String = "PLAY", initialBank: Int = 0) {
    var page by rememberSaveable { mutableStateOf(initialPage) }
    fun go(target: Page) { feed.releaseAll(); page = target.name }
    BackHandler(page != Page.PLAY.name) { go(Page.PLAY) }
    MaterialTheme(colorScheme = darkColorScheme(primary = Accent, onPrimary = Background,
        background = Background, surface = Surface, onSurface = Ink, onBackground = Ink)) {
        Column(Modifier.fillMaxSize().background(Background).safeDrawingPadding().padding(horizontal = 20.dp).testTag("host-root")) {
            Row(Modifier.fillMaxWidth().height(64.dp), verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                if (page == Page.PLAY.name) {
                    HostText("Instrument Host", color = Quiet, fontSize = 16.sp, modifier = Modifier.weight(1f), maxLines = 1)
                } else {
                    QuietButton("‹ Back", tag = "back", onClick = { go(Page.PLAY) })
                    HostText(Page.valueOf(page).name.lowercase().replaceFirstChar { it.uppercase() }, color = Ink,
                        fontSize = 20.sp, fontWeight = FontWeight.Medium, modifier = Modifier.weight(1f), maxLines = 1)
                }
                QuietButton("${state.name} ▾", tag = "instruments", onClick = { go(Page.INSTRUMENTS) })
                QuietButton("Menu", tag = "menu", onClick = { go(Page.MENU) })
                Button(onClick = { actions.sound(!state.soundOn) }, shape = Shape,
                    colors = ButtonDefaults.buttonColors(containerColor = if (state.soundOn) Surface else Accent,
                        contentColor = if (state.soundOn) Ink else Background),
                    modifier = Modifier.height(48.dp).testTag("audio-toggle")) {
                    HostText(if (state.soundOn) "Mute" else "Sound on", fontSize = 14.sp)
                }
            }
            Box(Modifier.fillMaxWidth().height(1.dp).background(Edge))
            Box(Modifier.weight(1f).fillMaxWidth().padding(vertical = 16.dp)) {
                when (Page.valueOf(page)) {
                    Page.PLAY -> key(state.selected) { Performance(feed, initialBank) }
                    Page.INSTRUMENTS -> Instruments(state) { actions.select(it); go(Page.PLAY) }
                    Page.MENU -> Menu { go(it) }
                    Page.SNAPSHOTS -> Snapshots(state, actions)
                    Page.AUDIO -> Audio(state, actions)
                    Page.CONTROLLERS -> Controllers()
                    Page.ABOUT -> About()
                }
            }
        }
    }
}

@Composable
private fun QuietButton(text: String, tag: String = text, onClick: () -> Unit) {
    TextButton(onClick, modifier = Modifier.heightIn(min = 48.dp).testTag(tag), shape = Shape,
        colors = ButtonDefaults.textButtonColors(contentColor = Ink)) { HostText(text, fontSize = 14.sp, maxLines = 1) }
}

@Composable
private fun Performance(feed: PanelFeed, initialBank: Int) {
    Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(16.dp)) {
        BoxWithConstraints(Modifier.fillMaxWidth().weight(1f)) {
            val wide = maxWidth > maxHeight && maxWidth >= 780.dp
            if (wide) {
                Row(Modifier.fillMaxSize(), horizontalArrangement = Arrangement.spacedBy(28.dp)) {
                    Display(feed, Modifier.weight(1f).fillMaxHeight())
                    ControlBank(feed, Modifier.width(340.dp).fillMaxHeight(), initialBank)
                }
            } else {
                Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(16.dp)) {
                    Display(feed, Modifier.weight(1f).fillMaxWidth())
                    ControlBank(feed, Modifier.weight(1f).fillMaxWidth(), initialBank)
                }
            }
        }
        Piano(feed, Modifier.fillMaxWidth().height(136.dp))
    }
}

@Composable
private fun Display(feed: PanelFeed, modifier: Modifier) {
    Canvas(modifier.testTag("instrument-display")) {
        feed.frames
        val side = min(size.width, size.height)
        val x = (size.width - side) / 2
        val y = (size.height - side) / 2
        drawRoundRect(Color.Black, Offset(x - 8.dp.toPx(), y - 8.dp.toPx()),
            Size(side + 16.dp.toPx(), side + 16.dp.toPx()), CornerRadius(12.dp.toPx()))
        drawImage(feed.image, dstOffset = IntOffset(x.toInt(), y.toInt()),
            dstSize = IntSize(side.toInt().coerceAtLeast(1), side.toInt().coerceAtLeast(1)), filterQuality = FilterQuality.None)
    }
}

@Composable
private fun ControlBank(feed: PanelFeed, modifier: Modifier, initialBank: Int) {
    var bank by rememberSaveable { mutableIntStateOf(initialBank) }
    Column(modifier.testTag("control-bank"), verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Row(Modifier.fillMaxWidth().height(44.dp).background(Surface, Shape), horizontalArrangement = Arrangement.spacedBy(4.dp)) {
            listOf("Sound", "Edit", "Panel").forEachIndexed { index, title ->
                TextButton(onClick = { feed.releaseAll(); bank = index }, shape = Shape,
                    modifier = Modifier.weight(1f).fillMaxHeight().testTag("bank-$index"),
                    colors = ButtonDefaults.textButtonColors(contentColor = if (bank == index) Ink else Quiet,
                        containerColor = if (bank == index) Edge else Color.Transparent)) { HostText(title, fontSize = 13.sp) }
            }
        }
        if (bank < 2) {
            val knobs = if (bank == 0) listOf(PanelKnob.PRESETS, PanelKnob.SELECT, PanelKnob.ALGORITHM, PanelKnob.MASTER)
                        else listOf(PanelKnob.K1, PanelKnob.K2, PanelKnob.K3, PanelKnob.K4)
            Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                knobs.chunked(2).forEach { row ->
                    Row(Modifier.weight(1f).fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                        row.forEach { knob -> Dial(feed, knob, Modifier.weight(1f).fillMaxHeight()) }
                    }
                }
            }
        } else {
            Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                PanelLabel.entries.chunked(3).forEach { row ->
                    Row(Modifier.fillMaxWidth().weight(1f), horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        row.forEach { label -> PanelButton(feed, label, Modifier.weight(1f).fillMaxHeight()) }
                        if (row.size < 3) Spacer(Modifier.weight((3 - row.size).toFloat()))
                    }
                }
            }
        }
    }
}

@Composable
private fun Dial(feed: PanelFeed, knob: PanelKnob, modifier: Modifier) {
    val title = when (knob) {
        PanelKnob.PRESETS -> "Preset"
        PanelKnob.SELECT -> "Select"
        PanelKnob.ALGORITHM -> "Algorithm"
        PanelKnob.MASTER -> "Master"
        else -> "Knob ${knob.ordinal - PanelKnob.K1.ordinal + 1}"
    }
    fun turn(steps: Int) {
        if (knob == PanelKnob.MASTER) feed.master((feed.knobs.master + steps * 8).coerceIn(0, 1023))
        else feed.turn(knob, steps)
    }
    Column(modifier.semantics {
        contentDescription = "$title, drag up or right to increase"
        customActions = listOf(CustomAccessibilityAction("Increase") { turn(1); true }, CustomAccessibilityAction("Decrease") { turn(-1); true })
    }.pointerInput(feed, knob) {
        var remainder = 0f
        detectDragGestures(onDragStart = { remainder = 0f }) { change, drag ->
            change.consume()
            remainder += (drag.x - drag.y) / (density * 6f)
            val steps = remainder.toInt()
            if (steps != 0) { remainder -= steps; turn(steps.coerceIn(-100, 100)) }
        }
    }.testTag("dial-${knob.name}"), horizontalAlignment = Alignment.CenterHorizontally) {
        Canvas(Modifier.weight(1f).fillMaxWidth()) {
            feed.knobs.version
            val radius = min(size.width, size.height) * 0.39f
            val center = Offset(size.width / 2, size.height / 2)
            val theta = if (knob == PanelKnob.MASTER) (135f + 270f * feed.knobs.master / 1023f)
                        else 270f + feed.knobs.turns[knob.ordinal] * 7.5f
            val radians = theta / 180 * PI.toFloat()
            drawCircle(Surface, radius, center)
            drawCircle(Edge, radius, center, style = Stroke(1.dp.toPx()))
            val inner = center + Offset(cos(radians), sin(radians)) * radius * 0.70f
            val outer = center + Offset(cos(radians), sin(radians)) * radius * 0.90f
            drawLine(Accent, inner, outer, 3.dp.toPx(), StrokeCap.Round)
            if (knob == PanelKnob.MASTER) {
                val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Quiet.toArgb(); textSize = 13.sp.toPx(); textAlign = Paint.Align.CENTER }
                drawContext.canvas.nativeCanvas.drawText("${feed.knobs.master * 100 / 1023}%", center.x, center.y + 5.sp.toPx(), paint)
            }
        }
        HostText(title, color = Ink, fontSize = 13.sp, modifier = Modifier.padding(bottom = 4.dp), maxLines = 1)
    }
}

@Composable
private fun PanelButton(feed: PanelFeed, label: PanelLabel, modifier: Modifier) {
    feed.lights.version
    val down = feed.lights.labelDown(label.ordinal)
    DisposableEffect(feed, label) { onDispose { feed.button(label, false) } }
    Box(modifier.background(if (down) Accent else Surface, Shape).border(1.dp, if (down) Accent else Edge, Shape)
        .testTag("panel-${label.name}").semantics { contentDescription = label.spoken }
        .pointerInput(feed, label) {
            detectTapGestures(onPress = { feed.button(label, true); try { tryAwaitRelease() } finally { feed.button(label, false) } })
        }.padding(horizontal = 5.dp), contentAlignment = Alignment.Center) {
        HostText(feed.names.label(label), color = if (down) Background else Ink, fontSize = 12.sp, maxLines = 2)
    }
}

@Composable
private fun Piano(feed: PanelFeed, modifier: Modifier) {
    Canvas(modifier.testTag("keyboard").semantics { contentDescription = "Playable keyboard, F3 to G5" }.pointerInput(feed) {
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
        feed.lights.version
        val pressed = feed.lights.touchedKeys or feed.lights.heldKeys
        val unit = size.width / 16
        val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = Quiet.toArgb(); textSize = 10.sp.toPx(); textAlign = Paint.Align.CENTER }
        for ((index, note) in HostKeyboard.white.withIndex()) {
            drawRoundRect(if (pressed and (1 shl note) != 0) Accent else Ink, Offset(index * unit + 1, 0f), Size(unit - 2, size.height), CornerRadius(4.dp.toPx()))
            if ((note + 53) % 12 == 0) drawContext.canvas.nativeCanvas.drawText("C${(note + 53) / 12 - 1}", (index + 0.5f) * unit, size.height - 10.dp.toPx(), paint)
            if (feed.lights.keys[note] >= 176) drawCircle(Accent, 2.dp.toPx(), Offset((index + 0.5f) * unit, size.height - 27.dp.toPx()))
        }
        for (note in HostKeyboard.black) drawRoundRect(if (pressed and (1 shl note) != 0) Accent else Background,
            Offset(HostKeyboard.blackLeft(note) * unit, 0f), Size(unit * 0.62f, size.height * 0.61f), CornerRadius(3.dp.toPx()))
    }
}

@Composable
private fun Pager(page: Int, count: Int, change: (Int) -> Unit) {
    Row(Modifier.fillMaxWidth().height(48.dp), verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.Center) {
        TextButton({ change(page - 1) }, enabled = page > 0) { HostText("‹ Previous") }
        HostText("${page + 1} / ${max(1, count)}", color = Quiet, fontSize = 12.sp, modifier = Modifier.padding(horizontal = 20.dp))
        TextButton({ change(page + 1) }, enabled = page + 1 < count) { HostText("Next ›") }
    }
}

@Composable
private fun Instruments(state: HostViewState, select: (String) -> Unit) {
    var page by rememberSaveable { mutableIntStateOf(0) }
    val rows = state.instruments.chunked(2)
    val current = rows.getOrElse(page) { emptyList() }
    Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(16.dp)) {
        BoxWithConstraints(Modifier.weight(1f).fillMaxWidth()) {
            val wide = maxWidth > maxHeight
            if (wide) Row(Modifier.fillMaxSize(), horizontalArrangement = Arrangement.spacedBy(20.dp)) {
                current.forEach { InstrumentCard(it, it.id == state.selected, Modifier.weight(1f).fillMaxHeight(), select) }
            } else Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(16.dp)) {
                current.forEach { InstrumentCard(it, it.id == state.selected, Modifier.weight(1f).fillMaxWidth(), select) }
            }
        }
        if (rows.size > 1) Pager(page, rows.size) { page = it }
    }
}

@Composable
private fun InstrumentCard(item: InstrumentItem, selected: Boolean, modifier: Modifier, select: (String) -> Unit) {
    Column(modifier.background(Surface, Shape).border(1.dp, if (selected) Accent else Edge, Shape).padding(24.dp).testTag("instrument-${item.id}"),
        verticalArrangement = Arrangement.SpaceBetween) {
        Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            HostText(if (selected) "Selected" else "Installed", color = Accent, fontSize = 12.sp)
            HostText(item.name, color = Ink, fontSize = 30.sp, fontWeight = FontWeight.Medium)
            HostText(if (item.id == "paleotone") "Evolving landscapes & synthesis" else "Multi-engine synthesis & sequencing", color = Quiet, fontSize = 14.sp, maxLines = 2)
        }
        Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
            HostText("${item.version} · ${item.license}", color = Quiet, fontSize = 12.sp, maxLines = 2)
            Button({ select(item.id) }, modifier = Modifier.heightIn(min = 48.dp), shape = Shape) { HostText(if (selected) "Open instrument" else "Play") }
        }
    }
}

@Composable
private fun Menu(go: (Page) -> Unit) {
    Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(16.dp)) {
        listOf(listOf(Page.SNAPSHOTS to "Saved snapshots", Page.AUDIO to "Audio output"),
            listOf(Page.CONTROLLERS to "Controllers", Page.ABOUT to "About & credits")).forEach { row ->
            Row(Modifier.fillMaxWidth().weight(1f), horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                row.forEach { (page, title) ->
                    Box(Modifier.weight(1f).fillMaxHeight().background(Surface, Shape).clickable { go(page) }
                        .padding(24.dp).testTag("menu-${page.name}"), contentAlignment = Alignment.CenterStart) {
                        HostText(title, color = Ink, fontSize = 22.sp, fontWeight = FontWeight.Medium)
                    }
                }
            }
        }
    }
}

@Composable
private fun Snapshots(state: HostViewState, actions: HostActions) {
    var page by rememberSaveable { mutableIntStateOf(0) }
    val groups = state.snapshots.chunked(3)
    Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            HostText(state.name, color = Quiet, modifier = Modifier.weight(1f), fontSize = 16.sp)
            Button(actions.save, shape = Shape, modifier = Modifier.heightIn(min = 48.dp)) { HostText("Save snapshot") }
        }
        Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(10.dp)) {
            if (state.snapshots.isEmpty()) HostText("No snapshots yet. Instrument memory saves automatically.", color = Quiet, fontSize = 15.sp)
            groups.getOrElse(page) { emptyList() }.forEach { item ->
                Row(Modifier.fillMaxWidth().height(64.dp).background(Surface, Shape).padding(horizontal = 18.dp),
                    verticalAlignment = Alignment.CenterVertically) {
                    HostText(item.name, color = Ink, fontSize = 15.sp, modifier = Modifier.weight(1f), maxLines = 1, overflow = TextOverflow.Ellipsis)
                    QuietButton("Load", tag = "load-${item.id}") { actions.load(item.id) }
                }
            }
            if (state.message.isNotEmpty()) HostText(state.message, color = Quiet, fontSize = 13.sp, maxLines = 2, overflow = TextOverflow.Ellipsis)
        }
        if (groups.size > 1) Pager(page, groups.size) { page = it }
    }
}

@Composable
private fun Audio(state: HostViewState, actions: HostActions) {
    var page by rememberSaveable { mutableIntStateOf(0) }
    val groups = state.outputs.chunked(4)
    Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(20.dp)) {
        HostText(state.audioLine, color = Quiet, fontSize = 16.sp, maxLines = 3)
        Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(12.dp)) {
            groups.getOrElse(page) { emptyList() }.chunked(2).forEach { row ->
                Row(Modifier.weight(1f).fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                    row.forEach { item ->
                        Box(Modifier.weight(1f).fillMaxHeight().background(Surface, Shape).border(1.dp, if (item.id == state.route) Accent else Edge, Shape)
                            .clickable { actions.output(item.id) }.padding(20.dp).testTag("output-${item.id}"), contentAlignment = Alignment.CenterStart) {
                            HostText(item.name, color = if (item.id == state.route) Accent else Ink, fontSize = 18.sp, maxLines = 2)
                        }
                    }
                }
            }
        }
        if (groups.size > 1) Pager(page, groups.size) { page = it }
    }
}

@Composable
private fun Controllers() {
    Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(20.dp)) {
        HostText("Touchscreen active. Hardware bridges are in development.", color = Quiet, fontSize = 15.sp)
        listOf("FM-1", "SMK-37").forEach { name ->
            Row(Modifier.fillMaxWidth().weight(1f).background(Surface, Shape).padding(24.dp), verticalAlignment = Alignment.CenterVertically) {
                HostText(name, color = Ink, fontSize = 24.sp, modifier = Modifier.weight(1f))
                HostText("Not connected", color = Quiet, fontSize = 14.sp)
            }
        }
    }
}

@Composable
private fun About() {
    Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(24.dp)) {
        HostText("Instrument Host", color = Ink, fontSize = 30.sp, fontWeight = FontWeight.Medium)
        HostText("DIY more powerful FM-1 · Development build 0.1.3", color = Quiet, fontSize = 14.sp)
        HostText("Interface by the Paleotone project. Runtime, audio and storage reuse RemoteFM1Android and FM1VST, with attribution and source retained.", color = Quiet, fontSize = 15.sp)
        HostText("Instruments: Paleotone and Felucca. GPL-3.0 notices and source references accompany the app.", color = Quiet, fontSize = 15.sp)
    }
}

// Layout tests observe real text measurement; production has no audit callback.
val LocalHostTextAudit = staticCompositionLocalOf<(String) -> Unit> { {} }
@Composable
private fun HostText(text: String, modifier: Modifier = Modifier, color: Color = Color.Unspecified,
                     fontSize: TextUnit = 14.sp, fontWeight: FontWeight? = null,
                     maxLines: Int = Int.MAX_VALUE, overflow: TextOverflow = TextOverflow.Clip) {
    val audit = LocalHostTextAudit.current
    androidx.compose.material3.Text(text, modifier = modifier, color = color, fontSize = fontSize,
        fontWeight = fontWeight, maxLines = maxLines, overflow = overflow, onTextLayout = {
            if (it.hasVisualOverflow && overflow != TextOverflow.Ellipsis) audit(text)
        })
}
