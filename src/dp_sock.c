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
#include 		<rte_memcpy.h>
#include 		<rte_atomic.h>
#include 		<rte_mbuf.h>
#include 		<rte_ethdev.h>
#include 		<rte_ether.h>

#include        "ofp_sock.h"
#include 		"dp_sock.h"
#include		"dp.h"

fd_set						dp_io_ready;
struct ifreq				ethreq;
static struct sockaddr_ll 	sll;
#define MAX_EVENTS 10
struct epoll_event 			ev;
int 						epollfd;
int 						max_fd;
extern sem_t 				*sem;
extern tDP_MSG 				*dp_buf;
extern int					*post_index, *pre_index;
extern tOFP_PORT			ofp_ports[MAX_USER_PORT_NUM+1]; 

#define BURST_SIZE 32
rte_atomic16_t				any2dp_cums;
int16_t						any2dp_prod;

void dp_drv_xmit(U8 *mu, U16 mulen, uint32_t port_id, uint32_t in_port, dp_io_fds_t *dp_io_fds_head);

/**************************************************************************
 * DP_SOCK_INIT :
 *
 * Some useful methods :
 * - from string to ulong
 *   inet_aton("10.5.5.217", (struct in_addr*)cSA_ip); 
 **************************************************************************/ 
int DP_SOCK_INIT(char *ifname, uint32_t port_id, dp_io_fds_t **dp_io_fds_head) 
{
	int fd;
	//static uint32_t port_no = 1;
	dp_io_fds_t **cur_io_fd;
	struct sock_fprog  			Filter;
	static struct sock_filter  	BPF_code[] = { // generated by tcpdump -dd 'not tcp port 6653'
		{ 0x28, 0, 0, 0x0000000c },
		{ 0x15, 0, 6, 0x000086dd },
		{ 0x30, 0, 0, 0x00000014 },
		{ 0x15, 0, 15, 0x00000006 },
		{ 0x28, 0, 0, 0x00000036 },
		{ 0x15, 12, 0, 0x000019fd },
		{ 0x28, 0, 0, 0x00000038 },
		{ 0x15, 10, 11, 0x000019fd },
		{ 0x15, 0, 10, 0x00000800 },
		{ 0x30, 0, 0, 0x00000017 },
		{ 0x15, 0, 8, 0x00000006 },
		{ 0x28, 0, 0, 0x00000014 },
		{ 0x45, 6, 0, 0x00001fff },
		{ 0xb1, 0, 0, 0x0000000e },
		{ 0x48, 0, 0, 0x0000000e },
		{ 0x15, 2, 0, 0x000019fd },
		{ 0x48, 0, 0, 0x00000010 },
		{ 0x15, 0, 1, 0x000019fd },
		{ 0x6, 0, 0, 0x00000000 },
		{ 0x6, 0, 0, 0x00040000 },
	};
	//struct sockaddr_in if_bind;

	Filter.len = sizeof(BPF_code)/sizeof(struct sock_filter);
	Filter.filter = BPF_code;
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
#if 1  	
  	ethreq.ifr_flags |= IFF_PROMISC;
  	if (ioctl(fd, SIOCSIFFLAGS, &ethreq)==-1) {
    	perror("ioctl (SIOCGIFCONF) 2\n");
    	close(fd);
    	return -1;
  	}
#endif
	if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &Filter, sizeof(Filter))<0){
    	perror("setsockopt: SO_ATTACH_FILTER");
    	close(fd);
    	return -1;
  	}
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
	strncpy(new_node->ifname, ifname, IFNAMSIZ-1);
	new_node->port_no = port_id;
	new_node->next = *cur_io_fd;
	new_node->pkt_count = 0;
	new_node->err_count = 0;
	*cur_io_fd = new_node;
	printf("cur_io_fd fd = %x\n", (*cur_io_fd)->fd);
	max_fd = fd + 1;
	printf("port id = %u\n", port_id);

	return 0;
}

/**************************************************************************
 * sockd_dp:
 *
 * iov structure will provide the memory, so parameter pBuf doesn't need to care it.
 **************************************************************************/
void sockd_dp(__attribute__((unused)) dp_io_fds_t *dp_io_fds_head)
{
	int			n, rxlen;
	tDP_MSG 	*msg;
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
	#if 0
	for(;;) {    
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
		for (cur_io_fd=dp_io_fds_head; cur_io_fd!=NULL; cur_io_fd=cur_io_fd->next) {
    		if (FD_ISSET(cur_io_fd->fd,&dp_io_ready)) {
				if (!((*post_index+1) ^ atomic_int32_add_after(*pre_index, 0))) {
					break;
				}
				msg = /*dp_buf +*/ *post_index;
				//printf("msg = %x at dp_sock.c\n", msg);
    			rxlen = recvfrom(cur_io_fd->fd,msg->buffer,ETH_MTU,0,NULL,NULL);
    			if (rxlen <= 0) {
      				printf("Error! recvfrom(): len <= 0 at DP\n");
					cur_io_fd->err_count++;
					msg->sockfd = 0;
					msg->port_no = 0;
					break;
    			}
				else {
					cur_io_fd->pkt_count++;
    				msg->sockfd = cur_io_fd->fd;
					msg->port_no = cur_io_fd->port_no;

					//printf("port id = %u, fd = %d\n", msg.port_no, msg.sockfd);
				}
				msg->len = rxlen;
				//PRINT_MESSAGE((char*)msg.buffer, rxlen);
   				//printf("=========================================================\n");
				//printf("rxlen=%d\n",rxlen);
    			//printf("*post_index = %d\n", *post_index);
				//sem_wait(sem);
				(*post_index)++;
				if (*post_index >= PKT_BUF)
					*post_index = 0;
				//atomic_int32_compare_swap(*post_index, PKT_BUF, 0);
            	//sem_post(sem);
				dp_send2mailbox((U8 *)post_index, sizeof(int));
				//dp_send2mailbox((U8*)&msg, rxlen+sizeof(int)+2);
   			} /* if select */
		}
   	} /* for */
	#endif
}

void dp_drv_xmit(U8 *mu, U16 mulen, uint32_t port_id, uint32_t in_port, dp_io_fds_t *dp_io_fds_head)
{
	dp_io_fds_t *cur_io_fd;

	if (port_id == OFPP_FLOOD || port_id == OFPP_ALL) {
		for(cur_io_fd=dp_io_fds_head; cur_io_fd!=NULL; cur_io_fd=cur_io_fd->next) {
			if (cur_io_fd->port_no == in_port)
				continue;
			sendto(cur_io_fd->fd, mu, mulen, 0, (struct sockaddr*)&sll, sizeof(sll));
			//printf("%d of %u bytes sent from %d\n", sendto(cur_io_fd->fd, mu, mulen, 0, (struct sockaddr*)&sll, sizeof(sll)), mulen, cur_io_fd->fd);
			//printf("cur_io_fd->port_no = %u, in_port = %u\n", cur_io_fd->port_no, in_port);
		}
		return;
	}
	if (port_id <= OFPP_MAX) {
		for(cur_io_fd=dp_io_fds_head; cur_io_fd!=NULL; cur_io_fd=cur_io_fd->next) {
			if (cur_io_fd->port_no == port_id) {
				sendto(cur_io_fd->fd, mu, mulen, 0, (struct sockaddr*)&sll, sizeof(sll));
				return;
			}
		}
	}
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
	
	if (mulen > MSG_LEN) {
	 	printf("Incoming frame length(%d) is too large at dp_sock.c!\n",mulen);
		return ERROR;
	}

	mail.len = mulen;
	memcpy(mail.refp,mu,mulen); /* mail content will be copied into mail queue */
	
	//printf("dp_send2mailbox(dp_sock.c %d): mulen=%d\n",__LINE__,mulen);
	mail.type = IPC_EV_TYPE_DRV;
	ipc_sw(dpQid, &mail, sizeof(mail), -1);
	return TRUE;
}

int dp_rx(uint16_t *port_id)
{
	struct rte_mbuf 	*pkt[BURST_SIZE];
	uint16_t 			nb_rx;
	struct rte_ether_hdr *eth_hdr;
	char 				str[20];
	snprintf(str,sizeof(str),"%s%u","any2dp_mail_",*port_id);
	tDP_MSG 			*msg;
	tOFP_MBX			*any2dp_mail = (tOFP_MBX *)rte_mempool_create(str, BURST_SIZE, sizeof(tOFP_MBX), 20, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
	int 				i;
	char 				*cur;

	while (rte_atomic16_read(&(ofp_ports[0].dp_enable)) == 0)
		rte_pause();
	
	if (any2dp_mail == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool at dp_rx, %s\n", rte_strerror(rte_errno));
	for(;;) {
		nb_rx = rte_eth_rx_burst(*port_id, 0, pkt, BURST_SIZE);
		if (nb_rx == 0)
			continue;
		//printf("<%d recv %u bytes\n", __LINE__, nb_rx);
		for(i=0; i<nb_rx; i++) {
			rte_prefetch0(rte_pktmbuf_mtod(pkt[i], void *));
			eth_hdr = rte_pktmbuf_mtod(pkt[i], struct rte_ether_hdr*);
			if (rte_atomic16_read(&any2dp_cums) != ((any2dp_prod + 1) % BURST_SIZE)) {
				msg = (tDP_MSG *)((any2dp_mail+any2dp_prod)->refp);
				msg->qid = 0;
				msg->port_no = *port_id;
				msg->len = pkt[i]->data_len;
				rte_memcpy(msg->buffer, eth_hdr, pkt[i]->data_len);
				(any2dp_mail + any2dp_prod)->type = IPC_EV_TYPE_DRV;
				(any2dp_mail + any2dp_prod)->len = pkt[i]->data_len;
				//enqueue eth_hdr pkt[i]->data_len
				cur = (char *)(any2dp_mail + any2dp_prod);
				rte_ring_enqueue_burst(any2dp,(void **)&cur,1,NULL);
				any2dp_prod++;
				if (any2dp_prod >= BURST_SIZE)
					any2dp_prod = 0;
			}
			//printf("<%d\n", __LINE__);
		}
		/*for(i=0; i<nb_rx; i++) {
			printf("<%d\n", __LINE__);
			rte_pktmbuf_free(pkt[i]);
			printf("<%d\n", __LINE__);
		}*/
	}
	return 0;
}

int dp_tx(uint16_t *port_id)
{
	int 		burst_size, ret;
	tOFP_MBX 	*dp2tx_mail[BURST_SIZE];// = (tOFP_MBX *)rte_mempool_create("dp2tx_mail", BURST_SIZE, sizeof(tOFP_MBX), 512, 0, NULL, NULL, NULL, NULL, rte_socket_id(), 0);
	struct rte_mbuf *pkt[BURST_SIZE];
	char *buf;
	
	ret = rte_pktmbuf_alloc_bulk(mbuf_pool, pkt, BURST_SIZE);
	if (ret != 0)
		rte_exit(EXIT_FAILURE, "Cannot alloc mbuf from mbufpool, %s\n", rte_strerror(rte_errno));
	for(;;) {
		burst_size = rte_ring_dequeue_burst(dp2tx, (void **)dp2tx_mail, BURST_SIZE, NULL);
		if (unlikely(burst_size == 0))
			continue;
		for(int i=0;i<burst_size;i++) {
			//pkt[i] = rte_pktmbuf_alloc(mbuf_pool);
			buf = rte_pktmbuf_mtod(pkt[i],char *);
			rte_memcpy(buf, dp2tx_mail[i]->refp, dp2tx_mail[i]->len);
			pkt[i]->data_len = dp2tx_mail[i]->len;
			pkt[i]->pkt_len = dp2tx_mail[i]->len;
		}
		uint16_t nb_tx = rte_eth_tx_burst(*port_id, 0, pkt, burst_size);
		if (unlikely(nb_tx < burst_size)) {
			for(uint16_t buf=nb_tx; buf<burst_size; buf++)
				rte_pktmbuf_free(pkt[buf]);
		}
		//printf("%u pkts sent\n", nb_tx);
	}
	return 0;
}

int msg_dequeue(struct rte_ring *ring, tOFP_MBX **mail)
{
	uint16_t burst_size;
	//char *buf = malloc(5);
	for(;;) {
		//single_pkt = rte_pktmbuf_alloc(mbuf_pool);
		burst_size = rte_ring_dequeue_burst(ring, (void **)mail, BURST_SIZE, NULL);
		//printf("<%d\n", __LINE__);
		if (likely(burst_size == 0)) {
			//rte_pktmbuf_free(single_pkt);
			continue;
		}
		//printf("mail->type = %u, len = %d\n", mail[0]->type, mail[0]->len);
		//printf("recv %u ring msg\n", burst_size);
		break;
		//puts(buf);
		//rte_pktmbuf_free(single_pkt);
	}
	return burst_size;
}