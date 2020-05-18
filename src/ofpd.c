/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFPD.C

    - purpose : for ofp detection
	
  Designed by THE on 30/09/'19
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include 			<libbridge.h>
#include        	<common.h>
#include 			<unistd.h>
#include			<pthread.h>
#include 			<sys/mman.h>
#include 			<semaphore.h>

#include        	"ofpd.h"
#include			"ofp_codec.h"
#include			"ofp_fsm.h"
#include        	"ofp_dbg.h"
#include 			"ofp_sock.h" 
#include			"ofp_cmd.h"
#include 			"dp_sock.h"

U32					ofp_ttl;
U32					ofp_interval;
U16					ofp_init_delay;
U8					ofp_max_msg_per_query;

tOFP_PORT			ofp_ports[MAX_USER_PORT_NUM+1]; //port is 1's based

tIPC_ID 			ofpQid=-1;
tIPC_ID 			dpQid=-1;

sem_t *sem;
tDP_MSG *dp_buf;
int *post_index, *pre_index;

extern int			ofp_io_fds[2];
extern void 		sockd_dp(dp_io_fds_t *dp_io_fds_head);
extern void 		dp(tIPC_ID dpQid);
extern void 		OFP_encode_packet_in(tOFP_MBX *mail, tOFP_PORT *port_ccb);
extern STATUS 		OFP_encode_port_status(tOFP_MBX *mail, tOFP_PORT *port_ccb, uint32_t *port);

pid_t ofp_cp_pid, ofp_dp_pid, dp_pid = 0, ofp_cmd_pid, tmr_pid;

/*---------------------------------------------------------
 * ofp_bye : signal handler for INTR-C only
 *--------------------------------------------------------*/
void OFP_bye()
{
    printf("ofp> delete Qid(0x%x)\n",ofpQid);
    DEL_MSGQ(ofpQid);
	printf("dp> delete Qid(0x%x)\n",dpQid);
    DEL_MSGQ(dpQid);
	sem_destroy(sem);
    munmap(sem, sizeof(sem_t));
    munmap(dp_buf, sizeof(tDP_MSG)*PKT_BUF);
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

	sem = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	dp_buf = mmap(NULL, sizeof(tDP_MSG)*PKT_BUF, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	post_index = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	pre_index = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	*post_index = 0;
	*pre_index = 0;
	sem_init(sem, 1, 1);
	memset(dp_buf, 0, sizeof(tDP_MSG)*PKT_BUF);

	return TRUE;
}

/**************************************************************
 * ofpdInit: 
 *
 **************************************************************/
int ofpdInit(char *of_ifname, char *ctrl_ip)
{
	U16  i;
	
	if (OFP_SOCK_INIT(of_ifname, ctrl_ip) < 0){ //must be located ahead of system init
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
	}
	
	sleep(1);
	ofp_ports[0].enable = TRUE;
	ofp_max_msg_per_query = MAX_OFP_QUERY_NUM;
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
	
	if (argc != 3) {
		puts("Usage: ./osw <OpenFlow NIC name> <SDN controller ip>");
		return -1;
	}

	if (ofpdInit(argv[1], argv[2]) < 0)
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
	printf("pid = %d %d %d %d", ofp_cp_pid, dp_pid, ofp_cmd_pid, tmr_pid);
	strncpy(ofp_ports[0].of_ifname, argv[1], strlen(argv[1]));
	strncpy(ofp_ports[0].ctrl_ip, argv[2], strlen(argv[2]));
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
			//ofp_ports[port].port = TEST_PORT_ID;
			//DBG_OFP(DBGLVL1,&ofp_ports[0],"<-- Rx ofp message\n");
			if ((ret=OFP_decode_frame(mail, &ofp_ports[0])) == ERROR) {
				//printf("<%d at ofpd\n", __LINE__);
				continue;
			}
			else if (ret == FALSE) {
				//printf("<%d at ofpd\n", __LINE__);
				for(;;) {
					sleep(1);
					if (ofpdInit(ofp_ports[0].of_ifname, ofp_ports[0].ctrl_ip) == 0)
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
			//printf("<%d at ofpd\n", __LINE__);
			//OFP_FSM(&ofp_ports[0], ofp_ports[0].event);
			break;
		
		case IPC_EV_TYPE_CLI:
			mail = (tOFP_MBX *)mbuf.mtext;
			cli_2_ofp = (cli_2_ofp_t *)(mail->refp);
			switch(cli_2_ofp->opcode)
			{
				case ADD_IF:
					OFP_encode_port_status(mail, &ofp_ports[0], &(cli_2_ofp->port_id));
					OFP_FSM(&ofp_ports[0], E_PORT_STATUS);
				case SHOW_FLOW:
					printf("<%d at ofpd\n", __LINE__);
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
				default:
					;
			}
			break;
		case IPC_EV_TYPE_DP:
			mail = (tOFP_MBX*)mbuf.mtext;
			//printf("<%d at ofpd\n", __LINE__);
			OFP_encode_packet_in(mail, &ofp_ports[0]);
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
