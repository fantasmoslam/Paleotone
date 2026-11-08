// SPDX-License-Identifier: GPL-3.0-only
package com.quixotic7.remotefm1
import org.junit.Assert.*
import org.junit.Test
class SmkKeysTest {
 @Test fun all37KeysHaveReachableCenters() {
  assertEquals(37,SmkKeys.keys.size);assertEquals(22,SmkKeys.keys.count{!it.black});assertEquals(15,SmkKeys.keys.count{it.black})
  for(k in SmkKeys.keys) assertEquals(k.index,SmkKeys.hit(k.x+k.w/2,if(k.black)0.2f else 0.8f))
 }
 @Test fun blackKeysWinAtTheOverlapAndOutsideIsSilent() {
  assertEquals(1,SmkKeys.hit(1f/22f,0.2f));assertEquals(0,SmkKeys.hit(0.9f/22f,0.8f))
  assertNull(SmkKeys.hit(-0.01f,0.5f));assertNull(SmkKeys.hit(1f,0.5f));assertNull(SmkKeys.hit(0.5f,1f))
 }
 @Test fun sixteenPadsUseTheSameWhiteKeysAsTheBridge() {
  assertEquals(16,SmkKeys.pads.size);assertEquals(16,SmkKeys.pads.distinct().size)
  for(k in SmkKeys.pads) assertFalse(SmkKeys.keys.first{it.index==k+5}.black)
 }
}
