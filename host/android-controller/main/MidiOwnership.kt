// SPDX-License-Identifier: GPL-3.0-only
package com.quixotic7.remotefm1

/** Serializes port ownership transfers. All requests and completions run on the main thread. */
internal class MidiOwnership(
    private val bridge: (Boolean, () -> Unit) -> Unit,
    private val controllers: (Boolean, () -> Unit) -> Unit,
) {
    private var desired = false
    private var current: Boolean? = null
    private var changing = false

    fun useBridge(enabled: Boolean) {
        desired = enabled
        advance()
    }

    private fun advance() {
        if (changing || current == desired) return
        changing = true
        val target = desired
        val complete = {
            current = target
            changing = false
            advance()
        }
        if (target) {
            controllers(true) { bridge(true, complete) }
        } else {
            bridge(false) { controllers(false, complete) }
        }
    }
}
