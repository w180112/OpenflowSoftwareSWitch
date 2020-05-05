/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   DP_FLOW.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _DP_FLOW_H_
#define _DP_FLOW_H_

#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <stdint.h>
#include "dp.h"


enum {
	PORT = 1,
	DST_MAC,
	SRC_MAC,
	ETHER_TYPE,
	DST_IP,
	SRC_IP,
	IP_PROTO,
	DST_PORT,
	SRC_PORT,
};

typedef struct flow {
	uint64_t cookie;
	uint16_t priority;
	uint8_t table_id;
    uint16_t idle_timeout; 
    uint16_t hard_timeout;
    uint32_t buffer_id;
    uint32_t out_port; 
    uint32_t out_group;
	uint8_t type;
	BOOL	is_exist;
	void *next_match;
	void *next_action;
}flow_t;

typedef struct port {
	uint32_t port_id;
	uint16_t max_len;
	uint8_t type;
	void *next;
}port_t;

typedef struct dst_mac {
	unsigned char dst_mac[ETH_ALEN];
	uint8_t type;
	void *next;
}dst_mac_t;

typedef struct src_mac {
	unsigned char src_mac[ETH_ALEN];
	uint8_t type;
	void *next;
}src_mac_t;

typedef struct ether_type {
	uint16_t ether_type;
	uint8_t type;
	void *next;
}ether_type_t;

typedef struct dst_ip {
	uint32_t dst_ip;
	uint32_t mask;
	uint8_t type;
	void *next;
}dst_ip_t;

typedef struct src_ip {
	uint32_t src_ip;
	uint32_t mask;
	uint8_t type;
	void *next;
}src_ip_t;

typedef struct ip_proto {
	uint8_t ip_proto;
	uint8_t type;
	void *next;
}ip_proto_t;

typedef struct dst_port {
	uint8_t dst_port;
	uint8_t type;
	void *next;
}dst_port_t;

typedef struct src_port {
	uint8_t src_port;
	uint8_t type;
	void *next;
}src_port_t;

typedef struct pkt_info {
	uint8_t type;
	uint32_t port_id;
	unsigned char dst_mac[ETH_ALEN];
	unsigned char src_mac[ETH_ALEN];
	uint16_t ether_type;
	uint32_t ip_dst;
	uint32_t ip_src;
	uint8_t ip_proto;
	uint8_t dst_port;
	uint8_t src_port;
	uint16_t max_len; /* for OFPCML_MAX or OFPCML_NO_BUFFER or 0 */
	BOOL	is_tail;
}pkt_info_t;

#endif