// SPDX-License-Identifier: GPL-3.0-only
package com.quixotic7.remotefm1

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.quixotic7.remotefm1.ui.AppUi
import com.quixotic7.remotefm1.ui.faceplate.*
import com.quixotic7.remotefm1.ui.theme.Theme
import com.quixotic7.remotefm1.ui.theme.color
import kotlinx.coroutines.isActive
import kotlin.math.min

private fun Modifier.held(key: Any, action: (Boolean)->Unit) = pointerInput(key) {
    awaitEachGesture {
        val down=awaitFirstDown(requireUnconsumed=false);down.consume();action(true)
        try { do { val event=awaitPointerEvent();event.changes.forEach{it.consume()} } while(event.changes.any{it.id==down.id && it.pressed}) }
        finally {action(false)}
    }
}

@Composable private fun SmkText(text:String, modifier:Modifier=Modifier, size:Int=12) {
    BasicText(text,modifier,style=TextStyle(color=Color(0xffe4e2e8),fontSize=size.sp,textAlign=TextAlign.Center),maxLines=2)
}
@Composable private fun SmkButton(text:String,modifier:Modifier=Modifier,onClick:()->Unit) {
    Box(modifier.height(42.dp).background(Color(0xff303239),RoundedCornerShape(6.dp)).clickable(onClick=onClick),contentAlignment=Alignment.Center) {SmkText(text)}
}

/** Only the controller panel changes; the active instrument keeps its original screen and behavior. */
@Composable fun SmkFaceplate(a:AppUi,theme:Theme) {
    val feed=a.feed;val engine=a.model.engine
    val snapshot=remember {IntArray(12)}
    var values by remember {mutableStateOf(List(12){0})}
    val outerTouch=remember {mutableMapOf<Int,Int>()}
    fun midi(st:Int,d1:Int,d2:Int) {engine.midiIn(byteArrayOf(st.toByte(),d1.toByte(),d2.toByte()),toFirmware=true)}
    fun key(k:Int,on:Boolean) {
        if(k in 5..31) {if(on)feed.keyDown(k-5) else feed.keyUp(k-5)}
        else {
            val count=outerTouch[k]?:0
            val next=(count+if(on)1 else -1).coerceAtLeast(0);outerTouch[k]=next
            if(count==0 && next>0)midi(0x90,48+k,100)
            if(count>0 && next==0)midi(0x80,48+k,0)
        }
    }
    fun bank(i:Int,on:Boolean) {engine.event(0x81,4+i,if(on)127 else 0)}
    LaunchedEffect(feed,engine) {
        try {while(isActive) withFrameNanos {
            feed.poll();engine.smkPanel(snapshot);val next=snapshot.toList();if(next!=values)values=next
        }} finally {
            feed.releaseAll()
            outerTouch.filterValues{it>0}.keys.toList().forEach{midi(0x80,48+it,0)};outerTouch.clear()
            midi(0xB0,64,0);midi(0xE0,0,64)
        }
    }
    val linked=a.model.state.isConnected && (a.model.hello?.caps?.and(1 shl 11)?:0)!=0
    Column(Modifier.fillMaxSize().background(theme.body.color).padding(12.dp),verticalArrangement=Arrangement.spacedBy(8.dp)) {
        Row(Modifier.fillMaxWidth(),verticalAlignment=Alignment.CenterVertically,horizontalArrangement=Arrangement.spacedBy(8.dp)) {
            SmkText(a.current?.name?:("Instrument Host"),Modifier.weight(1f),18)
            SmkText(if(linked)"SMK · linked" else "SMK · MIDI notes",Modifier.weight(1f))
            SmkButton(if(a.ui.displayAlone)"Controls" else "Screen",Modifier.width(86.dp)){a.ui.displayAlone=!a.ui.displayAlone}
            SmkButton("Settings",Modifier.width(86.dp)){a.openSheet()}
        }
        if(a.ui.displayAlone) {
            BoxWithConstraints(Modifier.weight(1f).fillMaxWidth(),contentAlignment=Alignment.Center){DisplayView(feed,theme,min(maxWidth.value,maxHeight.value))}
        } else {
            Row(Modifier.fillMaxWidth().weight(1.35f),horizontalArrangement=Arrangement.spacedBy(10.dp)) {
                BoxWithConstraints(Modifier.weight(0.36f).fillMaxHeight(),contentAlignment=Alignment.Center) {
                    DisplayView(feed,theme,min(maxWidth.value,maxHeight.value))
                }
                Column(Modifier.weight(0.43f).fillMaxHeight(),verticalArrangement=Arrangement.spacedBy(6.dp)) {
                    SmkText("PADS · ${if(linked) "host lights" else "bridge firmware enables feedback"}",Modifier.fillMaxWidth(),11)
                    repeat(2){row-> Row(Modifier.fillMaxWidth().weight(1f),horizontalArrangement=Arrangement.spacedBy(5.dp)) {
                        repeat(8){col->val p=row*8+col;val k=SmkKeys.pads[p];val _version=feed.lights.version
                            Box(Modifier.weight(1f).fillMaxHeight().background(if(feed.lights.keyDown(k)||LedLevel.lit(feed.lights.keys[k]))Color(0xffbc9ee9) else Color(0xff303239),RoundedCornerShape(5.dp)).held(p){on->if(on)feed.keyDown(k) else feed.keyUp(k)},contentAlignment=Alignment.Center){SmkText("${p+1}")}
                        }
                    }}
                    Row(Modifier.fillMaxWidth(),horizontalArrangement=Arrangement.spacedBy(6.dp)) {
                        Box(Modifier.weight(1f).height(42.dp).background(Color(0xff303239),RoundedCornerShape(5.dp)).held("knob-bank"){bank(0,it)},contentAlignment=Alignment.Center){SmkText(if(values[4]!=0)"KNOB BANK · fine" else "KNOB BANK · normal",size=10)}
                        Box(Modifier.weight(1f).height(42.dp).background(Color(0xff303239),RoundedCornerShape(5.dp)).held("fader-bank"){bank(1,it)},contentAlignment=Alignment.Center){SmkText(if(values[5]!=0)"FADER BANK · main" else "FADER BANK · KN1–4",size=10)}
                    }
                }
                Column(Modifier.weight(0.21f).fillMaxHeight()) {
                    SmkText("FADERS · relative takeover",Modifier.fillMaxWidth(),10)
                    Row(Modifier.weight(1f),horizontalArrangement=Arrangement.spacedBy(6.dp)) {
                        repeat(4){i->SmkFader(values[i],Modifier.weight(1f).fillMaxHeight()){v->engine.event(0x81,i,v)}}
                    }
                }
            }
            Row(Modifier.fillMaxWidth().heightIn(min=64.dp,max=88.dp),horizontalArrangement=Arrangement.SpaceEvenly) {
                for(k in listOf(PanelKnob.MASTER,PanelKnob.SELECT,PanelKnob.ALGORITHM,PanelKnob.PRESETS,PanelKnob.K1,PanelKnob.K2,PanelKnob.K3,PanelKnob.K4)) KnobView(k,44f,theme,feed)
            }
            Row(Modifier.fillMaxWidth(),horizontalArrangement=Arrangement.spacedBy(8.dp)) {
                Column(Modifier.weight(1f),verticalArrangement=Arrangement.spacedBy(5.dp)) {
                    repeat(2){row->Row(Modifier.fillMaxWidth(),horizontalArrangement=Arrangement.spacedBy(5.dp)) {
                        repeat(6){col->val label=PanelLabel.GRID[row*6+col];val _v=feed.lights.version;val physical=listOf("ARP","NOTE REPEAT","SCALE","CHORD","GLOBE","BT","PATCH","PARA","FX","SEQ","SEQ PLAY","SEQ REC")[row*6+col]
                            Box(Modifier.weight(1f).height(40.dp).background(if(LedLevel.lit(feed.lights.buttons[label.ordinal]))Color(0xff665579) else Color(0xff303239),RoundedCornerShape(5.dp)).held(label){feed.button(label,it)},contentAlignment=Alignment.Center){SmkText(physical+"\n"+feed.names.labels[label.ordinal],size=9)}
                        }
                    }}
                }
                Column(Modifier.width(72.dp),verticalArrangement=Arrangement.spacedBy(5.dp)) {
                    for(l in listOf(PanelLabel.OCT_DOWN,PanelLabel.OCT_UP)) Box(Modifier.fillMaxWidth().height(40.dp).background(Color(0xff303239),RoundedCornerShape(5.dp)).held(l){feed.button(l,it)},contentAlignment=Alignment.Center){SmkText(l.legend)}
                }
            }
            Row(Modifier.fillMaxWidth().weight(1f),horizontalArrangement=Arrangement.spacedBy(8.dp)) {
                Column(Modifier.width(58.dp),verticalArrangement=Arrangement.spacedBy(4.dp)) {
                    SmkText("PITCH",size=9)
                    SmkFader(values[8]*127/16383,Modifier.weight(1f),spring=true){midi(0xE0,(it*16383/127)and 127,(it*16383/127)shr 7)}
                    SmkText("MOD",size=9)
                    SmkFader(values[9],Modifier.weight(1f)){midi(0xB0,1,it)}
                    Box(Modifier.fillMaxWidth().height(34.dp).background(Color(0xff303239)).held("pedal"){midi(0xB0,64,if(it)127 else 0)},contentAlignment=Alignment.Center){SmkText("PEDAL",size=9)}
                }
                SmkKeyboard(feed,values,Modifier.weight(1f).fillMaxHeight(),::key)
            }
        }
    }
}

@Composable private fun SmkFader(value:Int,modifier:Modifier=Modifier,spring:Boolean=false,send:(Int)->Unit) {
    var local by remember {mutableIntStateOf(-1)}
    Canvas(modifier.fillMaxWidth().pointerInput(spring) {
        awaitEachGesture {
            val d=awaitFirstDown(requireUnconsumed=false)
            fun move(y:Float){local=((1-y/size.height)*127).toInt().coerceIn(0,127);send(local)}
            move(d.position.y);d.consume()
            try {do{val e=awaitPointerEvent();val p=e.changes.firstOrNull{it.id==d.id};if(p?.pressed==true){move(p.position.y);p.consume()}}while(p?.pressed==true)}
            finally {if(spring)send(64);local=-1}
        }
    }) {
        drawRect(Color(0xff25272d));val v=if(local>=0)local else value.coerceIn(0,127)
        drawLine(Color(0xff555760),Offset(size.width/2,10f),Offset(size.width/2,size.height-10f),4f)
        val y=10+(size.height-20)*(1-v/127f)
        drawRoundRect(Color(0xffbc9ee9),Offset(2f,y-6),Size((size.width-4).coerceAtLeast(1f),12f))
    }
}

@Composable private fun SmkKeyboard(feed:PanelFeed,values:List<Int>,modifier:Modifier,send:(Int,Boolean)->Unit) {
    val version=feed.lights.version
    Canvas(modifier.pointerInput(Unit) {
        val held=mutableMapOf<Long,Int>()
        try {while(true){awaitPointerEventScope {
            val event=awaitPointerEvent();val next=event.changes.filter{it.pressed}.mapNotNull{p->SmkKeys.hit(p.position.x/size.width,p.position.y/size.height)?.let{p.id.value to it}}.toMap()
            for(k in held.values.toSet()-next.values.toSet())send(k,false)
            for(k in next.values.toSet()-held.values.toSet())send(k,true)
            held.clear();held.putAll(next);event.changes.forEach{it.consume()}
        }}} finally {held.values.toSet().forEach{send(it,false)}}
    }) {
        val _version=version
        val outer=(values[6].toLong() and 0xffffffffL) or (values[7].toLong() shl 32)
        for(k in SmkKeys.keys.sortedBy{it.black}) {
            val down=if(k.index in 5..31)feed.lights.keyDown(k.index-5) else (outer shr k.index)and 1L!=0L
            val color=if(down)Color(0xffbc9ee9) else if(k.black)Color(0xff202228) else Color(0xffdfdfe4)
            drawRect(color,Offset(k.x*size.width+1,0f),Size(k.w*size.width-2,k.h*size.height-1))
            if(k.index in 5..31 && feed.lights.keys[k.index-5]>0)drawRect(Color(0xff72caa0),Offset((k.x+k.w*.25f)*size.width,8f),Size(k.w*size.width*.5f,5f))
        }
    }
}
