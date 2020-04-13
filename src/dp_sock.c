/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  dp_sock.c
  
  Created by THE on MAR 11,'20
\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include       	<common.h>
#include		<sys/socket.h>
#include 		<sys/types.h> 
#include 		<netinet/in.h>
#include 		<sys/epoll.h>
#include 		<fcntl.h>
#include        "ofp_sock.h"
#include 		"dp_sock.h"
#include		"dp.h"
#include 		"ofpd.h"

fd_set					dp_io_ready;
struct ifreq			ethreq;
static struct sockaddr_ll 	sll;
#define MAX_EVENTS 10
struct epoll_event ev;
int epollfd;
int max_fd;

/**************************************************************************
 * DP_SOCK_INIT :
 *
 * Some useful methods :
 * - from string to ulong
 *   inet_aton("10.5.5.217", (struct in_addr*)cSA_ip); 
 **************************************************************************/ 
int DP_SOCK_INIT(char *ifname, dp_io_fds_t **dp_io_fds_head) 
{
	int fd;
	static int port_no = 1;
	dp_io_fds_t **cur_io_fd;
	//struct sockaddr_in if_bind;

	if ((fd=socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
	    perror("br raw socket");
	    return -1;
	}
	
	FD_ZERO(&dp_io_ready);
    FD_SET(fd,&dp_io_ready);
	
	/*if_bind.sin_family = PF_PACKET;
	if_bind.sin_addr.s_addr = htonl("102.168.10.144");
	bind(fd,(struct sockaddr *)&if_bind,sizeof(if_bind));*/
	setsockopt(fd,SOL_SOCKET,SO_BINDTODEVICE,ifname,strlen(ifname));
	/* Set the network card in promiscuous mode */
  	strncpy(ethreq.ifr_name,ifname,IFNAMSIZ-1);
  	if (ioctl(fd, SIOCGIFFLAGS, &ethreq)==-1) {
    	perror("ioctl (SIOCGIFCONF) 1\n");
    	close(fd);
    	return -1;
  	}
#if 0  	
  	ethreq.ifr_flags |= IFF_PROMISC;
  	if (ioctl(fd, SIOCSIFFLAGS, &ethreq)==-1) {
    	perror("ioctl (SIOCGIFCONF) 2\n");
    	close(fd);
    	return -1;
  	}
#endif
	memset(&sll, 0, sizeof(sll));
	sll.sll_family = PF_PACKET;
	sll.sll_protocol = htons(ETH_P_ALL);
	sll.sll_halen = 6;
		
	ioctl(fd, SIOCGIFINDEX, &ethreq); //ifr_name must be set to "eth?" ahead
	sll.sll_ifindex = ethreq.ifr_ifindex;
	
	printf("ifindex=%d if name = %s\n",sll.sll_ifindex, ethreq.ifr_name);

    /*epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
	
	int flags = fcntl(fd, F_GETFL, 0);
  	if (flags == -1) {
    	perror("fcntl F_GETFL");
  	}
  	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    	perror("fcntl F_SETFL O_NONBLOCK");
  	}

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
	}*/

	/* Append new socket fd node to linked list */
	for(cur_io_fd=dp_io_fds_head; *cur_io_fd!=NULL; cur_io_fd=&(*cur_io_fd)->next);
	dp_io_fds_t *new_node = (dp_io_fds_t *)malloc(sizeof(dp_io_fds_t));
	new_node->fd = fd;
	strncpy(new_node->ifname,ifname,64);
	new_node->port_no = port_no;
	new_node->next = *cur_io_fd;
	*cur_io_fd = new_node;
	printf("dp_io_fds_head = %x\n", *dp_io_fds_head);
	max_fd = fd;
	port_no++;

	return 0;
}

/**************************************************************************
 * sockd_dp:
 *
 * iov structure will provide the memory, so parameter pBuf doesn't need to care it.
 **************************************************************************/
void sockd_dp(dp_io_fds_t *dp_io_fds_head)
{
	int			n,rxlen;
	tDP_MSG 	msg;
	struct epoll_event events[MAX_EVENTS];
	int nfds;
	dp_io_fds_t *cur_io_fd;
	#if 0
	for (;;) {
		printf("<%d\n", __LINE__);
		
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
		printf("<%d\n", __LINE__);
		for (n = 0; n < nfds; ++n) {
			printf("dp_io_fds_head = %x\n", dp_io_fds_head);
			for(cur_io_fd=dp_io_fds_head; cur_io_fd!=NULL&&cur_io_fd->fd!=events[n].data.fd; cur_io_fd=cur_io_fd->next);
            printf("cur_io_fd = %x\n", cur_io_fd);
			//printf("<%d\n", __LINE__);
			if (cur_io_fd != NULL) {
				printf("<%d\n", __LINE__);
				rxlen = recvfrom(cur_io_fd->fd,msg.buffer,ETH_MTU,0,NULL,NULL);
				if (rxlen <= 0) {
      				printf("Error! recvfrom(): len <= 0 at DP\n");
					msg.sockfd = 0;
					//msg.type = DRIV_FAIL;
    			}
				else {
					PRINT_MESSAGE((char*)msg.buffer,rxlen);
    				msg.sockfd = cur_io_fd->fd;
					//msg.type = DRIV_CP;
				}
				//ofp_send2mailbox((U8*)&msg, rxlen+sizeof(int)+1);
            } else {
                //do_use_fd(events[n].data.fd);
            }
        }
		
		//printf("<%d\n", __LINE__);
			if (dp_io_fds_head != NULL) {
				printf("<%d\n", __LINE__);
				rxlen = recv(dp_io_fds_head->fd,msg.buffer,ETH_MTU,0);
				if (rxlen <= 0) {
      				printf("Error! recv(): len <= 0 at DP\n");
					msg.sockfd = 0;
					//msg.type = DRIV_FAIL;
    			}
				else {
					PRINT_MESSAGE((char*)msg.buffer,rxlen);
    				msg.sockfd = dp_io_fds_head->fd;
					//msg.type = DRIV_CP;
				}
				//ofp_send2mailbox((U8*)&msg, rxlen+sizeof(int)+1);
            } else {
                //do_use_fd(events[n].data.fd);
            }
		
    }
	#endif
	#if 1
	for(;;) {    
		//printf("<%d\n", __LINE__);
		if ((n = select(max_fd+1,&dp_io_ready,(fd_set*)0,(fd_set*)0,NULL/*&to*/)) < 0){
   		    /* if "to" = NULL, then "select" will block indefinite */
   			printf("select error !!! Giveup this receiving.\n");
   			sleep(2);
   			continue;
   		}
		if (n == 0)  continue;
   		/*----------------------------------------------------------------------
       	 * rx data from "LOC_sockAddr" to "LOC_fd" in Blocking mode
     	 *---------------------------------------------------------------------*/
    	if (FD_ISSET(dp_io_fds_head->fd,&dp_io_ready)) {
			//printf("<%d\n", __LINE__);
    		rxlen = recvfrom(dp_io_fds_head->fd,msg.buffer,ETH_MTU,0,NULL,NULL);
			//printf("<%d\n", __LINE__);
    		if (rxlen <= 0) {
      			printf("Error! recvfrom(): len <= 0 at DP\n");
				msg.sockfd = 0;
				msg.port_no = 0;
				continue;
    		}
			else {
				if (*(U16 *)(msg.buffer+34) == 0xFD19 || *(U16 *)(msg.buffer+36) == 0xFD19)
					continue;
    			msg.sockfd = dp_io_fds_head->fd;
				msg.port_no = dp_io_fds_head->port_no;
			}
			//PRINT_MESSAGE((char*)msg.buffer, rxlen);
   			/*printf("=========================================================\n");
			printf("rxlen=%d\n",rxlen);*/
    		
    		dp_send2mailbox((U8*)&msg, rxlen+sizeof(int)+2);
   		} /* if select */
   	} /* for */
	#endif
}

/*********************************************************
 * dp_send2mailbox:
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
STATUS dp_send2mailbox(U8 *mu, int mulen)
{
	tOFP_MBX mail;

    if (dpQid == -1) {
		if ((dpQid=msgget(DP_Q_KEY,0600|IPC_CREAT)) < 0) {
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
	mail.type = IPC_EV_TYPE_DRV;
	ipc_sw(dpQid, &mail, sizeof(mail), -1);
	return TRUE;
}
