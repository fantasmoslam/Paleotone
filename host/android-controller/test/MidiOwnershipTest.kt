// SPDX-License-Identifier: GPL-3.0-only
package com.quixotic7.remotefm1

import org.junit.Assert.*
import org.junit.Test

class MidiOwnershipTest {
    private class Ports {
        val calls = mutableListOf<String>()
        val pending = ArrayDeque<() -> Unit>()
        var bridgeOwns = false
        var controllerOwns = false
        val routing = MidiOwnership(
            bridge = { enabled, done ->
                calls += "bridge:$enabled"
                pending.addLast {
                    bridgeOwns = enabled
                    assertFalse("both backends claimed the device", bridgeOwns && controllerOwns)
                    done()
                }
            },
            controllers = { reserved, done ->
                calls += "reserved:$reserved"
                pending.addLast {
                    controllerOwns = !reserved
                    assertFalse("both backends claimed the device", bridgeOwns && controllerOwns)
                    done()
                }
            },
        )
        fun finish() { pending.removeFirst().invoke() }
        fun drain() { while (pending.isNotEmpty()) finish() }
    }

    @Test fun tabletStartupReleasesBridgeBeforeOpeningController() {
        val p = Ports()
        p.routing.useBridge(false)
        assertEquals(listOf("bridge:false"), p.calls)
        p.finish()
        assertEquals(listOf("bridge:false", "reserved:false"), p.calls)
        p.drain()
        assertTrue(p.controllerOwns)
        assertFalse(p.bridgeOwns)
    }

    @Test fun bridgeStartupReservesControllerBeforeOpeningBridge() {
        val p = Ports()
        p.routing.useBridge(true)
        assertEquals(listOf("reserved:true"), p.calls)
        p.drain()
        assertTrue(p.bridgeOwns)
        assertFalse(p.controllerOwns)
    }

    @Test fun transfersInBothDirectionsNeverOverlap() {
        val p = Ports()
        p.routing.useBridge(false); p.drain()
        p.routing.useBridge(true); p.drain()
        p.routing.useBridge(false); p.drain()
        assertEquals(listOf("bridge:false", "reserved:false", "reserved:true", "bridge:true",
            "bridge:false", "reserved:false"), p.calls)
        assertTrue(p.controllerOwns)
    }

    @Test fun rapidModeChangesWaitForTheCurrentHandoff() {
        val p = Ports()
        p.routing.useBridge(true)
        p.routing.useBridge(false)
        assertEquals(listOf("reserved:true"), p.calls)
        p.finish()
        assertEquals(listOf("reserved:true", "bridge:true"), p.calls)
        p.drain()
        assertEquals(listOf("reserved:true", "bridge:true", "bridge:false", "reserved:false"), p.calls)
        assertTrue(p.controllerOwns)
    }

    @Test fun repeatedRequestsDoNotReopenPortsAndLatestRequestWins() {
        val p = Ports()
        p.routing.useBridge(false); p.routing.useBridge(true); p.routing.useBridge(false)
        p.drain()
        p.routing.useBridge(false)
        assertEquals(listOf("bridge:false", "reserved:false"), p.calls)
        assertTrue(p.controllerOwns)
    }
}
