/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFPD.H

     For ofp detection

  Designed by THE on SEP 26, 2019
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include "ofp_common.h"
#include "ofp_asyn.h"
#include "ofp_ctrl2sw.h"
#include <ip_codec.h>

#define OFP_Q_KEY				0x0b00
#define DP_Q_KEY				0x0c00
#define ETH_MTU					1514
#define TEST_PORT_ID			1

#define IF_NAME 				"wlan0"
#define	MIN_FRAME_SIZE			64
#define	MAX_QUE_EVT_CNT			(MBOX_QUE_SIZE/2)
#define _PBM(port)				(1<<(port-1))

#define MAX_USER_PORT_NUM		10
#define MAX_OFP_QUERY_NUM		10

typedef struct {
	U8		subt;
	U16		len;
	U8		value[255];
} tSUB_VAL;

//========= system capability ===========
typedef struct {
	U16		cap_map;
	U16		en_map;
} tSYS_CAP;

//========= management address ===========
typedef struct {
	U8		addr_strlen; //addr_subt + addr[]
	U8		addr_subt;
	U8		addr[31];
	
	U8		if_subt;
	U32		if_no;
	
	U8		oid_len;
	U32		oids[128];
} tMNG_ADDR;

typedef struct host_learn {
	unsigned char src_mac[MAC_ADDR_LEN];
	unsigned char dst_mac[MAC_ADDR_LEN];
	uint32_t src_ip;
	uint32_t dst_ip;
	uint16_t src_port;
	uint32_t buffer_id;
	struct host_learn *next;
}host_learn_t;

//========= The structure of port ===========
typedef struct {
	BOOL		enable;
	U8 			state;
	U8			query_cnt;
	U16			port;

	U32			imsg_cnt;
	U32			omsg_cnt;
	U32			err_imsg_cnt;	
	
	tSUB_VAL	port_id;
		
	U32			ttl;
	char		port_desc[80];
	char		sys_name[80];
	char		sys_desc[255];
	
	tSYS_CAP	sys_cap;
	tMNG_ADDR  	mng_addr;

	int 		sockfd;
	U16			event;
	ofp_header_t ofp_header;
	ofp_multipart_t ofp_multipart;
	ofp_packet_in_t ofp_packet_in;
	U8 			ofpbuf[ETH_MTU];
	uint16_t 	ofpbuf_len;
	host_learn_t *head;
} tOFP_PORT;

extern U8	 		g_loc_mac[]; //system mac addr -- global variable
extern tOFP_PORT	ofp_ports[];
extern tIPC_ID 		ofpQid;
extern tIPC_ID 		dpQid;
extern U32			ofp_interval;
extern U8			ofp_max_msg_per_query;

/*-----------------------------------------
 * Queue between IF driver and daemon
 *----------------------------------------*/
typedef struct {
	U16  			type;
	U8          	refp[ETH_MTU+6];
	int	        	len;
}tOFP_MBX;

/*-----------------------------------------
 * msg from cp
 *----------------------------------------*/
typedef struct {
	U8  			type;
	int 			sockfd;
	char          	buffer[ETH_MTU];
}tOFP_MSG;

typedef enum {
	/* Immutable messages. */
	OFPT_HELLO,
	OFPT_ERROR,
	OFPT_ECHO_REQUEST,
	OFPT_ECHO_REPLY,
	OFPT_EXPERIMENTER,
	/* Switch configuration messages */
	OFPT_FEATURES_REQUEST,
	OFPT_FEATURES_REPLY,
	OFPT_GET_CONFIG_REQUEST,
	OFPT_GET_CONFIG_REPLY, 
	OFPT_SET_CONFIG,
	/* Asynchronous messages. */
	OFPT_PACKET_IN,
	OFPT_FLOW_REMOVED, 
	OFPT_PORT_STATUS,
	/* Controller command messages. */
	OFPT_PACKET_OUT,
	OFPT_FLOW_MOD,
	OFPT_GROUP_MOD,
	OFPT_PORT_MOD,
	OFPT_TABLE_MOD,
	/* Multipart messages. */ 
	OFPT_MULTIPART_REQUEST, 
	OFPT_MULTIPART_REPLY,
	/* Barrier messages. */ 
	OFPT_BARRIER_REQUEST,
	OFPT_BARRIER_REPLY,
	/* Queue Configuration messages. */
	FPT_QUEUE_GET_CONFIG_REQUEST,
	OFPT_QUEUE_GET_CONFIG_REPLY,
	/* Controller role change request messages. */
	OFPT_ROLE_REQUEST,
	OFPT_ROLE_REPLY,
	/* Asynchronous message configuration. */
	OFPT_GET_ASYNC_REQUEST,
	OFPT_GET_ASYNC_REPLY,
	OFPT_SET_ASYNC,
	/* Meters and rate limiters configuration messages. */
	OFPT_METER_MOD,
}OFPT_t;
