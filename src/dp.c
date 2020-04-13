#include        	<common.h>
#include 			<ipc.h>
#include        	"ofpd.h"
#include 			"dp_flow.h"

typedef struct q {
	U8 pkt[ETH_MTU];
	uint16_t id;
	struct q *next;
}q_t;

flow_t flow[256];

extern STATUS DP_decode_frame(tOFP_MBX *mail);

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
	int 			id = 1;

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
			mail = (tOFP_MBX*)mbuf.mtext;
			//PRINT_MESSAGE(((tDP_MSG *)(mail->refp))->buffer,(mail->len) - (sizeof(int) + sizeof(uint16_t)));
			//ofp_ports[port].port = TEST_PORT_ID;
			//DBG_OFP(DBGLVL1,&ofp_ports[0],"<-- Rx ofp message\n");
			if ((ret = DP_decode_frame(mail)) == ERROR)
				continue;
			else if (ret == FALSE) {
				//puts("send packet_in");
				if (total_enq_node > 65535)
					continue;
				enq_pkt_in(mail,&q_head,id);
				id++;
				if (id > 65536)
					id = 1;
				tDP2OFP_MSG msg;
				msg.id = id;
				memcpy(msg.buffer,(U8 *)(((tDP_MSG *)(mail->refp))->buffer),(mail->len) - (sizeof(int) + sizeof(uint16_t)));
				dp_send2ofp((U8 *)&msg,(mail->len) - (sizeof(int) + sizeof(uint16_t)) + sizeof(int));
			}
			else {
				puts("send to dp port");
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
	
	if (mulen > ETH_MTU+6) {
	 	printf("Incoming frame length(%d) is too lmaile!\n",mulen);
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
	//printf("<%d\n", __LINE__);
	for(cur_q=head; (*cur_q)!=NULL; cur_q=&(*cur_q)->next);
	//printf("<%d\n", __LINE__);
    q_t *new_node = (q_t *)malloc(sizeof(q_t));
    new_node->id = id;
    memcpy(new_node->pkt,(U8 *)(msg->buffer),(mail->len) - (sizeof(int) + sizeof(uint16_t)));
    new_node->next = *cur_q;
    *cur_q = new_node;
	//printf("<%d\n", __LINE__);
	return TRUE;
}
