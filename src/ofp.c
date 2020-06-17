#include <common.h>
#include "ofpd.h"
#include "ofp_dbg.h"
#include "ofp_fsm.h"
#include "ofp_cmd.h"
#include "ofp_codec.h"
#include "ofp_sock.h"
#include <rte_timer.h>
#include <rte_memcpy.h>
#include "dp_sock.h"
#include "mailbox.h"

int ofpdInit(tOFP_PORT *port_ccb);
int ofp_loop(tOFP_PORT *port_ccb);
int ofp_timer(tOFP_PORT *port_ccb);

extern tOFP_PORT	ofp_ports[MAX_USER_PORT_NUM+1];
extern int			ofp_io_fds[2];

pid_t tmr_pid;
//sem_t 				*sem;
//tDP_MSG 			*dp_buf;

extern void 		OFP_encode_packet_in(tOFP_MBX *mail, tOFP_PORT *port_ccb);
extern STATUS 		OFP_encode_port_status(tOFP_MBX *mail, tOFP_PORT *port_ccb, uint32_t *port);
extern void 		init_tables(void);
extern void 		print_ofp_flow_table(void);
extern void 		print_tuple(void);

/**************************************************************
 * ofpdInit: 
 *
 **************************************************************/
int ofpdInit(__attribute__((unused)) tOFP_PORT *port_ccb)
{
	U16  i;
	
	if (OFP_SOCK_INIT(ofp_ports[0].of_ifname, ofp_ports[0].ctrl_ip) < 0){ //must be located ahead of system init
		return -1;
	}
	
	ofp_interval = (U32)(10*SEC);
	//tmr_pid = tmrInit();
    
    //--------- default of all ports ----------
	for(i=1; i<=MAX_USER_PORT_NUM; i++){
		rte_atomic16_init(&ofp_ports[i].cp_enable);
		rte_atomic16_set(&ofp_ports[i].cp_enable, 0);
		ofp_ports[i].query_cnt = 1;
		ofp_ports[i].state = S_CLOSED;
		ofp_ports[i].port = i;
	}
	init_tables();
	sleep(1);
	ofp_max_msg_per_query = MAX_OFP_QUERY_NUM;
	DBG_OFP(DBGLVL1,NULL,"============ ofp init successfully ==============\n");

	return 0;
}
#pragma GCC diagnostic push  // require GCC 4.6
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough="
int ofp_loop(tOFP_PORT *port_ccb)
{
	tIPC_PRIM		*ipc_prim;
	cli_2_ofp_t 	*cli_2_ofp;
	cli_2_dp_t 		cli_2_dp;
    tOFP_MBX		*mail[BURST_SIZE], mail_2_dp;
	tOFP_PORT		*ccb;
	tMBUF   		mbuf;
	int				msize, ret;
	//U16			port;
	U16				event;
	U16				recv_type;
	uint16_t 		burst_size;

	if (ofpdInit(port_ccb) < 0)
		return -1;
	rte_atomic16_set(&ofp_ports[0].cp_enable, 1);
	ofp_ports[0].sockfd = ofp_io_fds[0];
    OFP_FSM(&ofp_ports[0], E_START);

	for(;;) {
		
		//printf("%s> waiting for ipc_rcv2() ...\n",CODE_LOCATION);
	    /*if (ipc_rcv2(ofpQid, &mbuf, &msize) == ERROR) {
	    	//printf("\n%s> ipc_rcv2 error ...\n",CODE_LOCATION);
	    	continue;
	    }*/
		burst_size = msg_dequeue(any2ofp, mail);
	    
	    //printf("\n===============================================\n");
		for (int i=0; i<burst_size; i++) {
			recv_type = *(U16 *)mail[i];
			//printf("mail->type = %u, len = %d\n", mail[i]->type, mail[i]->len);
			switch(recv_type){
			case IPC_EV_TYPE_TMR:
				/*ipc_prim = (tIPC_PRIM *)mbuf.mtext;
				ccb = ipc_prim->ccb;
				event = ipc_prim->evt;
				OFP_FSM(ccb, event);*/
				break;
		
			case IPC_EV_TYPE_DRV:
				//mail[i] = (tOFP_MBX *)mbuf.mtext;
				//ofp_ports[port].port = TEST_PORT_ID;
				//DBG_OFP(DBGLVL1,&ofp_ports[0],"<-- Rx ofp message\n");
				if ((ret=OFP_decode_frame(mail[i], &ofp_ports[0])) == ERROR) {
					//printf("<%d at ofpd\n", __LINE__);
					continue;
				}
				else if (ret == FALSE) {
					//printf("<%d at ofpd\n", __LINE__);
					for(;;) {
						sleep(1);
						if (ofpdInit(port_ccb) == 0)
							break;
					}
					/*if ((ofp_cp_pid=fork()) == 0) {
						signal(SIGINT,SIG_DFL);
   						ofp_sockd_cp();
					}
					ofp_ports[0].sockfd = ofp_io_fds[0];
					OFP_FSM(&ofp_ports[0], E_START);
					puts("====================Restart connection.====================");
					continue;*/
				}
				//printf("<%d at ofpd\n", __LINE__);
				//OFP_FSM(&ofp_ports[0], ofp_ports[0].event);
				break;
		
			case IPC_EV_TYPE_CLI:
				//mail = (tOFP_MBX *)mbuf.mtext;
				cli_2_ofp = (cli_2_ofp_t *)(mail[i]->refp);
				switch(cli_2_ofp->opcode)
				{
					case ADD_IF:
						OFP_encode_port_status(mail[i], &ofp_ports[0], &(cli_2_ofp->port_id));
						OFP_FSM(&ofp_ports[0], E_PORT_STATUS);
					case SHOW_FLOW:
						//printf("<%d at ofpd\n", __LINE__);
						print_ofp_flow_table();
						print_tuple();
						rte_memcpy(&cli_2_dp.cli_2_ofp, cli_2_ofp, sizeof(cli_2_ofp_t));
						cli_2_dp.msg_type = CLI;
						U8 *mu = (U8 *)&cli_2_dp;
						mail_2_dp.len = sizeof(cli_2_dp_t);
						rte_memcpy(mail_2_dp.refp, mu, mail_2_dp.len);
						mail_2_dp.type = IPC_EV_TYPE_OFP;
						mailbox(any2dp, (U8 *)&mail_2_dp, 1);
    					/*if (dpQid == -1) {
							if ((dpQid=msgget(DP_Q_KEY,0600|IPC_CREAT)) < 0) {
								printf("send> Oops! dpQ(key=0x%x) not found\n",DP_Q_KEY);
   	 						}
						}
	
						if (mulen > MSG_LEN) {
	 						printf("Incoming frame length(%d) is too large at ofpd.c!\n",mulen);
							return ERROR;
						}

						mail_2_msgq.len = mulen;
						memcpy(mail_2_msgq.refp, mu, mulen);
	
						printf("dp_send2mailbox(dp_sock.c %d): mulen=%d\n",__LINE__,mulen);
						mail_2_msgq.type = IPC_EV_TYPE_OFP;
						printf("send cli msg to dp\n");
						ipc_sw(dpQid, &mail_2_msgq, sizeof(mail_2_msgq), -1);*/
						break;
					case SHOW_TUPLE:
						print_tuple();
						break;
					default:
						;
				}
				break;
			case IPC_EV_TYPE_DP:
				//mail = (tOFP_MBX*)mbuf.mtext;
				//printf("<%d at ofpd\n", __LINE__);
				OFP_encode_packet_in(mail[i], &ofp_ports[0]);
				//printf("<%d at ofpd\n", __LINE__);
				ofp_ports[0].event = E_PACKET_IN;
				//puts("recv pkt_in");
				//PRINT_MESSAGE(ofp_ports[0].ofpbuf, ofp_ports[0].ofpbuf_len);
				OFP_FSM(&ofp_ports[0], ofp_ports[0].event);
				//printf("<%d at ofpd\n", __LINE__);
				break;
			default:
		    	;
			}
		}
    }
	return 0;
}
#pragma GCC diagnostic pop 

#define TIMER_RESOLUTION_CYCLES 20000000ULL /* around 10ms at 2 Ghz */

int ofp_timer(__attribute__((unused)) tOFP_PORT *port_ccb) 
{
	uint64_t prev_tsc = 0, cur_tsc, diff_tsc;

	for(;;) {
		cur_tsc = rte_rdtsc();
		diff_tsc = cur_tsc - prev_tsc;
		if (diff_tsc > TIMER_RESOLUTION_CYCLES) {
			rte_timer_manage();
			prev_tsc = cur_tsc;
		}
	}
	return 0;
}