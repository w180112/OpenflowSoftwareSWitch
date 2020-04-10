/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  ofp_sock.c
  
  Created by THE on SEP 30,'19
\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include       	<common.h>
#include		<sys/socket.h>
#include 		<sys/types.h> 
#include 		<netinet/in.h>
#include        "ofp_sock.h"
#include		"ofpd.h"

struct ifreq	ethreq;
static struct   sockaddr_ll 	sll; 
int				ofpSockSize = sizeof(struct sockaddr_in);
int				ofp_io_fds[2];
fd_set			ofp_io_ready[2];

/**************************************************************************
 * OFP_SOCK_INIT :
 *
 * Some useful methods :
 * - from string to ulong
 *   inet_aton("10.5.5.217", (struct in_addr*)cSA_ip); 
 **************************************************************************/ 
int OFP_SOCK_INIT() 
{
	struct sockaddr_in 			sock_info[2], local_sock_info[2];
	struct sock_fprog  			Filter;
	struct ifreq 				ifr;
	static struct sock_filter  	BPF_code[]={
		{ 0x28, 0, 0, 0x0000000c },
		{ 0x15, 0, 10, 0x00000800 },
		{ 0x20, 0, 0, 0x0000001e },
		{ 0x15, 0, 8, 0xc0a80a8a },
		{ 0x30, 0, 0, 0x00000017 },
		{ 0x15, 0, 6, 0x00000011 },
		{ 0x28, 0, 0, 0x00000014 },
		{ 0x45, 4, 0, 0x00001fff },
		{ 0xb1, 0, 0, 0x0000000e },
		{ 0x48, 0, 0, 0x00000010 },
		{ 0x15, 0, 1, 0x000019ff },
		{ 0x6, 0, 0, 0x00040000 },
		{ 0x6, 0, 0, 0x00000000 },
	};

    if ((ofp_io_fds[0]=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    printf("socket");
	    return -1;
	}
	
	Filter.len = sizeof(BPF_code)/sizeof(struct sock_filter);
	Filter.filter = BPF_code;
	if ((ofp_io_fds[1]=socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
	    perror("raw socket");
	    return -1;
	}

    //FD_ZERO(&ofp_io_ready[0]);
    //FD_SET(ofp_io_fds[0],&ofp_io_ready[0]);

	FD_ZERO(&ofp_io_ready[1]);
    FD_SET(ofp_io_fds[1],&ofp_io_ready[1]);
    
	bzero(&sock_info[0], sizeof(sock_info[0]));
    sock_info[0].sin_family = PF_INET;
    sock_info[0].sin_addr.s_addr = inet_addr("192.168.10.172");
    sock_info[0].sin_port = htons(6653);
    
	ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name,IF_NAME,IFNAMSIZ-1);
	if (ioctl(ofp_io_fds[0],SIOCGIFADDR,&ifr) != 0) {
        perror("ioctl failed");
        close(ofp_io_fds[0]);
        return -1;
    }

	bzero(&local_sock_info[0], sizeof(local_sock_info[0]));
    local_sock_info[0].sin_family = PF_INET;
    local_sock_info[0].sin_addr.s_addr = inet_addr(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    local_sock_info[0].sin_port = htons(6654);

	/* Set the network card in promiscuous mode */
  	strncpy(ethreq.ifr_name,IF_NAME,IFNAMSIZ-1);
  	if (ioctl(ofp_io_fds[1], SIOCGIFFLAGS, &ethreq)==-1) {
    	perror("ioctl (SIOCGIFCONF) 1\n");
    	close(ofp_io_fds[1]);
    	return -1;
  	}
  	
  	ethreq.ifr_flags |= IFF_PROMISC;
  	if (ioctl(ofp_io_fds[1], SIOCSIFFLAGS, &ethreq)==-1) {
    	perror("ioctl (SIOCGIFCONF) 2\n");
    	close(ofp_io_fds[1]);
    	return -1;
  	}
  	
  	/* Attach the filter to the socket */
  	if (setsockopt(ofp_io_fds[1], SOL_SOCKET, SO_ATTACH_FILTER, &Filter, sizeof(Filter))<0){
    	perror("setsockopt: SO_ATTACH_FILTER");
    	close(ofp_io_fds[1]);
    	return -1;
  	}

	bind(ofp_io_fds[0],(struct sockaddr *)&local_sock_info[0],sizeof(local_sock_info[0]));
    int err = connect(ofp_io_fds[0],(struct sockaddr *)&sock_info,sizeof(sock_info));
    if (err == -1) {
        perror("Connection error.");
		return err;
    }

	 //--------------- configure TX ------------------
	memset(&sll, 0, sizeof(sll));
	sll.sll_family = PF_PACKET;
	sll.sll_protocol = htons(ETH_P_IP);
	sll.sll_halen = 6;
		
	ioctl(ofp_io_fds[1], SIOCGIFINDEX, &ethreq); //ifr_name must be set to "eth?" ahead
	sll.sll_ifindex = ethreq.ifr_ifindex;
	
	printf("ifindex=%d\n",sll.sll_ifindex);

	return 0;
}

/**************************************************************************
 * ofp_sockd_cp:
 *
 * iov structure will provide the memory, so parameter pBuf doesn't need to care it.
 **************************************************************************/
void ofp_sockd_cp(void)
{
	int			n,rxlen;
	tOFP_MSG 	msg;
		
	/* 
	** to.tv_sec = 1;  ie. non-blocking; "select" will return immediately; =polling 
    ** to.tv_usec = 0; ie. blocking
    */
	for(;;) {   
		#if 0 
		if ((n = select(ofp_io_fds[0]+1,&ofp_io_ready[0],(fd_set*)0,(fd_set*)0,NULL/*&to*/)) < 0){
   		    /* if "to" = NULL, then "select" will block indefinite */
   			printf("select error !!! Giveup this receiving.\n");
   			sleep(2);
   			continue;
   		}
		if (n == 0)  continue;
		
   		/*----------------------------------------------------------------------
       	 * rx data from "LOC_sockAddr" to "LOC_fd" in Blocking mode
     	 *---------------------------------------------------------------------*/
    	if (FD_ISSET(ofp_io_fds[0],&ofp_io_ready[0])) {
		#endif
    		rxlen = recv(ofp_io_fds[0],msg.buffer,ETH_MTU,0);
    		if (rxlen <= 0) {
      			printf("Error! recv(): len <= 0 at CP\n");
				msg.sockfd = 0;
				msg.type = DRIV_FAIL;
    		}
			else {
    			msg.sockfd = ofp_io_fds[0];
				msg.type = DRIV_CP;
			}
   			/*printf("=========================================================\n");
			printf("rxlen=%d\n",rxlen);
    		PRINT_MESSAGE((char*)msg.buffer, rxlen);*/
    		
    		ofp_send2mailbox((U8*)&msg, rxlen+sizeof(int)+1);
   		//} /* if select */
   	} /* for */
}

/**************************************************************************
 * ofp_sockd_dp:
 *
 * iov structure will provide the memory, so parameter pBuf doesn't need to care it.
 **************************************************************************/
void ofp_sockd_dp(void)
{
	int			n, rxlen;
	tOFP_MSG 	msg;
		
	for(;;) {
		if ((n = select(ofp_io_fds[1]+1,&ofp_io_ready[1],(fd_set*)0,(fd_set*)0,NULL/*&to*/)) < 0) {
   		    /* if "to" = NULL, then "select" will block indefinite */
   			printf("select error !!! Giveup this receiving.\n");
   			sleep(2);
   			continue;
   		}
		if (n == 0)  continue;
		if (FD_ISSET(ofp_io_fds[1],&ofp_io_ready[1])) {
    		rxlen = recvfrom(ofp_io_fds[1],msg.buffer,ETH_MTU,0,NULL,NULL);
    		if (rxlen <= 0) {
      			printf("Error! recvfrom(): len <= 0 at DP\n");
       			continue;
    		}
    		msg.type = DRIV_DP;
			msg.sockfd = ofp_io_fds[1];
   			/*printf("=========================================================\n");
			printf("rxlen=%d\n",rxlen);
    		PRINT_MESSAGE((char*)buffer, rxlen);*/
    		
    		ofp_send2mailbox((U8*)&msg, rxlen+sizeof(int)+1);
   		} /* if select */
	}
}

/**************************************************************
 * drv_xmit :
 *
 * - sid  : socket id
 **************************************************************/
void drv_xmit(U8 *mu, U16 mulen, int sockfd)
{
	//printf("\ndrv_xmit ............\n");
	//PRINT_MESSAGE((unsigned char*)mu, mulen);
	if (sockfd == ofp_io_fds[0])
		send(sockfd, mu, mulen, 0);
	else
		sendto(sockfd, mu, mulen, 0, (struct sockaddr*)&sll, sizeof(sll));
}

/*********************************************************
 * ofp_send2mailbox:
 *
 * Input  : mu - incoming ethernet+igmp message unit
 *          mulen - mu length
 *          port - 0's based
 *          sid - need tag ? (Y/N)
 *          prior - if sid=1, then need set this field
 *          vlan - if sid=1, then need set this field
 *
 * return : TRUE or ERROR(-1)
 *********************************************************/
STATUS ofp_send2mailbox(U8 *mu, int mulen)
{
	tOFP_MBX mail;

    if (ofpQid == -1) {
		if ((ofpQid=msgget(OFP_Q_KEY,0600|IPC_CREAT)) < 0) {
			printf("send> Oops! ofpQ(key=0x%x) not found\n",OFP_Q_KEY);
   	 	}
	}
	
	if (mulen > ETH_MTU) {
	 	printf("Incoming frame length(%d) is too lmaile!\n",mulen);
		return ERROR;
	}

	mail.len = mulen;
	memcpy(mail.refp,mu,mulen); /* mail content will be copied into mail queue */
	
	//printf("ofp_send2mailbox(ofp_sock.c %d): mulen=%d\n",__LINE__,mulen);
	mail.type = IPC_EV_TYPE_DRV;
	ipc_sw(ofpQid, &mail, sizeof(mail), -1);
	return TRUE;
}
