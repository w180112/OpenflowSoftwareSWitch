/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  dp_flow.c
  
  Created by THE on MAR 11,'20
\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include 		<common.h>
#include 		"ofpd.h"
#include 		"dp_codec.h"
#include		"dp_flow.h"
#include 		"dp_sock.h"

extern flow_t flow[256];
extern void dp_drv_xmit(U8 *mu, U16 mulen, uint16_t port_id, dp_io_fds_t *dp_io_fds_head);

STATUS apply_flow(U8 *mu, U16 mulen, uint32_t flow_index, dp_io_fds_t *dp_io_fds_head);
STATUS flow_type_cmp(pkt_info_t pkt_info, void **cur, uint8_t *type);
STATUS flowmod_match_process(flowmod_info_t flowmod_info, uint32_t *flow_index);
STATUS flowmod_action_process(flowmod_info_t flowmod_info, uint32_t flow_index);

STATUS find_flow(pkt_info_t pkt_info, uint32_t *flow_index)
{
	uint16_t 	max_pri = 0;
	int 		ret;
	BOOL 		is_found = FALSE;
	uint8_t 	match_type;
	//uint8_t index = find_index(pkt_info.dst_mac,6);
	for(int i=0; i<256; i++) {
		//max_pri = (max_pri > flow[i].priority) ? max_pri : flow[i].priority;
		//printf("<%d\n", __LINE__);
		if (flow[i].is_exist == FALSE)
			continue;
		void *cur;
		match_type = flow[i].type;
		for(cur=flow[i].next_match;;) {
			//printf("<%d\n", __LINE__);
			if ((ret = flow_type_cmp(pkt_info, &cur, &match_type)) == FALSE)
				break;
			else if (ret == END) {
				if (max_pri <= flow[i].priority) {
					*flow_index = i;
					is_found = TRUE;
					break;
				}
				//puts("flow found.");
				//return TRUE;
			}
		}
	}

	return is_found;
}

STATUS apply_flow(U8 *mu, U16 mulen, uint32_t flow_index, dp_io_fds_t *dp_io_fds_head)
{
	uint8_t type = flow[flow_index].type;
	int 	i = 0;

	for(void *cur=(void *)(flow[flow_index].next_action);; i++) {
		if (i >= 20)
			return ERROR;
		//printf("type = %u\n", type);
		switch (type) {
		case 0:
			return TRUE;
		case PORT:
			//send to port
			//printf("flow port id = %u\n", ((port_t *)cur)->port_id);
			if (((port_t *)cur)->port_id == 0xfffffffd)
				return FALSE;
			else {
				dp_drv_xmit(mu, mulen, ((port_t *)cur)->port_id, dp_io_fds_head);
				return TRUE;
			}
			cur = (void *)(((port_t *)cur)->next);
			type = ((port_t *)cur)->type;
			break;
		case DST_MAC:
			memcpy(mu, ((dst_mac_t *)cur)->dst_mac, ETH_ALEN);
			cur = (void *)(((dst_mac_t *)cur)->next);
			type = ((dst_mac_t *)cur)->type;
			break;
		case SRC_MAC:
			memcpy(mu+ETH_ALEN, ((dst_mac_t *)cur)->dst_mac, ETH_ALEN);
			cur = (void *)(((src_mac_t *)cur)->next);
			type = ((src_mac_t *)cur)->type;
			break;
		case ETHER_TYPE:
			((struct ethhdr *)mu)->h_proto = htons(((ether_type_t *)cur)->ether_type);
			cur = (void *)(((ether_type_t *)cur)->next);
			type = ((ether_type_t *)cur)->type;
			break;
		case DST_IP:
			((struct iphdr *)(((struct ethhdr *)mu) + 1))->daddr = htonl(((dst_ip_t *)cur)->dst_ip);
			cur = (void *)(((dst_ip_t *)cur)->next);
			type = ((dst_ip_t *)cur)->type;
			puts("modify IP");
			break;
		case SRC_IP:
			((struct iphdr *)(((struct ethhdr *)mu) + 1))->saddr = htonl(((src_ip_t *)cur)->src_ip);
			cur = (void *)(((src_ip_t *)cur)->next);
			type = ((src_ip_t *)cur)->type;
			puts("modify IP");
			break;
		case IP_PROTO:
			((struct iphdr *)(((struct ethhdr *)mu) + 1))->protocol = ((ip_proto_t *)cur)->ip_proto;
			cur = (void *)(((ip_proto_t *)cur)->next);
			type = ((ip_proto_t *)cur)->type;
			break;
		case DST_PORT:
			*(uint16_t *)(((struct iphdr *)(((struct ethhdr *)mu) + 1)) + 1) = htons(((dst_port_t *)cur)->dst_port);
			cur = (void *)(((dst_port_t *)cur)->next);
			type = ((dst_port_t *)cur)->type;
			break;
		case SRC_PORT:
			*(((uint16_t *)(((struct iphdr *)(((struct ethhdr *)mu) + 1)) + 1)) + 1) = htons(((src_port_t *)cur)->src_port);
			cur = (void *)(((src_port_t *)cur)->next);
			type = ((src_port_t *)cur)->type;
			break;
		default:
			;
		}
	}
	return TRUE;
}

uint8_t find_index(U8 *info, int len)
{
	uint16_t index = 0;

	for(int i=0; i<len; i++)
		index += *(uint16_t *)info;
	return index % 256;
}

STATUS flow_type_cmp(pkt_info_t pkt_info, void **cur, uint8_t *type)
{
	if (*cur == NULL)
		return END;
	switch(*type) {
		case PORT:
			if (BYTES_CMP((U8 *)&(pkt_info.port_id), (U8 *)&(((port_t *)(*cur))->port_id), 2) == FALSE)
				return FALSE;
			/*if (((port_t *)(*cur))->next == NULL)
				return END;*/
			*type = ((port_t *)(*cur))->type;
			*cur = (void *)(((port_t *)(*cur))->next);
			break;
		case DST_MAC:
			if (BYTES_CMP((U8 *)pkt_info.dst_mac,(U8 *)((dst_mac_t *)(*cur))->dst_mac,ETH_ALEN) == FALSE)
				return FALSE;
			/*if (((dst_mac_t *)(*cur))->next == NULL)
				return END;*/
			*type = ((dst_mac_t *)(*cur))->type;
			*cur = (void *)(((dst_mac_t *)(*cur))->next);
			break;
		case SRC_MAC:
			if (BYTES_CMP((U8 *)pkt_info.src_mac,(U8 *)((src_mac_t *)(*cur))->src_mac,ETH_ALEN) == FALSE)
				return FALSE;
			/*if (((src_mac_t *)(*cur))->next == NULL)
				return END;*/
			*type = ((src_mac_t *)(*cur))->type;
			*cur = (void *)(((src_mac_t *)(*cur))->next);
			break;
		case ETHER_TYPE:
			if (BYTES_CMP((U8 *)&(pkt_info.ether_type),(U8 *)&(((ether_type_t *)(*cur))->ether_type),2) == FALSE)
				return FALSE;
			/*if (((ether_type_t *)(*cur))->next == NULL)
				return END;*/
			*type = ((ether_type_t *)(*cur))->type;
			*cur = (void *)(((ether_type_t *)(*cur))->next);
			break;
		case DST_IP:
			if (BYTES_CMP((U8 *)&(pkt_info.ip_dst),(U8 *)&(((dst_ip_t *)(*cur))->dst_ip),((dst_ip_t *)(*cur))->mask) == FALSE)
				return FALSE;
			/*if (((dst_ip_t *)(*cur))->next == NULL)
				return END;*/
			*type = ((dst_ip_t *)(*cur))->type;
			*cur = (void *)(((dst_ip_t *)(*cur))->next);
			break;
		case SRC_IP:
			if (BYTES_CMP((U8 *)&(pkt_info.ip_src),(U8 *)&(((src_ip_t *)(*cur))->src_ip),((src_ip_t *)(*cur))->mask) == FALSE)
				return FALSE;
			/*if (((src_ip_t *)(*cur))->next == NULL)
				return END;*/
			*type = ((src_ip_t *)(*cur))->type;
			*cur = (void *)(((src_ip_t *)(*cur))->next);
			break;
		case IP_PROTO:
			if (BYTES_CMP((U8 *)&(pkt_info.ip_proto),(U8 *)&(((ip_proto_t *)(*cur))->ip_proto),1) == FALSE)
				return FALSE;
			/*if (((ip_proto_t *)(*cur))->next == NULL)
				return END;*/
			*type = ((ip_proto_t *)(*cur))->type;
			*cur = (void *)(((ip_proto_t *)(*cur))->next);
			break;
		case DST_PORT:
			if (BYTES_CMP((U8 *)&(pkt_info.dst_port),(U8 *)&(((dst_port_t *)(*cur))->dst_port),2) == FALSE)
				return FALSE;
			/*if (((dst_port_t *)(*cur))->next == NULL)
				return END;*/
			*type = ((dst_port_t *)(*cur))->type;
			*cur = (void *)(((dst_port_t *)(*cur))->next);
			break;
		case SRC_PORT:
			if (BYTES_CMP((U8 *)&(pkt_info.src_port),(U8 *)&(((src_port_t *)(*cur))->src_port),2) == FALSE)
				return FALSE;
			/*if (((src_port_t *)(*cur))->next == NULL)
				return END;*/
			*type = ((src_port_t *)(*cur))->type;
			*cur = (void *)(((src_port_t *)(*cur))->next);
			break;
		default:
			;
	}
	return TRUE;
}

STATUS flowmod_match_process(flowmod_info_t flowmod_info, uint32_t *flow_index)
{
	//PRINT_MESSAGE(flowmod_info.match_info,sizeof(pkt_info_t)*20);
	for(int i=0; i<256; i++) {
		if (flowmod_info.command == OFPFC_ADD) {
			if (flow[i].is_exist == TRUE)
				continue;
			flow[i].buffer_id = flowmod_info.buffer_id;
			*flow_index = i;
			flow[i].is_exist = TRUE;
			if (flowmod_info.is_tail == TRUE)
				return TRUE;
			void *cur = NULL, *head = NULL;
			int j = 0;
			for(flow[i].next_match=cur;; j++) {
				if (j >= 20) {
					puts("reach max number match field in flow table");
					return FALSE;
				}
				void *new_node;
				//printf("match type = %u\n", flowmod_info.match_info[j].type);
				switch (flowmod_info.match_info[j].type) {
				case PORT:
					puts("create in_port match field in flow table");
					new_node = (void *)malloc(sizeof(port_t));
					if (head == NULL)
						head = new_node;
					((port_t *)new_node)->port_id = flowmod_info.match_info[j].port_id;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((port_t *)new_node)->type = flowmod_info.match_info[j+1].type;
					else {
						((port_t *)new_node)->type = 0; // 0 means there's no next match field type
						flow[i].next_match = head;
						return TRUE;
					}
					((port_t *)cur)->next = (port_t *)new_node;
					cur = (void *)((port_t *)cur)->next;
					break;
				case DST_MAC:
					puts("create dst_mac match field in flow table");
					new_node = (void *)malloc(sizeof(dst_mac_t));
					if (head == NULL)
						head = new_node;
					memcpy(((dst_mac_t *)new_node)->dst_mac,flowmod_info.match_info[j].dst_mac,ETH_ALEN);
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((dst_mac_t *)new_node)->type = flowmod_info.match_info[j+1].type;
					else {
						((dst_mac_t *)new_node)->type = 0;
						flow[i].next_match = head;
						return TRUE;
					}
					((dst_mac_t *)cur)->next = (dst_mac_t *)new_node;
					cur = (void *)((dst_mac_t *)cur)->next;
					break;
				case SRC_MAC:
					puts("create src_mac match field in flow table");
					new_node = (void *)malloc(sizeof(src_mac_t));
					if (head == NULL)
						head = new_node;
					memcpy(((src_mac_t *)new_node)->src_mac,flowmod_info.match_info[j].src_mac,ETH_ALEN);
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((src_mac_t *)new_node)->type = flowmod_info.match_info[j+1].type;
					else {
						((src_mac_t *)new_node)->type = 0;
						flow[i].next_match = head;
						return TRUE;
					}
					((src_mac_t *)cur)->next = (src_mac_t *)new_node;
					cur = (void *)((src_mac_t *)cur)->next;
					break;
				case ETHER_TYPE:
					new_node = (void *)malloc(sizeof(ether_type_t));
					if (head == NULL)
						head = new_node;
					((ether_type_t *)cur)->ether_type = flowmod_info.match_info[j].ether_type;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((ether_type_t *)new_node)->type = flowmod_info.match_info[j+1].type;
					else {
						((ether_type_t *)new_node)->type = 0;
						flow[i].next_match = head;
						return TRUE;
					}
					((ether_type_t *)cur)->next = (ether_type_t *)new_node;
					cur = (void *)((ether_type_t *)cur)->next;
					break;
				case DST_IP:
					new_node = (void *)malloc(sizeof(dst_ip_t));
					if (head == NULL)
						head = new_node;
					((dst_ip_t *)new_node)->dst_ip = flowmod_info.match_info[j].ip_dst;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((dst_ip_t *)new_node)->type = flowmod_info.match_info[j+1].type;
					else {
						((dst_ip_t *)new_node)->type = 0;
						flow[i].next_match = head;
						return TRUE;
					}
					((dst_ip_t *)cur)->next = (dst_ip_t *)new_node;
					cur = (void *)((dst_ip_t *)cur)->next;
					break;
				case SRC_IP:
					new_node = (void *)malloc(sizeof(src_ip_t));
					if (head == NULL)
						head = new_node;
					((src_ip_t *)new_node)->src_ip = flowmod_info.match_info[j].ip_dst;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((src_ip_t *)new_node)->type = flowmod_info.match_info[j+1].type;
					else {
						((src_ip_t *)new_node)->type = 0;
						flow[i].next_match = head;
						return TRUE;
					}
					((src_ip_t *)cur)->next = (src_ip_t *)new_node;
					cur = (void *)((src_ip_t *)cur)->next;
					break;
				case IP_PROTO:
					new_node = (void *)malloc(sizeof(ip_proto_t));
					if (head == NULL)
						head = new_node;
					((ip_proto_t *)new_node)->ip_proto = flowmod_info.match_info[j].ip_proto;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((ip_proto_t *)new_node)->type = flowmod_info.match_info[j+1].type;
					else {
						((ip_proto_t *)new_node)->type = 0;
						flow[i].next_match = head;
						return TRUE;
					}
					((ip_proto_t *)cur)->next = (ip_proto_t *)new_node;
					cur = (void *)((ip_proto_t *)cur)->next;
					break;
				case DST_PORT:
					new_node = (void *)malloc(sizeof(dst_port_t));
					if (head == NULL)
						head = new_node;
					((dst_port_t *)new_node)->dst_port = flowmod_info.match_info[j].dst_port;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((dst_port_t *)new_node)->type = flowmod_info.match_info[j+1].type;
					else {
						((dst_port_t *)new_node)->type = 0;
						flow[i].next_match = head;
						return TRUE;
					}
					((dst_port_t *)cur)->next = (dst_port_t *)new_node;
					cur = (void *)((dst_port_t *)cur)->next;
					break;
				case SRC_PORT:
					new_node = (void *)malloc(sizeof(src_port_t));
					if (head == NULL)
						head = new_node;
					((src_port_t *)new_node)->src_port = flowmod_info.match_info[j].src_port;
					if (flowmod_info.match_info[j].is_tail == FALSE)
						((src_port_t *)new_node)->type = flowmod_info.match_info[j+1].type;
					else {
						((src_port_t *)new_node)->type = 0;
						flow[i].next_match = head;
						return TRUE;
					}
					((src_port_t *)cur)->next = (src_port_t *)new_node;
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

STATUS flowmod_action_process(flowmod_info_t flowmod_info, uint32_t flow_index)
{
	//PRINT_MESSAGE(flowmod_info.match_info,sizeof(pkt_info_t)*20);
	
	if (flowmod_info.command == OFPFC_ADD) {
		void *cur = NULL, *head = NULL;
		int j = 0;
		flow[flow_index].type = flowmod_info.action_info[0].type;
		for(flow[flow_index].next_action=cur;; j++) {
			if (j >= 20) {
				puts("reach max number action field in flow table");
				return FALSE;
			}
			printf("aciton type = %u\n", flowmod_info.action_info[j].type);
			void *new_node;
			switch (flowmod_info.action_info[j].type) {
			case PORT:
				new_node = (void *)malloc(sizeof(port_t));
				if (head == NULL)
					head = new_node;
				((port_t *)new_node)->port_id = flowmod_info.action_info[j].port_id;
				((port_t *)new_node)->max_len = flowmod_info.action_info[j].max_len;
				if (flowmod_info.action_info[j].is_tail == FALSE)
					((port_t *)new_node)->type = flowmod_info.action_info[j+1].type;
				else {
					((port_t *)new_node)->type = 0; // 0 means there's no next match field type
					flow[flow_index].next_action = head;
					//printf("<%d %x\n", __LINE__, flow[flow_index].next_action);
					return TRUE;
				}
				((port_t *)cur)->next = (port_t *)new_node;
				cur = (void *)((port_t *)cur)->next;
				break;
			case DST_MAC:
				new_node = (void *)malloc(sizeof(dst_mac_t));
				if (head == NULL)
					head = new_node;
				memcpy(((dst_mac_t *)new_node)->dst_mac,flowmod_info.action_info[j].dst_mac,ETH_ALEN);
				if (flowmod_info.action_info[j].is_tail == FALSE)
					((dst_mac_t *)new_node)->type = flowmod_info.action_info[j+1].type;
				else {
					((dst_mac_t *)new_node)->type = 0;
					flow[flow_index].next_action = head;
					return TRUE;
				}
				((dst_mac_t *)cur)->next = (dst_mac_t *)new_node;
				cur = (void *)((dst_mac_t *)cur)->next;
				break;
			case SRC_MAC:
				new_node = (void *)malloc(sizeof(src_mac_t));
				if (head == NULL)
					head = new_node;
				memcpy(((src_mac_t *)new_node)->src_mac,flowmod_info.action_info[j].src_mac,ETH_ALEN);
				if (flowmod_info.action_info[j].is_tail == FALSE)
					((src_mac_t *)new_node)->type = flowmod_info.action_info[j+1].type;
				else {
					((src_mac_t *)new_node)->type = 0;
					flow[flow_index].next_action = head;
					return TRUE;
				}
				((src_mac_t *)cur)->next = (src_mac_t *)new_node;
				cur = (void *)((src_mac_t *)cur)->next;
				break;
			case ETHER_TYPE:
				new_node = (void *)malloc(sizeof(ether_type_t));
				if (head == NULL)
					head = new_node;
				((ether_type_t *)new_node)->ether_type = flowmod_info.action_info[j].ether_type;
				if (flowmod_info.action_info[j].is_tail == FALSE)
					((ether_type_t *)new_node)->type = flowmod_info.action_info[j+1].type;
				else {
					((ether_type_t *)new_node)->type = 0;
					flow[flow_index].next_action = head;
					return TRUE;
				}
				((ether_type_t *)cur)->next = (ether_type_t *)new_node;
				cur = (void *)((ether_type_t *)cur)->next;
				break;
			case DST_IP:
				new_node = (void *)malloc(sizeof(dst_ip_t));
				if (head == NULL)
					head = new_node;
				((dst_ip_t *)new_node)->dst_ip = flowmod_info.action_info[j].ip_dst;
				if (flowmod_info.action_info[j].is_tail == FALSE)
					((dst_ip_t *)new_node)->type = flowmod_info.action_info[j+1].type;
				else {
					((dst_ip_t *)new_node)->type = 0;
					flow[flow_index].next_action = head;
					return TRUE;
				}
				((dst_ip_t *)cur)->next = (dst_ip_t *)new_node;
				cur = (void *)((dst_ip_t *)cur)->next;
				break;
			case SRC_IP:
				new_node = (void *)malloc(sizeof(src_ip_t));
				if (head == NULL)
					head = new_node;
				((src_ip_t *)new_node)->src_ip = flowmod_info.action_info[j].ip_dst;
				if (flowmod_info.action_info[j].is_tail == FALSE)
					((src_ip_t *)new_node)->type = flowmod_info.action_info[j+1].type;
				else {
					((src_ip_t *)new_node)->type = 0;
					flow[flow_index].next_action = head;
					return TRUE;
				}
				((src_ip_t *)cur)->next = (src_ip_t *)new_node;
				cur = (void *)((src_ip_t *)cur)->next;
				break;
			case IP_PROTO:
				new_node = (void *)malloc(sizeof(ip_proto_t));
				if (head == NULL)
					head = new_node;
				((ip_proto_t *)new_node)->ip_proto = flowmod_info.action_info[j].ip_proto;
				if (flowmod_info.action_info[j].is_tail == FALSE)
					((ip_proto_t *)new_node)->type = flowmod_info.action_info[j+1].type;
				else {
					((ip_proto_t *)new_node)->type = 0;
					flow[flow_index].next_action = head;
					return TRUE;
				}
				((ip_proto_t *)cur)->next = (ip_proto_t *)new_node;
				cur = (void *)((ip_proto_t *)cur)->next;
				break;
			case DST_PORT:
				new_node = (void *)malloc(sizeof(dst_port_t));
				if (head == NULL)
					head = new_node;
				((dst_port_t *)new_node)->dst_port = flowmod_info.action_info[j].dst_port;
				if (flowmod_info.action_info[j].is_tail == FALSE)
					((dst_port_t *)new_node)->type = flowmod_info.action_info[j+1].type;
				else {
					((dst_port_t *)new_node)->type = 0;
					flow[flow_index].next_action = head;
					return TRUE;
				}
				((dst_port_t *)cur)->next = (dst_port_t *)new_node;
				cur = (void *)((dst_port_t *)cur)->next;
				break;
			case SRC_PORT:
				new_node = (void *)malloc(sizeof(src_port_t));
				if (head == NULL)
					head = new_node;
				((src_port_t *)new_node)->src_port = flowmod_info.action_info[j].src_port;
				if (flowmod_info.action_info[j].is_tail == FALSE)
					((src_port_t *)new_node)->type = flowmod_info.action_info[j+1].type;
				else {
					((src_port_t *)new_node)->type = 0;
					flow[flow_index].next_action = head;
					return TRUE;
				}
				((src_port_t *)cur)->next = (src_port_t *)new_node;
				cur = (void *)((src_port_t *)cur)->next;
				break;
			default:
				break;
			}
			if (flowmod_info.action_info[j].is_tail == TRUE)
				return TRUE;
		}
	}
	return FALSE;
}

STATUS print_field(void **cur, uint8_t *type)
{
	if (*cur == NULL)
		return END;
	switch(*type) {
		case PORT:
			if (((port_t *)(*cur))->port_id == OFPP_CONTROLLER)
				printf("port = %s", "OFPP_CONTROLLER");
			else if (((port_t *)(*cur))->port_id == OFPP_FLOOD)
				printf("port = %s", "OFPP_FLOOD");
			else
				printf("port = %u", ((port_t *)(*cur))->port_id);
			*type = ((port_t *)(*cur))->type;
			*cur = (void *)(((port_t *)(*cur))->next);
			break;
		case DST_MAC:
			printf("dst_mac = %x:%x:%x:%x:%x:%x", ((dst_mac_t *)(*cur))->dst_mac[0], ((dst_mac_t *)(*cur))->dst_mac[1], ((dst_mac_t *)(*cur))->dst_mac[2], ((dst_mac_t *)(*cur))->dst_mac[3], ((dst_mac_t *)(*cur))->dst_mac[4], ((dst_mac_t *)(*cur))->dst_mac[5]);
			*type = ((dst_mac_t *)(*cur))->type;
			*cur = (void *)(((dst_mac_t *)(*cur))->next);
			break;
		case SRC_MAC:
			printf("src_mac = %x:%x:%x:%x:%x:%x", ((src_mac_t *)(*cur))->src_mac[0], ((src_mac_t *)(*cur))->src_mac[1], ((src_mac_t *)(*cur))->src_mac[2], ((src_mac_t *)(*cur))->src_mac[3], ((src_mac_t *)(*cur))->src_mac[4], ((src_mac_t *)(*cur))->src_mac[5]);
			*type = ((src_mac_t *)(*cur))->type;
			*cur = (void *)(((src_mac_t *)(*cur))->next);
			break;
		case ETHER_TYPE:
			printf("ether_type = %u", ((ether_type_t *)(*cur))->ether_type);
			*type = ((ether_type_t *)(*cur))->type;
			*cur = (void *)(((ether_type_t *)(*cur))->next);
			break;
		case DST_IP:
			printf("dst_ip = %u", ((dst_ip_t *)(*cur))->dst_ip);
			*type = ((dst_ip_t *)(*cur))->type;
			*cur = (void *)(((dst_ip_t *)(*cur))->next);
			break;
		case SRC_IP:
			printf("src_ip = %u", ((src_ip_t *)(*cur))->src_ip);
			*type = ((src_ip_t *)(*cur))->type;
			*cur = (void *)(((src_ip_t *)(*cur))->next);
			break;
		case IP_PROTO:
			printf("ip_proto = %u", ((ip_proto_t *)(*cur))->ip_proto);
			*type = ((ip_proto_t *)(*cur))->type;
			*cur = (void *)(((ip_proto_t *)(*cur))->next);
			break;
		case DST_PORT:
			printf("dst_port = %u", ((dst_port_t *)(*cur))->dst_port);
			*type = ((dst_port_t *)(*cur))->type;
			*cur = (void *)(((dst_port_t *)(*cur))->next);
			break;
		case SRC_PORT:
			printf("src_port = %u", ((src_port_t *)(*cur))->src_port);
			*type = ((src_port_t *)(*cur))->type;
			*cur = (void *)(((src_port_t *)(*cur))->next);
			break;
		default:
			;
	}
	return TRUE;
}