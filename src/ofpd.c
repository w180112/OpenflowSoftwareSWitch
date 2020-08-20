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
#include 			"ofp.h"
#include 			"dp.h"

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

pid_t ofp_cp_pid, ofp_dp_pid = 0, ofp_cmd_pid, tmr_pid, ofp_main_pid;
dp_io_fds_t			*dp_io_fds_head = NULL;

STATUS OFP_ipc_init(void);
int dp_init(void);
void OFP_bye(int signal_num);

extern void 		init_tables(void);

/*---------------------------------------------------------
 * ofp_bye : signal handler for INTR-C only
 *--------------------------------------------------------*/
void OFP_bye(__attribute__((unused)) int signal_num)
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
		ofp_ports[i].cp_enable = FALSE;
		ofp_ports[i].dp_enable = FALSE;
		ofp_ports[i].query_cnt = 1;
		ofp_ports[i].state = S_CLOSED;
		ofp_ports[i].port = i;
	}
	init_tables();
	sleep(1);
	ofp_ports[0].cp_enable = TRUE;
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
	int 			ret;
	
	if (argc != 3) {
		puts("Usage: ./osw <OpenFlow NIC name> <SDN controller ip>");
		return -1;
	}
	signal(SIGINT, OFP_bye);
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
   		ofp_sockd_cp(&ofp_ports[0]);
    }
	if ((ofp_dp_pid=fork()) == 0) {
   		dp(dpQid);
    }
	/*if ((ofp_dp_pid=fork()) == 0) {
   		ofp_sockd_dp();
    }*/
	printf("pid = %d %d %d %d", ofp_cp_pid, ofp_dp_pid, ofp_cmd_pid, tmr_pid);
	
	strncpy(ofp_ports[0].of_ifname, argv[1], strlen(argv[1]));
	strncpy(ofp_ports[0].ctrl_ip, argv[2], strlen(argv[2]));
    signal(SIGINT, OFP_bye);
	ofp_loop(&ofp_ports[0]);

	return 0;
}
