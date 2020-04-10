/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  IP_CODEC.C

  Designed by Dennis Tseng on Jan 1, 2002
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include 	"common.h"
#include 	"ip_codec.h" 
 
U8  		*ENCODE_IP_PKT(tIP_PKT *ip_pkt, U8 *cp);
U16 		CHKSUM_IP_HDR(tIP_PKT *ip_pkt);
U16         CHKSUM_UDP(tIP_PKT *ip_pkt, tUDP_PKT *udp_pkt);
void 		PRINT_IP_PKT(tIP_PKT *ip_pkt);

/*============================== IP PACKET =================================*/

/************************************************
 * DECODE_IP_PKT
 *
 ************************************************/
STATUS DECODE_IP_PKT(tIP_PKT *ip_pkt, U8 *data, U16 *len)
{
    U16  chksum,tmp;
    U8   *cp;
    int  i;

    /*
    printf("********************************\n");
    for(i=0; i<(*len); i++)
        printf("%x ",*(data+i));
    printf("\n********************************\n");
    */

    cp = (U8*)&ip_pkt->ver_ihl;
    *cp = *data++;
    if (ip_pkt->ver_ihl.ver != IP_VERSION){
        printf("DECODE_IP_PKT() : ip version(%x) error\07\n",ip_pkt->ver_ihl.ver);
        return ERROR;
    }
    ip_pkt->tos = *data++;

    data = DECODE_U16(&ip_pkt->total_len, data);
    data = DECODE_U16(&ip_pkt->id, data);

	/* 2008.10.27: replaced by the following ...
    cp = (U8*)&ip_pkt->flag_frag;
    *cp++ = *data++;
    ip_pkt->flag_frag.frag_off = ip_pkt->flag_frag.frag_off<<8 | *data++;
    */
    ip_pkt->flag_frag.flag = (U8)(*data >> 5);
    data = DECODE_U16(&tmp, data);
	ip_pkt->flag_frag.frag_off = tmp & 0x1fff;
	
	/*
	printf("DECODE_IP_PKT: flag=%x frag_off=%x\n",
	ip_pkt->flag_frag.flag,
	ip_pkt->flag_frag.frag_off);*/
	
    ip_pkt->ttl = *data++;
    ip_pkt->proto = *data++;

    data = DECODE_U16(&ip_pkt->head_chksum, data);

    for(i=0; i<IP_ADDR_LEN; i++){
        ip_pkt->cSA[i] = *data++;
    }

    for(i=0; i<IP_ADDR_LEN; i++){
        ip_pkt->cDA[i] = *data++;
    }

    i = ip_pkt->ver_ihl.IHL - IP_MIN_HDR_LEN;
    if (i){
    	/* malloc is dangerous
        ip_pkt->opt_pad = (U8 *)malloc(4*i);
        bcopy(data,ip_pkt->opt_pad,i*4);
        */
        ip_pkt->opt_pad = data; //just points to incoming data
        data += i * 4;
    }
    else{
        ip_pkt->opt_pad = NULL;
    }

    *len -= ip_pkt->ver_ihl.IHL*4;
    ip_pkt->data = data;

	//PRINT_IP_PKT(ip_pkt);

    /* ip chksum is mandatory, while udp chksum is optional */
    chksum = CHKSUM_IP_HDR(ip_pkt);
    if (chksum != ip_pkt->head_chksum){
        printf("DECODE_IP_PKT: my-chksum(0x%x) != given-chksum(0x%x)\n",
        chksum, ip_pkt->head_chksum);
        return ERROR;
    }
    return TRUE;
}

/************************************************************************
 * ENCODE_IP_PKT
 *
 * Example :
 *   
 *  == ip packet header ==
 *  ip_pkt.ver_ihl.ver = IP_VERSION;
 *	ip_pkt.ver_ihl.IHL = IP_MIN_HDR_LEN;
 *	ip_pkt.tos = 0x00;  == 0xC0 for internetwork protocol ==
 *	ip_pkt.total_len = (IP_MIN_HDR_LEN*4) + UDP_HDR_LEN + dhcplen;
 *	ip_pkt.id = 0; == fragment datagram id ==
 *	ip_pkt.flag_frag.flag = 2; == 3 bits = 010, may-fragment,last-segment,0 ==
 *	ip_pkt.flag_frag.frag_off = 0;
 *	ip_pkt.ttl = MY_IP_TTL;
 *	ip_pkt.proto = PROTO_TYPE_UDP;
 *	ip_pkt.opt_pad = opt;
 *		
 *  memcpy(ip_pkt.cDA, cDA_ip, IP_ADDR_LEN); == DA IP ==
 *	memcpy(ip_pkt.cSA, cSA_ip, IP_ADDR_LEN); == SA IP ==
 *	
 *	== udp ==
 *	udp_pkt.src = DHCPR_SRV_PORT;
 *  udp_pkt.dst = DHCPR_CLI_PORT;
 *  udp_pkt.len = UDP_HDR_LEN + dhcplen;
 *  udp_pkt.data = dhcpm;
 *   
 *	mp = ENCODE_IP_PKT(&ip_pkt, mp);
 *	mp = ENCODE_UDP_PKT(&ip_pkt, &udp_pkt, mp);        
 **************************************************************************/
U8 *ENCODE_IP_PKT(tIP_PKT *ip_pkt, U8 *mp)
{
    UINT  opt_len;
    U16	  tmp;

	*mp++ = (U8)(ip_pkt->ver_ihl.ver<<4 | ip_pkt->ver_ihl.IHL);
	*mp++ = ip_pkt->tos;
	
    mp = ENCODE_U16(mp,ip_pkt->total_len);
	mp = ENCODE_U16(mp,ip_pkt->id);
	tmp = *(U16*)&ip_pkt->flag_frag;
	mp = ENCODE_U16(mp,tmp);

	*mp++ = ip_pkt->ttl;
    *mp++ = ip_pkt->proto;

    ip_pkt->head_chksum = CHKSUM_IP_HDR(ip_pkt);
    mp = ENCODE_U16(mp,ip_pkt->head_chksum);
        
    memcpy(mp, ip_pkt->cSA, IP_ADDR_LEN);
    mp += IP_ADDR_LEN;
	
	memcpy(mp, ip_pkt->cDA, IP_ADDR_LEN);
    mp += IP_ADDR_LEN;
	
    if (ip_pkt->opt_pad){
        opt_len = (ip_pkt->ver_ihl.IHL - IP_MIN_HDR_LEN) * 4;
        if (opt_len){
            memcpy(mp,ip_pkt->opt_pad,opt_len);
            mp += opt_len;
        }
    }
    return mp;
}

/*-----------------------------------------------
 * CHKSUM_IP_HDR
 *
 *----------------------------------------------*/
U16 CHKSUM_IP_HDR(tIP_PKT *ip_pkt)
{
    U32  sum=0;
    U8   *u8p,i;

    sum += (U16)(((ip_pkt->ver_ihl.ver<<4 | ip_pkt->ver_ihl.IHL)<<8) | ip_pkt->tos);
    sum += ip_pkt->total_len;
    sum += ip_pkt->id;
    sum += (U16)(ip_pkt->flag_frag.flag<<13 | ip_pkt->flag_frag.frag_off);
    sum += (U16)(ip_pkt->ttl<<8 | ip_pkt->proto);
    sum += (U16)(ip_pkt->cSA[0]<<8 | ip_pkt->cSA[1]);
    sum += (U16)(ip_pkt->cSA[2]<<8 | ip_pkt->cSA[3]);
    sum += (U16)(ip_pkt->cDA[0]<<8 | ip_pkt->cDA[1]);
    sum += (U16)(ip_pkt->cDA[2]<<8 | ip_pkt->cDA[3]);

    if (ip_pkt->opt_pad){ /* options and padding */
        u8p = (U8*)ip_pkt->opt_pad;
        for(i=0; i<(ip_pkt->ver_ihl.IHL-5)*2; i++){ 
            sum += (U16)(*u8p<<8 | *(u8p+1));
            u8p += 2;
        }
    }
    
    return CHECK_SUM(sum);
}

/*-----------------------------------------------
 * CHKSUM_IP_HDR2
 *
 *----------------------------------------------*/
U16 CHKSUM_IP_HDR2(tIP_PKT *ip_pkt, unsigned char opt_en, unsigned long *opt_addr)
{
    U32  sum=0;
    U8   *u8p,i;

    sum += (U16)(((ip_pkt->ver_ihl.ver<<4 | ip_pkt->ver_ihl.IHL)<<8) | ip_pkt->tos);
    sum += ip_pkt->total_len;
    sum += ip_pkt->id;
    sum += (U16)(ip_pkt->flag_frag.flag<<13 | ip_pkt->flag_frag.frag_off);
    sum += (U16)(ip_pkt->ttl<<8 | ip_pkt->proto);
    sum += (U16)(ip_pkt->cSA[0]<<8 | ip_pkt->cSA[1]);
    sum += (U16)(ip_pkt->cSA[2]<<8 | ip_pkt->cSA[3]);
    sum += (U16)(ip_pkt->cDA[0]<<8 | ip_pkt->cDA[1]);
    sum += (U16)(ip_pkt->cDA[2]<<8 | ip_pkt->cDA[3]);

    if (opt_en){ /* options and padding */
    /*
        if(rx_debug)
            printf("opt_pas\n\r");
            */
        u8p = (U8*)opt_addr;
        for(i=0; i<(ip_pkt->ver_ihl.IHL-5)*2; i++){
            /*printf("[%x] [%x]", *u8p, *(u8p+1)); */
            sum += (U16)(*u8p<<8 | *(u8p+1));
            u8p += 2;
        }
        /*printf("\n\r");*/
    }
    
    return CHECK_SUM(sum);
}


/*-----------------------------------------------
 * PRINT_IP_PKT
 *
 *----------------------------------------------*/
void PRINT_IP_PKT(tIP_PKT *ip_pkt)
{
    U8  i;

    printf("-----------------------------------\n");
    printf("ip_pkt->ver_ihl.ver=%x\n",ip_pkt->ver_ihl.ver);
    printf("ip_pkt->ver_ihl.IHL=%x\n",ip_pkt->ver_ihl.IHL);
    printf("ip_pkt->tos=%x\n",ip_pkt->tos);
    printf("ip_pkt->total_len=%x\n",ip_pkt->total_len);
    printf("ip_pkt->id=%x\n",ip_pkt->id);
    printf("ip_pkt->ttl=%x\n",ip_pkt->ttl);
    printf("ip_pkt->proto=%x\n",ip_pkt->proto);
    printf("ip_pkt->head_chksum=%x\n",ip_pkt->head_chksum);

    printf("ip_pkt->src=> ");
    for(i=0; i<IP_ADDR_LEN; i++){
        printf("%x",ip_pkt->cSA[i]);
        if (i<3)  printf(".");
    }
    printf("\n");

    printf("ip_pkt->dst=> ");
    for(i=0; i<IP_ADDR_LEN; i++){
        printf("%x",ip_pkt->cDA[i]);
        if (i<3)  printf(".");
    }
    printf("\n");
    printf("-----------------------------------\n");
}

#if 0
/*-----------------------------------------------
 * CHECK_SUM
 *
 *----------------------------------------------*/
U16 CHECK_SUM(sum)
    U32 sum;
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
#endif

/*=================================  UDP  =================================*/

/*************************************************
 * DECODE_UDP_PKT
 *
 * [input] : ip_pkt, len - ip's data length
 *************************************************/
int DECODE_UDP_PKT(tUDP_PKT *udp_pkt, tIP_PKT *ip_pkt, U16 *data_len)
{
    U16  chksum;
    U8   *data=ip_pkt->data;

	data = DECODE_U16(&udp_pkt->src, data); 
    data = DECODE_U16(&udp_pkt->dst, data);
    data = DECODE_U16(&udp_pkt->len, data);
    if (udp_pkt->len != (ip_pkt->total_len - ip_pkt->ver_ihl.IHL*4)){
    	//#ifdef _ENABLE_DBG_
    	printf("udp len%d(data+head) != ip-data-len%d\n",
    		udp_pkt->len,(ip_pkt->total_len - ip_pkt->ver_ihl.IHL*4));
    	//#endif
    	return -1;
    }
    data = DECODE_U16(&udp_pkt->chksum, data);
    udp_pkt->data = data;
    
    /*
     * UDP chksum is optional;
     * a value of zero means it has not been computed.
     */
    if (udp_pkt->chksum){
        chksum = CHKSUM_UDP(ip_pkt,udp_pkt);
        if (chksum != udp_pkt->chksum){
            //#ifdef _ENABLE_DBG_
    		printf("DECODE_UDP_PKT : chksum(0x%x) error!\07\n",chksum);
            //#endif
            return -1;
        }
    }
    
    //PRINT_UDP_PKT(udp_pkt);
            
    *data_len -= UDP_HDR_LEN;
    return 0;
}

/****************************************************
 * ENCODE_UDP_PKT
 *
 * [input] : ip_pkt, udp_pkt, cp
 * [return] : cp
 ****************************************************/
U8* ENCODE_UDP_PKT(tIP_PKT *ip_pkt, tUDP_PKT *udp_pkt, U8 *mp)
{
    mp = ENCODE_U16(mp,udp_pkt->src);
    mp = ENCODE_U16(mp,udp_pkt->dst);
    mp = ENCODE_U16(mp,udp_pkt->len);
    udp_pkt->chksum = CHKSUM_UDP(ip_pkt, udp_pkt);
    mp = ENCODE_U16(mp,udp_pkt->chksum);
    return mp;
}

/*-----------------------------------------------
 * CHKSUM_UDP
 *
 *----------------------------------------------*/
U16 CHKSUM_UDP(tIP_PKT *ip_pkt, tUDP_PKT *udp_pkt)
{
    U32  sum=0;
    U8   *ptr;
    U8   *cp;
    int  i,j,data_len;

    /*------ compute pseudo-header ------*/
    sum += (U16)(ip_pkt->cSA[0]<<8 | ip_pkt->cSA[1]);
    sum += (U16)(ip_pkt->cSA[2]<<8 | ip_pkt->cSA[3]);
    sum += (U16)(ip_pkt->cDA[0]<<8 | ip_pkt->cDA[1]);
    sum += (U16)(ip_pkt->cDA[2]<<8 | ip_pkt->cDA[3]);
    sum += PROTO_TYPE_UDP; /* ZERO/PROTO */
    sum += udp_pkt->len;  /* UDP LENGTH */

    /*------ compute udp header+body ------*/
    sum += udp_pkt->src;
    sum += udp_pkt->dst;
    sum += udp_pkt->len;
    data_len = udp_pkt->len - 8; /* 8 : includes chksum field */

    if (data_len & 0x01){  /* odd len, but checksum is in short base(even) */
        cp = udp_pkt->data;
        cp[data_len] = 0;
        data_len++; /* while not modify the true data length - udp_pkt->len */
    }
    ptr = (U8*)udp_pkt->data; /* only includes data */

    j = data_len >> 1;
    for(i=0; i<j; i++){
        sum += (U16)(*ptr<<8 | *(ptr+1));
        ptr += 2;

    }
    return CHECK_SUM(sum);
}

/*-----------------------------------------------
 * PRINT_UDP_PKT
 *
 *----------------------------------------------*/
void PRINT_UDP_PKT(tUDP_PKT * udp_pkt)
{
    U16  i;

    printf("-----------------------------------\n");
    printf("udp_pkt->src_port=%x\n",udp_pkt->src);
    printf("udp_pkt->dst_port=%x\n",udp_pkt->dst);
    printf("udp_pkt->len=%x\n",udp_pkt->len);
    printf("udp_pkt->chksum=%x\n",udp_pkt->chksum);
    printf("udp_pkt->data=");
    for(i=0; i<udp_pkt->len-UDP_HDR_LEN; i++)
        printf("%02x ",(U8)*(udp_pkt->data+i));
    printf("\n-----------------------------------\n");
}

/*=================================  ARP  =================================*/

#if 0
/************************************************
 * DECODE_ARP_PKT
 *
 ************************************************/
U16  DECODE_ARP_PKT(arp_pkt, data, len)
ARP_PKT  *arp_pkt;
char     *data;
U16      *len;
{
    int  i;

    SRV_IP_DECODE_INTEGER(&arp_pkt->hardware, data);
    data += 2;
    *len -= 2;

    SRV_IP_DECODE_INTEGER(&arp_pkt->proto, data);
    data += 2;
    *len -= 2;

    arp_pkt->hlen = *data++;
    arp_pkt->plen = *data++;

    SRV_IP_DECODE_INTEGER(&arp_pkt->op, data);
    data += 2;
    *len -= 2;

    data += MAC_ADDR_LEN;
    *len -= MAC_ADDR_LEN;

    for(i=0; i<IP_ADDR_LEN; i++)
        arp_pkt->sa_ip[i] = *data++;
    *len -= IP_ADDR_LEN;

    data += MAC_ADDR_LEN;
    *len -= MAC_ADDR_LEN;

    for(i=0; i<IP_ADDR_LEN; i++)
        arp_pkt->da_ip[i] = *data++;
    *len -= IP_ADDR_LEN;

    return TRUE;
}
#endif

/****************************************************
 * ENCODE_ARP_PKT
 *
 ****************************************************/
U8 *ENCODE_ARP_PKT(tARP_PKT *arp_pkt, U8 *cp)
{
    cp = ENCODE_U16(cp, arp_pkt->hardware); 
    cp = ENCODE_U16(cp, arp_pkt->proto); 
    *cp++ = arp_pkt->hlen;
    *cp++ = arp_pkt->plen;
    cp = ENCODE_U16(cp, arp_pkt->op); 
    
    /* sender HA */
    memcpy(cp, arp_pkt->sa_mac, 6);  
    cp += 6;
    
    /* sender IA */
    memcpy(cp, arp_pkt->sa_ip, 4);
    cp += 4;
    
    /* target HA */
	memcpy(cp, arp_pkt->da_mac, 6);
	cp += 6;
	
	/* target IA */
	memcpy(cp, arp_pkt->da_ip, 4);
	cp += 4;
	
    return cp;
}

#if 0
/*-----------------------------------------------
 * PRINT_ARP_PKT
 *
 *----------------------------------------------*/
PRINT_ARP_PKT(arp)
ARP_PKT *arp;
{
    U16  i;

    printf("-----------------------------------\n");
    printf("arp->hardware=%x\n",arp->hardware);
    printf("arp->proto=%x\n",arp->proto);
    printf("arp->hlen=%x plen=%x\n",arp->hlen,arp->plen);
    printf("arp->op=%x\n",arp->op);
    printf("arp->sa_mac=");
    for(i=0; i<MAC_ADDR_LEN; i++)
        printf("%x ",arp->sa_mac[i]);
    printf("\n");

    printf("arp->sa_ip=");
    for(i=0; i<IP_ADDR_LEN; i++)
        printf("%x ",arp->sa_ip[i]);
    printf("\n");

    printf("arp->da_mac=");
    for(i=0; i<MAC_ADDR_LEN; i++)
        printf("%x ",arp->da_mac[i]);
    printf("\n");

    printf("arp->da_ip=");
    for(i=0; i<IP_ADDR_LEN; i++)
        printf("%x ",arp->da_ip[i]);
    printf("\n");
    printf("-----------------------------------\n");
}
#endif
