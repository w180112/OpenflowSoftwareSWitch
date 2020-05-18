/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  dp.c
  
  Created by THE on MAR 11,'20
\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include        	<common.h>
#include 			<ipc.h>
#include        	"ofpd.h"
#include 			"dp_flow.h"
#include 			"ofp_cmd.h"
#include 			"dp_sock.h"
#include			"ofp_common.h" 		

typedef struct q {
	U8 pkt[ETH_MTU];
	uint16_t id;
	struct q *next;
}q_t;

flow_t flow[TABLE_SIZE]; //flow table

extern STATUS DP_decode_frame(tDP_MSG *msg, dp_io_fds_t *dp_io_fds_head, uint32_t *buffer_id);
extern STATUS flowmod_match_process(flowmod_info_t flowmod_info, uint32_t *flow_index);
extern STATUS flowmod_action_process(flowmod_info_t flowmod_info, uint32_t flow_index);
extern STATUS pkt_out_process(packet_out_info_t packet_out_info, dp_io_fds_t *dp_io_fds_head);
extern STATUS print_field(void **cur, uint16_t *type);

extern sem_t 				*sem;
extern tDP_MSG 				*dp_buf;
extern int					*post_index, *pre_index;

void dp(tIPC_ID dpQid);
STATUS enq_pkt_in(tOFP_MBX *mail, q_t **head, int id);
STATUS dp_send2ofp(U8 *mu, int mulen);

void dp(tIPC_ID dpQid)
{
	tOFP_MBX		*mail;
	tMBUF   		mbuf;
	int				msize;
	U16				ipc_type;
	int 			ret;
	q_t				*q_head = NULL;
	int 			total_enq_node = 0;
	dp_io_fds_t		*dp_io_fds_head = NULL;
	pthread_t 		dp_thread = 0;
	uint32_t 		buffer_id, id = 1;
	tDP_MSG 		*msg;

	memset(flow, 0, sizeof(flow_t)*TABLE_SIZE);
	for(;;) {
		//printf("\n===============================================\n");
		//printf("%s> waiting for ipc_rcv2() ...\n", "dp.c");
	    if (ipc_rcv2(dpQid,&mbuf,&msize) == ERROR) {
	    	printf("\n%s> ipc_rcv2 error ...\n","dp.c");
	    	continue;
	    }
	    
	    ipc_type = *(U16*)mbuf.mtext;
	    //printf("ipc_type=%d\n",ipc_type);
		
		switch(ipc_type){
		case IPC_EV_TYPE_DRV:
			/* recv pkts from dp */
			//mail = (tOFP_MBX*)mbuf.mtext;
			msg = (tDP_MSG *)(dp_buf + atomic_int32_add_after(*pre_index, 0));
			//printf("msg = %x\n", msg);
			*pre_index = atomic_int32_add_after(*pre_index, 1);
			//printf("*pre_index = %d\n", *pre_index);
			if (*pre_index >= PKT_BUF) {
				//printf("*pre_index = %d\n", *pre_index);
				atomic_int32_sub_after(*pre_index, 1);
				//printf("*pre_index = %d\n", *pre_index);
				sem_wait(sem);
				*pre_index = 0;
				sem_post(sem);
			}
			//atomic_int32_compare_swap(*pre_index, PKT_BUF, 0);
			/*if (*pre_index == PKT_BUF)
				(*pre_index) = 0;*/
			
			//PRINT_MESSAGE(((tDP_MSG *)(mail->refp))->buffer,(mail->len) - (sizeof(int) + sizeof(uint16_t)));
			//ofp_ports[port].port = TEST_PORT_ID;
			//DBG_OFP(DBGLVL1,&ofp_ports[0],"<-- Rx ofp message\n");
			if ((ret = DP_decode_frame(msg, dp_io_fds_head, &buffer_id)) == ERROR)
				continue;
			else if (ret == FALSE) {
				if (buffer_id != OFP_NO_BUFFER) {
					if (total_enq_node >= 0xfffffffe)
						continue;
					enq_pkt_in(mail,&q_head,id);
					id++;
					if (id >= 0xfffffffe)
						id = 1;
				}
				else
					id = OFP_NO_BUFFER;
				tDP2OFP_MSG msg_2_ofp;
				msg_2_ofp.id = id;
				memcpy(msg_2_ofp.buffer, (U8 *)msg, (msg->len) + sizeof(tDP_MSG) - ETH_MTU);
				//printf("<%d send packet_in\n", __LINE__);
				dp_send2ofp((U8 *)&msg_2_ofp, (msg->len)+ sizeof(tDP_MSG) - ETH_MTU + sizeof(int));
				//printf("<%d send packet_in\n", __LINE__);
				//puts("send to ofp");
			}
			else {
				//puts("send to dp port");
			}
			break;
		case IPC_EV_TYPE_OFP:
			mail = (tOFP_MBX*)mbuf.mtext;
			if (*(mail->refp) == FLOWMOD) {
				//printf("<%d at dp.c\n", __LINE__);
				/* TODO: if buffer_id exist, needs to make the buffered packet match the flow refer to this buffer_id */
				flowmod_info_t flowmod_info;
				uint32_t flow_index;
				memset(flowmod_info.match_info, 0, sizeof(pkt_info_t)*20);
				//PRINT_MESSAGE(mail->refp,1524);
				//PRINT_MESSAGE(flowmod_info.match_info,sizeof(pkt_info_t)*20);
				//PRINT_MESSAGE(flowmod_info.action_info,sizeof(pkt_info_t)*20);
				memcpy(&flowmod_info, mail->refp, sizeof(flowmod_info_t));
				if (flowmod_match_process(flowmod_info, &flow_index) == TRUE)
					flowmod_action_process(flowmod_info, flow_index);
				
				//printf("<%d at dp.c %d\n", __LINE__, flow[flow_index].is_exist);
				
				//printf("flowmod_info action len = %u\n", flowmod_info.action_info[0].max_len);
				//free(flowmod_info.action_info);
			}
			else if (*(mail->refp) == PACKET_OUT) {
				packet_out_info_t packet_out_info;

				memset(packet_out_info.action_info, 0, sizeof(pkt_info_t)*20);
				memcpy(&packet_out_info, mail->refp, sizeof(packet_out_info_t));
				//puts("recv pkt_out from ofp");
				if (pkt_out_process(packet_out_info, dp_io_fds_head) == FALSE)
					puts("pkt_out processing exit unexpected.");
			}
			else if (*(mail->refp) == CLI) {
				cli_2_dp_t *cli_2_dp = (cli_2_dp_t *)(mail->refp);
				switch(cli_2_dp->cli_2_ofp.opcode)
				{
					case ADD_IF:
						DP_SOCK_INIT(cli_2_dp->cli_2_ofp.ifname, cli_2_dp->cli_2_ofp.port_id, &dp_io_fds_head);
						//printf("dp_io_fds_head = %x\n", dp_io_fds_head);
						if (dp_thread == 0)
							pthread_create(&dp_thread, NULL, (void *)sockd_dp, dp_io_fds_head);
						break;
					case SHOW_FLOW:
						for(int i=0; i<TABLE_SIZE; i++) {
							if (flow[i].is_exist == FALSE)
								continue;
							printf("flow %d: cookie=%lx, priority=%u, pkt count=%lu, match field: ", i, flow[i].cookie, flow[i].priority, flow[i].pkt_count);
							void *cur;
							uint16_t type;
							type = flow[i].match_type;
							for(cur=flow[i].next_match;;) {
								if (print_field(&cur, &type) == END)
									break;
								printf(", ");
							}
							printf("action field: ");
							type = flow[i].action_type;
							for(cur=flow[i].next_action;;) {
								if (print_field(&cur, &type) == END)
									break;
								printf(", ");
							}
							printf("\n");
						}
						break;
					default:
						;
				}
			}
			break;
		default:
		    ;
		}
    }
}

/*********************************************************
 * dp_send2ofp:
 *
 * Input  : mu - packet_in packet
 *          mulen - mu length
 *
 * return : TRUE or ERROR(-1)
 *********************************************************/
STATUS dp_send2ofp(U8 *mu, int mulen)
{
	tOFP_MBX mail;

    if (ofpQid == -1) {
		if ((ofpQid=msgget(DP_Q_KEY,0600|IPC_CREAT)) < 0) {
			printf("send> Oops! dpQ(key=0x%x) not found\n",DP_Q_KEY);
   	 	}
	}
	
	if (mulen > MSG_LEN) {
	 	printf("Incoming frame length(%d) is too large at dp.c!\n",mulen);
		return ERROR;
	}

	mail.len = mulen;
	memcpy(mail.refp,mu,mulen); /* mail content will be copied into mail queue */
	
	//printf("dp_send2mailbox(dp_sock.c %d): mulen=%d\n",__LINE__,mulen);
	mail.type = IPC_EV_TYPE_DP;
	ipc_sw(ofpQid, &mail, sizeof(mail), -1);
	return TRUE;
}

STATUS enq_pkt_in(tOFP_MBX *mail, q_t **head, int id)
{
	q_t **cur_q;
	tDP_MSG *msg = (tDP_MSG *)(mail->refp);
	for(cur_q=head; (*cur_q)!=NULL; cur_q=&(*cur_q)->next);
    q_t *new_node = (q_t *)malloc(sizeof(q_t));
    new_node->id = id;
    memcpy(new_node->pkt,(U8 *)(msg->buffer),(mail->len) - (sizeof(int) + sizeof(uint16_t)));
    new_node->next = *cur_q;
    *cur_q = new_node;
	return TRUE;
}
