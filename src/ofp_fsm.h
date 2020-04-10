/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  ofp_fsm.h
  
     Finite State Machine for OpenFlow Protocol connection/call

  Designed by THE
  Date: 30/9/2019
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFP_FSM_H_
#define _OFP_FSM_H_

typedef struct{
    U8   	state;
    U16   	event;
    U8   	next_state;
    STATUS 	(*hdl[10])(tOFP_PORT *ccb);
} tOFP_STATE_TBL;

/*--------- STATE TYPE ----------*/
typedef enum {
    S_CLOSED,
    S_HELLO_WAIT,
    S_FEATURE_WAIT,
    S_ESTABLISHED,
    S_INVALID,
} OFP_STATE;

/*----------------- EVENT TYPE --------------------
Q_ : Quest primitive 
E_ : Event */
typedef enum {
  E_START,
	E_RECV_HELLO,
  E_FEATURE_REQUEST,
  E_MULTIPART_REQUEST,
  E_OTHERS,
  E_ECHO_REPLY,
  E_OFP_TIMEOUT,
  E_OFP_CACHE_TIMEOUT,
  E_PACKET_FROM_HOST,
  E_PACKET_OUT,
  E_PACKET_IN,
  E_FLOW_MOD,
} OFP_EVENT_TYPE;

/*======================= external ==========================*/
#ifdef __cplusplus
extern	"C" {
#endif

extern STATUS   OFP_FSM(tOFP_PORT*, U16); 

#ifdef __cplusplus
}
#endif

#endif /* header */
