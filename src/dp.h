/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   DP.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _DP_H_
#define _DP_H_


#define ETH_MTU					1514
#define MSG_LEN					4074

/*-----------------------------------------
 * msg from dp sock
 *----------------------------------------*/
typedef struct {
	uint16_t  		port_no;
	int 			sockfd;
	char          	buffer[ETH_MTU];
}tDP_MSG;

/*-----------------------------------------
 * msg from dp to ofp
 *----------------------------------------*/
typedef struct {
	int   			id;
	char          	buffer[MSG_LEN];
}tDP2OFP_MSG;

#endif