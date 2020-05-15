/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFP_DBG.H

  Designed by THE 30/09/'19
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFP_DBG_H_
#define _OFP_DBG_H_

#define	DBGLVL1		1
#define DBGLVL2		2

extern 	void 		DBG_OFP(U8 level, tOFP_PORT *port_ccb, char *fmt,...);
extern  char 		*OFP_state2str(U16 state);
extern  U8       	ofpDbgFlag;
#endif
