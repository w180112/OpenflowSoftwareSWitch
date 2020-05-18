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
#include 		"ofp_cmd.h"
#include		"dp.h"
#include 		<unistd.h>
#include 		<signal.h>

void 	OFP_encode_packet_in(tOFP_MBX *mail, tOFP_PORT *port_ccb);
STATUS 	OFP_decode_flowmod(tOFP_PORT *port_ccb, U8 *mu, U16 mulen);
STATUS 	OFP_decode_packet_out(tOFP_PORT *port_ccb, U8 *mu, U16 mulen);
STATUS OFP_encode_multipart_reply(tOFP_PORT *port_ccb, U8 *mu, U16 mulen);

extern pid_t ofp_cp_pid;
extern pid_t ofp_dp_pid;
extern pid_t tmr_pid;

port_status_t *port_status_head;

U8 			tmp_buf[16384];
U8 			*tmp_buf_ptr = tmp_buf;

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
    U16			mulen;
	U8			*mu;
	tOFP_MSG 	*msg;

	msg = (tOFP_MSG *)(mail->refp);
	port_ccb->sockfd = msg->sockfd;
	mu = (U8 *)(msg->buffer);
	mulen = (mail->len) - (sizeof(int) + 1);
	
	if (mulen > ETH_MTU) {
	    DBG_OFP(DBGLVL1,0,"error! too large frame(%d)\n",mail->len);
	    return ERROR;
	}

	if (msg->type == DRIV_FAIL) {
		kill(tmr_pid,SIGTERM);
        kill(ofp_cp_pid,SIGTERM);
        //kill(ofp_dp_pid,SIGINT);
		//kill(getpid(),SIGINT);
		return FALSE;
	}
    //PRINT_MESSAGE(mu,mulen);
	memcpy(tmp_buf_ptr,mu,mulen);
	tmp_buf_ptr += mulen;
	//PRINT_MESSAGE(tmp_buf,tmp_buf_ptr - tmp_buf);
	for(U16 ofp_len=0; ofp_len<(tmp_buf_ptr-tmp_buf);) {
		//printf("ofp_len = %u, tmp_buf_ptr-tmp_buf = %u\n", ofp_len, tmp_buf_ptr-tmp_buf);
		switch(((ofp_header_t *)(tmp_buf+ofp_len))->type) {
		case OFPT_HELLO:
			printf("----------------------------------\nrecv hello msg\n");
			port_ccb->event = E_RECV_HELLO;
			memcpy(&(port_ccb->ofp_header),tmp_buf+ofp_len,sizeof(ofp_header_t));
			break;
		case OFPT_FEATURES_REQUEST:
			printf("----------------------------------\nrecv feature request\n");
			port_ccb->event = E_FEATURE_REQUEST;
			memcpy(&(port_ccb->ofp_header),tmp_buf+ofp_len,sizeof(ofp_header_t));
			break;
		case OFPT_MULTIPART_REQUEST:
			printf("----------------------------------\nrecv multipart\n");
			port_ccb->event = E_MULTIPART_REQUEST;
			OFP_encode_multipart_reply(port_ccb, tmp_buf+ofp_len, ntohs(((ofp_header_t *)(tmp_buf+ofp_len))->length));
			break;
		case OFPT_ECHO_REPLY:
			port_ccb->event = E_ECHO_REPLY;
			break;
		case OFPT_FLOW_MOD:
			port_ccb->event = E_FLOW_MOD;
			//PRINT_MESSAGE(tmp_buf+ofp_len, mulen);
			//printf("----------------------------------\nrecv flow mod\n");
			OFP_decode_flowmod(port_ccb, tmp_buf+ofp_len, ntohs(((ofp_header_t *)(tmp_buf+ofp_len))->length));
			//printf("<%d at ofp_codec.c\n", __LINE__);
			break;
		case OFPT_PACKET_OUT:
			port_ccb->event = E_PACKET_OUT;
			//printf("----------------------------------\nrecv packet out\n");
			//PRINT_MESSAGE(mu, mulen);
			OFP_decode_packet_out(port_ccb, tmp_buf+ofp_len, ntohs(((ofp_header_t *)(tmp_buf+ofp_len))->length));
			break;
		default:
			port_ccb->event = E_OTHERS;
			break;
		}
		ofp_len += ntohs(((ofp_header_t *)(tmp_buf+ofp_len))->length);
		OFP_FSM(port_ccb, port_ccb->event);
		//printf("<%d at ofp_codec.c\n", __LINE__);
		if ((tmp_buf_ptr - tmp_buf - ofp_len) < 4 || (tmp_buf_ptr - tmp_buf - ofp_len) < ntohs(((ofp_header_t *)(tmp_buf+ofp_len))->length)) {
			memcpy(tmp_buf,tmp_buf+ofp_len,tmp_buf_ptr-tmp_buf-ofp_len);
			tmp_buf_ptr = tmp_buf + (tmp_buf_ptr - tmp_buf - ofp_len);
			break;
		}
	}
	
	return TRUE;
}

/*============================== ENCODING ===============================*/
void OFP_encode_packet_in(tOFP_MBX *mail, tOFP_PORT *port_ccb)
{
	U16			mulen;
	U8			*mu;
	tDP2OFP_MSG *msg;
	int 		buffer_id;

	msg = (tDP2OFP_MSG *)(mail->refp);
	//ofp_ports[0].sockfd = msg->sockfd;
	mu = (U8 *)(((tDP_MSG *)(msg->buffer))->buffer);
	mulen = ((tDP_MSG *)(msg->buffer))->len;
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

	memset(port_ccb->ofpbuf,0,MSG_LEN);
	memcpy(port_ccb->ofpbuf,&(port_ccb->ofp_packet_in),sizeof(ofp_packet_in_t));
	memcpy(port_ccb->ofpbuf+sizeof(ofp_packet_in_t),&port_no,sizeof(uint32_t));
	memcpy(port_ccb->ofpbuf+sizeof(ofp_packet_in_t)+sizeof(uint32_t)+6,mu,mulen);
	port_ccb->ofpbuf_len = mulen + sizeof(ofp_packet_in_t) + 2 + 4 + sizeof(uint32_t);
	//printf("----------------------------------\nencode packet in\n");
}

STATUS OFP_encode_port_status(tOFP_MBX *mail, tOFP_PORT *port_ccb, uint32_t *port)
{
	ofp_port_status_t *ofp_port_status = (ofp_port_status_t *)port_ccb->ofpbuf;
	int32_t port_id, speed, duplex;
	int fd;
    struct ifreq ifr;

	ofp_port_status->header.version = 0x4;
	ofp_port_status->header.type = OFPT_PORT_STATUS;
	ofp_port_status->header.length = htons(sizeof(ofp_port_status_t));
	ofp_port_status->header.xid = 0;
	cli_2_ofp_t *cli_2_ofp = (cli_2_ofp_t *)(mail->refp);
	switch (cli_2_ofp->opcode) {
	case ADD_IF:
		ofp_port_status->reason = OFPPR_ADD;

    	fd = socket(AF_INET, SOCK_DGRAM, 0);
 		ifr.ifr_addr.sa_family = AF_INET;
    	strncpy(ifr.ifr_name,cli_2_ofp->ifname,IFNAMSIZ-1);
		
 		ioctl(fd,SIOCGIFHWADDR,&ifr);
    	close(fd);
    	memcpy(ofp_port_status->desc.hw_addr,(unsigned char *)ifr.ifr_hwaddr.sa_data,OFP_ETH_ALEN);
		strcpy(ofp_port_status->desc.name,cli_2_ofp->ifname);
		//ofp_port_status->desc.config = OFPPC_PORT_DOWN | OFPPC_NO_RECV | OFPPC_NO_FWD | OFPPC_NO_PACKET_IN;
		ofp_port_status->desc.config = 0;//htonl(ofp_port_status->desc.config);
		//ofp_port_status->desc.state = OFPPS_LINK_DOWN | OFPPS_BLOCKED | OFPPS_LIVE;
		ofp_port_status->desc.state = 0;//htonl(ofp_port_status->desc.state);
		if (ethernet_interface(cli_2_ofp->ifname, &port_id, &speed, &duplex))
			return ERROR;
		if (port_id == -1)
            printf("%s: (no interface index)", cli_2_ofp->ifname);
        else {
			ofp_port_status->desc.port_no = htonl((uint32_t)port_id);
			*port = port_id;
		}
		if (speed == -1)
            printf(", unknown bandwidth");
        else {
			int i;
			for(i=0; speed/10>10; i++) {
				speed = speed / 10;
			}
			if (duplex >= 0) {
				ofp_port_status->desc.curr = OFPPF_10MB_HD << (2*i);
				for (int j=0; j<=i; j++) {
					ofp_port_status->desc.supported = ofp_port_status->desc.supported | OFPPF_10MB_HD << (2*j);
				}
				if (duplex == 1) {
					ofp_port_status->desc.curr = ofp_port_status->desc.curr << 1;
					for (int j=0; j<=i; j++) {
						ofp_port_status->desc.supported = ofp_port_status->desc.supported | OFPPF_10MB_HD << (2*j+1);
					}
				}
			}
		}
		//TODO: detect autonegotiation, advertisement
		ofp_port_status->desc.curr = ofp_port_status->desc.curr | OFPPF_AUTONEG;
		ofp_port_status->desc.supported = ofp_port_status->desc.supported | OFPPF_AUTONEG;
		ofp_port_status->desc.supported = ofp_port_status->desc.supported | OFPPF_PAUSE;
		ofp_port_status->desc.supported = ofp_port_status->desc.supported | OFPPF_PAUSE_ASYM;
		ofp_port_status->desc.curr = htonl(ofp_port_status->desc.curr);
		ofp_port_status->desc.supported = htonl(ofp_port_status->desc.supported);
		ofp_port_status->desc.advertised = ofp_port_status->desc.supported;
		ofp_port_status->desc.peer = 0;
		ofp_port_status->desc.curr_speed = htonl(speed * 1000);
		ofp_port_status->desc.max_speed = htonl(speed * 1000);
		break;
	case DEL_IF:
		break;
	default:
		break;
	}
	port_ccb->ofpbuf_len = sizeof(ofp_port_status_t);

	port_status_t *new_node = (port_status_t *)malloc(sizeof(ofp_port_status_t));
	memcpy(&(new_node->ofp_port_status),ofp_port_status,sizeof(ofp_port_status_t));
	new_node->next = NULL;
	if (port_status_head == NULL) {
		port_status_head = new_node;
		return TRUE;
	}
	port_status_t *cur;
	for(cur=port_status_head; cur->next!=NULL; cur=cur->next);
	cur->next = new_node;
	return TRUE;
}

STATUS OFP_encode_multipart_reply(tOFP_PORT *port_ccb, U8 *mu, U16 mulen) 
{
	ofp_multipart_t *ofp_multipart = (ofp_multipart_t *)(port_ccb->ofpbuf);
	U8 *ofpbuf_ptr = port_ccb->ofpbuf;

	ofp_multipart->ofp_header.version = ((ofp_multipart_t *)mu)->ofp_header.version;
	ofp_multipart->ofp_header.type = OFPT_MULTIPART_REPLY;
	ofp_multipart->ofp_header.xid = ((ofp_multipart_t *)mu)->ofp_header.xid;
	ofp_multipart->ofp_header.length = sizeof(ofp_multipart_t);

	ofp_multipart->type = ((ofp_multipart_t *)mu)->type;
	ofp_multipart->flags = ((ofp_multipart_t *)mu)->flags;
	ofpbuf_ptr = port_ccb->ofpbuf + sizeof(ofp_multipart_t);
	port_status_t *cur;
	for(cur=port_status_head; cur!=NULL; cur=cur->next) {
		if ((ofp_multipart->ofp_header.length + sizeof(ofp_port_status_t)) > MSG_LEN) {
			puts("too many ports info in ofp_multipart");
			break;
		}
		memcpy(ofpbuf_ptr,&(cur->ofp_port_status),sizeof(ofp_port_status_t));
		ofpbuf_ptr += sizeof(ofp_port_status_t);
		ofp_multipart->ofp_header.length += sizeof(ofp_port_status_t);
	}
	port_ccb->ofpbuf_len = ofp_multipart->ofp_header.length;
	ofp_multipart->ofp_header.length = htons(ofp_multipart->ofp_header.length);
	//PRINT_MESSAGE(port_ccb->ofpbuf,port_ccb->ofpbuf_len);
	return TRUE;
}

STATUS OFP_decode_flowmod(tOFP_PORT *port_ccb, U8 *mu, U16 mulen) 
{
	ofp_instruction_actions_t *ofp_instruction_actions;

	port_ccb->flowmod_info.msg_type = FLOWMOD;
	port_ccb->flowmod_info.msg_len = sizeof(flowmod_info_t);
	port_ccb->flowmod_info.buffer_id = ntohl(((ofp_flow_mod_t *)mu)->buffer_id); 
	port_ccb->flowmod_info.command = ((ofp_flow_mod_t *)mu)->command;
	port_ccb->flowmod_info.cookie = bitswap64(((ofp_flow_mod_t *)mu)->cookie);
	port_ccb->flowmod_info.hard_timeout = ntohs(((ofp_flow_mod_t *)mu)->hard_timeout);
	port_ccb->flowmod_info.idle_timeout = ntohs(((ofp_flow_mod_t *)mu)->idle_timeout);
	port_ccb->flowmod_info.out_group = ntohl(((ofp_flow_mod_t *)mu)->out_group);
	port_ccb->flowmod_info.out_port = ntohl(((ofp_flow_mod_t *)mu)->out_port);
	port_ccb->flowmod_info.priority = ntohs(((ofp_flow_mod_t *)mu)->priority);
	port_ccb->flowmod_info.table_id = ((ofp_flow_mod_t *)mu)->table_id;

	//PRINT_MESSAGE((unsigned char *)&(((ofp_flow_mod_t *)mu)->match), 32);
	uint16_t match_len = ntohs(((ofp_flow_mod_t *)mu)->match.length) - (sizeof(((ofp_flow_mod_t *)mu)->match.length) + sizeof(((ofp_flow_mod_t *)mu)->match.type));
	//printf("match len = %u\n", match_len);
	if (match_len < 8) {
		/* This means match field in flowmod is empty, and we should align to 8 bits */
		ofp_instruction_actions = (ofp_instruction_actions_t *)(mu + sizeof(ofp_flow_mod_t));
		port_ccb->flowmod_info.is_tail = TRUE;
	}
	else {
		int i = 0;
		//PRINT_MESSAGE(&(((ofp_flow_mod_t *)mu)->match.oxm_header), match_len);
		/* sotre match fields with a for loop, in each time loop we store match types */
		for(ofp_oxm_header_t *cur = &(((ofp_flow_mod_t *)mu)->match.oxm_header); match_len>0; i++) {
			if (i > 20) { // we only store up to 20 match fields
				puts("reach max match field limit");
				break;
			}
			cur->oxm_union.oxm_value = ntohs(cur->oxm_union.oxm_value);
			switch (cur->oxm_union.oxm_struct.oxm_field) {
			case OFPXMT_OFB_IN_PORT:
				port_ccb->flowmod_info.match_info[i].port_id = ntohl(*(uint32_t *)((U8 *)cur + sizeof(ofp_oxm_header_t)));
				port_ccb->flowmod_info.match_info[i].type = PORT;
				//printf("match port id = %u\n", port_ccb->flowmod_info.match_info[i].port_id);
				break;
			case OFPXMT_OFB_ETH_DST:
				memcpy(port_ccb->flowmod_info.match_info[i].dst_mac, (U8 *)cur + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->flowmod_info.match_info[i].type = DST_MAC;
				//printf("fill match field dst_mac= %x\n", port_ccb->flowmod_info.match_info[i].dst_mac[0]);
				break;
			case OFPXMT_OFB_ETH_SRC:
				memcpy(port_ccb->flowmod_info.match_info[i].src_mac, (U8 *)cur + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->flowmod_info.match_info[i].type = SRC_MAC;
				//printf("fill match field src_mac= %x\n", port_ccb->flowmod_info.match_info[i].src_mac[0]);
				break;
			case OFPXMT_OFB_ETH_TYPE:
				port_ccb->flowmod_info.match_info[i].ether_type = ntohs(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = ETHER_TYPE;
				printf("fill match field eth_type= %u\n", port_ccb->flowmod_info.match_info[i].ether_type);
				break;
			case OFPXMT_OFB_IP_PROTO:
				port_ccb->flowmod_info.match_info[i].ip_proto = *(U8 *)((U8 *)cur + sizeof(ofp_oxm_header_t));
				port_ccb->flowmod_info.match_info[i].type = IP_PROTO;
				break;
			case OFPXMT_OFB_IPV4_SRC:
				port_ccb->flowmod_info.match_info[i].ip_src = ntohl(*((uint32_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = SRC_IP;
				printf("match ip src = %u\n", port_ccb->flowmod_info.match_info[i].ip_src);
				break;
			case OFPXMT_OFB_IPV4_DST:
				port_ccb->flowmod_info.match_info[i].ip_dst = ntohl(*((uint32_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = DST_IP;
				break;
			case OFPXMT_OFB_TCP_SRC:
				port_ccb->flowmod_info.match_info[i].src_port = ntohs(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_TCP_DST:
				port_ccb->flowmod_info.match_info[i].dst_port = ntohs(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = DST_PORT;
				break;
			case OFPXMT_OFB_UDP_SRC:
				port_ccb->flowmod_info.match_info[i].src_port = ntohs(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_UDP_DST:
				port_ccb->flowmod_info.match_info[i].dst_port = ntohs(*((uint16_t *)((U8 *)cur + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.match_info[i].type = DST_PORT;
				break;
			default:
				return FALSE;
			}
			match_len = match_len - cur->oxm_union.oxm_struct.oxm_length - sizeof(ofp_oxm_header_t);
			//printf("match len = %u %u %x\n", match_len, cur->oxm_union.oxm_struct.oxm_length, cur);
			cur = (ofp_oxm_header_t *)(((U8 *)cur) + cur->oxm_union.oxm_struct.oxm_length + sizeof(ofp_oxm_header_t));
		}
		port_ccb->flowmod_info.match_info[--i].is_tail = TRUE;
	}
	uint16_t padding;
	uint16_t shift_ret;
	match_len = ntohs(((ofp_flow_mod_t *)mu)->match.length);
	//printf("match len = %u\n", match_len);
	/*if ((tmp = ((match_len + sizeof(((ofp_flow_mod_t *)mu)->match.length) + sizeof(((ofp_flow_mod_t *)mu)->match.type)) << 13)) > 0)
		padding = (((match_len + sizeof(((ofp_flow_mod_t *)mu)->match.length) + sizeof(((ofp_flow_mod_t *)mu)->match.type)) >> 3) + 1) << 3;
	else
		padding = ((match_len + sizeof(((ofp_flow_mod_t *)mu)->match.length) + sizeof(((ofp_flow_mod_t *)mu)->match.type)) >> 3) << 3;*/
	/* make the match field length alignment */
	if ((shift_ret = match_len << 13) > 0)
		padding = ((match_len >> 3) + 1) << 3;
	else
		padding = (match_len >> 3) << 3;
	//printf("padding = %u\n", padding);
	ofp_instruction_actions = (ofp_instruction_actions_t *)(mu + sizeof(ofp_flow_mod_t) - sizeof(struct ofp_match) + padding);
	ofp_action_header_t *ofp_action_header = (ofp_action_header_t *)(((U8 *)ofp_instruction_actions) + sizeof(ofp_instruction_actions_t));
	ofp_oxm_header_t *ofp_oxm_header = (ofp_oxm_header_t *)(ofp_action_header->pad);
	uint16_t action_len = ntohs(ofp_instruction_actions->len) - sizeof(ofp_instruction_actions_t);
	//PRINT_MESSAGE(ofp_instruction_actions, 16);
	uint16_t len = 0;
	int i = 0;
	memset(port_ccb->flowmod_info.action_info, 0, 20*sizeof(pkt_info_t));
	//pkt_info_t *head_action_info = NULL, *cur_action_info;
	for(U8 *cur=(U8 *)ofp_action_header;; len+=ntohs(((ofp_action_header_t *)cur)->len),cur+=len,i++) {
		if (i >= 20) {
			puts("Reach max flowmod action list, drop remaining actions");
			break;
		}
		//printf("total len = %u %u\n", len, action_len);
		if (len == action_len)
			break;
		if (((ofp_action_header_t *)cur)->type == htons(OFPAT_SET_FIELD)) {
			//pkt_info_t *new_action = (pkt_info_t *)malloc(sizeof(pkt_info_t));
			ofp_action_set_field_t *ofp_action_set_field = (ofp_action_set_field_t *)cur;
			ofp_oxm_header->oxm_union.oxm_value = ntohs(ofp_oxm_header->oxm_union.oxm_value);
			switch (ofp_oxm_header->oxm_union.oxm_struct.oxm_field) {
			/*case OFPXMT_OFB_IN_PORT:
				port_ccb->flowmod_info.action_info[i].port_id = ntohl(*(uint32_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t)));
				port_ccb->flowmod_info.action_info[i].type = PORT;
				break;*/
			case OFPXMT_OFB_ETH_DST:
				memcpy(port_ccb->flowmod_info.action_info[i].dst_mac, ((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->flowmod_info.action_info[i].type = DST_MAC;
				break;
			case OFPXMT_OFB_ETH_SRC:
				memcpy(port_ccb->flowmod_info.action_info[i].src_mac, ((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->flowmod_info.action_info[i].type = SRC_MAC;
				break;
			case OFPXMT_OFB_ETH_TYPE:
				port_ccb->flowmod_info.action_info[i].ether_type = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = ETHER_TYPE;
				break;
			case OFPXMT_OFB_IP_PROTO:
				port_ccb->flowmod_info.action_info[i].ip_proto = *(U8 *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t));
				port_ccb->flowmod_info.action_info[i].type = IP_PROTO;
				break;
			case OFPXMT_OFB_IPV4_SRC:
				port_ccb->flowmod_info.action_info[i].ip_src = ntohl(*((uint32_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = SRC_IP;
				break;
			case OFPXMT_OFB_IPV4_DST:
				port_ccb->flowmod_info.action_info[i].ip_dst = ntohl(*((uint32_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = DST_IP;
				printf("action ip dst = %u\n", port_ccb->flowmod_info.action_info[i].ip_dst);
				break;
			case OFPXMT_OFB_TCP_SRC:
				port_ccb->flowmod_info.action_info[i].src_port = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_TCP_DST:
				port_ccb->flowmod_info.action_info[i].dst_port = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = DST_PORT;
				break;
			case OFPXMT_OFB_UDP_SRC:
				port_ccb->flowmod_info.action_info[i].src_port = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->flowmod_info.action_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_UDP_DST:
				port_ccb->flowmod_info.action_info[i].dst_port = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
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
			port_ccb->flowmod_info.action_info[i].max_len = ntohs(ofp_action_output->max_len);
			port_ccb->flowmod_info.action_info[i].port_id = ntohl(ofp_action_output->port);
			port_ccb->flowmod_info.action_info[i].type = PORT;
			//printf("action port id = %u %u\n", port_ccb->flowmod_info.action_info[i].port_id, ntohl(ofp_action_output->port));
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
	port_ccb->flowmod_info.action_info[--i].is_tail = TRUE;
	//PRINT_MESSAGE(port_ccb->flowmod_info.match_info,sizeof(pkt_info_t)*20);
	return TRUE;
}
STATUS OFP_decode_packet_out(tOFP_PORT *port_ccb, U8 *mu, U16 mulen) 
{
	ofp_packet_out_t 	*ofp_packet_out = (ofp_packet_out_t *)mu;
	uint16_t 			len = 0, action_len = ntohs(ofp_packet_out->actions_len);
	int 				i = 0;
	ofp_action_header_t *ofp_action_header = (ofp_action_header_t *)(mu + sizeof(ofp_packet_out_t));
	ofp_oxm_header_t 	*ofp_oxm_header = (ofp_oxm_header_t *)(ofp_action_header->pad);

	port_ccb->packet_out_info.msg_type = PACKET_OUT;
	port_ccb->packet_out_info.msg_len = sizeof(packet_out_info_t) - ETH_MTU + mulen - action_len - sizeof(ofp_packet_out_t);
	//printf("mulen = %u, action_len = %u\n", mulen, action_len);
	port_ccb->packet_out_info.buffer_id = ntohl(ofp_packet_out->buffer_id);
	port_ccb->packet_out_info.in_port = ntohl(ofp_packet_out->in_port);
	memcpy(port_ccb->packet_out_info.ofpbuf, (U8 *)ofp_action_header + action_len, mulen - action_len - sizeof(ofp_packet_out_t));
	
	for(U8 *cur=(U8 *)ofp_action_header;; len+=ntohs(((ofp_action_header_t *)cur)->len),cur+=len,i++) {
		if (i >= 20) {
			puts("Reach max packet_out action list, drop remaining actions");
			break;
		}
		//printf("total len = %u %u\n", len, action_len);
		if (len == action_len)
			break;
		if (((ofp_action_header_t *)cur)->type == htons(OFPAT_SET_FIELD)) {
			//pkt_info_t *new_action = (pkt_info_t *)malloc(sizeof(pkt_info_t));
			//ofp_action_set_field_t *ofp_action_set_field = (ofp_action_set_field_t *)cur;
			ofp_oxm_header->oxm_union.oxm_value = ntohs(ofp_oxm_header->oxm_union.oxm_value);
			switch (ofp_oxm_header->oxm_union.oxm_struct.oxm_field) {
			/*case OFPXMT_OFB_IN_PORT:
				port_ccb->packet_out_info.action_info[i].port_id = ntohl(*(uint32_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t)));
				port_ccb->packet_out_info.action_info[i].type = PORT;
				break;*/
			case OFPXMT_OFB_ETH_DST:
				memcpy(port_ccb->packet_out_info.action_info[i].dst_mac, ((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->packet_out_info.action_info[i].type = DST_MAC;
				break;
			case OFPXMT_OFB_ETH_SRC:
				memcpy(port_ccb->packet_out_info.action_info[i].src_mac, ((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t), ETH_ALEN);
				port_ccb->packet_out_info.action_info[i].type = SRC_MAC;
				break;
			case OFPXMT_OFB_ETH_TYPE:
				port_ccb->packet_out_info.action_info[i].ether_type = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->packet_out_info.action_info[i].type = ETHER_TYPE;
				break;
			case OFPXMT_OFB_IP_PROTO:
				port_ccb->packet_out_info.action_info[i].ip_proto = *(U8 *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t));
				port_ccb->packet_out_info.action_info[i].type = IP_PROTO;
				break;
			case OFPXMT_OFB_IPV4_SRC:
				port_ccb->packet_out_info.action_info[i].ip_src = ntohl(*((uint32_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->packet_out_info.action_info[i].type = SRC_IP;
				break;
			case OFPXMT_OFB_IPV4_DST:
				port_ccb->packet_out_info.action_info[i].ip_dst = ntohl(*((uint32_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->packet_out_info.action_info[i].type = DST_IP;
				printf("action ip dst = %u\n", port_ccb->packet_out_info.action_info[i].ip_dst);
				break;
			case OFPXMT_OFB_TCP_SRC:
				port_ccb->packet_out_info.action_info[i].src_port = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->packet_out_info.action_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_TCP_DST:
				port_ccb->packet_out_info.action_info[i].dst_port = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->packet_out_info.action_info[i].type = DST_PORT;
				break;
			case OFPXMT_OFB_UDP_SRC:
				port_ccb->packet_out_info.action_info[i].src_port = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->packet_out_info.action_info[i].type = SRC_PORT;
				break;
			case OFPXMT_OFB_UDP_DST:
				port_ccb->packet_out_info.action_info[i].dst_port = ntohs(*((uint16_t *)(((U8 *)ofp_oxm_header) + sizeof(ofp_oxm_header_t))));
				port_ccb->packet_out_info.action_info[i].type = DST_PORT;
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
			port_ccb->packet_out_info.action_info[i].max_len = ntohs(ofp_action_output->max_len);
			port_ccb->packet_out_info.action_info[i].port_id = ntohl(ofp_action_output->port);
			port_ccb->packet_out_info.action_info[i].type = PORT;
			//PRINT_MESSAGE(cur,16);
			//printf("action port id = %u %u\n", port_ccb->packet_out_info.action_info[i].port_id, ntohl(ofp_action_output->port));
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
			puts("Unsupported packet_out action");
		}
	}

	return TRUE;
}
