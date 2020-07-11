/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  ofp_sock.c
  
  Created by THE on SEP 30,'19
\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include       	<common.h>
#include		<sys/socket.h>
#include 		<sys/types.h> 
#include 		<netinet/in.h>
#include 		<rte_memcpy.h>
#include 		"dp_sock.h"
#include 		"mailbox.h"

#if 0 //use with splice()
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif 

#include        "ofp_sock.h"
#include		"ofpd.h"

struct ifreq	ethreq;
//static struct   sockaddr_ll 	sll; 
int				ofpSockSize = sizeof(struct sockaddr_in);
int				ofp_io_fds[2];
fd_set			ofp_io_ready[2];

int 			ofp_sockd_cp(tOFP_PORT *port_ccb);

/**************************************************************************
 * OFP_SOCK_INIT :
 *
 * Some useful methods :
 * - from string to ulong
 *   inet_aton("10.5.5.217", (struct in_addr*)cSA_ip); 
 **************************************************************************/ 
int OFP_SOCK_INIT(char *if_name, char *ctrl_ip) 
{
	struct sockaddr_in 			sock_info[2], local_sock_info[2];
	//struct sock_fprog  			Filter;
	struct ifreq 				ifr;
	/*static struct sock_filter  	BPF_code[] = {
		{ 0x28, 0, 0, 0x0000000c },
		{ 0x15, 0, 10, 0x00000800 },
		{ 0x20, 0, 0, 0x0000001e },
		{ 0x15, 0, 8, 0xc0a80a9d },
		{ 0x30, 0, 0, 0x00000017 },
		{ 0x15, 0, 6, 0x00000011 },
		{ 0x28, 0, 0, 0x00000014 },
		{ 0x45, 4, 0, 0x00001fff },
		{ 0xb1, 0, 0, 0x0000000e },
		{ 0x48, 0, 0, 0x00000010 },
		{ 0x15, 0, 1, 0x000019ff },
		{ 0x6, 0, 0, 0x00040000 },
		{ 0x6, 0, 0, 0x00000000 },
	};*/

    if ((ofp_io_fds[0]=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    printf("socket");
	    return -1;
	}
#if 0	
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
#endif    
	bzero(&sock_info[0], sizeof(sock_info[0]));
    sock_info[0].sin_family = PF_INET;
    sock_info[0].sin_addr.s_addr = inet_addr(ctrl_ip);
    sock_info[0].sin_port = htons(6653);
    
	ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name,if_name,IFNAMSIZ-1);
	if (ioctl(ofp_io_fds[0],SIOCGIFADDR,&ifr) != 0) {
        perror("ioctl failed");
        close(ofp_io_fds[0]);
        return -1;
    }

	bzero(&local_sock_info[0], sizeof(local_sock_info[0]));
    local_sock_info[0].sin_family = PF_INET;
    local_sock_info[0].sin_addr.s_addr = inet_addr(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    local_sock_info[0].sin_port = htons(6654);
#if 0
	/* Set the network card in promiscuous mode */
  	strncpy(ethreq.ifr_name,if_name,IFNAMSIZ-1);
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
#endif
	bind(ofp_io_fds[0],(struct sockaddr *)&local_sock_info[0],sizeof(local_sock_info[0]));
    int err = connect(ofp_io_fds[0],(struct sockaddr *)&sock_info,sizeof(sock_info));
    if (err == -1) {
        perror("Connection error.");
		return err;
    }
#if 0
	 //--------------- configure TX ------------------
	memset(&sll, 0, sizeof(sll));
	sll.sll_family = PF_PACKET;
	sll.sll_protocol = htons(ETH_P_IP);
	sll.sll_halen = 6;
		
	ioctl(ofp_io_fds[1], SIOCGIFINDEX, &ethreq); //ifr_name must be set to "eth?" ahead
	sll.sll_ifindex = ethreq.ifr_ifindex;
	
	printf("ifindex=%d\n",sll.sll_ifindex);
#endif
	return 0;
}

/**************************************************************************
 * ofp_sockd_cp:
 *
 * iov structure will provide the memory, so parameter pBuf doesn't need to care it.
 **************************************************************************/
int ofp_sockd_cp(tOFP_PORT *port_ccb)
{
	//int			n;
	int 		rxlen;
	tOFP_MSG 	msg;
	tOFP_MBX	mail;
	
	while (rte_atomic16_read(&(port_ccb->cp_enable)) == 0)
		rte_pause();
	/* 
	** to.tv_sec = 1;  ie. non-blocking; "select" will return immediately; =polling 
    ** to.tv_usec = 0; ie. blocking
    */

   	#if 0 /* Using splice() system call */
		int pipe_fd[2];
		int fd;
		char tmpfile[] = "/tmp/fooXXXXXX";
		void *buffer;

		fd = mkostemp(tmpfile, O_NOATIME);
		unlink(tmpfile);
		lseek(fd, 4095, SEEK_SET);
		write(fd, "", 1);
		lseek(fd, 0, SEEK_SET);
		pipe(pipe_fd);
		buffer = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
	#endif 

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
    		rxlen = recv(ofp_io_fds[0], msg.buffer, ETH_MTU,0);
			/*rxlen = splice(ofp_io_fds[0], NULL, pipe_fd[1], NULL, 4096, SPLICE_F_MOVE);
			printf("%d %x\n", rxlen, buffer);
    		splice(pipe_fd[0], NULL, fd, NULL, rxlen, SPLICE_F_MOVE);
			PRINT_MESSAGE((char*)buffer, rxlen);
			memcpy(msg.buffer, buffer, rxlen);
			PRINT_MESSAGE((char*)msg.buffer, rxlen);*/
			if (rxlen <= 0) {
      			printf("Error! recv(): len <= 0 at CP\n");
				perror("recv failed:");
				msg.sockfd = 0;
				msg.type = DRIV_FAIL;
				mail.len = sizeof(int)+1;
    		}
			else {
    			msg.sockfd = ofp_io_fds[0];
				msg.type = DRIV_CP;
				mail.len = rxlen + sizeof(int) + 1;
			}
			mail.type = IPC_EV_TYPE_DRV;
			rte_memcpy(mail.refp, &msg, mail.len);
			mailbox(any2ofp, (U8 *)&mail, 1);
   			/*printf("=========================================================\n");
			printf("rxlen=%d\n",rxlen);*/
    		//PRINT_MESSAGE((char*)msg.buffer, rxlen);
    		
   		//} /* if select */
   	} /* for */
	
	return 0;
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
	send(sockfd, mu, mulen, 0);
}
