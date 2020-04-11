#include 		<common.h>
#include 		"dp_codec.h"
#include		"dp_flow.h"

extern flow_t flow[256];
STATUS flow_type_cmp(pkt_info_t pkt_info, void **cur, uint8_t type);

STATUS find_flow(pkt_info_t pkt_info)
{
	uint16_t max_pri = 0;
	int ret;
	//uint8_t index = find_index(pkt_info.dst_mac,6);

	for(int i=0; i<256; i++) {
		max_pri = (max_pri > flow[i].priority) ? max_pri : flow[i].priority;
		void *cur;
		for(cur=(void *)&flow[i];;) {
			if (flow->next == NULL)
				break;
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
