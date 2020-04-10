#ifndef _OFP_ASYN_H_
#define _OFP_ASYN_H_

#include "ofp_common.h"

typedef struct ofp_packet_in {
    struct ofp_header header;
    uint32_t buffer_id; /* ID assigned by datapath. */
    uint16_t total_len; /* Full length of frame. */
    uint8_t reason; /* Reason packet is being sent (one of OFPR_*) */
    uint8_t table_id; /* ID of the table that was looked up */
    uint64_t cookie; /* Cookie of the flow entry that was looked up. */
    struct ofp_match match; /* Packet metadata. Variable size. */
        /* Followed by:
         * - Exactly 2 all-zero padding bytes, then
         * - An Ethernet frame whose length is inferred from header.length. 
         * The padding bytes preceding the Ethernet frame ensure that the IP
         * header (if any) following the Ethernet header is 32-bit aligned. */
    //uint8_t pad[2]; /* Align to 64 bit + 16 bit */
    //uint8_t data[0]; /* Ethernet frame */ 
}ofp_packet_in_t;
OFP_ASSERT(sizeof(struct ofp_packet_in) == 32);

#endif