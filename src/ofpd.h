/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFPD.H

     For ofp detection

  Designed by THE on SEP 26, 2019
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include "ofp_common.h"
#include "ofp_asyn.h"
#include "ofp_ctrl2sw.h"
#include "dp_flow.h"
#include <ip_codec.h>

#define OFP_Q_KEY				0x0b00
#define DP_Q_KEY				0x0c00
#define ETH_MTU					1514
#define MSG_LEN					4074
#define JUMBO_MTU				9000
#define TEST_PORT_ID			1

#define	MIN_FRAME_SIZE			64
#define	MAX_QUE_EVT_CNT			(MBOX_QUE_SIZE/2)
#define _PBM(port)				(1<<(port-1))

#define MAX_USER_PORT_NUM		10
#define MAX_OFP_QUERY_NUM		10

typedef struct flowmod_info {
	uint8_t		msg_type;
	U16			msg_len; 
    uint64_t 	cookie;
    uint8_t 	table_id;
    uint8_t 	command;
    uint16_t 	idle_timeout; 
    uint16_t 	hard_timeout;
    uint16_t 	priority;
    uint32_t 	buffer_id;
    uint32_t 	out_port; 
    uint32_t 	out_group;
    pkt_info_t	match_info[20];
	pkt_info_t	action_info[20];
	BOOL 		is_tail;
}flowmod_info_t;

typedef struct packet_out_info {
	uint8_t		msg_type;
	U16			msg_len; 
	uint32_t 	buffer_id;
	uint32_t 	in_port;
	pkt_info_t	action_info[20];
	U8 			ofpbuf[ETH_MTU];
}packet_out_info_t;

enum {
	FLOWMOD = 0,
	PACKET_OUT,
	CLI,
};

//========= The structure of port ===========
typedef struct {
	BOOL				enable;
	U8 					state;
	U8					query_cnt;
	U16					port;

	char 				of_ifname[16];
	char 				ctrl_ip[16];
	int 				sockfd;
	U16					event;
	ofp_header_t 		ofp_header;
	//ofp_multipart_t 	ofp_multipart;
	ofp_packet_in_t 	ofp_packet_in;
	flowmod_info_t 		flowmod_info;
	packet_out_info_t 	packet_out_info;
	U8 					ofpbuf[MSG_LEN];
	uint16_t 			ofpbuf_len;
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
	U8          	refp[MSG_LEN];
	int	        	len;
}tOFP_MBX;

/*-----------------------------------------
 * msg from cp
 *----------------------------------------*/
typedef struct {
	U8  			type;
	int 			sockfd;
	char          	buffer[MSG_LEN];
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
