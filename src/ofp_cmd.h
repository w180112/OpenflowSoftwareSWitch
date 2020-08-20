/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   OFP_CMD.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFP_CMD_H_
#define _OFP_CMD_H_

#ifdef __cplusplus  
extern "C" {
#endif

#include "ofp_sock.h"

enum {
    ADD_BR = 0,
    DEL_BR,
    ADD_IF,
    DEL_IF,
    SHOW_FLOW,
    SHOW_TUPLE,
};

typedef struct cmd_list {
    int		nargs;
    const char	*name;
	int		(*func)(int argc, char argv[4][64]);
	const char 	*help;
}cmd_list_t;

typedef struct cli_2_ofp {
    uint8_t opcode;
    char brname[16];
    char ifname[16];
    uint32_t port_id;
    char msg[256];
}cli_2_ofp_t;

typedef struct cli_2_dp {
    uint8_t     msg_type;
    cli_2_ofp_t cli_2_ofp;
}cli_2_dp_t;

extern void 			ofp_cmd(void);
extern STATUS 			ofp_send2mailbox(U8 *mu, int mulen);

#ifdef __cplusplus
}
#endif

#endif 