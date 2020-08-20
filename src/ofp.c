/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFP.C

    - purpose : for ofp detection
	
  Designed by THE on 1/Aug/'20
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include 			<libbridge.h>
#include        	<common.h>
#include 			<unistd.h>

#include        	"ofpd.h"
#include			"ofp_codec.h"
#include			"ofp_fsm.h"
#include        	"ofp_dbg.h"
#include 			"ofp_sock.h" 
#include			"ofp_cmd.h"
#include 			"dp_sock.h"
#include 			"ofp_flow.h"

extern tIPC_ID 		ofpQid;
extern tIPC_ID 		dpQid;

extern int			ofp_io_fds[2];
extern pid_t		ofp_cp_pid;
extern void 		OFP_encode_packet_in(tOFP_MBX *mail, tOFP_PORT *port_ccb);
extern STATUS 		OFP_encode_port_status(tOFP_MBX *mail, tOFP_PORT *port_ccb, uint32_t *port);
extern void 		print_ofp_flow_table(void);
extern void 		print_tuple(void);
           
/***************************************************************
 * ofp : 
 *
 ***************************************************************/
int ofp_loop(tOFP_PORT *port_ccb)
{
	tIPC_PRIM		*ipc_prim;
	cli_2_ofp_t 	*cli_2_ofp;
	cli_2_dp_t 		cli_2_dp;
    tOFP_MBX		*mail, mail_2_msgq;
	tOFP_PORT		*ccb;
	tMBUF   		mbuf;
	int				msize;
	//U16			port;
	U16				event;
	U16				ipc_type;
	int 			ret;

	port_ccb->sockfd = ofp_io_fds[0];
    OFP_FSM(port_ccb, E_START);
    
	for(;;) {
		//printf("\n===============================================\n");
		//printf("%s> waiting for ipc_rcv2() ...\n",CODE_LOCATION);
	    if (ipc_rcv2(ofpQid, &mbuf, &msize) == ERROR) {
	    	//printf("\n%s> ipc_rcv2 error ...\n",CODE_LOCATION);
	    	continue;
	    }
	    ipc_type = *(U16 *)mbuf.mtext;
	    //printf("ipc_type=%d\n",ipc_type);
		
		switch(ipc_type){
		case IPC_EV_TYPE_TMR:
			ipc_prim = (tIPC_PRIM *)mbuf.mtext;
			ccb = ipc_prim->ccb;
			event = ipc_prim->evt;
			OFP_FSM(ccb, event);
			break;
		
		case IPC_EV_TYPE_DRV:
			mail = (tOFP_MBX *)mbuf.mtext;
			//printf("<%d at %s\n", __LINE__, __FILE__);
			//ofp_ports[port].port = TEST_PORT_ID;
			//DBG_OFP(DBGLVL1,&ofp_ports[0],"<-- Rx ofp message\n");
			if ((ret=OFP_decode_frame(mail, port_ccb)) == ERROR) {
				//printf("<%d at %s\n", __LINE__, __FILE__);
				continue;
			}
			else if (ret == FALSE) {
				//printf("<%d at ofpd\n", __LINE__);
				for(;;) {
					sleep(1);
					if (ofpdInit(port_ccb->of_ifname, port_ccb->ctrl_ip) == 0)
						break;
				}
				if ((ofp_cp_pid=fork()) == 0) {
					signal(SIGINT,SIG_DFL);
   					ofp_sockd_cp(port_ccb);
				}
				port_ccb->sockfd = ofp_io_fds[0];
				OFP_FSM(port_ccb, E_START);
				puts("====================Restart connection.====================");
				continue;
			}
			//printf("<%d at ofpd\n", __LINE__);
			//OFP_FSM(&ofp_ports[0], ofp_ports[0].event);
			//printf("<%d at %s\n", __LINE__, __FILE__);
			break;
		
		case IPC_EV_TYPE_CLI:
			mail = (tOFP_MBX *)mbuf.mtext;
			cli_2_ofp = (cli_2_ofp_t *)(mail->refp);
			switch(cli_2_ofp->opcode)
			{
				case ADD_IF:
					OFP_encode_port_status(mail, port_ccb, &(cli_2_ofp->port_id));
					OFP_FSM(port_ccb, E_PORT_STATUS);
				case SHOW_FLOW:
					//printf("<%d at ofpd\n", __LINE__);
					print_ofp_flow_table();
					print_tuple();
					memcpy(&cli_2_dp.cli_2_ofp, cli_2_ofp, sizeof(cli_2_ofp_t));
					cli_2_dp.msg_type = CLI;
					U16 mulen = sizeof(cli_2_dp_t);
					U8 *mu = (U8 *)&cli_2_dp;

    				if (dpQid == -1) {
						if ((dpQid=msgget(DP_Q_KEY,0600|IPC_CREAT)) < 0) {
							printf("send> Oops! dpQ(key=0x%x) not found\n",DP_Q_KEY);
   	 					}
					}
	
					if (mulen > MSG_LEN) {
	 					printf("Incoming frame length(%d) is too large at ofpd.c!\n",mulen);
						return ERROR;
					}

					mail_2_msgq.len = mulen;
					memcpy(mail_2_msgq.refp, mu, mulen); /* mail content will be copied into mail queue */
	
					//printf("dp_send2mailbox(dp_sock.c %d): mulen=%d\n",__LINE__,mulen);
					mail_2_msgq.type = IPC_EV_TYPE_OFP;
					printf("send cli msg to dp\n");
					ipc_sw(dpQid, &mail_2_msgq, sizeof(mail_2_msgq), -1);
					break;
				case SHOW_TUPLE:
 					print_tuple();
 					break;
				default:
					;
			}
			break;
		case IPC_EV_TYPE_DP:
			mail = (tOFP_MBX*)mbuf.mtext;
			if ((ret = find_tuple_and_rule(mail)) >= 0)
 				send_back2dp(ret);
			OFP_encode_packet_in(mail, port_ccb);
			port_ccb->event = E_PACKET_IN;
			//PRINT_MESSAGE(ofp_ports[0].ofpbuf, ofp_ports[0].ofpbuf_len);
			OFP_FSM(port_ccb, port_ccb->event);
			break;
		default:
		    ;
		}
    }
	return 0;
}
