/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFPD.H

     For ofp detection

  Designed by THE on SEP 26, 2019
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFPD_H_
#define _OFPD_H_

#include <ip_codec.h>
#include <rte_timer.h>
#include "ofp_flow.h"
#include "ofp.h"

#define OFP_Q_KEY				0x0b00
#define DP_Q_KEY				0x0c00
#define JUMBO_MTU				9000
#define TEST_PORT_ID			1
#define PKT_BUF					1000

#define	MIN_FRAME_SIZE			64
#define	MAX_QUE_EVT_CNT			(MBOX_QUE_SIZE/2)
#define _PBM(port)				(1<<(port-1))

#define MAX_USER_PORT_NUM		10
#define MAX_OFP_QUERY_NUM		10

#define BURST_SIZE 				32

extern struct rte_mempool 		*mbuf_pool;
extern struct rte_ring 			*any2dp;
extern struct rte_ring 			*dp2tx;
extern struct rte_ring			*any2dp;
extern struct rte_ring			*any2ofp;

extern U8	 		g_loc_mac[]; //system mac addr -- global variable
extern tOFP_PORT	ofp_ports[];
extern tIPC_ID 		ofpQid;
extern tIPC_ID 		dpQid;
extern U32			ofp_interval;
extern U8			ofp_max_msg_per_query;

#endif