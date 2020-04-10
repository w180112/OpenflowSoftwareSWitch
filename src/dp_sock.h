/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   DP_SOCK.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _DP_SOCK_H_
#define _DP_SOCK_H_

#include       	<common.h>
#include		<sys/socket.h>
#include 		<sys/types.h> 
#include 		<netinet/in.h>

typedef struct dp_io_fds {
	int fd;
	uint16_t port_no;
	char ifname[64];
	struct dp_io_fds *next;
}dp_io_fds_t;

int DP_SOCK_INIT(char *ifname, dp_io_fds_t **dp_io_fds_head);
void sockd_dp(dp_io_fds_t *dp_io_fds_head);
extern STATUS dp_send2mailbox(U8 *mu, int mulen);

#endif