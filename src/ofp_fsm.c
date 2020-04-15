/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  ofp_fsm.c
  
     State Transition Table 

  Designed by THE
  Date: 30/09/2019
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include 		<common.h>
#include		"ofpd.h"
#include		"ofp_codec.h"
#include    	"ofp_sock.h"
#include    	"ofp_fsm.h"
#include		"ofp_dbg.h"
#include 		"ofp_asyn.h"
#include 		"ofp_ctrl2sw.h"
#include 		<ifaddrs.h>
#include		<inttypes.h>

extern int		ofp_io_fds[10];

char 			*OFP_state2str(U16 state);

static STATUS 	A_send_hello(tOFP_PORT*);
static STATUS 	A_send_echo_request(tOFP_PORT*);
static STATUS   A_send_feature_reply(tOFP_PORT*);
static STATUS   A_send_multipart_reply(tOFP_PORT*);
static STATUS   A_start_timer(tOFP_PORT*);
static STATUS   A_clear_query_cnt(tOFP_PORT*);
static STATUS   A_stop_query_tmr(tOFP_PORT*);
static STATUS   A_query_tmr_expire(tOFP_PORT*);
static STATUS   A_send_packet_in(tOFP_PORT*);
static STATUS   A_send_to_dp(tOFP_PORT*);

tOFP_STATE_TBL  ofp_fsm_tbl[] = { 
/*//////////////////////////////////////////////////////////////////////////////////
  	STATE   		EVENT           		NEXT-STATE        HANDLER       
///////////////////////////////////////////////////////////////////////////////////\*/
{ S_CLOSED,			E_START,     			S_HELLO_WAIT,	{ A_send_hello, 0 }},

{ S_HELLO_WAIT,		E_RECV_HELLO,     		S_FEATURE_WAIT,	{ 0 }},

{ S_FEATURE_WAIT,	E_FEATURE_REQUEST,     	S_ESTABLISHED,	{ A_send_feature_reply, A_send_echo_request, A_start_timer, 0 }},

{ S_ESTABLISHED,	E_MULTIPART_REQUEST,    S_ESTABLISHED,	{ A_send_multipart_reply, 0 }},

{ S_ESTABLISHED,	E_OTHERS,    			S_ESTABLISHED,	{ 0 }},

{ S_ESTABLISHED,	E_ECHO_REPLY,    		S_ESTABLISHED,	{ A_stop_query_tmr, A_clear_query_cnt, A_start_timer, 0 }},

{ S_ESTABLISHED,	E_OFP_TIMEOUT,    		S_ESTABLISHED,	{ A_query_tmr_expire, A_send_echo_request, A_start_timer, 0 }},

{ S_ESTABLISHED,    E_PACKET_IN,            S_ESTABLISHED,  { A_send_packet_in, 0 }},

{ S_ESTABLISHED,    E_FLOW_MOD,            	S_ESTABLISHED,  { A_send_to_dp, 0 }},

{ S_ESTABLISHED,    E_PACKET_OUT,           S_ESTABLISHED,  { /*A_send_to_dp,*/ 0 }},

{ S_INVALID, 0 }
};
 
/***********************************************************************
 * OFP_FSM
 *
 * purpose : finite state machine.
 * input   : tnnl - tunnel pointer
 *           event -
 *           arg - signal(primitive) or pdu
 * return  : error status
 ***********************************************************************/
STATUS OFP_FSM(tOFP_PORT *port_ccb, U16 event)
{	
    register int  	i,j;
    STATUS			retval;
    char 			str1[30],str2[30];

	if (!port_ccb){
        DBG_OFP(DBGLVL1,port_ccb,"Error! No port found for the event(%d)\n",event);
        return FALSE;
    }
    
    /* Find a matched state */
    for(i=0; ofp_fsm_tbl[i].state!=S_INVALID; i++)
        if (ofp_fsm_tbl[i].state == port_ccb->state)
            break;

    if (ofp_fsm_tbl[i].state == S_INVALID){
        DBG_OFP(DBGLVL1,port_ccb,"Error! unknown state(%d) specified for the event(%d)\n",
        	port_ccb->state,event);
        return FALSE;
    }

    /*
     * Find a matched event in a specific state.
     * Note : a state can accept several events.
     */
    for(;ofp_fsm_tbl[i].state==port_ccb->state; i++)
        if (ofp_fsm_tbl[i].event == event)
            break;
    
    if (ofp_fsm_tbl[i].state != port_ccb->state){ /* search until meet the next state */
        DBG_OFP(DBGLVL1,port_ccb,"error! invalid event(%d) in state(%s)\n",
            event, OFP_state2str(port_ccb->state));
  		return TRUE; /* still pass to endpoint */
    }
    
    /* Correct state found */
    if (port_ccb->state != ofp_fsm_tbl[i].next_state){
    	strcpy(str1,OFP_state2str(port_ccb->state));
    	strcpy(str2,OFP_state2str(ofp_fsm_tbl[i].next_state));
        DBG_OFP(DBGLVL1,port_ccb,"state changed from %s to %s\n",str1,str2);
        port_ccb->state = ofp_fsm_tbl[i].next_state;
    }
    
	for(j=0; ofp_fsm_tbl[i].hdl[j]; j++){
       	retval = (*ofp_fsm_tbl[i].hdl[j])(port_ccb);
       	if (!retval)  return TRUE;
    }
    return TRUE;
}

/*********************************************************************
 * A_send_hello: 
 *
 *********************************************************************/
STATUS A_send_hello(tOFP_PORT *port_ccb)	
{
	unsigned char buffer[256];
	struct ofp_header of_header;

	of_header.version = 0x04;
	of_header.type = OFPT_HELLO;
	uint16_t length = of_header.length = sizeof(struct ofp_header);
	of_header.length = htons(of_header.length);
	of_header.xid = 0x45;
	memcpy(buffer,&of_header,sizeof(struct ofp_header));
	drv_xmit(buffer, length, port_ccb->sockfd);
	//printf("send hello message\n");

	return TRUE;
}

/*********************************************************************
 * A_send_echo_request: 
 *
 *********************************************************************/
STATUS A_send_echo_request(tOFP_PORT *port_ccb)	
{
	unsigned char buffer[256];
	ofp_header_t ofp_header;

	ofp_header.version = 0x04;
	ofp_header.type = OFPT_ECHO_REQUEST;
	uint16_t length = ofp_header.length = sizeof(struct ofp_header);
	ofp_header.length = htons(ofp_header.length);
	ofp_header.xid = 0;

	port_ccb->sockfd = ofp_io_fds[0];

	memcpy(buffer, &ofp_header, length);
	drv_xmit(buffer, length, port_ccb->sockfd);

	return TRUE;
}

/*********************************************************************
 * A_send_feature_reply: 
 *
 *********************************************************************/
STATUS A_send_feature_reply(tOFP_PORT *port_ccb)	
{
	unsigned char buffer[256];
	ofp_switch_features_t ofp_switch_features;
	//struct ifaddrs *ifaddr;
	//struct ifaddrs *ifa;

	ofp_switch_features.ofp_header.version = 0x04;
	ofp_switch_features.ofp_header.type = OFPT_FEATURES_REPLY;
	ofp_switch_features.ofp_header.length = sizeof(struct ofp_header);
	ofp_switch_features.ofp_header.xid = 0;

	int fd;
    struct ifreq ifr;
	uint64_t tmp;
	char *eth_name = IF_NAME;

	/*if (getifaddrs(&ifaddr) == -1) {
    	perror("getifaddrs");
    	return -1;
  	}*/
    fd = socket(AF_INET, SOCK_DGRAM, 0);
 	ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name,eth_name,IFNAMSIZ-1);
 	ioctl(fd,SIOCGIFHWADDR,&ifr);
    close(fd);
	//freeifaddrs(ifaddr);
	ofp_switch_features.datapath_id = 0x0;
	for(int i=5; i>=0; i--) {
        tmp = ifr.ifr_hwaddr.sa_data[i];
        ofp_switch_features.datapath_id += tmp << (i*8);
    }
	//printf("%"PRIu64"\n", ofp_switch_features.datapath_id);
	ofp_switch_features.datapath_id = bitswap64(ofp_switch_features.datapath_id);
	ofp_switch_features.n_buffers = htonl(256);
	ofp_switch_features.n_tables = 254;
	ofp_switch_features.auxiliary_id = 0;
	ofp_switch_features.capabilities = OFPC_FLOW_STATS | OFPC_PORT_STATS | OFPC_QUEUE_STATS;
	ofp_switch_features.capabilities = htonl(ofp_switch_features.capabilities);
	ofp_switch_features.ofp_header.length += sizeof(ofp_switch_features_t);
	uint16_t length = ofp_switch_features.ofp_header.length;
	ofp_switch_features.ofp_header.length = htons(ofp_switch_features.ofp_header.length);

	memcpy(buffer, &ofp_switch_features, length);
	drv_xmit(buffer, length, port_ccb->sockfd);

	return TRUE;
}

/*********************************************************************
 * A_start_timer: 
 *
 *********************************************************************/
STATUS A_start_timer(tOFP_PORT *port_ccb)
{
	if (port_ccb->query_cnt > ofp_max_msg_per_query) {
		printf("OpenFlow controller does not respond.\n");
		//kill();
	}
	//DBG_OFP(DBGLVL1,port_ccb,"start query timer(%ld secs)\n",ofp_interval/SEC);
	OSTMR_StartTmr(ofpQid, port_ccb, ofp_interval, "ofp:txT", E_OFP_TIMEOUT);
	
	return TRUE;
}

/*********************************************************************
 * A_stop_query_tmr: 
 *
 *********************************************************************/
STATUS A_stop_query_tmr(tOFP_PORT *port_ccb)
{
	//DBG_OFP(DBGLVL1,port_ccb,"stop query timer\n");
	OSTMR_StopXtmr(port_ccb,E_OFP_TIMEOUT);
	return TRUE;
} 

/*********************************************************************
 * A_query_tmr_expire: 
 *
 *********************************************************************/
STATUS A_query_tmr_expire(tOFP_PORT *port_ccb)
{
	//DBG_OFP(DBGLVL1,port_ccb,"query timer expired\n");
	port_ccb->query_cnt++;
	return TRUE;
}

/*********************************************************************
 * A_clear_query_cnt: 
 *
 *********************************************************************/
STATUS A_clear_query_cnt(tOFP_PORT *port_ccb)
{
	port_ccb->query_cnt = 1;
	//DBG_OFP(DBGLVL1,port_ccb,"clear query count\n");
	return TRUE;
}

/*********************************************************************
 * A_send_multipart_reply: 
 *
 *********************************************************************/
STATUS A_send_multipart_reply(tOFP_PORT *port_ccb)
{
	ofp_multipart_t ofp_multipart;
	struct ofp_port ofp_port_desc;
	struct ifaddrs *ifaddr; 
	struct ifaddrs *ifa;
	U8 buf[1024];
	uintptr_t buf_ptr;
	int i;

	if (getifaddrs(&ifaddr) == -1) {
    	perror("getifaddrs");
    	return -1;
  	}

	ofp_multipart = port_ccb->ofp_multipart;
	ofp_multipart.ofp_header.type = OFPT_MULTIPART_REPLY;
	buf_ptr = (uintptr_t)(buf + sizeof(ofp_multipart_t));
	ofp_multipart.ofp_header.length = sizeof(ofp_multipart_t);

	memset(&ofp_port_desc,0,sizeof(struct ofp_port));

#if 0
	ofp_port_desc.port_no = htonl(1);
	strcpy(ofp_port_desc.name,IF_NAME);
	int fd;
    struct ifreq ifr;
 
    fd = socket(AF_INET, SOCK_DGRAM, 0);
 	ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name,IF_NAME,IFNAMSIZ-1);
 	ioctl(fd,SIOCGIFHWADDR,&ifr);
    close(fd);
    memcpy(ofp_port_desc.hw_addr,(unsigned char *)ifr.ifr_hwaddr.sa_data,OFP_ETH_ALEN);
	memcpy((U8 *)buf_ptr,&ofp_port_desc,sizeof(struct ofp_port));
	buf_ptr += sizeof(struct ofp_port);
	ofp_multipart.ofp_header.length += sizeof(struct ofp_port);
#else
	i = 0;
	for(ifa=ifaddr; ifa != NULL; ifa=ifa->ifa_next) {
		ifa->ifa_addr = (struct sockaddr *)((uintptr_t)(ifa->ifa_addr) >> 32);
 		//if (((uintptr_t)(ifa->ifa_addr) & 0x00000000ffffffff) == 0 || ifa->ifa_addr->sa_family != AF_PACKET) 
 		if (ifa->ifa_addr == NULL)
 			continue;
 		if (ifa->ifa_addr->sa_family != AF_PACKET)	
		 	continue;
		i++;
		ofp_port_desc.port_no = htonl(i);
		strcpy(ofp_port_desc.name,ifa->ifa_name);
		
		int fd;
    	struct ifreq ifr;

    	fd = socket(AF_INET, SOCK_DGRAM, 0);
 		ifr.ifr_addr.sa_family = AF_INET;
    	strncpy(ifr.ifr_name,ifa->ifa_name,IFNAMSIZ-1);
		
 		ioctl(fd,SIOCGIFHWADDR,&ifr);
    	close(fd);
    	memcpy(ofp_port_desc.hw_addr,(unsigned char *)ifr.ifr_hwaddr.sa_data,OFP_ETH_ALEN);
		memcpy((U8 *)buf_ptr,&ofp_port_desc,sizeof(struct ofp_port));
		
		buf_ptr += sizeof(struct ofp_port);
		ofp_multipart.ofp_header.length += sizeof(struct ofp_port);
	}
#endif
	uint16_t length = ofp_multipart.ofp_header.length;
	ofp_multipart.ofp_header.length = htons(ofp_multipart.ofp_header.length);
	memcpy(buf, &ofp_multipart, sizeof(ofp_multipart_t));
	drv_xmit(buf, length, port_ccb->sockfd);
	freeifaddrs(ifaddr);
	return TRUE;
}

/*********************************************************************
 * A_send_packet_in: 
 *
 *********************************************************************/
STATUS A_send_packet_in(tOFP_PORT *port_ccb)	
{
	printf("port_ccb->ofpbuf_len = %u\n", port_ccb->ofpbuf_len);
	drv_xmit(port_ccb->ofpbuf, port_ccb->ofpbuf_len, ofp_io_fds[0]);
	printf("send packet_in message\n");

	return TRUE;
}

/*********************************************************************
 * A_send_to_host: 
 *
 *********************************************************************/
STATUS A_send_to_dp(tOFP_PORT *port_ccb)	
{
	tOFP_MBX mail;
	U16 mulen = port_ccb->flowmod_info.msg_len;
	U8 *mu = (U8 *)&(port_ccb->flowmod_info);

    if (dpQid == -1) {
		if ((dpQid=msgget(DP_Q_KEY,0600|IPC_CREAT)) < 0) {
			printf("send> Oops! dpQ(key=0x%x) not found\n",DP_Q_KEY);
   	 	}
	}
	
	if (mulen > MSG_LEN) {
	 	printf("Incoming frame length(%d) is too lmaile!\n",mulen);
		return ERROR;
	}

	mail.len = mulen;
	memcpy(mail.refp, mu, mulen); /* mail content will be copied into mail queue */
	
	//printf("dp_send2mailbox(dp_sock.c %d): mulen=%d\n",__LINE__,mulen);
	mail.type = IPC_EV_TYPE_OFP;
	ipc_sw(dpQid, &mail, sizeof(mail), -1);
	memset(&(port_ccb->flowmod_info), 0, sizeof(flowmod_info_t));
	printf("send msg to dp\n");

	return TRUE;
}

//============================== others =============================

/*-------------------------------------------------------------------
 * OFP_state2str
 *
 * input : state
 * return: string of corresponding state value
 *------------------------------------------------------------------*/
char *OFP_state2str(U16 state)
{
	static struct {
		OFP_STATE	state;
		char		str[20];
	} ofp_state_desc_tbl[] = {
    	{ S_CLOSED,  		"CLOSED  " },
    	{ S_HELLO_WAIT,  	"WAIT_HELLO    " },
    	{ S_FEATURE_WAIT,  	"WAIT_FEATURE" },
    	{ S_ESTABLISHED,	"ESTABLISHED" },
    	{ S_INVALID,  		"INVALID " },
	};

	U8  i;
	
	for(i=0; ofp_state_desc_tbl[i].state != S_INVALID; i++) {
		if (ofp_state_desc_tbl[i].state == state)  break;
	}
	if (ofp_state_desc_tbl[i].state == S_INVALID) {
		return NULL;
	}
	return ofp_state_desc_tbl[i].str;
}
