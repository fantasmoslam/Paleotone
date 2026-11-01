/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../firmware/src/editor_remote_queue.h"
int main(void)
{
    ed_remote_queue_t q = {0};
    uint8_t frame[8] = {0x7D, 0x46, 0x4C, 93, 1, 0, 9, 0};
    assert(edrq_offer(&q, frame, sizeof frame, 7));
    /* The USB/OTA receiver is allowed to release and overwrite its buffer. */
    memset(frame, 0, sizeof frame);
    for (uint32_t used = 64; used > 4; used--) {
        assert(edrq_ready(&q, used, 7, 1) == 0);
        assert(q.len == 8); /* the old dispatch guard consumed the request here */
    }
    assert(edrq_ready(&q, 4, 7, 1) == 1);
    assert(q.frame[3] == 93 && q.frame[6] == 9);
    uint8_t encoder[7] = {0x7D, 0x46, 0x4C, 94, 1, 3, 65};
    assert(!edrq_offer(&q, encoder, sizeof encoder, 7));
    q.len = 0; /* dispatched exactly once by ed_service */
    assert(edrq_offer(&q, encoder, sizeof encoder, 7));
    assert(edrq_ready(&q, 0, 8, 1) == -1 && !q.len); /* USB reset: no delayed knob turn */
    assert(edrq_offer(&q, encoder, sizeof encoder, 8));
    assert(edrq_ready(&q, 0, 8, 0) == -1 && !q.len); /* disconnect */
    assert(!edrq_offer(&q, encoder, 3, 8));
    assert(!edrq_offer(&q, encoder, 9, 8));
    for (uint32_t request = 0; request < 512; request++) {
        uint8_t msg[5] = {0x7D, 0x46, 0x4C, 92, 1};
        assert(edrq_offer(&q, msg, sizeof msg, 8));
        assert(edrq_ready(&q, 32, 8, 1) == 0);
        assert(edrq_ready(&q, 0, 8, 1) == 1);
        q.len = 0;
        assert(edrq_ready(&q, 0, 8, 1) == 0);
    }
    uint8_t focus[5] = {0x7D, 0x46, 0x4C, 96, 1};
    assert(edrq_offer(&q, focus, sizeof focus, 8));
    assert(edrq_ready(&q, 32, 8, 1) == 0);
    assert(edrq_ready(&q, 0, 8, 1) == 1 && q.frame[3] == 96);
    q.len = 0;
    focus[3] = 95; assert(!edrq_offer(&q, focus, sizeof focus, 8));
    uint8_t burst[11]={0x7D,0x46,0x4C,95,1,0,4,0,1,2,3};
    assert(edrq_offer(&q,burst,sizeof burst,8));assert(q.len==11);q.len=0;
    puts("remote busy-queue and request ownership tests passed");
}
