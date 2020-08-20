/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFP_FLOW.C :

  Designed by THE on MAY 20, 2020
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include "ofp_flow.h"
#include "ofpd.h"

extern void print_in_port(table_in_port_t in_port);
extern void print_mac(table_mac_t mac);
extern void print_ip(table_ip_t ip);
extern void print_port(table_port_t port);
extern void print_action(action_info_t action);

void insert_flow_table(pkt_info_t match_info[], pkt_info_t action_info[], uint32_t flow_index, uint8_t tuple_mask[], uint16_t match_type);

flow_table_t flow_table[OFP_TABLE_SIZE];
tuple_table_t tuple_table[OFP_TABLE_SIZE];
STATUS add_huge_flow_table(flowmod_info_t flowmod_info, uint8_t tuple_mask[], uint16_t match_type);
void init_tables(void);
void print_ofp_flow_table(void);

STATUS add_huge_flow_table(flowmod_info_t flowmod_info, uint8_t tuple_mask[], uint16_t match_type)
{
    int32_t new_index;// = (tuple_mask[0] * 1000 + tuple_mask[1] * 100 + tuple_mask[2] * 10 + tuple_mask[3]) % OFP_FABLE_SIZE;
    int32_t flow_index;

    int32_t hash_index = 0;
	uint16_t hash_type = 0;
	for(int i=0; i<20&&flowmod_info.match_info[i].type>0; i++) {
		PRINT_MESSAGE(&(flowmod_info.match_info[i]),sizeof(pkt_info_t));
		hash_index += hash_func((U8*)&(flowmod_info.match_info[i]), sizeof(pkt_info_t)-1);
		hash_type |= flowmod_info.match_info[i].type;
        //printf("hash id = %d hash type = %u\n", hash_index, hash_type);
	}
    hash_index = ((hash_index >> 31) ^ hash_index) - (hash_index >> 31);
    //printf("<%d at %s\n", __LINE__, __FILE__);
    //printf("hash id = %d hash type = %u\n", hash_index, hash_type);
    for(flow_index=(hash_index % OFP_TABLE_SIZE);; flow_index++) {
        if (flow_index >= OFP_TABLE_SIZE)
            flow_index -= OFP_TABLE_SIZE;
        //printf("<%d flow index = %u\n", __LINE__, flow_index);
        if (hash_index - flow_index == 1)
            return FALSE;
        //printf("<%d flow index = %u\n", __LINE__, flow_index);
        if (flow_table[flow_index].table_flow_info.is_exist == TRUE) {
			if (flow_table[flow_index].table_flow_info.hash_type == hash_type) 
				return FALSE;
            continue;
		}
        //printf("<%d flow index = %u\n", __LINE__, flow_index);
        break;
    }
    //printf("<%d at %s\n", __LINE__, __FILE__);
    flow_table[flow_index].table_flow_info.buffer_id = flowmod_info.buffer_id;
    flow_table[flow_index].table_flow_info.cookie = flowmod_info.cookie;
    flow_table[flow_index].table_flow_info.priority = flowmod_info.priority;
    flow_table[flow_index].table_flow_info.hard_timeout = flowmod_info.hard_timeout;
	flow_table[flow_index].table_flow_info.idle_timeout = flowmod_info.idle_timeout;
    flow_table[flow_index].table_flow_info.out_group = flowmod_info.out_group;
    flow_table[flow_index].table_flow_info.out_port = flowmod_info.out_port;
    flow_table[flow_index].table_flow_info.table_id = flowmod_info.table_id;
    flow_table[flow_index].table_flow_info.hash_type = hash_type;
    flow_table[flow_index].table_flow_info.is_exist = TRUE;
    flow_table[flow_index].table_flow_info.is_tail = flowmod_info.is_tail;
    insert_flow_table(flowmod_info.match_info, flowmod_info.action_info, flow_index, tuple_mask, match_type);
    memcpy(&(flow_table[flow_index].flowmod_info), &flowmod_info, sizeof(flowmod_info_t));
    /* insert to tuple space search table */
    hash_index = hash_func(tuple_mask, 4) % OFP_TABLE_SIZE;
    for(int32_t i=hash_index;; i++) {
        if (i >= OFP_TABLE_SIZE)
            i -= OFP_TABLE_SIZE;
        if (tuple_table[i].is_exist == FALSE) {
            new_index = i;
            tuple_table[new_index].is_exist = TRUE;
            memcpy(tuple_table[new_index].match_bits, tuple_mask, 4);           
            break;
        }
        /* if the new rule is exist, then just insert the rule index to tuple space table */
        if (BYTES_CMP(tuple_table[i].match_bits, tuple_mask, 4) == TRUE) {
            new_index = i;
            break;
        }
        /* if tuple space table is full, then drop the new rule */
        if (hash_index - i == 1)
            return FALSE;
    }
    struct flow_entry_list **cur;
    for(cur=&(tuple_table[new_index].list); *cur&&flow_table[flow_index].table_flow_info.priority<=flow_table[(*cur)->entry_id].table_flow_info.priority; cur=&(*cur)->next);
    struct flow_entry_list *new_node = (struct flow_entry_list *)malloc(sizeof(struct flow_entry_list));
    new_node->entry_id = flow_index;
    new_node->next = *cur;
    *cur = new_node;

    return TRUE;
}

void init_tables(void) 
{
    memset(flow_table, 0, OFP_TABLE_SIZE * sizeof(flow_table_t));
    memset(tuple_table, 0, sizeof(tuple_table_t) * OFP_TABLE_SIZE);
}

void insert_flow_table(pkt_info_t match_info[], pkt_info_t action_info[], uint32_t flow_index, uint8_t tuple_mask[], __attribute__((unused)) uint16_t match_type)
{
    //int32_t in_port_index = -1, mac_index = -1, ip_index = -1, port_index = -1, action_index = -1;
    uint8_t in_port_amount = 0, out_port_amount = 0;
    //uint32_t pre_index = flow_index, next_index = 0;

    for(int i=0; i<20; i++) {
        //printf("match_info[i].type = %u\n", match_info[i].type);
        switch (match_info[i].type) {
        case PORT:
            if (flow_table[flow_index].table_in_port.is_exist == FALSE) {
                flow_table[flow_index].table_in_port.is_exist = TRUE;
            }
            flow_table[flow_index].table_in_port.in_port[in_port_amount++] = match_info[i].port_id;
            break;
        case DST_MAC:
            if (flow_table[flow_index].table_mac.is_exist == FALSE)
                flow_table[flow_index].table_mac.is_exist = TRUE;
            memcpy(flow_table[flow_index].table_mac.dst_mac, match_info[i].dst_mac, ETH_ALEN);
            flow_table[flow_index].table_mac.dst_mask = match_info[i].mask;
            flow_table[flow_index].table_mac.dst_mask_len = tuple_mask[0];
            break;
        case SRC_MAC:
            if (flow_table[flow_index].table_mac.is_exist == FALSE)
                flow_table[flow_index].table_mac.is_exist = TRUE;
            memcpy(flow_table[flow_index].table_mac.src_mac, match_info[i].src_mac, ETH_ALEN);
            flow_table[flow_index].table_mac.src_mask = match_info[i].mask;
            flow_table[flow_index].table_mac.src_mask_len = tuple_mask[1];
            break;
        case ETHER_TYPE:
            if (flow_table[flow_index].table_mac.is_exist == FALSE)
                flow_table[flow_index].table_mac.is_exist = TRUE;
            flow_table[flow_index].table_mac.eth_type = match_info[i].ether_type;
            break;
        case DST_IP:
            if (flow_table[flow_index].table_ip.is_exist == FALSE)
                flow_table[flow_index].table_ip.is_exist = TRUE;
            flow_table[flow_index].table_ip.dst_ip_addr = match_info[i].ip_dst;
            flow_table[flow_index].table_ip.dst_mask = match_info[i].mask;
            flow_table[flow_index].table_ip.dst_mask_len = tuple_mask[2];
            break;
        case SRC_IP:
            if (flow_table[flow_index].table_ip.is_exist == FALSE)
                flow_table[flow_index].table_ip.is_exist = TRUE;
            flow_table[flow_index].table_ip.src_ip_addr = match_info[i].ip_src;
            flow_table[flow_index].table_ip.src_mask = match_info[i].mask;
            flow_table[flow_index].table_ip.src_mask_len = tuple_mask[3];
            break;
        case IP_PROTO:
            if (flow_table[flow_index].table_ip.is_exist == FALSE)
                flow_table[flow_index].table_ip.is_exist = TRUE;
            flow_table[flow_index].table_ip.ip_proto = match_info[i].ip_proto;
            break;
        case DST_PORT:
            if (flow_table[flow_index].table_port.is_exist == FALSE)
                flow_table[flow_index].table_port.is_exist = TRUE;
            flow_table[flow_index].table_port.dst_ip_port = match_info[i].dst_port;
            break;
        case SRC_PORT:
            if (flow_table[flow_index].table_port.is_exist == FALSE)
                flow_table[flow_index].table_port.is_exist = TRUE;
            flow_table[flow_index].table_port.src_ip_port = match_info[i].src_port;
            break;
        default:
            ;
        }
        if (match_info[i].is_tail)
            break;
    }

    flow_table[flow_index].action_info.is_exist = TRUE;
    for(int i=0; i<20; i++) {
        switch (action_info[i].type) {
        case PORT:
            flow_table[flow_index].action_info.out_port[out_port_amount++].port = action_info[i].port_id;
            flow_table[flow_index].action_info.out_port[out_port_amount++].max_len = action_info[i].max_len;
            break;
        case DST_MAC:
            memcpy(flow_table[flow_index].action_info.dst_mac, action_info[i].dst_mac, ETH_ALEN);
            break;
        case SRC_MAC:
            memcpy(flow_table[flow_index].action_info.src_mac, action_info[i].src_mac, ETH_ALEN);
            break;
        case ETHER_TYPE:
            flow_table[flow_index].action_info.ether_type = action_info[i].ether_type;
            break;
        case DST_IP:
            flow_table[flow_index].action_info.ip_dst = action_info[i].ip_dst;
            break;
        case SRC_IP:
            flow_table[flow_index].action_info.ip_src = action_info[i].ip_src;
            break;
        case IP_PROTO:
            flow_table[flow_index].action_info.ip_proto = action_info[i].ip_proto;
            break;
        case DST_PORT:
            flow_table[flow_index].action_info.dst_port = action_info[i].dst_port;
            break;
        case SRC_PORT:
            flow_table[flow_index].action_info.src_port = action_info[i].src_port;
            break;
        default:
            ;
        }
        if (action_info[i].is_tail == TRUE)
            break;
    }

#if 0
    switch(match_type) {
        case 0:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = action_index;
            break;
        case 1:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = in_port_index;
            flow_table[match_type].table_in_port[in_port_index].next_table_index = action_index;
            break;
        case 2:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = mac_index;
            flow_table[match_type].table_mac[mac_index].next_table_index = action_index;
            break;
        case 3:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = in_port_index;
            flow_table[match_type].table_in_port[in_port_index].next_table_index = mac_index;
            flow_table[match_type].table_mac[mac_index].next_table_index = action_index;
            break;
        case 4:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = ip_index;
            flow_table[match_type].table_ip[ip_index].next_table_index = action_index;
            break;
        case 5:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = in_port_index;
            flow_table[match_type].table_in_port[in_port_index].next_table_index = ip_index;
            flow_table[match_type].table_ip[ip_index].next_table_index = action_index;
            break;
        case 6:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = mac_index;
            flow_table[match_type].table_mac[mac_index].next_table_index = ip_index;
            flow_table[match_type].table_ip[ip_index].next_table_index = action_index;
            break;
        case 7:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = in_port_index;
            flow_table[match_type].table_in_port[in_port_index].next_table_index = mac_index;
            flow_table[match_type].table_mac[mac_index].next_table_index = ip_index;
            flow_table[match_type].table_ip[ip_index].next_table_index = action_index;
            break;
        case 8:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = port_index;
            flow_table[match_type].table_port[port_index].next_table_index = action_index;
            break;
        case 9:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = in_port_index;
            flow_table[match_type].table_in_port[in_port_index].next_table_index = port_index;
            flow_table[match_type].table_port[port_index].next_table_index = action_index;
            break;
        case 10:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = mac_index;
            flow_table[match_type].table_mac[mac_index].next_table_index = port_index;
            flow_table[match_type].table_port[port_index].next_table_index = action_index;
            break;
        case 11:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = in_port_index;
            flow_table[match_type].table_in_port[in_port_index].next_table_index = mac_index;
            flow_table[match_type].table_mac[mac_index].next_table_index = port_index;
            flow_table[match_type].table_port[port_index].next_table_index = action_index;
            break;
        case 12:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = ip_index;
            flow_table[match_type].table_ip[ip_index].next_table_index = port_index;
            flow_table[match_type].table_port[port_index].next_table_index = action_index;
            break;
        case 13:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = in_port_index;
            flow_table[match_type].table_in_port[in_port_index].next_table_index = ip_index;
            flow_table[match_type].table_ip[ip_index].next_table_index = port_index;
            flow_table[match_type].table_port[port_index].next_table_index = action_index;
            break;
        case 14:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = mac_index;
            flow_table[match_type].table_mac[mac_index].next_table_index = ip_index;
            flow_table[match_type].table_ip[ip_index].next_table_index = port_index;
            flow_table[match_type].table_port[port_index].next_table_index = action_index;
            break;
        case 15:
            flow_table[match_type].table_flow_info[flow_index].next_table_index = in_port_index;
            flow_table[match_type].table_in_port[in_port_index].next_table_index = mac_index;
            flow_table[match_type].table_mac[mac_index].next_table_index = ip_index;
            flow_table[match_type].table_ip[ip_index].next_table_index = port_index;
            flow_table[match_type].table_port[port_index].next_table_index = action_index;
            break;
        default:
            ;
    }
#endif
}

void print_ofp_flow_table(void)
{
    int32_t flow_index;//, in_port_index, mac_index, ip_index, port_index, action_index;
    
    puts("ofp flow table: ");
    
    for(flow_index=0; flow_index<OFP_TABLE_SIZE; flow_index++) {
        if (flow_table[flow_index].table_flow_info.is_exist != TRUE)
            continue;
        printf("flow %d: cookie=%lx, priority=%u, ", flow_index, flow_table[flow_index].table_flow_info.cookie, flow_table[flow_index].table_flow_info.priority);
        printf("hard=%u, idle=%u, table id=%u, ", flow_table[flow_index].table_flow_info.hard_timeout, flow_table[flow_index].table_flow_info.idle_timeout, flow_table[flow_index].table_flow_info.table_id);
        printf("match field: ");
        if (flow_table[flow_index].table_in_port.is_exist)
            print_in_port(flow_table[flow_index].table_in_port);
        if (flow_table[flow_index].table_mac.is_exist)
            print_mac(flow_table[flow_index].table_mac);
        if (flow_table[flow_index].table_ip.is_exist)
            print_ip(flow_table[flow_index].table_ip);
        if (flow_table[flow_index].table_port.is_exist)
            print_port(flow_table[flow_index].table_port);
        printf("action field: ");
        print_action(flow_table[flow_index].action_info);
        puts("");
    }
}

int find_tuple_and_rule(tOFP_MBX *mail)
{
    uint8_t dst_mac_mask, src_mac_mask, src_ip_mask, dst_ip_mask;
    //U16			mulen;
	U8			*mu;
	tany2ofp_MSG *msg;
    int32_t cur_entry_id, flow_entry_id = -1;
    uint32_t in_port_id;
    uint16_t cur_priority_id = 0;

	msg = (tany2ofp_MSG *)(mail->refp);
	//ofp_ports[0].sockfd = msg->sockfd;
	mu = (U8 *)(((tDP_MSG *)(msg->buffer))->buffer);
	//mulen = ((tDP_MSG *)(msg->buffer))->len;
    in_port_id = ((tDP_MSG *)(msg->buffer))->port_no;

    for(int i=0; i<OFP_TABLE_SIZE; i++) {
        if (tuple_table[i].is_exist == FALSE)
            continue;
        dst_mac_mask = tuple_table[i].match_bits[0];
        src_mac_mask = tuple_table[i].match_bits[1];
        src_ip_mask = tuple_table[i].match_bits[2];
        dst_ip_mask = tuple_table[i].match_bits[3];
        for(struct flow_entry_list *cur=tuple_table[i].list; cur!=NULL; cur=cur->next) {
            cur_entry_id = cur->entry_id;
            if (flow_table[cur_entry_id].table_in_port.is_exist && flow_table[cur_entry_id].table_in_port.in_port[0] != in_port_id)
                continue;
            if (flow_table[cur_entry_id].table_mac.is_exist) {
                if (BYTES_CMP(((struct ethhdr *)mu)->h_dest, flow_table[cur_entry_id].table_mac.dst_mac, dst_mac_mask) == FALSE)
                    continue;
                if (BYTES_CMP(((struct ethhdr *)mu)->h_source, flow_table[cur_entry_id].table_mac.src_mac, src_mac_mask) == FALSE)
                    continue;
                if (flow_table[cur_entry_id].table_mac.eth_type != 0 && flow_table[cur_entry_id].table_mac.eth_type != ntohs(((struct ethhdr *)mu)->h_proto))
                    continue;
            }
            if (flow_table[cur_entry_id].table_ip.is_exist) {
                if (BYTES_CMP((U8 *)&(((struct iphdr *)(((struct ethhdr *)mu) + 1))->saddr), (U8 *)&(flow_table[cur_entry_id].table_ip.src_ip_addr), src_ip_mask) == FALSE)
                    continue;
                if (BYTES_CMP((U8 *)&(((struct iphdr *)(((struct ethhdr *)mu) + 1))->daddr), (U8 *)&(flow_table[cur_entry_id].table_ip.dst_ip_addr), dst_ip_mask) == FALSE)
                    continue;
                if (flow_table[cur_entry_id].table_ip.ip_proto != 0 && flow_table[cur_entry_id].table_ip.ip_proto != ((struct iphdr *)(((struct ethhdr *)mu) + 1))->protocol)
                    continue;
            }
            if (flow_table[cur_entry_id].table_port.is_exist) {               
                if (flow_table[cur_entry_id].table_port.src_ip_port != 0 && flow_table[cur_entry_id].table_port.src_ip_port != ntohs(*(uint16_t *)(mu + 34)))
                    continue;
                if (flow_table[cur_entry_id].table_port.dst_ip_port != 0 && flow_table[cur_entry_id].table_port.dst_ip_port != ntohs(*(uint16_t *)(mu + 36)))
                    continue;
            }
            if (flow_table[cur_entry_id].table_flow_info.priority >= cur_priority_id)
                flow_entry_id = cur_entry_id;
            if (flow_table[cur_entry_id].action_info.out_port[0].port == OFPP_CONTROLLER)
                flow_entry_id = -1;
        }
    }
    return flow_entry_id;
}

STATUS send_back2dp(int flow_entry_id)
{
    tOFP_MBX mail;

    flow_table[flow_entry_id].flowmod_info.command = OFPFC_ADD;
    flow_table[flow_entry_id].flowmod_info.msg_type = FLOWMOD;
    flow_table[flow_entry_id].flowmod_info.msg_len = sizeof(flowmod_info_t);

    if (dpQid == -1) {
		if ((dpQid=msgget(DP_Q_KEY,0600|IPC_CREAT)) < 0) {
			printf("send> Oops! dpQ(key=0x%x) not found\n",DP_Q_KEY);
   	 	}
	}

	mail.len = sizeof(flowmod_info_t);
	memcpy(mail.refp, &(flow_table[flow_entry_id].flowmod_info), mail.len);
	//printf("dp_send2mailbox(dp_sock.c %d): mulen=%d\n",__LINE__,mulen);
	mail.type = IPC_EV_TYPE_OFP;
	ipc_sw(dpQid, &mail, sizeof(mail), -1);
	//printf("send msg to dp\n");

	return TRUE;
}