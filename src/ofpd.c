/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFPD.C

    - purpose : for ofp detection
	
  Designed by THE on 30/09/'19
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include 			<libbridge.h>
#include        	<common.h>
#include 			<unistd.h>
#include			<pthread.h>

#include        	"ofpd.h"
#include			"ofp_codec.h"
#include			"ofp_fsm.h"
#include        	"ofp_dbg.h"
#include 			"ofp_sock.h" 
#include			"ofp_cmd.h"
#include 			"dp_sock.h"

BOOL				ofp_testEnable=FALSE;
U32					ofp_ttl;
U32					ofp_interval;
U16					ofp_init_delay;
U8					ofp_max_msg_per_query;

tOFP_PORT			ofp_ports[MAX_USER_PORT_NUM+1]; //port is 1's based

tIPC_ID 			ofpQid=-1;
tIPC_ID 			dpQid=-1;

extern int			ofp_io_fds[2];
extern int 			DP_SOCK_INIT(char *ifname, dp_io_fds_t **dp_io_fds_head);
extern void 		sockd_dp(dp_io_fds_t *dp_io_fds_head);
extern void 		dp(tIPC_ID dpQid);
extern void 		OFP_encode_packet_in(tOFP_MBX *mail, tOFP_PORT *port_ccb);

pid_t ofp_cp_pid;
pid_t ofp_dp_pid;
pid_t dp_pid = 0;
pid_t ofp_cmd_pid;
pid_t tmr_pid;

/*---------------------------------------------------------
 * ofp_bye : signal handler for INTR-C only
 *--------------------------------------------------------*/
void OFP_bye()
{
    printf("ofp> delete Qid(0x%x)\n",ofpQid);
    DEL_MSGQ(ofpQid);
	printf("dp> delete Qid(0x%x)\n",dpQid);
    DEL_MSGQ(dpQid);
    printf("bye!\n");
	exit(0);
}

/*---------------------------------------------------------
 * OFP_ipc_init
 *--------------------------------------------------------*/
STATUS OFP_ipc_init(void)
{
	if (ofpQid != -1){
		printf("ofp> Qid(0x%x) is already existing\n",ofpQid);
		return TRUE;
	}
	
	if (GET_MSGQ(&ofpQid,OFP_Q_KEY) == ERROR){
	   	printf("ofp> can not create a msgQ for key(0x0%x)\n",OFP_Q_KEY);
	   	return ERROR;
	}
	printf("ofp> new Qid(0x%x)\n",ofpQid);
	return TRUE;
}

/*---------------------------------------------------------
 * DP_ipc_init
 *--------------------------------------------------------*/
STATUS DP_ipc_init(void)
{
	if (dpQid != -1){
		printf("dp> Qid(0x%x) is already existing\n",dpQid);
		return TRUE;
	}
	
	if (GET_MSGQ(&dpQid,DP_Q_KEY) == ERROR){
	   	printf("dp> can not create a msgQ for key(0x0%x)\n",DP_Q_KEY);
	   	return ERROR;
	}
	printf("dp> new Qid(0x%x)\n",dpQid);
	return TRUE;
}

/**************************************************************
 * ofpdInit: 
 *
 **************************************************************/
int ofpdInit(void)
{
	U16  i;
	
	if (OFP_SOCK_INIT() < 0){ //must be located ahead of system init
		return -1;
	}
	
	ofp_interval = (U32)(10*SEC);
	tmr_pid = tmrInit();
	OFP_ipc_init();
    
    //--------- default of all ports ----------
	for(i=1; i<=MAX_USER_PORT_NUM; i++){
		ofp_ports[i].enable = FALSE;
		ofp_ports[i].query_cnt = 1;
		ofp_ports[i].state = S_CLOSED;
		ofp_ports[i].port = i;
		
		ofp_ports[i].imsg_cnt =
		ofp_ports[i].err_imsg_cnt =
		ofp_ports[i].omsg_cnt = 0;
		ofp_ports[i].head = NULL;
	}
	
	sleep(1);
	ofp_max_msg_per_query = MAX_OFP_QUERY_NUM;
	ofp_testEnable = TRUE; //to let driver ofp msg come in ...
	DBG_OFP(DBGLVL1,NULL,"============ ofp init successfully ==============\n");

	return 0;
}

/**************************************************************
 * dp_init: 
 *
 **************************************************************/
int dp_init(void)
{	
	DP_ipc_init();
	DBG_OFP(DBGLVL1,NULL,"============ dp init successfully ==============\n");

	return 0;
}
            
/***************************************************************
 * ofpd : 
 *
 ***************************************************************/
int main(int argc, char **argv)
{
	extern STATUS	OFP_FSM(tOFP_PORT *port_ccb, U16 event);
	tIPC_PRIM		*ipc_prim;
	cli_2_main_t 	*cli_2_main;
    tOFP_MBX		*mail;
	tOFP_PORT		*ccb;
	tMBUF   		mbuf;
	int				msize;
	//U16				port;
	U16				event;
	U16				ipc_type;
	int 			ret;
	dp_io_fds_t		*dp_io_fds_head = NULL;
	pthread_t 		dp_thread = 0;
	
	if (ofpdInit() < 0)
		return -1;
	if (dp_init() < 0)
		return -1;
	if ((ret=br_init()) != 0) {
		perror("bridge init failed:");
		return -1;
	}
	if ((ofp_cmd_pid=fork()) == 0)
		ofp_cmd();
	if ((ofp_cp_pid=fork()) == 0) {
   		ofp_sockd_cp();
    }
	if ((dp_pid=fork()) == 0) {
   		dp(dpQid);
    }
	/*if ((ofp_dp_pid=fork()) == 0) {
   		ofp_sockd_dp();
    }*/
	
    signal(SIGINT, OFP_bye);
	ofp_ports[0].sockfd = ofp_io_fds[0];
    OFP_FSM(&ofp_ports[0], E_START);
    
	for(;;) {
		//printf("\n===============================================\n");
		//printf("%s> waiting for ipc_rcv2() ...\n",CODE_LOCATION);
	    if (ipc_rcv2(ofpQid, &mbuf, &msize) == ERROR) {
	    	//printf("\n%s> ipc_rcv2 error ...\n",CODE_LOCATION);
	    	continue;
	    }
	    
	    ipc_type = *(U16*)mbuf.mtext;
	    //printf("ipc_type=%d\n",ipc_type);
		
		switch(ipc_type){
		case IPC_EV_TYPE_TMR:
			ipc_prim = (tIPC_PRIM*)mbuf.mtext;
			ccb = ipc_prim->ccb;
			event = ipc_prim->evt;
			OFP_FSM(ccb, event);
			break;
		
		case IPC_EV_TYPE_DRV:
			mail = (tOFP_MBX*)mbuf.mtext;
			//ofp_ports[port].port = TEST_PORT_ID;
			//DBG_OFP(DBGLVL1,&ofp_ports[0],"<-- Rx ofp message\n");
			if ((ret=OFP_decode_frame(mail, &ofp_ports[0])) == ERROR)
				continue;
			else if (ret == FALSE) {
				for(;;) {
					sleep(1);
					if (ofpdInit() == 0)
						break;
				}
				if ((ofp_cp_pid=fork()) == 0) {
					signal(SIGINT,SIG_DFL);
   					ofp_sockd_cp();
				}
				ofp_ports[0].sockfd = ofp_io_fds[0];
				OFP_FSM(&ofp_ports[0], E_START);
				puts("====================Restart connection.====================");
				continue;
			}
			OFP_FSM(&ofp_ports[0], ofp_ports[0].event);
			break;
		
		case IPC_EV_TYPE_CLI:
			mail = (tOFP_MBX*)mbuf.mtext;
			cli_2_main = (cli_2_main_t *)(mail->refp);
			if (cli_2_main->opcode == ADD_IF) {
				//printf("<%d\n", __LINE__);
				DP_SOCK_INIT(cli_2_main->ifname, &dp_io_fds_head);
				printf("dp_io_fds_head = %x\n", dp_io_fds_head);
				if (dp_thread == 0) {
					pthread_create(&dp_thread, NULL, (void *)sockd_dp, dp_io_fds_head);
    			}
			}
			break;
		case IPC_EV_TYPE_DP:
			mail = (tOFP_MBX*)mbuf.mtext;
			OFP_encode_packet_in(mail, &ofp_ports[0]);
			ofp_ports[0].event = E_PACKET_IN;
			puts("recv pkt_in");
			PRINT_MESSAGE(ofp_ports[0].ofpbuf, ofp_ports[0].ofpbuf_len);
			OFP_FSM(&ofp_ports[0], ofp_ports[0].event);
			break;
		default:
		    ;
		}
    }
}
