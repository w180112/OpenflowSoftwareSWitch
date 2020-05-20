/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   OFP_FLOW.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFP_FLOW_H_
#define _OFP_FLOW_H_

#include <common.h>

#define OFP_FABLE_SIZE 65536

typedef struct flow_table {
	uint64_t cookie;
	uint16_t priority;
	uint8_t table_id;
    uint16_t idle_timeout; 
    uint16_t hard_timeout;
    uint32_t buffer_id;
    uint32_t out_port; 
    uint32_t out_group;
	uint64_t pkt_count;
	uint16_t hash_type;
	uint32_t in_port;
    uint32_t next_table_index;
	BOOL	is_exist;
}flow_table_t;

typedef struct table_mac {
    uint8_t mac[ETH_ALEN];
    uint64_t mask;
    uint32_t next_table_index;
}table_mac_t;

typedef struct table_ip {
    uint32_t ip_addr;
    uint64_t mask;
    uint32_t next_table_index;
}table_ip_t;

typedef struct table_eth_type {
    uint32_t ip_addr;
    uint32_t next_table_index;
}table_eth_type_t;

typedef struct table_ip_proto {
    uint32_t ip_addr;
    uint32_t next_table_index;
}table_ip_proto_t;

typedef struct table_port {
    uint32_t ip_addr;
    uint32_t next_table_index;
}table_port_t;

typedef struct tuple_table {
    uint8_t match_bits[4]; 
    struct flow_entry_list *list;
    BOOL is_exist;
}tuple_table_t;

struct flow_entry_list {
    uint16_t entry_id;
    struct flow_entry_list *next;
};

#endif