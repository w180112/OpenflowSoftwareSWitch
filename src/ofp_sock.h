/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   OFP_SOCK.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFP_SOCK_H_
#define _OFP_SOCK_H_

#ifdef __cplusplus  
extern "C" {
#endif   

#include "ofpd.h"

#define CP_FD 0
#define DP_FD 1

#define DRIV_CP 0
#define DRIV_DP 1
#define DRIV_FAIL 2

extern void           drv_xmit(U8 *mu, U16 mulen, int fd_id);
extern int 						OFP_SOCK_INIT(char *if_name, char *ctrl_ip);
extern int 					  ofp_sockd_cp(tOFP_PORT *port_ccb);

#ifdef __cplusplus
}
#endif

#endif 
