/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  UTIL.C
    the common utilities of all files are saved in this file.
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include    "common.h"

/*------------------------------------------------------------------
 * GetStrTok
 *
 * input   : cpp - points to the searched string.
 *                 The pointed string will be modified permanently 
 *                 after running this procedure.
 *           delimiters - e.g. "." or "., " or just " "
 *
 * return  : token stored in cpp memory
 *
 * side-eff: cpp will go forward to the 1st char of
 *           next token.
 *------------------------------------------------------------------*/
char *GetStrTok(char **cpp, char *delimiters)
{
    char  *token=*cpp,*old_cpp;
    int   i,j;

    if (!(**cpp) || !delimiters[0])
        return NULL;

    for(j=0; delimiters[j];){ /* skip delimiter(s) until meet the 1st valid char */
        for(old_cpp=*cpp; **cpp && (**cpp)==delimiters[j]; (*cpp)++);
        if (!**cpp)
            return NULL;
        if (old_cpp != *cpp) /* *cpp has skipped the delimiter(s), and points to a new position of string */
            j = 0;
        else
            j++;
    }

    token = *cpp;
    for(i=0;;i++,(*cpp)++) {
    	for(j=0; delimiters[j]; j++) {
            if ((**cpp) == delimiters[j] /* end of token */ || !(**cpp) /* end of string */ ||
				(**cpp) == 0x0a || (**cpp) == 0x0d) {
                /*for(;**cpp && (**cpp)==delimiters[j]; (*cpp)++);*/
                if (**cpp && (**cpp)!=0x0a && (**cpp)!=0x0d)  (*cpp)++;
                /* move *cpp, otherwise token[i]='\0' will cause **cpp='\0' */
    			token[i] = '\0';
    			return token;
    		}
    	}
    }
    return NULL; /* Abnormal case */
}

/*----------------------------------------------------
 * DECODE_LIT_ENDIAN_U16()
 *---------------------------------------------------*/
U16  DECODE_LIT_ENDIAN_U16(U8 *message)
{
        char  cc[2];

        cc[1] = *message;  cc[0] = *(message+1);
        return *(U16*)cc;
}

/*----------------------------------------------------
 * DECODE_LIT_ENDIAN_U24
 *
 * Note: first incoming byte(biggest) saved in high mem
 *       small value <-> in low memory
 *           large value <-> in high memory
 * -+--+--+       +-------------
 *  |  |a | ----> |  |  |  |a |
 * -+--+--+       +-------------
 *                  0  1  2  3
 *---------------------------------------------------*/
U32  DECODE_LIT_ENDIAN_U24(U8 *message)
{
        char  cc[3];

        cc[2] = *(message+1);
        cc[1] = *(message+2);
        cc[0] = *(message+3);
        return *(U32*)cc;
}

/*----------------------------------------------------
 * DECODE_LIT_ENDIAN_U32()
 *
 * Note: first incoming byte(biggest) saved in high mem
 *       small value <-> in low memory
 *           large value <-> in high memory
 * -+--+--+       +-------------
 *  |  |a | ----> |  |  |  |a |
 * -+--+--+       +-------------
 *                  0  1  2  3
 *---------------------------------------------------*/
U32  DECODE_LIT_ENDIAN_U32(U8 *message)
{
        char  cc[4];

        cc[3] = *message;               cc[2] = *(message+1);
        cc[1] = *(message+2);   cc[0] = *(message+3);
        return *(U32*)cc;
}

/*----------------------------------------------------
 * DECODE_U16 :
 *
 * input  : mp
 * output : val
 *---------------------------------------------------*/
U8  *DECODE_U16(U16 *val, U8 *mp)
{
#       ifdef _BIG_ENDIAN
        *val = *(U16*)mp;
#       else
        *val = DECODE_LIT_ENDIAN_U16(mp);
#       endif

        return (mp+2);
}

/*----------------------------------------------------
 * DECODE_U24 :
 *
 * input  : mp
 * output : val
 *---------------------------------------------------*/
U8  *DECODE_U24(U32 *val, U8 *mp)
{
#       ifdef _BIG_ENDIAN
        *val = *(U32*)mp;
#       else
        *val = DECODE_LIT_ENDIAN_U32(mp);
#       endif

        return (mp+3);
}

/*----------------------------------------------------
 * DECODE_U32()
 *
 * input  : mp
 * output : val
 *---------------------------------------------------*/
U8  *DECODE_U32(U32 *val, U8 *mp)
{
#       ifdef _BIG_ENDIAN
        *val = *(U32*)mp;
#       else
        *val = DECODE_LIT_ENDIAN_U32(mp);
#       endif

        return (mp+4);
}

/*----------------------------------------------------
 * ENCODE_LIT_ENDIAN_U16
 *---------------------------------------------------*/
void ENCODE_LIT_ENDIAN_U16(U16 val, U8 *mp)
{
        U8 *cp=(U8*)&val;

        *mp++ = *(cp+1); /* biggest value will be put in the outgoing beginning addr */
        *mp = *cp;
}

/*---------------------------------------------------------------------
 * ENCODE_LIT_ENDIAN_U24
 *
 * Note: first outgoing byte(biggest) retrieved from lowest
 *       i.e. sent from high memory which used to save biggest value.
 *---------------------------------------------------------------------*/
void ENCODE_LIT_ENDIAN_U24(U32 val, U8 *mp)
{
        U8 *cp=(U8*)&val;

        *mp++ = *(cp+2); /* biggest value will be put in the outgoing beginning addr */
        *mp++ = *(cp+1);
        *mp = *cp;
}

/*----------------------------------------------------------------------
 * ENCODE_LIT_ENDIAN_U32
 *
 * Note: first outgoing byte(biggest) retrieved from lowest
 *       i.e. sent from high memory which used to save biggest value.
 *---------------------------------------------------------------------*/
void ENCODE_LIT_ENDIAN_U32(U32 val, U8 *mp)
{
        U8 *cp=(U8*)&val;

        *mp++ = *(cp+3); /* biggest value will be put in the outgoing beginning addr */
        *mp++ = *(cp+2);
        *mp++ = *(cp+1);
        *mp = *cp;
}

/*----------------------------------------------------
 * ENCODE_U16
 *
 * input  : val
 * output : mp
 *---------------------------------------------------*/
U8 *ENCODE_U16(U8 *mp, U16 val)
{
#       ifdef _BIG_ENDIAN
        *(U16*)mp = val;
#       else
        ENCODE_LIT_ENDIAN_U16(val,mp);
#       endif

        return (mp+2);
}

/*----------------------------------------------------
 * ENCODE_U24
 *
 * input  : val
 * output : mp
 *---------------------------------------------------*/
U8 *ENCODE_U24(U8 *mp, U32 val)
{
#       ifdef _BIG_ENDIAN
        *(U32*)mp = val;
#       else
        ENCODE_LIT_ENDIAN_U24(val,mp);
#       endif

        return (mp+3);
}

/*----------------------------------------------------
 * ENCODE_U32
 *
 * input  : val
 * output : mp
 *---------------------------------------------------*/
U8 *ENCODE_U32(U8 *mp, U32 val)
{
#       ifdef _BIG_ENDIAN
        *(U32*)mp = val;
#       else
        ENCODE_LIT_ENDIAN_U32(val,mp);
#       endif

        return (mp+4);
}

/*-----------------------------------------------
 * ADD_CARRY_FOR_CHKSUM
 *
 *----------------------------------------------*/
U16 ADD_CARRY_FOR_CHKSUM(U32 sum)
{
    U32  value;
    U8   carry;

    carry = (U8)(sum >> 16); //get carry
    value = sum & 0x0000ffff;
    sum = carry + value;

    carry = (U8)(sum >> 16);
    sum += carry;

    return sum;
}

/*-----------------------------------------------
 * CHECK_SUM
 *
 *----------------------------------------------*/
U16 CHECK_SUM(U32 sum)
{
    U32  value;
    U8   carry;

    carry = (U8)(sum >> 16); //get carry
    value = sum & 0x0000ffff;
    sum = carry + value;

    carry = (U8)(sum >> 16);
    sum += carry;

    return (U16)~sum;
}

/*------------------------------------------------------------
 * get_local_mac:
 *-----------------------------------------------------------*/
int get_local_mac(U8 *mac, char *sif)
{
    struct ifreq    sifreq; //socket Interface request
    int                     fd;
    
    fd = socket(PF_INET,SOCK_STREAM,0);
    if (fd < 0){
       printf("error! can not open socket for getting mac\n");
       return -1;
    }
    
    strncpy(sifreq.ifr_name, sif, IF_NAMESIZE);
    if (ioctl(fd, SIOCGIFHWADDR, &sifreq) != 0){
        printf("error! ioctl failed when getting mac\n");
        close(fd);
        return -1;
    }
    memmove((void*)&mac[0], (void*)&sifreq.ifr_ifru.ifru_hwaddr.sa_data[0], 6);
    return 0;
}

/*------------------------------------------------------------
 * get_local_ip:
 *
 * ioctl(sockfd, SIOCGIFFLAGS, &ifr);
 * if (ifr.ifr_flags & IFF_UP) {
 *     printf("eth0 is up!");
 * }
 *-----------------------------------------------------------*/
int get_local_ip(U8 *ip_str, char *sif)
{
    struct ifreq    ifr; //socket Interface request
    struct in_addr  sin_addr;
    int             fd;
    
    fd = socket(PF_INET,SOCK_STREAM,0);
    if (fd < 0){
        printf("error! can not open socket for getting ip\n");
        return -1;
    }
    
    strncpy(ifr.ifr_name, sif, IF_NAMESIZE);    
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFADDR error");
        return -1;
    }
    
    sin_addr.s_addr = *(U32*)&ifr.ifr_addr.sa_data[2];
    strcpy((char*)ip_str, inet_ntoa(sin_addr));
    return 0;
}

/*------------------------------------------------------------
 * set_local_ip:
 *
 * ioctl(sockfd, SIOCSIFFLAGS, &ifr);
 * if (ifr.ifr_flags & IFF_UP) {
 *     printf("eth0 is up!");
 * }
 *-----------------------------------------------------------*/
int set_local_ip(char *ip_str, char *sif) 
{ 
    struct ifreq ifr; //socket Interface request 
    struct sockaddr_in addr,mask; 
    int fd; 
 
    fd = socket(AF_INET,SOCK_STREAM,0); 
    if (fd < 0){ 
        printf("error! can not open socket for getting ip\n"); 
        return -1; 
    } 
 
    bzero(&ifr,sizeof(ifr));  
    bzero(&addr,sizeof(addr));  
    bzero(&mask,sizeof(mask));  
 
    //sprintf(ifr.ifr_name, sif); 
    strncpy(ifr.ifr_name, sif, IF_NAMESIZE);  
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = inet_addr(ip_str); 
    //printf("addr.sin_addr.s_addr=%x\n",addr.sin_addr.s_addr); 
    memcpy(&ifr.ifr_addr, &addr, sizeof(struct sockaddr)); 
    if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) { 
        perror("ioctl SIOCSIFADDR error"); 
        return -1; 
    } 
 
    mask.sin_family = AF_INET; 
    mask.sin_addr.s_addr = inet_addr("255.255.0.0"); 
    memcpy(&ifr.ifr_addr, &mask, sizeof(struct sockaddr)); 
    if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0) { 
        perror("ioctl SIOCSIFNETMASK error"); 
        return -1; 
    } 
 
    ifr.ifr_flags |= IFF_UP |IFF_RUNNING;  
 
    //get the status of the device  
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) { 
        perror("SIOCSIFFLAGS");  
        return -1;  
    }  
    if (ifr.ifr_flags & IFF_UP) { 
        printf("%s is up!\n",sif); 
    } 
    close(fd);  
    return 0; 
}

int ethernet_interface(const char *const name, int *const index, int *const speed, int *const duplex)
{
    struct ifreq        ifr;
    struct ethtool_cmd  cmd;
    int                 fd, result;

    if (!name || !*name) {
        fprintf(stderr, "Error: NULL interface name.\n");
        fflush(stderr);
        return errno = EINVAL;
    }

    if (index)  *index = -1;
    if (speed)  *speed = -1;
    if (duplex) *duplex = -1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        const int err = errno;
        fprintf(stderr, "%s: Cannot create AF_INET socket: %s.\n", name, strerror(err));
        fflush(stderr);
        return errno = err;
    }

    strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
    ifr.ifr_data = (void *)&cmd;
    cmd.cmd = ETHTOOL_GSET;
    if (ioctl(fd, SIOCETHTOOL, &ifr) < 0) {
        const int err = errno;
        do {
            result = close(fd);
        } while (result == -1 && errno == EINTR);
        fprintf(stderr, "%s: SIOCETHTOOL ioctl: %s.\n", name, strerror(err));
        return errno = err;
    }

    if (speed)
        *speed = ethtool_cmd_speed(&cmd);

    if (duplex)
        switch (cmd.duplex) {
        case DUPLEX_HALF: *duplex = 0; break;
        case DUPLEX_FULL: *duplex = 1; break;
        default:
            fprintf(stderr, "%s: Unknown mode (0x%x).\n", name, cmd.duplex);
            fflush(stderr);
            *duplex = -1;
        }

    if (index && ioctl(fd, SIOCGIFINDEX, &ifr) >= 0)
        *index = ifr.ifr_ifindex;

    do {
        result = close(fd);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        const int err = errno;
        fprintf(stderr, "%s: Error closing socket: %s.\n", name, strerror(err));
        return errno = err;
    }

    return 0;
}

/********************************************************************
 * PRINT_MESSAGE()
 *
 ********************************************************************/
void  PRINT_MESSAGE(unsigned char *msg, int len)
{
	int  row_cnt,rows,rest_bytes,hex_cnt,ch_cnt,cnt,xi,ci;
	
	if (NULL == msg){
		printf("PRINT_MESSAGE(): NULL message ?\n");
		return;
	}
	
	/*if ((len*5) > 2048){ // 5 format bytes for one raw data byte
		printf("Too large[len(%d) > max(%d)] to print out!\n",len,2048);
		return;
	}*/
	
	rest_bytes = len % 16;
	rows = len / 16;
	ci = xi = 0;
	
	for(row_cnt=0; row_cnt<rows; row_cnt++){
		/*------------- print label for each row --------------*/
		printf("%04x:  ",(row_cnt+1)<<4);
		
		/*------------- print hex-part --------------*/
		for(hex_cnt=1; hex_cnt<=8; hex_cnt++){
		    if (hex_cnt < 8)
		        printf("%02x ",msg[xi++]); /* Must be unsigned, otherwise garbage displayed */
		    else
		        printf("%02x",msg[xi++]); /* Must be unsigned, otherwise garbage displayed */
		}
		
		/* delimiters space for each 8's Hex char */
		printf("  ");
		
		for(hex_cnt=9; hex_cnt<=16; hex_cnt++){
		    if (hex_cnt < 16)
		        printf("%02x ",msg[xi++]);
		    else
		        printf("%02x",msg[xi++]);
		}
		
		/* delimiters space bet. Hex and Character row */
		printf("    ");
		
		/*------------- print character-part --------------*/
		for(ch_cnt=1; ch_cnt<=16; ch_cnt++,ci++){
			if (msg[ci]>0x20 && msg[ci]<=0x7e){
		    	printf("%c",msg[ci]);
		    }
		    else{
		    	printf(".");
			}
		}
		printf("\n");
	} //for
	
	/*================ print the rest bytes(hex & char) ==================*/
	if (rest_bytes == 0) {
		printf("\n");
		return;
	}
	
	/*------------- print label for last row --------------*/
	printf("%04x:  ",(row_cnt+1)<<4);
		
	/*------------- print hex-part(rest) --------------*/
	if (rest_bytes < 8){
	    for(hex_cnt=1; hex_cnt<=rest_bytes; hex_cnt++){
	        printf("%02x ",msg[xi++]);
	    }
	
	    /* fill in the space for 16's Hex-part alignment */
	    for(cnt=rest_bytes+1; cnt<=8; cnt++){ /* from rest_bytes+1 to 8 */
	        if (cnt < 8)
	            printf("   ");
	        else
	            printf("  ");
	    }
	
	    /* delimiters bet. hex and char */
	    printf("  ");
	
	    for(cnt=9; cnt<=16; cnt++){
	        if (cnt < 16)
	            printf("   ");
	        else
	            printf("  ");
	    }
	    printf("    ");
	}
	else if (rest_bytes == 8){
	    for(hex_cnt=1; hex_cnt<=rest_bytes; hex_cnt++){
	        if (hex_cnt < 8)
	            printf("%02x ",msg[xi++]);
	        else
	            printf("%02x",msg[xi++]);
	    }
	    printf("  ");
	
	    for(cnt=9; cnt<=16; cnt++){
	        if (cnt < 16)
	            printf("   ");
	        else
	            printf("  ");
	    }
	    printf("    ");
	}
	else{ /* rest_bytes > 8 */
	    for(hex_cnt=1; hex_cnt<=8; hex_cnt++){
	        if (hex_cnt < 8)
	            printf("%02x ",msg[xi++]);
	        else
	            printf("%02x",msg[xi++]);
	    }
	
	    /* delimiters space for each 8's Hex char */
	    printf("  ");
	
	    for(hex_cnt=9; hex_cnt<=rest_bytes; hex_cnt++){ /* 9 - rest_bytes */
	        if (hex_cnt < 16)
	            printf("%02x ",msg[xi++]);
	        else
	            printf("%02x",msg[xi++]);
	    }
	
	    for(cnt=rest_bytes+1; cnt<=16; cnt++){
	        if (cnt < 16)
	            printf("   ");
	        else
	            printf("  ");
	    }
	    /* delimiters space bet. Hex and Character row */
	    printf("    ");
	} /* else */
	
	/*------------- print character-part --------------*/
	for(ch_cnt=1; ch_cnt<=rest_bytes; ch_cnt++,ci++){
		if (msg[ci]>0x20 && msg[ci]<=0x7e){
	        printf("%c",msg[ci]);
	    }
	    else
	        printf(".");
	}
	printf("\n");
}

STATUS BYTES_CMP(U8 *var1, U8 *var2, U32 len)
{
    for(U32 i=0; i<len; i++) {
		if (*(var1+i) != *(var2+i))
			return FALSE;
	}
    return TRUE;
}

uint32_t hash_func(unsigned char *key, size_t len)
{
    uint32_t hash, i; 
    for (hash = i = 0; i < len; ++i) {
        hash += key[i];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

int abs_int32(int *x)
{
    // x < 0 ? -x : x;
    return (*x ^ (*x >> 31)) - (*x >> 31);
}
