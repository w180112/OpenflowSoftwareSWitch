/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFP_CODEC.C :

  Designed by THE on SEP 16, 2019
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include 		<common.h>
#include		"ofpd.h"
#include		"ofp_codec.h"
#include        "ofp_fsm.h"
#include        "ofp_dbg.h"
#include 		"ofp_sock.h"
#include 		"ofp_asyn.h"
#include 		"ofp_ctrl2sw.h"
#include 		"ofp_oxm.h"
#include		"dp.h"
#include 		<unistd.h>
#include 		<signal.h>

void OFP_encode_packet_in(tOFP_MBX *mail, tOFP_PORT *port_ccb);
STATUS OFP_decode_flowmod(tOFP_PORT *port_ccb, U8 *mu, U16 mulen);
STATUS insert_node(host_learn_t **head, host_learn_t *node);
host_learn_t *find_node(host_learn_t *head, uint32_t buffer_id);
STATUS ip_hdr_init(tIP_PKT *ip_hdr, uint32_t src_ip, uint32_t dst_ip);
STATUS udp_hdr_init(tUDP_PKT *udp_hdr, uint8_t *payload);

extern pid_t ofp_cp_pid;
extern pid_t ofp_dp_pid;
extern pid_t tmr_pid;

/*============================ DECODE ===============================*/

/*****************************************************
 * OFP_decode_frame
 * 
 * input : pArg - mail.param
 * output: imsg, event
 * return: session ccb
 *****************************************************/
STATUS OFP_decode_frame(tOFP_MBX *mail, tOFP_PORT *port_ccb)
{
    U16	mulen;
	U8	*mu;
	tOFP_MSG *msg;
	
	if (mail->len > ETH_MTU) {
	    DBG_OFP(DBGLVL1,0,"error! too large frame(%d)\n",mail->len);
	    return ERROR;
	}

	msg = (tOFP_MSG *)(mail->refp);
	port_ccb->sockfd = msg->sockfd;
	mu = (U8 *)(msg->buffer);
	mulen = (mail->len) - (sizeof(int) + 1);
	
	if (msg->type == DRIV_FAIL) {
		kill(tmr_pid,SIGTERM);
        kill(ofp_cp_pid,SIGTERM);
        //kill(ofp_dp_pid,SIGINT);
		//kill(getpid(),SIGINT);
		return FALSE;
	}
    //PRINT_MESSAGE(mu,mulen);
	switch(((ofp_header_t *)mu)->type) {
	case OFPT_HELLO:
		printf("----------------------------------\nrecv hello msg\n");
		port_ccb->event = E_RECV_HELLO;
		memcpy(&(port_ccb->ofp_header),mu,sizeof(ofp_header_t));
		break;
	case OFPT_FEATURES_REQUEST:
		printf("----------------------------------\nrecv feature request\n");
		port_ccb->event = E_FEATURE_REQUEST;
		memcpy(&(port_ccb->ofp_header),mu,sizeof(ofp_header_t));
		break;
	case OFPT_MULTIPART_REQUEST:
		printf("----------------------------------\nrecv multipart\n");
		port_ccb->event = E_MULTIPART_REQUEST;
		memcpy(&(port_ccb->ofp_multipart),mu,sizeof(ofp_multipart_t));
		break;
	case OFPT_ECHO_REPLY:
		port_ccb->event = E_ECHO_REPLY;
		//printf("----------------------------------\nrecv echo reply\n");
		break;
	case OFPT_FLOW_MOD:
		port_ccb->event = E_FLOW_MOD;
		printf("----------------------------------\nrecv flow mod\n");
		OFP_decode_flowmod(port_ccb, mu, mulen);
		//PRINT_MESSAGE(mu, mulen);
		break;
	case OFPT_PACKET_OUT:
		port_ccb->event = E_PACKET_OUT;
		printf("----------------------------------\nrecv packet out\n");
		PRINT_MESSAGE(mu, mulen);
		break;
	default:
		break;
	}
	
	return TRUE;
}

/*============================== ENCODING ===============================*/
void OFP_encode_packet_in(tOFP_MBX *mail, tOFP_PORT *port_ccb) {
	U16	mulen;
	U8	*mu;
	tDP2OFP_MSG *msg;
	int buffer_id;

	msg = (tDP2OFP_MSG *)(mail->refp);
	//ofp_ports[0].sockfd = msg->sockfd;
	mu = (U8 *)(((tDP_MSG *)(msg->buffer))->buffer);
	mulen = (mail->len) - sizeof(int) - sizeof(uint16_t) - sizeof(int);
	//printf("mlen = %u\n", mulen);
	//PRINT_MESSAGE(mu, mulen);
	buffer_id = msg->id;

	uint16_t ofp_match_length = 0; 
	uint32_t port_no = htonl(((tDP_MSG *)(msg->buffer))->port_no);

	port_ccb->ofp_packet_in.header.version = OFP13_VERSION;
	port_ccb->ofp_packet_in.header.type = OFPT_PACKET_IN;
	uint16_t length = mulen + sizeof(ofp_packet_in_t) + 4/*pad*/ + 2/*pad*/ + sizeof(uint32_t);/*port no.*/
	port_ccb->ofp_packet_in.header.xid = 0x0;

	port_ccb->ofp_packet_in.buffer_id = htonl(buffer_id);
	port_ccb->ofp_packet_in.total_len = htons(mulen);
	port_ccb->ofp_packet_in.reason = OFPR_NO_MATCH;
	port_ccb->ofp_packet_in.table_id = 0;
	port_ccb->ofp_packet_in.cookie = 0x00000000;

	port_ccb->ofp_packet_in.match.type = htons(OFPMT_OXM);
	port_ccb->ofp_packet_in.match.oxm_header.oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
	port_ccb->ofp_packet_in.match.oxm_header.oxm_union.oxm_struct.oxm_field = OFPXMT_OFB_IN_PORT;
	port_ccb->ofp_packet_in.match.oxm_header.oxm_union.oxm_struct.oxm_hasmask = 0;
	port_ccb->ofp_packet_in.match.oxm_header.oxm_union.oxm_struct.oxm_length = sizeof(uint32_t);
	// align to 16 bytes
	ofp_match_length = sizeof(struct ofp_match) + port_ccb->ofp_packet_in.match.oxm_header.oxm_union.oxm_struct.oxm_length;
	port_ccb->ofp_packet_in.match.oxm_header.oxm_union.oxm_value = htons(port_ccb->ofp_packet_in.match.oxm_header.oxm_union.oxm_value);
	port_ccb->ofp_packet_in.match.length = htons(ofp_match_length);
	port_ccb->ofp_packet_in.header.length = htons(length);

	memset(port_ccb->ofpbuf,0,ETH_MTU);
	memcpy(port_ccb->ofpbuf,&(port_ccb->ofp_packet_in),sizeof(ofp_packet_in_t));
	memcpy(port_ccb->ofpbuf+sizeof(ofp_packet_in_t),&port_no,sizeof(uint32_t));
	memcpy(port_ccb->ofpbuf+sizeof(ofp_packet_in_t)+sizeof(uint32_t)+6,mu,mulen);
	port_ccb->ofpbuf_len = mulen + sizeof(ofp_packet_in_t) + 2 + 4 + sizeof(uint32_t);
	printf("----------------------------------\nencode packet in\n");

#if 0 
	// use linked list instead
	host_learn_t *node = malloc(sizeof(host_learn_t));
	memcpy(node->dst_mac,mu,MAC_ADDR_LEN);
	memcpy(node->src_mac,mu+MAC_ADDR_LEN,MAC_ADDR_LEN);
	node->src_ip = *((uint32_t *)(mu + ETH_HDR_LEN + ETH_TYPE_LEN + 12/* IP header except IP */));
	node->dst_ip = *((uint32_t *)(mu + ETH_HDR_LEN + ETH_TYPE_LEN + 12/* IP header except IP */ + IP_ADDR_LEN));
	node->buffer_id = buffer_id;
	node->next = NULL;
	insert_node(&(port_ccb->head),node);
	buffer_id++;
#endif
}

STATUS OFP_decode_flowmod(tOFP_PORT *port_ccb, U8 *mu, U16 mulen) 
{
	ofp_instruction_actions_t *ofp_instruction_actions;

	port_ccb->flowmod_info.msg_type = FLOWMOD;
	port_ccb->flowmod_info.msg_len = sizeof(flowmod_info_t);
	port_ccb->flowmod_info.buffer_id = htonl(((ofp_flow_mod_t *)mu)->buffer_id); 
	port_ccb->flowmod_info.command = ((ofp_flow_mod_t *)mu)->command;
	port_ccb->flowmod_info.cookie = bitswap64(((ofp_flow_mod_t *)mu)->cookie);
	port_ccb->flowmod_info.hard_timeout = htons(((ofp_flow_mod_t *)mu)->hard_timeout);
	port_ccb->flowmod_info.idle_timeout = htons(((ofp_flow_mod_t *)mu)->idle_timeout);
	port_ccb->flowmod_info.out_group = htonl(((ofp_flow_mod_t *)mu)->out_group);
	port_ccb->flowmod_info.out_port = htonl(((ofp_flow_mod_t *)mu)->out_port);
	port_ccb->flowmod_info.priority = htons(((ofp_flow_mod_t *)mu)->priority);
	port_ccb->flowmod_info.table_id = ((ofp_flow_mod_t *)mu)->table_id;

	//PRINT_MESSAGE((unsigned char *)&(((ofp_flow_mod_t *)mu)->match), 32);
	uint16_t match_len = ntohs(((ofp_flow_mod_t *)mu)->match.length) - (sizeof(((ofp_flow_mod_t *)mu)->match.length) + sizeof(((ofp_flow_mod_t *)mu)->match.type));
	if (match_len < 8) {
		/* This means match field in flowmod is empty, and we should align to 8 bits */
		ofp_instruction_actions = (ofp_instruction_actions_t *)(mu + sizeof(ofp_flow_mod_t));
		port_ccb->flowmod_info.is_tail = TRUE;
	}
	else {
		int i = 0;
		//PRINT_MESSAGE(&(((ofp_flow_mod_t *)mu)->match.oxm_header), match_len);
		for(ofp_oxm_header_t *cur = &(((ofp_flow_mod_t *)mu)->match.oxm_header); match_len>0; i++) {
			if (i > 20) {
				puts("reach max match field limit");
				break;
			}
			cur->oxm_union.oxm_value = htons(cur->oxm_union.oxm_value);
			switch (cur->oxm_union.oxm_struct.oxm_field) {
			case OFPXMT_OFB_IN_PORT:
				port_ccb->flowmod_info.match_info[i].port_id = htonl(*(uint32_t *)((U8 *)cur + sizeof(ofp_oxm_header_t)));
				port_ccb->flowmod_info.match_info[i].type = PORT;
				printf("match port id = %u\n", port_ccb->flowmod_info.match_info[i].port_id);
				break;
			case OFPXMT_OFB_ETH_DST:
				memcpy(port_ccb->flowmod_info.match_info[i].dst_mac, (U8 *)cur + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->flowmod_info.match_info[i].type = DST_MAC;
				break;
			case OFPXMT_OFB_ETH_SRC:
				memcpy(port_ccb->flowmod_info.match_info[i].src_mac, (U8 *)cur + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->flowmod_info.match_info[i].type = SRC_MAC;
				break;
			case OFPXMT_OFB_ETH_TYPE:
				port_ccb->flowmod_info.match_info[i].ether_type = htons(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = ETHER_TYPE;
				break;
			case OFPXMT_OFB_IP_PROTO:
				port_ccb->flowmod_info.match_info[i].ip_proto = *(U8 *)((U8 *)cur + sizeof(ofp_oxm_header_t));
				port_ccb->flowmod_info.match_info[i].type = IP_PROTO;
				break;
			case OFPXMT_OFB_IPV4_SRC:
				port_ccb->flowmod_info.match_info[i].ip_src = htonl(*((uint32_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = SRC_IP;
				printf("match ip src = %u\n", port_ccb->flowmod_info.match_info[i].ip_src);
				break;
			case OFPXMT_OFB_IPV4_DST:
				port_ccb->flowmod_info.match_info[i].ip_dst = htonl(*((uint32_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = DST_IP;
				break;
			case OFPXMT_OFB_TCP_SRC:
				port_ccb->flowmod_info.match_info[i].src_port = htonl(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_TCP_DST:
				port_ccb->flowmod_info.match_info[i].dst_port = htonl(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = DST_PORT;
				break;
			case OFPXMT_OFB_UDP_SRC:
				port_ccb->flowmod_info.match_info[i].src_port = htonl(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_UDP_DST:
				port_ccb->flowmod_info.match_info[i].dst_port = htonl(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = DST_PORT;
				break;
			default:
				return FALSE;
			}
			match_len = match_len - cur->oxm_union.oxm_struct.oxm_length - sizeof(ofp_oxm_header_t);
			cur = (ofp_oxm_header_t *)(((U8 *)cur) + cur->oxm_union.oxm_struct.oxm_length + sizeof(ofp_oxm_header_t));
		}
		port_ccb->flowmod_info.match_info[i].is_tail = TRUE;
	}
	uint16_t padding, tmp;
	match_len = ntohs(((ofp_flow_mod_t *)mu)->match.length);
	if ((tmp = ((match_len + sizeof(((ofp_flow_mod_t *)mu)->match.length) + sizeof(((ofp_flow_mod_t *)mu)->match.type)) << 13)) > 0)
		padding = (((match_len + sizeof(((ofp_flow_mod_t *)mu)->match.length) + sizeof(((ofp_flow_mod_t *)mu)->match.type)) >> 3) + 1) << 3;
	else
		padding = ((match_len + sizeof(((ofp_flow_mod_t *)mu)->match.length) + sizeof(((ofp_flow_mod_t *)mu)->match.type)) >> 3) << 3;
	ofp_instruction_actions = (ofp_instruction_actions_t *)(mu + sizeof(ofp_flow_mod_t) - sizeof(struct ofp_match) + padding);
	ofp_action_header_t *ofp_action_header = (ofp_action_header_t *)(((U8 *)ofp_instruction_actions) + sizeof(ofp_instruction_actions_t));
	ofp_oxm_header_t *ofp_oxm_header = (ofp_oxm_header_t *)(ofp_action_header->pad);
	uint16_t action_len = htons(ofp_instruction_actions->len) - sizeof(ofp_instruction_actions_t);
	//PRINT_MESSAGE(ofp_instruction_actions, 16);
	uint16_t len = 0;
	int i = 0;
	memset(port_ccb->flowmod_info.action_info, 0, 20*sizeof(pkt_info_t));
	//pkt_info_t *head_action_info = NULL, *cur_action_info;
	for(U8 *cur=(U8 *)ofp_action_header;; len+=htons(((ofp_action_header_t *)cur)->len),cur+=len,i++) {
		if (i == 20) {
			puts("Reach max flowmod action list, drop remaining actions");
			break;
		}
		//printf("total len = %u %u\n", len, action_len);
		if (len == action_len)
			break;
		if (((ofp_action_header_t *)cur)->type == htons(OFPAT_SET_FIELD)) {
			//pkt_info_t *new_action = (pkt_info_t *)malloc(sizeof(pkt_info_t));
			ofp_action_set_field_t *ofp_action_set_field = (ofp_action_set_field_t *)cur;
			ofp_oxm_header->oxm_union.oxm_value = htons(ofp_oxm_header->oxm_union.oxm_value);
			switch (ofp_oxm_header->oxm_union.oxm_struct.oxm_field) {
			case OFPXMT_OFB_IN_PORT:
				port_ccb->flowmod_info.action_info[i].port_id = htonl(*(uint32_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t)));
				port_ccb->flowmod_info.action_info[i].type = PORT;
				break;
			case OFPXMT_OFB_ETH_DST:
				memcpy(port_ccb->flowmod_info.action_info[i].dst_mac, ((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->flowmod_info.action_info[i].type = DST_MAC;
				break;
			case OFPXMT_OFB_ETH_SRC:
				memcpy(port_ccb->flowmod_info.action_info[i].src_mac, ((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->flowmod_info.action_info[i].type = SRC_MAC;
				break;
			case OFPXMT_OFB_ETH_TYPE:
				port_ccb->flowmod_info.action_info[i].ether_type = htons(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = ETHER_TYPE;
				break;
			case OFPXMT_OFB_IP_PROTO:
				port_ccb->flowmod_info.action_info[i].ip_proto = *(U8 *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t));
				port_ccb->flowmod_info.action_info[i].type = IP_PROTO;
				break;
			case OFPXMT_OFB_IPV4_SRC:
				port_ccb->flowmod_info.action_info[i].ip_src = htonl(*((uint32_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = SRC_IP;
				break;
			case OFPXMT_OFB_IPV4_DST:
				port_ccb->flowmod_info.action_info[i].ip_dst = htonl(*((uint32_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = DST_IP;
				printf("action ip dst = %u\n", port_ccb->flowmod_info.action_info[i].ip_dst);
				break;
			case OFPXMT_OFB_TCP_SRC:
				port_ccb->flowmod_info.action_info[i].src_port = htonl(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_TCP_DST:
				port_ccb->flowmod_info.action_info[i].dst_port = htonl(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = DST_PORT;
				break;
			case OFPXMT_OFB_UDP_SRC:
				port_ccb->flowmod_info.action_info[i].src_port = htonl(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_UDP_DST:
				port_ccb->flowmod_info.action_info[i].dst_port = htonl(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = DST_PORT;
				break;
			default:
				break;
			}
			/*if (head_action_info == NULL) {
				head_action_info = new_action;
				cur_action_info = head_action_info;
			}
			else {
				cur_action_info->next = new_action;
				cur_action_info = cur_action_info->next;
			}*/
		}
		else if(((ofp_action_header_t *)cur)->type == htons(OFPAT_OUTPUT)) {
			//pkt_info_t *new_action = (pkt_info_t *)malloc(sizeof(pkt_info_t));
			ofp_action_output_t *ofp_action_output = (ofp_action_output_t *)cur;
			port_ccb->flowmod_info.action_info[i].max_len = htons(ofp_action_output->max_len);
			port_ccb->flowmod_info.action_info[i].port_id = htonl(ofp_action_output->port);
			printf("action port id = %u\n", port_ccb->flowmod_info.action_info[i].port_id);
			/*if (head_action_info == NULL) {
				head_action_info = new_action;
				cur_action_info = head_action_info;
			}
			else {
				cur_action_info->next = new_action;
				cur_action_info = cur_action_info->next;
			}*/
		}
		else {
			printf("action type = %u\n", ntohs(((ofp_action_header_t *)cur)->type));
			puts("Unsupported flowmod action");
		}
	}
	//port_ccb->flowmod_info.action_info = head_action_info;
	//printf("addr = %x\n", port_ccb->flowmod_info.action_info);
	//printf("flowmod_info action len = %u\n", port_ccb->flowmod_info.action_info[i]max_len);
	port_ccb->flowmod_info.action_info[i].is_tail = TRUE;
	PRINT_MESSAGE(port_ccb->flowmod_info.match_info,sizeof(pkt_info_t)*20);
	return TRUE;
}

STATUS insert_node(host_learn_t **head, host_learn_t *node)
{
	host_learn_t *cur;

	if (*head == NULL) {
		*head = node;
	}
	for(cur=*head; cur->next!=NULL; cur=cur->next);
	cur->next = node;
	return TRUE;
}

host_learn_t *find_node(host_learn_t *head, uint32_t buffer_id)
{
	host_learn_t *prev = NULL, *cur = head;
	for(;;prev=cur, cur=cur->next) {
		if (cur == NULL)
			return NULL;
		if (cur->buffer_id == buffer_id) {
			if (prev != NULL)
				prev->next = cur->next;
			return cur; 
		}
	}
}

STATUS ip_hdr_init(tIP_PKT *ip_hdr, uint32_t src_ip, uint32_t dst_ip)
{
	ip_hdr->ver_ihl.ver = 4;
	ip_hdr->ver_ihl.IHL = 5;
	ip_hdr->tos = 0;
	ip_hdr->total_len = 36;
	ip_hdr->id = 1;
	ip_hdr->flag_frag.flag = 2;
	ip_hdr->flag_frag.frag_off = 0;
	ip_hdr->ttl = 64;
	ip_hdr->proto = PROTO_TYPE_UDP;
	memcpy(ip_hdr->cSA,&dst_ip,IP_ADDR_LEN);
	memcpy(ip_hdr->cDA,&src_ip,IP_ADDR_LEN);

	return TRUE;
}

STATUS udp_hdr_init(tUDP_PKT *udp_hdr, uint8_t *payload)
{
	udp_hdr->src = 6655;
	udp_hdr->dst = 8000;
	udp_hdr->len = 16;
	udp_hdr->data = payload;
	return TRUE;
}

#if 0
/*************************************************************************
 * OFP_encode_frame
 *
 * input : 
 *         omsg - ofp msg structure
 * output: mu - for buffer encoded message
 *         mulen - total omsg packet length after encoding.
 *************************************************************************/
void OFP_encode_hello(tOFP_MSG *omsg, U8 *mu, U16 *mulen)
{
	U8	*mp,i;
	
	mp = mu;
	memcpy(mp,ofp_da_mac,MAC_ADDR_LEN); /* from 2nd to 1st */
	mp += MAC_ADDR_LEN; /* DA */

	memcpy(mp,g_loc_mac,MAC_ADDR_LEN);
	mp += MAC_ADDR_LEN; /* SA = ??? */
	
	#if 0
	mp = ENCODE_U16(mp,0x8100); /* pre-2-bytes of VLAN TAG */
	mp = ENCODE_U16(mp,vid); /* post-2-bytes of VLAN TAG */
	#endif
	
	//ethernet type
	mp = ENCODE_U16(mp,(U16)ETH_TYPE_OFP);

	//ofp message	
	mp += ENCODE_OFP_TLVs(omsg->tlvs, mp);
	
	*mulen = mp - mu;
	if (*mulen < MIN_FRAME_SIZE){
		for(i=*mulen; i<MIN_FRAME_SIZE; i++)  *mp++ = 0;
		*mulen = MIN_FRAME_SIZE;  
	}
	//PRINT_MESSAGE(mu,*mulen);
}
#endif
