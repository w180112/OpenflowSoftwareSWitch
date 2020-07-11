/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFPD.C

    - purpose : for ofp detection
	
  Designed by THE on 30/09/'19
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

//#include 			<libbridge.h>
#include        	<common.h>
#include 			<unistd.h>
#include			<pthread.h>
#include 			<sys/mman.h>
#include 			<semaphore.h>
#include			<rte_log.h>
#include 			<rte_errno.h>
#include 			<rte_mbuf.h>
#include 			<rte_ethdev.h>

#include        	"ofpd.h"
#include			"ofp_codec.h"
#include			"ofp_fsm.h"
#include        	"ofp_dbg.h"
#include 			"ofp_sock.h" 
#include 			"ofp.h"
#include			"ofp_cmd.h"
#include 			"dp_sock.h"
#include 			"dp_port_info.h"
#include 			"dp.h"
 
#define 				RING_SIZE 		16384
#define 				NUM_MBUFS 		8191
#define 				MBUF_CACHE_SIZE 512

U32					ofp_ttl;
U32					ofp_interval;
U16					ofp_init_delay;
U8					ofp_max_msg_per_query;

tOFP_PORT			ofp_ports[MAX_USER_PORT_NUM+1]; //port is 1's based

tIPC_ID 			dpQid=-1;
tIPC_ID 			ofpQid=-1;

int 				*post_index, *pre_index;
struct rte_mempool 	*mbuf_pool;
struct rte_ring 	*any2dp, *dp2tx, *any2dp, *any2ofp, *cmd2ofp;

pid_t ofp_cp_pid, ofp_dp_pid, dp_pid = 0, ofp_main_pid, ofp_cmd_pid;
dp_io_fds_t			*dp_io_fds_head = NULL;

STATUS OFP_ipc_init(void);
int dp_init(void);
void OFP_bye(int signal_num);

/*---------------------------------------------------------
 * ofp_bye : signal handler for INTR-C only
 *--------------------------------------------------------*/
void OFP_bye(__attribute__((unused)) int signal_num)
{
    /*printf("ofp> delete Qid(0x%x)\n",ofpQid);
    DEL_MSGQ(ofpQid);
	printf("dp> delete Qid(0x%x)\n",dpQid);
    DEL_MSGQ(dpQid);*/
	//sem_destroy(sem);
    //munmap(sem, sizeof(sem_t));
    //munmap(dp_buf, sizeof(tDP_MSG)*PKT_BUF);
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

/**************************************************************
 * dp_init: 
 *
 **************************************************************/
int dp_init(void)
{	
	uint16_t portid;
	//any2dp = rte_ring_create("rx_2_dp", RING_SIZE, rte_socket_id(), 0);
	//dp2tx = rte_ring_create("dp_2_tx", RING_SIZE, rte_socket_id(), 0);
	rte_atomic16_init(&ofp_ports[0].dp_enable);
	rte_atomic16_set(&ofp_ports[0].dp_enable, 0);
	/* Initialize all ports. */
	RTE_ETH_FOREACH_DEV(portid) {
		printf("port %u init\n", portid);
		if (dp_port_init(portid) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",portid);
	}

	DBG_OFP(DBGLVL1,NULL,"============ dp init successfully ==============\n");

	return 0;
}
 
/***************************************************************
 * ofpd : 
 *
 ***************************************************************/
int main(int argc, char **argv)
{
	//int 			ret;
	uint16_t 		portid;
	unsigned 		cpu_id = 1;
	
	if (argc < 8) {
		puts("Usage: ./osw <dpdk eal options> -- <OpenFlow NIC name> <SDN controller ip>");
		return -1;
	}
	int logtype = rte_log_register("osw");
	if (logtype < 0)
		rte_exit(EXIT_FAILURE, "Cannot register log type");
	rte_log_set_level(logtype, RTE_LOG_DEBUG);

	int arg = rte_eal_init(argc, argv);
	if (arg < 0) 
		rte_exit(EXIT_FAILURE, "Cannot init EAL: %s\n", rte_strerror(rte_errno));
	if (rte_lcore_count() < 10)
		rte_exit(EXIT_FAILURE, "We need at least 10 cores.\n");
	argc -= arg;
	argv += arg;
	printf("of nic name = %s, of controller = %s\n", argv[1], argv[2]);
	
	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool, %s\n", rte_strerror(rte_errno));
	//OFP_ipc_init();
	signal(SIGINT, OFP_bye);
	if (dp_init() < 0)
		return -1;
	if (rte_eth_dev_count_avail() < 2)
		rte_exit(EXIT_FAILURE, "We need at least 2 eth ports.\n");
	/*if ((ret=br_init()) != 0) {
		perror("bridge init failed:");
		return -1;
	}*/
	any2dp = rte_ring_create("any2dp", RING_SIZE, rte_socket_id(), 0);
	dp2tx = rte_ring_create("dp2tx", RING_SIZE, rte_socket_id(), 0);
	any2ofp = rte_ring_create("any2ofp", RING_SIZE, rte_socket_id(), 0);
	//any2dp->prod.head = 0;
	//dp2tx->prod.head = 0;
	//any2ofp->prod.head = 0;
	rte_atomic16_init(&any2dp_cums);
	rte_atomic16_set(&any2dp_cums, 0);
	any2dp_prod = 0;
	rte_eal_remote_launch((lcore_function_t *)dp, NULL, cpu_id++);
	RTE_ETH_FOREACH_DEV(portid) {
		uint16_t port_id = portid;
		rte_eal_remote_launch((lcore_function_t *)dp_rx, (void *)&port_id, cpu_id++);
	}
	RTE_ETH_FOREACH_DEV(portid) {
		uint16_t port_id = portid;
		rte_eal_remote_launch((lcore_function_t *)dp_tx, (void *)&port_id, cpu_id++);
	}
	strncpy(ofp_ports[0].of_ifname, argv[1], IFNAMSIZ - 1);
	strncpy(ofp_ports[0].ctrl_ip, argv[2], 15);
	rte_timer_subsystem_init();
	rte_timer_init(&(ofp_ports[0].ofp_timer));
	rte_eal_remote_launch((lcore_function_t *)ofp_timer, (void *)&(ofp_ports[0]), cpu_id++);
	rte_eal_remote_launch((lcore_function_t *)ofp_loop, (void *)&(ofp_ports[0]), cpu_id++);
	rte_eal_remote_launch((lcore_function_t *)ofp_sockd_cp,(void *)&(ofp_ports[0]), cpu_id++);
	rte_eal_remote_launch((lcore_function_t *)ofp_cmd, NULL, cpu_id);

	rte_eal_mp_wait_lcore();
 
	return 0;
}
