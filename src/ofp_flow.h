/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   OFP_FLOW.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFP_FLOW_H_
#define _OFP_FLOW_H_

#include <common.h>

#define OFP_TABLE_SIZE 65536
#define MAX_IN_PORT 128
#define MAX_OUT_PORT 128

typedef struct table_flow_info {
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
    int32_t next_table_index;
    uint16_t next_table_type;
    BOOL	is_exist;
}table_flow_info_t;

typedef struct table_mac {
    uint8_t dst_mac[ETH_ALEN];
    uint8_t src_mac[ETH_ALEN];
    uint16_t eth_type;
    uint64_t dst_mask;
    uint8_t dst_mask_len;
    uint64_t src_mask;
    uint8_t src_mask_len;
    int32_t next_table_index;
    uint16_t next_table_type;
    BOOL	is_exist;
}table_mac_t;

typedef struct table_ip {
    uint32_t dst_ip_addr;
    uint32_t src_ip_addr;
    uint8_t ip_proto;
    uint64_t dst_mask;
    uint8_t dst_mask_len;
    uint64_t src_mask;
    uint8_t src_mask_len;
    int32_t next_table_index;
    uint16_t next_table_type;
    BOOL	is_exist;
}table_ip_t;

typedef struct table_in_port {
    uint32_t in_port[MAX_IN_PORT];
    int32_t next_table_index;
    uint16_t next_table_type;
    BOOL	is_exist;
}table_in_port_t;

typedef struct table_port {
    uint16_t dst_ip_port;
    uint16_t src_ip_port;
    int32_t next_table_index;
    uint16_t next_table_type;
    BOOL	is_exist;
}table_port_t;

struct flow_entry_list {
    uint16_t entry_id;
    struct flow_entry_list *next;
};

typedef struct tuple_table {
    uint8_t match_bits[4]; 
    struct flow_entry_list *list;
    BOOL is_exist;
}tuple_table_t;

struct output_port {
    uint32_t port;
    uint32_t max_len;
};

typedef struct action_info {
    struct output_port out_port[MAX_OUT_PORT];
    unsigned char dst_mac[ETH_ALEN];
	unsigned char src_mac[ETH_ALEN];
	uint16_t ether_type;
	uint32_t ip_dst;
	uint32_t ip_src;
	uint8_t ip_proto;
	uint16_t dst_port;
	uint16_t src_port;
    BOOL is_exist;
}action_info_t;

typedef struct flow_table {
	table_flow_info_t table_flow_info;
    table_mac_t table_mac;
    table_ip_t table_ip;
    table_in_port_t table_in_port;
    table_port_t table_port;
    action_info_t action_info;
}flow_table_t;

#endif