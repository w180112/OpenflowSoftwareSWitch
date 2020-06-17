/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   DP.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _DP_H_
#define _DP_H_

#define ETH_MTU					1514
#define MSG_LEN					4074
#define TABLE_SIZE 				1024

extern void 		dp(void);

/*-----------------------------------------
 * msg from dp sock
 *----------------------------------------*/
typedef struct {
	uint32_t  		port_no;
	int 			qid;
	uint16_t		len;
	char          	buffer[ETH_MTU];
}tDP_MSG;

/*-----------------------------------------
 * msg from dp to ofp
 *----------------------------------------*/
typedef struct {
	int   			id;
	char          	buffer[MSG_LEN];
}tany2ofp_MSG;

#endif