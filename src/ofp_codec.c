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
void OFP_encode_back_to_host(tOFP_PORT *port_ccb, U8 *mu, U16 mulen);
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
		//OFP_encode_back_to_host(port_ccb, mu, mulen);
		PRINT_MESSAGE(mu, mulen);
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
	mu = (U8 *)(msg->buffer);
	mulen = (mail->len) - sizeof(int);
	buffer_id = msg->id;

	uint16_t ofp_match_length = 0; 
	uint32_t port_no = htonl(0x1);

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

void OFP_encode_back_to_host(tOFP_PORT *port_ccb, U8 *mu, U16 mulen) 
{
	uint32_t buffer_id = htonl(((ofp_flow_mod_t *)mu)->buffer_id); 
	uint32_t dst_ip, src_ip = *((uint32_t *)(mu + sizeof(ofp_flow_mod_t)));
	ofp_action_set_field_t *ofp_action_set_field = (ofp_action_set_field_t *)(mu + sizeof(ofp_flow_mod_t) + sizeof(src_ip) + sizeof(4)/* padding */ + sizeof(ofp_instruction_actions_t));
	ofp_oxm_header_t *ofp_oxm_header = (ofp_oxm_header_t *)(ofp_action_set_field->pad);
	tIP_PKT ip_hdr;
	tUDP_PKT udp_hdr;
	if (ofp_action_set_field->type != htons(OFPAT_SET_FIELD)) {
		port_ccb->event = E_OTHERS;
		return;
	}
	ofp_oxm_header->oxm_union.oxm_value = htons(ofp_oxm_header->oxm_union.oxm_value);
	if (ofp_oxm_header->oxm_union.oxm_struct.oxm_field == OFPXMT_OFB_IPV4_DST)
		dst_ip = *((uint32_t *)(mu + sizeof(ofp_flow_mod_t) + sizeof(src_ip) + sizeof(4)/* padding */ + sizeof(ofp_instruction_actions_t) + sizeof(ofp_action_set_field_t)));
	printf("src ip = %x, dst ip = %x\n", src_ip, dst_ip);

	host_learn_t *node = find_node(port_ccb->head,buffer_id);

	memcpy(port_ccb->ofpbuf,node->src_mac,MAC_ADDR_LEN);
	memcpy(port_ccb->ofpbuf+MAC_ADDR_LEN,node->dst_mac,MAC_ADDR_LEN);
	*((uint16_t *)(port_ccb->ofpbuf+MAC_ADDR_LEN*2)) = htons(FRAME_TYPE_IP);
	port_ccb->ofpbuf_len = 14;

	ip_hdr_init(&ip_hdr,node->src_ip,node->dst_ip);
	U8 *tmp = ENCODE_IP_PKT(&ip_hdr,(port_ccb->ofpbuf)+MAC_ADDR_LEN*2+2);
	port_ccb->ofpbuf_len += 20;

	uint8_t payload[8];
	memcpy(payload,&src_ip,IP_ADDR_LEN);
	memcpy(payload+IP_ADDR_LEN,&dst_ip,IP_ADDR_LEN);
	udp_hdr_init(&udp_hdr,payload);
	tmp = ENCODE_UDP_PKT(&ip_hdr,&udp_hdr,tmp);
	port_ccb->ofpbuf_len += UDP_HDR_LEN;
	memcpy(tmp,payload,8);
	port_ccb->ofpbuf_len += IP_ADDR_LEN*2;
	free(node);
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
