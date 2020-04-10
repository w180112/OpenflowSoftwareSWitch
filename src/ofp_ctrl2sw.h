#ifndef _OFP_CTRL2SW_H_
#define _OFP_CTRL2SW_H_

#include "ofp_common.h"

typedef struct ofp_multipart { 
	struct ofp_header ofp_header; 
	uint16_t type;
	uint16_t flags;
	uint8_t pad[4]; 
	uint8_t body[0];
}ofp_multipart_t;
OFP_ASSERT(sizeof(struct ofp_multipart) == 16);

/* Send packet (controller -> datapath). */ 
typedef struct ofp_packet_out {
    struct ofp_header header; 
    uint32_t buffer_id; /* ID assigned by datapath (OFP_NO_BUFFER if none). */
    uint32_t in_port; /* Packet's input port or OFPP_CONTROLLER. */
    uint16_t actions_len; /* Size of action array in bytes. */
    uint8_t pad[6];
    struct ofp_action_header actions[0]; /* Action list. */
    /* uint8_t data[0]; */ /* Packet data. The length is inferred
                                from the length field in the header. 
                                (Only meaningful if buffer_id == -1.) */
}ofp_packet_out_t;
OFP_ASSERT(sizeof(struct ofp_packet_out) == 24);

/* Switch features. */ 
typedef struct ofp_switch_features {
    struct ofp_header ofp_header;
    uint64_t datapath_id; /* Datapath unique ID. The lower 48-bits are for a MAC 
        address, while the upper 16-bits are implementer-defined. */
    uint32_t n_buffers; /* Max packets buffered at once. */
    uint8_t n_tables; /* Number of tables supported by datapath. */
    uint8_t auxiliary_id; /* Identify auxiliary connections */
    uint8_t pad[2]; /* Align to 64-bits. */
    /* Features. */
    uint32_t capabilities; /* Bitmap of support "ofp_capabilities". */
    uint32_t reserved;
}ofp_switch_features_t;
OFP_ASSERT(sizeof(struct ofp_switch_features) == 32);

/* Flow setup and teardown (controller -> datapath). */ 
typedef struct ofp_flow_mod {
    struct ofp_header header; 
    uint64_t cookie; /* Opaque controller-issued identifier. */
    uint64_t cookie_mask; /* Mask used to restrict the cookie bits
                            that must match when the command is 
                            OFPFC_MODIFY* or OFPFC_DELETE*. A value 
                            of 0 indicates no restriction. */
    /* Flow actions. */ 
    uint8_t table_id; /* ID of the table to put the flow in. 
                        For OFPFC_DELETE_* commands, OFPTT_ALL 
                        can also be used to delete matching 
                        flows from all tables. */
    uint8_t command; /* One of OFPFC_*. */
    uint16_t idle_timeout; /* Idle time before discarding (seconds). */
    uint16_t hard_timeout; /* Max time before discarding (seconds). */
    uint16_t priority; /* Priority level of flow entry. */
    uint32_t buffer_id; /* Buffered packet to apply to, or
                            OFP_NO_BUFFER.
                            Not meaningful for OFPFC_DELETE*. */
    uint32_t out_port; /* For OFPFC_DELETE* commands, require
                            matching entries to include this as an 
                            output port. A value of OFPP_ANY 
                            indicates no restriction. */
    uint32_t out_group; /* For OFPFC_DELETE* commands, require 
                            matching entries to include this as an 
                            output group. A value of OFPG_ANY 
                            indicates no restriction. */
    uint16_t flags; /* One of OFPFF_*. */
    uint8_t pad[2];
    struct ofp_match match; /* Fields to match. Variable size. */
    //struct ofp_instruction instructions[0]; /* Instruction set */
}ofp_flow_mod_t;
OFP_ASSERT(sizeof(struct ofp_flow_mod) == 56);

enum ofp_flow_mod_command { 
    OFPFC_ADD = 0, /* New flow. */
    OFPFC_MODIFY = 1, /* Modify all matching flows. */
    OFPFC_MODIFY_STRICT = 2, /* Modify entry strictly matching wildcards and priority. */
    OFPFC_DELETE = 3, /* Delete all matching flows. */
    OFPFC_DELETE_STRICT = 4, /* Delete entry strictly matching wildcards and priority. */
};

enum ofp_flow_mod_flags {
    OFPFF_SEND_FLOW_REM = 1 << 0, /* Send flow removed message when flow * expires or is deleted. */
    OFPFF_CHECK_OVERLAP = 1 << 1, /* Check for overlapping entries first. */
    OFPFF_RESET_COUNTS = 1 << 2, /* Reset flow packet and byte counts. */
    OFPFF_NO_PKT_COUNTS = 1 << 3, /* Don’t keep track of packet count. */
    OFPFF_NO_BYT_COUNTS = 1 << 4, /* Don’t keep track of byte count. */
};
#endif