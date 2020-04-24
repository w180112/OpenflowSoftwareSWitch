#include 		<common.h>
#include 		"ofpd.h"
#include 		"dp_codec.h"
#include		"dp_flow.h"

extern flow_t flow[256];
STATUS flow_type_cmp(pkt_info_t pkt_info, void **cur, uint8_t type);
STATUS flowmod_match_process(flowmod_info_t flowmod_info);

STATUS find_flow(pkt_info_t pkt_info)
{
	uint16_t max_pri = 0;
	int ret;
	//uint8_t index = find_index(pkt_info.dst_mac,6);

	for(int i=0; i<256; i++) {
		max_pri = (max_pri > flow[i].priority) ? max_pri : flow[i].priority;
		if (flow[i].is_exist == FALSE)
			continue;
		void *cur;
		for(cur=(void *)&flow[i];;) {
			if ((ret = flow_type_cmp(pkt_info,&cur,flow[i].type)) == FALSE)
				break;
			else if (ret == END) {
				puts("flow found.");
				return TRUE;
			}
		}
	}

	return FALSE;
}

uint8_t find_index(U8 *info, int len)
{
	uint16_t index = 0;

	for(int i=0; i<len; i++)
		index += *(uint16_t *)info;
	return index % 256;
}

STATUS flow_type_cmp(pkt_info_t pkt_info, void **cur, uint8_t type)
{
	switch(type) {
		case PORT:
			if (BYTES_CMP((U8 *)&(pkt_info.port_id),(U8 *)&(((port_t *)(*cur))->port_id),2) == FALSE)
				return FALSE;
			if (((port_t *)(*cur))->next == NULL)
				return END;
			*cur = (void *)(((port_t *)(*cur))->next);
			break;
		case DST_MAC:
			if (BYTES_CMP((U8 *)pkt_info.dst_mac,(U8 *)((dst_mac_t *)(*cur))->dst_mac,ETH_ALEN) == FALSE)
				return FALSE;
			if (((dst_mac_t *)(*cur))->next == NULL)
				return END;
			*cur = (void *)(((dst_mac_t *)(*cur))->next);
			break;
		case SRC_MAC:
			if (BYTES_CMP((U8 *)pkt_info.src_mac,(U8 *)((src_mac_t *)(*cur))->src_mac,ETH_ALEN) == FALSE)
				return FALSE;
			if (((src_mac_t *)(*cur))->next == NULL)
				return END;
			*cur = (void *)(((src_mac_t *)(*cur))->next);
			break;
		case ETHER_TYPE:
			if (BYTES_CMP((U8 *)&(pkt_info.ether_type),(U8 *)&(((ether_type_t *)(*cur))->ether_type),2) == FALSE)
				return FALSE;
			if (((ether_type_t *)(*cur))->next == NULL)
				return END;
			*cur = (void *)(((ether_type_t *)(*cur))->next);
			break;
		case DST_IP:
			if (BYTES_CMP((U8 *)&(pkt_info.ip_dst),(U8 *)&(((dst_ip_t *)(*cur))->dst_ip),((dst_ip_t *)(*cur))->mask) == FALSE)
				return FALSE;
			if (((dst_ip_t *)(*cur))->next == NULL)
				return END;
			*cur = (void *)(((dst_ip_t *)(*cur))->next);
			break;
		case SRC_IP:
			if (BYTES_CMP((U8 *)&(pkt_info.ip_src),(U8 *)&(((src_ip_t *)(*cur))->src_ip),((src_ip_t *)(*cur))->mask) == FALSE)
				return FALSE;
			if (((src_ip_t *)(*cur))->next == NULL)
				return END;
			*cur = (void *)(((src_ip_t *)(*cur))->next);
			break;
		case IP_PROTO:
			if (BYTES_CMP((U8 *)&(pkt_info.ip_proto),(U8 *)&(((ip_proto_t *)(*cur))->ip_proto),1) == FALSE)
				return FALSE;
			if (((ip_proto_t *)(*cur))->next == NULL)
				return END;
			*cur = (void *)(((ip_proto_t *)(*cur))->next);
			break;
		case DST_PORT:
			if (BYTES_CMP((U8 *)&(pkt_info.dst_port),(U8 *)&(((dst_port_t *)(*cur))->dst_port),2) == FALSE)
				return FALSE;
			if (((dst_port_t *)(*cur))->next == NULL)
				return END;
			*cur = (void *)(((dst_port_t *)(*cur))->next);
			break;
		case SRC_PORT:
			if (BYTES_CMP((U8 *)&(pkt_info.src_port),(U8 *)&(((src_port_t *)(*cur))->src_port),2) == FALSE)
				return FALSE;
			if (((src_port_t *)(*cur))->next == NULL)
				return END;
			*cur = (void *)(((src_port_t *)(*cur))->next);
			break;
		default:
			;
	}
	return TRUE;
}

STATUS flowmod_match_process(flowmod_info_t flowmod_info)
{
	//PRINT_MESSAGE(flowmod_info.match_info,sizeof(pkt_info_t)*20);
	for(int i=0; i<256; i++) {
		if (flowmod_info.command == OFPFC_ADD) {
			if (flow[i].is_exist == TRUE)
				continue;
			flow[i].cookie = bitswap64(flowmod_info.cookie);
			flow[i].priority = htons(flowmod_info.priority);
			flow[i].table_id = flowmod_info.table_id;
			flow[i].idle_timeout = htons(flowmod_info.idle_timeout);
			flow[i].hard_timeout = htonl(flowmod_info.hard_timeout);
			flow[i].buffer_id = htonl(flowmod_info.buffer_id);
			flow[i].out_port = htonl(flowmod_info.out_port);
			flow[i].out_group = htonl(flowmod_info.out_group);
			flow[i].is_exist = TRUE;
			if (flowmod_info.is_tail == TRUE)
				return TRUE;
			void *cur = NULL;
			int j = 0;
			for(flow[i].next=cur;; j++) {
				if (j >= 20) {
					puts("reach max number match field in flow table");
					return FALSE;
				}
				printf("match type = %u\n", flowmod_info.match_info[j].type);
				switch (flowmod_info.match_info[j].type) {
				case PORT:
					cur = (void *)malloc(sizeof(port_t));
					((port_t *)cur)->port_id = flowmod_info.match_info[j].port_id;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((port_t *)cur)->type = flowmod_info.match_info[j+1].type;
					else {
						((port_t *)cur)->type = 0; // 0 means there's no next match field type
						return TRUE;
					}
					cur = (void *)((port_t *)cur)->next;
					break;
				case DST_MAC:
					cur = (void *)malloc(sizeof(dst_mac_t));
					memcpy(((dst_mac_t *)cur)->dst_mac,flowmod_info.match_info[j].dst_mac,ETH_ALEN);
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((dst_mac_t *)cur)->type = flowmod_info.match_info[j+1].type;
					else {
						((dst_mac_t *)cur)->type = 0;
						return TRUE;
					}
					cur = (void *)((dst_mac_t *)cur)->next;
					break;
				case SRC_MAC:
					cur = (void *)malloc(sizeof(src_mac_t));
					memcpy(((src_mac_t *)cur)->src_mac,flowmod_info.match_info[j].src_mac,ETH_ALEN);
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((src_mac_t *)cur)->type = flowmod_info.match_info[j+1].type;
					else {
						((src_mac_t *)cur)->type = 0;
						return TRUE;
					}
					cur = (void *)((src_mac_t *)cur)->next;
					break;
				case ETHER_TYPE:
					cur = (void *)malloc(sizeof(ether_type_t));
					((ether_type_t *)cur)->ether_type = flowmod_info.match_info[j].ether_type;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((ether_type_t *)cur)->type = flowmod_info.match_info[j+1].type;
					else {
						((ether_type_t *)cur)->type = 0;
						return TRUE;
					}
					cur = (void *)((ether_type_t *)cur)->next;
					break;
				case DST_IP:
					cur = (void *)malloc(sizeof(dst_ip_t));
					((dst_ip_t *)cur)->dst_ip = flowmod_info.match_info[j].ip_dst;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((dst_ip_t *)cur)->type = flowmod_info.match_info[j+1].type;
					else {
						((dst_ip_t *)cur)->type = 0;
						return TRUE;
					}
					cur = (void *)((dst_ip_t *)cur)->next;
					break;
				case SRC_IP:
					cur = (void *)malloc(sizeof(src_ip_t));
					((src_ip_t *)cur)->src_ip = flowmod_info.match_info[j].ip_dst;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((src_ip_t *)cur)->type = flowmod_info.match_info[j+1].type;
					else {
						((src_ip_t *)cur)->type = 0;
						return TRUE;
					}
					cur = (void *)((src_ip_t *)cur)->next;
					break;
				case IP_PROTO:
					cur = (void *)malloc(sizeof(ip_proto_t));
					((ip_proto_t *)cur)->ip_proto = flowmod_info.match_info[j].ip_proto;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((ip_proto_t *)cur)->type = flowmod_info.match_info[j+1].type;
					else {
						((ip_proto_t *)cur)->type = 0;
						return TRUE;
					}
					cur = (void *)((ip_proto_t *)cur)->next;
					break;
				case DST_PORT:
					cur = (void *)malloc(sizeof(dst_port_t));
					((dst_port_t *)cur)->dst_port = flowmod_info.match_info[j].dst_port;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((dst_port_t *)cur)->type = flowmod_info.match_info[j+1].type;
					else {
						((dst_port_t *)cur)->type = 0;
						return TRUE;
					}
					cur = (void *)((dst_port_t *)cur)->next;
					break;
				case SRC_PORT:
					cur = (void *)malloc(sizeof(src_port_t));
					((src_port_t *)cur)->src_port = flowmod_info.match_info[j].src_port;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((src_port_t *)cur)->type = flowmod_info.match_info[j+1].type;
					else {
						((src_port_t *)cur)->type = 0;
						return TRUE;
					}
					cur = (void *)((src_port_t *)cur)->next;
					break;
				default:
					break;
				}
			}
		}
	}
	return FALSE;
}
