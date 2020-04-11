/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  DP_CODEC.C :

  Designed by THE on SEP 16, 2019
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include 		<common.h>
#include		"dp.h"
#include		"dp_flow.h"
#include		"ofpd.h"
#include		"ofp_codec.h"
#include        "ofp_fsm.h"
#include        "ofp_dbg.h"
#include 		"ofp_sock.h"
#include 		"ofp_asyn.h"
#include 		"ofp_ctrl2sw.h"
#include 		"ofp_oxm.h"
#include 		<unistd.h>

STATUS parse_tcp(struct ethhdr *eth_hdr, struct iphdr *ip_hdr);
STATUS parse_udp(struct ethhdr *eth_hdr, struct iphdr *ip_hdr);
STATUS parse_ip(struct ethhdr *eth_hdr);

extern STATUS find_flow(pkt_info_t pkt_info);

/*============================ DECODE ===============================*/

/*****************************************************
 * DP_decode_frame
 * 
 * input : pArg - mail.param
 * output: imsg, event
 * return: session ccb
 *****************************************************/
STATUS DP_decode_frame(tOFP_MBX *mail)
{
	U16	mulen;
	U8	*mu;
	tDP_MSG *msg;
	struct ethhdr *eth_hdr;
	
	if (mail->len > ETH_MTU+6) {
	    DBG_OFP(DBGLVL1,0,"error! too large frame(%d)\n",mail->len);
	    return ERROR;
	}
	
	msg = (tDP_MSG *)(mail->refp);
	mu = (U8 *)(msg->buffer);
	mulen = (mail->len) - (sizeof(int) + sizeof(uint16_t));
	//PRINT_MESSAGE(mu,mulen);

	eth_hdr = (struct ethhdr *)mu;
	if (eth_hdr->h_proto == htons(ETH_P_IP)) {
		if (parse_ip(eth_hdr) == FALSE)
			return FALSE;
	}
	else if (eth_hdr->h_proto == htons(ETH_P_ARP)) {
		pkt_info_t pkt_info;
		memset(&pkt_info,0,sizeof(pkt_info));

		memcpy(&pkt_info,eth_hdr,sizeof(struct ethhdr));
		//TODO: save L2 payload

		if (find_flow(pkt_info) == FALSE)
			return FALSE;
	}
	else {
		return FALSE;
	}
	//TODO: send pkt from rule
	return TRUE;
}

STATUS parse_ip(struct ethhdr *eth_hdr)
{
	pkt_info_t pkt_info;

	struct iphdr *ip_hdr = (struct iphdr *)(eth_hdr + 1);

	ip_hdr = (struct iphdr *)(eth_hdr + 1);
	if (ip_hdr->version != 4)
		return FALSE;
	switch (ip_hdr->protocol) {
	case IPPROTO_ICMP:
		memcpy(&pkt_info,eth_hdr,sizeof(struct ethhdr));
		pkt_info.ip_proto = ip_hdr->protocol;
		pkt_info.ip_dst = ip_hdr->daddr;
		pkt_info.ip_src = ip_hdr->saddr;

		if (find_flow(pkt_info) == FALSE) {
			return FALSE;
		}
		return TRUE;
	case IPPROTO_TCP:
		if (parse_tcp(eth_hdr,ip_hdr) == FALSE)
			return FALSE;
		return TRUE;
	case IPPROTO_UDP:
		if (parse_udp(eth_hdr,ip_hdr) == FALSE)
			return FALSE;
		return TRUE;
	default:
		return FALSE;
	}
}

STATUS parse_tcp(struct ethhdr *eth_hdr, struct iphdr *ip_hdr)
{
	pkt_info_t pkt_info;
	struct tcphdr *tcp_hdr = (struct tcphdr *)(ip_hdr + 1);

	memcpy(&pkt_info,eth_hdr,sizeof(struct ethhdr));
	pkt_info.ip_proto = ip_hdr->protocol;
	pkt_info.ip_dst = ip_hdr->daddr;
	pkt_info.ip_src = ip_hdr->saddr;
	pkt_info.dst_port = tcp_hdr->dest;
	pkt_info.src_port = tcp_hdr->source;

	if (find_flow(pkt_info) == FALSE) {
		return FALSE;
	}
	return TRUE;
}

STATUS parse_udp(struct ethhdr *eth_hdr, struct iphdr *ip_hdr)
{
	pkt_info_t pkt_info;
	struct udphdr *udp_hdr = (struct udphdr *)(ip_hdr + 1);

	memcpy(&pkt_info,eth_hdr,sizeof(struct ethhdr));
	pkt_info.ip_proto = ip_hdr->protocol;
	pkt_info.ip_dst = ip_hdr->daddr;
	pkt_info.ip_src = ip_hdr->saddr;
	pkt_info.dst_port = udp_hdr->dest;
	pkt_info.src_port = udp_hdr->source;

	if (find_flow(pkt_info) == FALSE) {
		return FALSE;
	}
	return TRUE;
}
