#ifndef _OFP_COMMON_H_
#define _OFP_COMMON_H_

#include <stdint.h>
#include "ofp_oxm.h"

#define OFP_ETH_ALEN 6
#define OFP_MAX_PORT_NAME_LEN 16
#define OFP_ASSERT(EXPR)                                                \
        extern int (*build_assert(void))[ sizeof(struct {               \
                    unsigned int build_assert_failed : (EXPR) ? 1 : -1; })]
enum ofp_version {
    OFP10_VERSION = 0x01,
    OFP11_VERSION = 0x02,
    OFP12_VERSION = 0x03,
    OFP13_VERSION = 0x04,
    OFP14_VERSION = 0x05,
    OFP15_VERSION = 0x06
};

typedef struct ofp_header {
	uint8_t version; /* OFP_VERSION. */
	uint8_t type; /* One of the OFPT_ constants. */
	uint16_t length; /* Length including this ofp_header. */
	uint32_t xid; /* Transaction id associated with this packet.
	            Replies use the same id as was in the request to facilitate pairing. */
}ofp_header_t;
OFP_ASSERT(sizeof(struct ofp_header) == 8);

struct ofp_port {
    uint32_t port_no;
    uint8_t pad[4];
    uint8_t hw_addr[OFP_ETH_ALEN]; 
    uint8_t pad2[2]; /* Align to 64 bits. */
    char name[OFP_MAX_PORT_NAME_LEN]; /* Null-terminated */
    uint32_t config; /* Bitmap of OFPPC_* flags. */
    uint32_t state; /* Bitmap of OFPPS_* flags. */
    /* Bitmaps of OFPPF_* that describe features. All bits zeroed if * unsupported or unavailable. */
    uint32_t curr; /* Current features. */
    uint32_t advertised; /* Features being advertised by the port. */
    uint32_t supported; /* Features supported by the port. */
    uint32_t peer; /* Features advertised by peer. */
    uint32_t curr_speed; /* Current port bitrate in kbps. */
    uint32_t max_speed; /* Max port bitrate in kbps */
};
OFP_ASSERT(sizeof(struct ofp_port) == 64);

enum ofp_multipart_types {
    OFPMP_DESC = 0,
    OFPMP_FLOW = 1,
    OFPMP_AGGREGATE = 2,
    OFPMP_TABLE = 3,
    OFPMP_PORT_STATS = 4,
    OFPMP_QUEUE = 5,
    OFPMP_GROUP = 6,
    OFPMP_GROUP_DESC = 7,
    OFPMP_GROUP_FEATURES = 8,
    OFPMP_METER = 9,
    OFPMP_METER_CONFIG = 10,
    OFPMP_METER_FEATURES = 11,
    OFPMP_TABLE_FEATURES = 12,
    OFPMP_PORT_DESC = 13,
    OFPMP_EXPERIMENTER = 0xffff 
};

/* Capabilities supported by the datapath. */ 
enum ofp_capabilities {
    OFPC_FLOW_STATS = 1 << 0, /* Flow statistics. */
    OFPC_TABLE_STATS = 1 << 1, /* Table statistics. */
    OFPC_PORT_STATS  = 1 << 2, /* Port statistics. */
    OFPC_GROUP_STATS  = 1 << 3, /* Group statistics. */
    OFPC_IP_REASM  = 1 << 5, /* Can reassemble IP fragments. */
    OFPC_QUEUE_STATS  = 1 << 6, /* Queue statistics. */
    OFPC_PORT_BLOCKED = 1 << 8, /* Switch will block looping ports. */
};

enum ofp_packet_in_reason {
    OFPR_NO_MATCH = 0, /* No matching flow (table-miss flow entry). */
    OFPR_ACTION = 1, /* Action explicitly output to controller. */
    OFPR_INVALID_TTL = 2, /* Packet has invalid TTL */
};

/* Fields to match against flows */ 
struct ofp_match {
    uint16_t type; /* One of OFPMT_* */
    uint16_t length; /* Length of ofp_match (excluding padding) */ 
    /* Followed by:
     * - Exactly (length - 4) (possibly 0) bytes containing OXM TLVs, then 
     * - Exactly ((length + 7)/8*8 - length) (between 0 and 7) bytes of all-zero bytes
     * In summary, ofp_match is padded as needed, to make its overall size 
     * a multiple of 8, to preserve alignement in structures using it.
     */
    ofp_oxm_header_t oxm_header; /* OXMs start here - Make compiler happy */ 
};
OFP_ASSERT(sizeof(struct ofp_match) == 8);

/* Action structure for OFPAT_SET_FIELD. */
typedef struct ofp_action_set_field {
    uint16_t type; /* OFPAT_SET_FIELD. */
    uint16_t len; /* Length of action, including this
                    header. This is the length of action, 
                    including any padding to make it 64-bit aligned. */
    uint8_t pad[4]; /* OXM TLV - Make compiler happy */
}ofp_action_set_field_t;
OFP_ASSERT(sizeof(struct ofp_action_set_field) == 8);

/* Action structure for OFPAT_*. */
typedef struct ofp_action_header {
    uint16_t type; /* OFPAT_*. */
    uint16_t len; /* Length of action, including this
                    header. This is the length of action, 
                    including any padding to make it 64-bit aligned. */
    uint8_t pad[4];
}ofp_action_header_t;
OFP_ASSERT(sizeof(struct ofp_action_header) == 8);

enum ofp_action_type { 
    OFPAT_OUTPUT = 0,/* Output to switch port. */
    OFPAT_COPY_TTL_OUT = 11, /* Copy TTL "outwards" -- from next-to-outermost to outermost */
    OFPAT_COPY_TTL_IN = 12, /* Copy TTL "inwards" -- from outermost to next-to-outermost */
    OFPAT_SET_MPLS_TTL = 15, /* MPLS TTL */
    OFPAT_DEC_MPLS_TTL = 16, /* Decrement MPLS TTL */
    OFPAT_PUSH_VLAN = 17,/* Push a new VLAN tag */
    OFPAT_POP_VLAN = 18,/* Pop the outer VLAN tag */
    OFPAT_PUSH_MPLS = 19,/* Push a new MPLS tag */
    OFPAT_POP_MPLS = 20,/* Pop the outer MPLS tag */
    OFPAT_SET_QUEUE = 21,/* Set queue id when outputting to a port */
    OFPAT_GROUP = 22,/* Apply group. */
    OFPAT_SET_NW_TTL = 23,/* IP TTL. */
    OFPAT_DEC_NW_TTL = 24,/* Decrement IP TTL. */
    OFPAT_SET_FIELD = 25,/* Set a header field using OXM TLV format. */
    OFPAT_PUSH_PBB = 26,/* Push a new PBB service tag (I-TAG) */
    OFPAT_POP_PBB = 27,/* Pop the outer PBB service tag (I-TAG) */
    OFPAT_EXPERIMENTER = 0xffff
};

/* The match type indicates the match structure (set of fields that compose the * match) in use. The match type is placed in the type field at the beginning
* of all match structures. The "OpenFlow Extensible Match" type corresponds
* to OXM TLV format described below and must be supported by all OpenFlow
* switches. Extensions that define other match types may be published on the * ONF wiki. Support for extensions is optional.
*/
enum ofp_match_type {
    OFPMT_STANDARD = 0, /* Deprecated. */
    OFPMT_OXM = 1, /* OpenFlow Extensible Match */
};


/* Action structure for OFPAT_OUTPUT, which sends packets out ’port’. * When the ’port’ is the OFPP_CONTROLLER, ’max_len’ indicates the max * number of bytes to send. A ’max_len’ of zero means no bytes of the * packet should be sent. A ’max_len’ of OFPCML_NO_BUFFER means that
* the packet is not buffered and the complete packet is to be sent to * the controller. */
typedef struct ofp_action_output { 
    uint16_t type; /* OFPAT_OUTPUT. */
    uint16_t len; /* Length is 16. */
    uint32_t port; /* Output port. */
    uint16_t max_len; /* Max length to send to controller. */
    uint8_t pad[6]; /* Pad to 64 bits. */
}ofp_action_output_t;
OFP_ASSERT(sizeof(struct ofp_action_output) == 16);

/* Instruction structure for OFPIT_WRITE/APPLY/CLEAR_ACTIONS */
typedef struct ofp_instruction_actions {
    uint16_t type; /* One of OFPIT_*_ACTIONS */
    uint16_t len; /* Length of this struct in bytes. */
    uint8_t pad[4]; /* Align to 64-bits */
    struct ofp_action_header actions[0]; /* Actions associated with 
                                            OFPIT_WRITE_ACTIONS and 
                                            OFPIT_APPLY_ACTIONS */
}ofp_instruction_actions_t;
OFP_ASSERT(sizeof(struct ofp_instruction_actions) == 8);

enum ofp_instruction_type { 
    OFPIT_GOTO_TABLE = 1, /* Setup the next table in the lookup pipeline */ 
    OFPIT_WRITE_METADATA = 2, /* Setup the metadata field for use later in pipeline */ 
    OFPIT_WRITE_ACTIONS = 3, /* Write the action(s) onto the datapath action set */
    OFPIT_APPLY_ACTIONS = 4, /* Applies the action(s) immediately */
    OFPIT_CLEAR_ACTIONS = 5, /* Clears all actions from the datapath action set */
    OFPIT_METER = 6, /* Apply meter (rate limiter) */
    OFPIT_EXPERIMENTER = 0xFFFF /* Experimenter instruction */ 
};

#endif