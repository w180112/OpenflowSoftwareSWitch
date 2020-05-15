/**************************************************************************
 * OFP_DBG.C
 *
 * Debug methods for ofp detection
 *
 * Created by THE on SEP 16,'19
 **************************************************************************/

#include    <common.h>
#include	"ofpd.h"
#include    "ofp_dbg.h"

#define 	DBG_OFP_MSG_LEN     		128
	
U8       	ofpDbgFlag=1;

/***************************************************
 * DBG_OFP:
 ***************************************************/	
void DBG_OFP(U8 level, tOFP_PORT *port_ccb, char *fmt,...)
{
	va_list ap; /* points to each unnamed arg in turn */
	char    buf[256],msg[DBG_OFP_MSG_LEN],sstr[20];
	
	//user offer level must > system requirement
    if (ofpDbgFlag < level){
    	return;
    }

	va_start(ap, fmt); /* set ap pointer to 1st unnamed arg */
    vsnprintf(msg, DBG_OFP_MSG_LEN, fmt, ap);
    
    if (port_ccb){
    	strcpy(sstr,OFP_state2str(port_ccb->state));
    	sprintf(buf,"ofpd> [%d.%s] ",port_ccb->port,sstr);
    }
    else{
    	sprintf(buf,"ofpd> ");
	}
    
  	strcat(buf,msg);
   	printf("%s",buf);
    va_end(ap);
}
