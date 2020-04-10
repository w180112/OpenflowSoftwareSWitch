/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   OFP_CMD.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFP_CMD_H_
#define _OFP_CMD_H_

#ifdef __cplusplus  
extern "C" {
#endif

#define ADD_BR 0
#define DEL_BR 1
#define ADD_IF 2
#define DEL_IF 3

typedef struct cmd_list {
    int		nargs;
    const char	*name;
	  int		(*func)(int argc, char argv[4][64]);
	  const char 	*help;
}cmd_list_t;

typedef struct cli_2_main {
    uint8_t opcode;
    char brname[64];
    char ifname[64];
    char msg[256];
}cli_2_main_t;

extern void 			ofp_cmd(void);
extern STATUS 			ofp_send2mailbox(U8 *mu, int mulen);

#ifdef __cplusplus
}
#endif

#endif 