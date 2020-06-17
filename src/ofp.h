
#ifndef _OFP_H_
#define _OFP_H_

extern int ofpdInit(char *of_ifname, char *ctrl_ip);
extern int ofp_loop(tOFP_PORT *port_ccb);
extern int ofp_timer(tOFP_PORT *port_ccb);

#endif 