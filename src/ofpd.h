/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFPD.H

     For ofp detection

  Designed by THE on SEP 26, 2019
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFPD_H_
#define _OFPD_H_

#include "ofp_common.h"
#include "ofp_asyn.h"
#include "ofp_ctrl2sw.h"
#include "dp_flow.h"
#include <ip_codec.h>
#include "ofp.h"

#define OFP_Q_KEY				0x0b00
#define DP_Q_KEY				0x0c00
#define ETH_MTU					1514
#define MSG_LEN					4074
#define JUMBO_MTU				9000
#define TEST_PORT_ID			1
#define PKT_BUF					1000

#define	MIN_FRAME_SIZE			64
#define	MAX_QUE_EVT_CNT			(MBOX_QUE_SIZE/2)
#define _PBM(port)				(1<<(port-1))

#define MAX_USER_PORT_NUM		10
#define MAX_OFP_QUERY_NUM		10

extern U8	 		g_loc_mac[]; //system mac addr -- global variable
extern tOFP_PORT	ofp_ports[];
extern tIPC_ID 		ofpQid;
extern tIPC_ID 		dpQid;
extern U32			ofp_interval;
extern U8			ofp_max_msg_per_query;

extern int ofpdInit(char *of_ifname, char *ctrl_ip);

#endif