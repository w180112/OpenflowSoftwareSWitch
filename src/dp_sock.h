/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   DP_SOCK.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _DP_SOCK_H_
#define _DP_SOCK_H_

#include       	<common.h>
#include		<sys/socket.h>
#include 		<sys/types.h> 
#include 		<netinet/in.h>
#include 		<rte_atomic.h>
#include 		"ofpd.h"

typedef struct dp_io_fds {
	int 				fd;
	uint32_t 			port_no;
	char 				ifname[16];
	uint64_t 			pkt_count;
	uint32_t 			err_count;
	struct dp_io_fds 	*next;
}dp_io_fds_t;

extern rte_atomic16_t		any2dp_cums;
extern int16_t				any2dp_prod;

int 			DP_SOCK_INIT(char *ifname, uint32_t port_id, dp_io_fds_t **dp_io_fds_head);
//void 			sockd_dp(dp_io_fds_t *dp_io_fds_head);
extern STATUS 	dp_send2mailbox(U8 *mu, int mulen);
extern int 		dp_rx(uint16_t *port_id);
extern int 		dp_tx(uint16_t *port_id);
extern int		msg_dequeue(struct rte_ring *ring, tOFP_MBX **mail);

#endif