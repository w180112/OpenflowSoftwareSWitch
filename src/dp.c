#include        	<common.h>
#include 			<ipc.h>
#include        	"ofpd.h"
#include 			"dp_flow.h"

flow_t flow[256];

extern STATUS DP_decode_frame(tOFP_MBX *mail);

void dp(tIPC_ID dpQid);

void dp(tIPC_ID dpQid)
{
	tOFP_MBX		*mail;
	tMBUF   		mbuf;
	int				msize;
	U16				ipc_type;
	int 			ret;

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
				puts("send packet_in");
			}
			else {
				puts("send to dp port");
			}
			//printf("<%d\n", __LINE__);
			break;
		default:
		    ;
		}
    }
}
