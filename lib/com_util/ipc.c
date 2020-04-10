/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
  IPC.C
     for share memory & semaphore

  Created by Dennis Tseng on Dec 15,'99
\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include    "common.h"

#define   	IFLAGS_EXCL 	(0600|IPC_CREAT|IPC_EXCL)
#define   	IFLAGS_FORCE  	(0600|IPC_CREAT)

#define   	ERR_SHM   		((void *)-1)

#define 	ipc_tbl_shmkey	0x04
#define 	ipc_tbl_msgkey	0x05

STATUS 		RCV_MSGQT2(tIPC_ID qid, tMBUF *mbuf, int *mlen, int DA);
STATUS 		SND_MSGQT2(tIPC_ID qid, tMBUF *msg);
STATUS 		SND_MSGQT(tIPC_ID qid, U8 *msg, int mlen, long mtype);

int			hk_pid;
void 		house_keep();
void  		PRINT_IPC_TBL(tIPC_TBL *ipc_tbl);

tIPC_ID		ipc_tbl_msgid=-1;	
BOOL		alarm_flag;

/*-----------------------------------------
 * hk_exit :
 *
 * purpose : kill house-keeping task
 *----------------------------------------*/
void hk_exit()
{
    exit(0);
}

#if 0
/*************************************************************
 * ipcInit :
 *
 *************************************************************/
STATUS ipcInit()
{
	FILE  *ipc_fp;
	
	ipc_fp = fopen("/tmp/ipc.dat","r");
	if (ipc_fp == NULL){
		printf("ipc> error! ipc baseQ not existing\n");
		return -1;
	}
	fscanf(ipc_fp,"%x",&ipc_tbl_msgid);
	fclose(ipc_fp);
	
	/*
	ipc_tbl = (tIPC_TBL*)shmat(ipc_tbl_shmid,0,0);
  	if (p == ERR_SHM){
    	perror("ipcInit():shmat");
    	return -1;
  	}*/
  		
  	return TRUE;
}
#endif

/*************************************************************
 * ipcdInit()
 *
 *************************************************************/
STATUS ipcdInit()
{
	//create a msgQ for house-keeping process
	if (GET_MSGQ(&ipc_tbl_msgid, ipc_tbl_msgkey) == ERROR){
	    printf("ipc> error! can not create a ipc-baseQ\n");
	    return -1;
	}
	
	if (ipc_tbl_msgid){
		printf("ipc> terrible error! ipc default msgQ <> 0 ?\n");
		return -1;
	}
	
	/*
	FILE  *ipc_fp;
	
	ipc_fp = fopen("/tmp/ipc.dat","w");
	if (ipc_fp == NULL){
		printf("ipc> error! can not create a file(ipc.dat)\n");
		return;
	}
	fprintf(ipc_fp,"%x",ipc_tbl_msgid);
	fclose(ipc_fp);
	*/
	
	/*
	ipc_tbl = (tIPC_TBL*)GET_SHM(&ipc_tbl_shmid, ipc_tbl_shmkey, sizeof(tIPC_TBL)*MAX_IPC_NUM);
	if (ipc_tbl == NULL){
	    printf("can not create a ipc shm\n");
	    return;
	}
	for(i=0; i<MAX_IPC_NUM; i++){
		ipc_tbl[i].valid = FALSE;
	}*/
	
	signal(SIGINT, hk_exit); //only useful in non-background mode
	if ((hk_pid=fork()) == 0){
		house_keep();
	}
	return TRUE;
}

/*************************************************************
 * ipcdExit()
 *
 * - Be should be called ahead of any application.
 *************************************************************/
void ipcdExit()
{
    kill(hk_pid,SIGINT);
    DEL_MSGQ(ipc_tbl_msgid);
    //system("rm /tmp/ipc.dat");
}

/*------------------------------------------------------------
 * keepalived() : 
 *-----------------------------------------------------------*/     
void *keepalived(void *arg)
{
    int  i;
    
    for(;;){
    	sleep(SEC*2);
    	for(i=0; i<MAX_IPC_NUM; i++){
    		/*
			if (ipc_tbl[i].valid){
				printf("key(0x%x)  Qid(0x%x)  name(%s)\n",
					ipc_tbl[i].key, ipc_tbl[i].qid, ipc_tbl[i].name);
					
				if (ipc_sendw(ipc_tbl[i].qid, &ipc_tbl[i], sizeof(tIPC_TBL), 1, IPC_EV_TYPE_OAM) < 0){
					printf("ipc> %s is leaving\n",ipc_tbl[i].name);
				}
				else{
					printf("ipc> %s is happy\n",ipc_tbl[i].name);
				}
			}*/
		}
    }
}
  
/*************************************************************
 * house_keep: 
 *
 *************************************************************/
void house_keep()
{
	static tIPC_TBL ipc_tbl[MAX_IPC_NUM];
	tIPC_TBL		*ipc_entry;
	//pthread_t   	keepalive_tid;
	tMAIL			mail;
	int   			mlen,i;
	
	//pthread_create(&keepalive_tid, NULL, &keepalived, NULL);
	
	for(i=0; i<MAX_IPC_NUM; i++){
		ipc_tbl[i].valid = FALSE;
	}
	
	for(;;){
		if (ipc_rcv(ipc_tbl_msgid, (U8*)&mail, &mlen) == ERROR){
			continue;
		}
		
		ipc_entry = (tIPC_TBL*)mail.data;
			
		switch(mail.type){
		case IPC_EV_TYPE_REG:
			for(i=0; i<MAX_IPC_NUM; i++){
				if (ipc_tbl[i].valid && (ipc_tbl[i].key == ipc_entry->key)){
					DEL_MSGQ(ipc_tbl[i].qid);
					printf("ipc> remove old Qid(%x)\n",ipc_tbl[i].qid);
					break; //overwrite
				}
				if (ipc_tbl[i].valid == FALSE){
					break;
				}
			}
			if (i == MAX_IPC_NUM){
				printf("ipc> error! not more ipc resources\n");
				continue;
			}
			
			//*(tIPC_TBL*)&ipc_tbl[i] = *(tIPC_TBL*)mail.data;
			
			if (GET_MSGQ(&ipc_tbl[i].qid, ipc_entry->key) == ERROR){
	    		printf("ipc> can not create a msgQ for key(0x0%x)\n",ipc_entry->key);
	    		continue;
			}
			ipc_tbl[i].key = ipc_entry->key;
			strcpy(ipc_tbl[i].name, ipc_entry->name);
			ipc_tbl[i].valid = TRUE;
			
			PRINT_IPC_TBL(ipc_tbl);
			
			if (ipc_send2who(ipc_tbl_msgid, &ipc_tbl[i], sizeof(tIPC_TBL), mail.who) < 0){
				printf("ipc> can not reply to a ipc queue\n");
			}
			break;
	
		case IPC_EV_TYPE_MAP:	
			ipc_entry = (tIPC_TBL*)mail.data;
			
			//*(tIPC_TBL*)&entry = *(tIPC_TBL*)mail.data;
			
			for(i=0; i<MAX_IPC_NUM; i++){
				if (ipc_tbl[i].valid && (ipc_tbl[i].key == ipc_entry->key)){
					break;
				}
			}	
			if (i == MAX_IPC_NUM){
				printf("ipc> error! can not get mapped qid\n");
				continue;
			}
			
			if (ipc_send2who(ipc_tbl_msgid, &ipc_tbl[i], sizeof(tIPC_TBL), mail.who) < 0){
				printf("ipc> can not reply to a ipc queue\n");
			}
			break;
					
		default:;
		}
	}
}

/*************************************************************
 * ipc_reg: 
 *
 * input : type - what kind of IPC
 *         key - used to get Qid,Shmid,Semid
 *         pname - process name
 * output: 
 * return: id
 *************************************************************/
tIPC_ID ipc_reg(U8 type, key_t key, char *pname)
{
	tIPC_TBL	entry;
	
	entry.key = key;
	entry.pid = getpid();
	strcpy(entry.name, pname);
	entry.type = type;
	
	if (ipc_sendw(IPC_DEF_QID/*ipc_tbl_msgid*/, &entry, sizeof(entry), 0, IPC_EV_TYPE_REG) < 0){
		printf("ipc> can not create a new ipc queue\n");
		return -1;
	}
	return entry.qid;
}

/*************************************************************
 * ipc_map: 
 *
 * input : key
 * return: qid
 *************************************************************/
tIPC_ID ipc_map(key_t key)
{
	tIPC_TBL entry;

	{ /*
		FILE 	 *ipc_fp;
		
		ipc_fp = fopen("/tmp/ipc.dat","r");
		if (ipc_fp == NULL){
			printf("ipc> error! ipc baseQ not existing\n");
			return -1;
		}
		fscanf(ipc_fp,"%x",&ipc_tbl_msgid);
		fclose(ipc_fp);*/
	}
	
	entry.key = key;
	if (ipc_sendw(0/*ipc_tbl_msgid*/, &entry, sizeof(entry), 0, IPC_EV_TYPE_MAP) < 0){
		printf("ipc> error! old ipc-queue not existing for key(%x)\n",key);
		return -1;
	}
	return entry.qid;
}

/*********************************************************
 * ipc_send2who :
 *
 * Input : qid - which msgQ you want to send to
 *         data - user data to be sent
 *		   len - data length
 *		   who - to which process
 * return: 
 *********************************************************/
STATUS ipc_send2who(tIPC_ID qid, void *data, int len, int who)
{
	if (who == 0){
		return SND_MSGQ(qid, (U8*)data, len);
	}
	else{
   		return SND_MSGQT(qid, (U8*)data, len, who);
   	}
}

/*------------------------------------
 * ipc_alarm_func :
 *-----------------------------------*/
void ipc_alarm_func()
{
	alarm_flag = TRUE;
	signal(SIGALRM, ipc_alarm_func);
}

/*********************************************************
 * ipc_sw :
 * Input : qid - which msgQ you want to send to
 *         data - user data to be sent
 *		   len - data length
 *         to - timeout time; 
 *				<0 : do NOT wait; immediately return
 *				=0 : forever wait
 *				>0 : wait for a short time
 * return: data len received or -1
 *********************************************************/
STATUS ipc_sw(tIPC_ID qid, void *data, int size, int to)
{
	tMBUF	mbuf;
	int  	len;
	
	memcpy(mbuf.mtext, data, size);
	
    if (to == 0){ //forever wait
    	//send to DA-Qid and wait for SA-Pid
    	//printf("ipc_sw> %s qid=0x%x\n",CODE_LOCATION,qid);
    	
    	mbuf.DA = IPC_DEF_USR;
    	mbuf.SA = getpid();
    	if (SND_MSGQT2(qid, &mbuf) == ERROR){
        	printf("fail to send data to msgQ(0x%x)\n",qid);
        	return ERROR;
    	}
    	
    	if (RCV_MSGQT2(qid, &mbuf, &len, mbuf.SA) == ERROR){
    		printf("fail to receive data from msgQ(0x%x)\n",qid);
    	}
    	else{
    		memcpy(data, mbuf.mtext, size);
    		return len;
    	}
    }
    else if (to > 0){ //wait for a short time
    	STATUS	status;
    	
    	//printf("ipc_sw> %s qid=0x%x\n",CODE_LOCATION,qid);
    	
    	mbuf.DA = IPC_DEF_USR;
    	mbuf.SA = getpid();
    	if (SND_MSGQT2(qid, &mbuf) == ERROR){
        	printf("fail to send data to msgQ(0x%x)\n",qid);
        	return ERROR;
    	}
    	
    	alarm_flag = FALSE;
    	signal(SIGALRM, ipc_alarm_func);
    	
    	alarm(to);
   		status = RCV_MSGQT2(qid, &mbuf, &len, mbuf.SA);
   		alarm(0);
   		
   		#if 0 //test
   		{
   			char *cp=&mbuf;
   			printf("1:\n");
   			PRINT_MESSAGE(cp,0x10);
   			
   			cp = mbuf.mtext;
   			printf("2:\n");
   			PRINT_MESSAGE(cp,0x08);
   		}
   		#endif
   		
		if (alarm_flag || status == ERROR){   		
	   		printf("fail to receive data from msgQ(0x%x)\n",qid);
	   		return ERROR;
	   	}
	   	else{
	   		memcpy(data, mbuf.mtext, size);
    		return len;
    	}
    }
    else{ //to < 0
    	mbuf.DA = IPC_DEF_USR;
    	if (SND_MSGQT2(qid, &mbuf) == ERROR){
        	printf("can not send data to msgQ(0x%x)\n",qid);
        	return ERROR;
    	}
    }
    return 0;
}

/*-------------------------------------------------------
 * ipc_reply :
 *------------------------------------------------------*/
STATUS ipc_reply(tIPC_ID qid, tMBUF *mbuf)
{
	mbuf->DA = mbuf->SA;
	if (SND_MSGQT2(qid, mbuf) == ERROR){
        printf("can not reply data to msgQ(0x%x)\n",qid);
        return ERROR;
    }
    return TRUE;
}
    
/*********************************************************
 * ipc_sendw : send and then wait for reply
 *
 * Input : qid - which msgQ you want to send to
 *         data - user data to be sent
 *		   len - data length
 *         to - timeout time; 
 *				<0 : do NOT wait; immediately return
 *				=0 : forever wait
 *				>0 : wait for a short time
 * return: data len received or -1
 *********************************************************/
STATUS ipc_sendw(tIPC_ID qid, void *data, int size, int to, int type)
{
	int  	len;
	tMAIL	mail;	
	
    memcpy(mail.data,data,size);
    mail.who = getpid();
	mail.type = type;
	
    if (to == 0){ 
    	/*
    	 * - forever wait
    	 * - send to DA-Qid and wait for SA-Pid
    	 */
    	if (SND_MSGQT(qid, (U8*)&mail, sizeof(tMAIL), IPC_DEF_USR) == ERROR){
        	printf("can not send data to msgQ(0x%x)\n",qid);
        	return ERROR;
    	}
    	
    	if (RCV_MSGQT(qid, mail.data, &len, mail.who) == ERROR){
   			printf("can not receive data from msgQ(0x%x)\n",qid);
	   		return ERROR;
	   	}
	   	else{
	   		memcpy(data, mail.data, len);
    		return len;
    	}
    }
    else if (to > 0){ //wait for a short time
    	STATUS	status;
    	
    	if (SND_MSGQT(qid, (U8*)&mail, sizeof(tMAIL), IPC_DEF_USR) == ERROR){
        	printf("can not send data to msgQ(0x%x)\n",qid);
        	return ERROR;
    	}
    	
    	alarm_flag = FALSE;
    	signal(SIGALRM, ipc_alarm_func);
    	alarm(to);
   		status = RCV_MSGQT(qid, mail.data, &len, mail.who);
   		alarm(0);
		if (alarm_flag || status == ERROR){   		
	   		printf("can not receive data from msgQ(0x%x)\n",qid);
	   		return ERROR;
	   	}
	   	else{
	   		memcpy(data, mail.data, len);
    		return len;
    	}
    }
    else{ //to < 0
    	if (SND_MSGQT(qid, (U8*)&mail, sizeof(tMAIL), IPC_DEF_USR) == ERROR){
        	printf("can not send data to msgQ(0x%x)\n",qid);
        	return ERROR;
    	}
    }
    return 0;
}

/*********************************************************
 * ipc_recvw :
 *
 * Input : qid - which msgQ you want to send to
 *         data - user data to be receive
 *		   len - data length
 *         to - timeout time; 
 *				<0 : do NOT wait; immediately return
 *				=0 : forever wait
 *				>0 : wait for a short time
 * return: data len received or -1
 *********************************************************/
STATUS  ipc_recvw(tIPC_ID qid, void *data, int size, int to, int type)
{
	int  	len;
	tMAIL	mail;	
	
    memcpy(mail.data,data,size);
    mail.who = getpid();
    mail.type = type;
	
    if (to == 0){ 
    	if (RCV_MSGQT(qid, mail.data, &len, mail.who) == ERROR){
   			printf("can not receive data from msgQ(0x%x)\n",qid);
	   		return ERROR;
	   	}
	   	else{
	   		memcpy(data, mail.data, len);
    		return len;
    	}
    }
    else if (to > 0){ //wait for a short time
    	STATUS	status;
    	
    	alarm_flag = FALSE;
    	signal(SIGALRM, ipc_alarm_func);
    	alarm(to);
   		status = RCV_MSGQT(qid, mail.data, &len, mail.who);
   		alarm(0);
		if (alarm_flag || status == ERROR){   		
	   		printf("can not receive data from msgQ(0x%x)\n",qid);
	   		return ERROR;
	   	}
	   	else{
	   		memcpy(data, mail.data, len);
    		return len;
    	}
    }
    return 0;
}

/*************************************************************
 * PRINT_IPC_TBL: 
 *
 * input : tbl
 *************************************************************/
void  PRINT_IPC_TBL(tIPC_TBL *ipc_tbl)
{
	U16  i;
	
	printf("-------------------------------------------\n");
	for(i=0; i<MAX_IPC_NUM; i++){
		if (ipc_tbl[i].valid){
			printf("key(%x)  Qid(%06x)  name(%s)\n",
			ipc_tbl[i].key, ipc_tbl[i].qid, ipc_tbl[i].name);
		}
	}
	printf("-------------------------------------------\n");
}

/*************************************************************
 * GET_MSGQ : 
 *
 * input : msgkey
 * output: qid 
 * return: result
 * note  : change queue size
 * In kernel-2.4 :
 * - vi /usr/src/linuxppc-2.4/include/msg.h
 * - edit 16k to 20k
 * - linuxppc-2.4> make menuconfig (to quiet mode)
 * - linuxppc-2.4> make dep;make
 * In kernel-2.6 :
 * - kernel-2.6/include/linux/msg.h
 *************************************************************/
STATUS GET_MSGQ(tIPC_ID *qid, int msgkey)
{
	struct msqid_ds stat;
	
  	if ((*qid=msgget(msgkey,IFLAGS_EXCL)) < 0){
  		*qid = msgget(msgkey,IFLAGS_FORCE);
  		msgctl(*qid, IPC_RMID, 0);
  		*qid = msgget(msgkey,IFLAGS_EXCL);
  		if (*qid < 0){
  			perror("semget");
  			return ERROR;
  		}
  	}
  	
	if (msgctl(*qid, IPC_STAT, &stat)<0){
        printf("Can not get queue information!\n");
        return ERROR;
    }
    //printf("---> qbytes=%d qnum=%d\n",stat.msg_qbytes, stat.msg_qnum);
   
    stat.msg_qbytes *= 10; //enlarge size
    if (msgctl(*qid, IPC_SET, &stat)<0){
        printf("Can not enlarge msg-queue size!\n");
        return ERROR;
    }
    //printf("===> qbytes=%d qnum=%d\n",stat.msg_qbytes, stat.msg_qnum);
    
    return TRUE;
}

/*************************************************************
 * SND_MSGQ: send msg to message queue
 *
 * input : qid, msg, mlen
 * output:  
 * return: result
 *************************************************************/
STATUS SND_MSGQ(tIPC_ID qid, U8 *msg, int mlen)
{
    struct mbuf {
        long  mtype;
        U8    mtext[sizeof(tMAIL)];
    } buf;
    int status;
  
  	buf.mtype = 0; //for FIFO
    memcpy(buf.mtext,msg,mlen);
    
    status = msgsnd(qid,&buf,mlen,0);
    if (status < 0){
        perror("msgsnd");
        return ERROR;
    }
    return TRUE;
}

/*************************************************************
 * SND_MSGQT: send msg to message queue with mtype
 *
 * input : qid, msg, mlen, mtype
 * output:  
 * return: result
 *************************************************************/
STATUS SND_MSGQT(tIPC_ID qid, U8 *msg, int mlen, long mtype)
{
    struct {
        long  mtype;
        U8    mtext[sizeof(tMAIL)];
    } buf;
  
    buf.mtype = mtype;
    memcpy(buf.mtext,msg,mlen);
              
    if ((msgsnd(qid,&buf,mlen,0))<0){
        perror("msgsnd");
        return ERROR;
    }
    return TRUE;
}

/*----------------------------------------
 * SND_MSGQT2 :
 *---------------------------------------*/
STATUS SND_MSGQT2(tIPC_ID qid, tMBUF *mbuf)
{
    if ((msgsnd(qid,mbuf,sizeof(tMBUF)-sizeof(long),0))<0){
        perror("msgsnd");
        return ERROR;
    }
    return TRUE;
}

/*************************************************************
 * RCV_MSGQ: receive msg from message queue
 *
 * note -
 * - MSG_NOERROR :
 *   max length can be received is 2048, and if too long, 
 *   just truncate it without error 
 * 
 * - IPC_NOWAIT :
 *   If no message of the requested type is available and IPC_NOWIAT 
 *   isn't asserted in msgflg, then the calling process is blocked.
 *
 * input : qid 
 * output: msg, mlen
 * return: result
 *************************************************************/
STATUS RCV_MSGQ(tIPC_ID qid, U8 *msg, int *mlen)
{
    struct {
        long  mtype;
        U8    mtext[sizeof(tMAIL)];
    } buf;
  
    /* 
     * 4th parameter is mtype : 
     * 0L with FIFO :
     * >0 with FIFO for specific mtype
     */
    if ((*mlen=msgrcv(qid,&buf,sizeof(tMAIL),0L,0)) == -1){
        perror("msgrcv");
        return ERROR;
    }
    
    memcpy(msg, buf.mtext, *mlen);
    return TRUE;
}

/*************************************************************
 * ipc_rcv: showdow of RCV_MSGQT
 *************************************************************/
STATUS ipc_rcv(tIPC_ID qid, U8 *msg, int *mlen)
{
	return RCV_MSGQT(qid, msg, mlen, IPC_DEF_USR);
}

STATUS ipc_rcv2(tIPC_ID qid, tMBUF *mbuf, int *mlen)
{
	return RCV_MSGQT2(qid, mbuf, mlen, IPC_DEF_USR);
}

/*************************************************************
 * RCV_MSGQT: receive msg with specific type from message queue
 *
 * note -
 * - MSG_NOERROR :
 *   max length can be received is 2048, and if too long, 
 *   just truncate it without error 
 * 
 * - IPC_NOWAIT :
 *   If no message of the requested type is available and IPC_NOWIAT 
 *   isn't asserted in msgflg, then the calling process is blocked.
 *
 * input : qid 
 * output: msg, mlen
 * return: result
 *************************************************************/
STATUS RCV_MSGQT(tIPC_ID qid, void *msg, int *mlen, int mtype)
{
    struct {
        long  mtype;
        U8    mtext[sizeof(tMAIL)];
    } buf;
  
    /* 
     * 4th parameter is mtype : 
     * 0L with FIFO :
     * >0 with FIFO for specific mtype
     */
    if ((*mlen=msgrcv(qid,&buf,sizeof(tMAIL),mtype,0)) == -1){
        perror("msgrcv");
        return ERROR;
    }
    memcpy((U8*)msg, buf.mtext, *mlen);
    return TRUE;
}

STATUS RCV_MSGQT2(tIPC_ID qid, tMBUF *mbuf, int *mlen, int mtype)
{
    /* 
     * 4th parameter(mtype) is DA : 
     * a) 0L with FIFO :
     * b) >0 with FIFO for specific DA
     *
     * If msgtyp is 0, the first message on the queue is received.
	 * If msgtyp is greater than 0, the first message of type msgtyp is received.
	 * If msgtyp is less than 0, the first message of the lowest type that is 
	 * less than or equal to the absolute value of msgtyp is received. 
     */
    if ((*mlen=msgrcv(qid,mbuf,sizeof(tMBUF)-sizeof(long),mtype,0)) == -1){
        perror("msgrcv");
        return ERROR;
    }
    return TRUE;
}

/*************************************************************
 * DEL_MSGQ: 
 *
 * input : msgkey
 * output: qid 
 * return: result
 *************************************************************/
STATUS DEL_MSGQ(tIPC_ID msgid)
{
  	if (msgctl(msgid,(int)IPC_RMID,(struct msqid_ds*)NULL) < 0){
  		perror("msgctl(RMID)");
  	}
  	return TRUE;
}

/*************************************************************
 * GET_SHM: 
 *
 * input : shmid, shmkey, size
 * output: shmid 
 * return: the share memory got
 *************************************************************/
void* GET_SHM(tIPC_ID *shmid, key_t shmkey, int size)
{
    void  *p;
    
  	if ((*shmid=shmget(shmkey,size,IFLAGS_EXCL))<0){
  		*shmid = shmget(shmkey,size,IFLAGS_FORCE);
  		if (*shmid < 0){
    		perror("shmget");
    		return NULL;
    	}
  	}
    
  	p = (void*)shmat(*shmid,0,0);
  	if (p == ERR_SHM){
    	perror("shmat");
    	return NULL;
  	}
  	return p;
}

/*************************************************************
 * DEL_SEM: 
 *
 * input : msgkey
 * output: qid 
 * return: result
 *************************************************************/
STATUS DEL_SEM(tIPC_ID semid)
{
  	semctl(semid,IPC_RMID,0);
  	return TRUE;
}

/*************************************************************
 * GET_SEM: create 'val' semaphore
 *************************************************************/
STATUS GET_SEM(tIPC_ID *semid, key_t semkey, int val)
{
    union semun {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } val_arg;
    
    if ((*semid=semget(semkey,1,IFLAGS_EXCL)) < 0){
    	*semid = semget(semkey,1,IFLAGS_FORCE);
  		semctl(*semid,IPC_RMID,0);
  		*semid = semget(semkey,1,IFLAGS_EXCL);
  		if (*semid < 0){
  			perror("semget");
  			return ERROR;
  		}
  	}
  
    val_arg.val = val;
    if (semctl(*semid,0,SETVAL,val_arg) < 0){
        printf("semctl error\n");
        return ERROR;
    }
    return TRUE;
}

/*************************************************************
 * p: lock semaphore
 *************************************************************/
void p(int semid)
{
    struct sembuf p_act;
  
    p_act.sem_num = 0;
    p_act.sem_op = -1;
    p_act.sem_flg = SEM_UNDO;
    semop(semid,&p_act,1);
}

/*************************************************************
 * v: release semaphore
 *************************************************************/
void v(int semid)
{
    struct sembuf   v_act;
  
    v_act.sem_num = 0;
    v_act.sem_op = 1;
    v_act.sem_flg = SEM_UNDO;
    semop(semid,&v_act,1);
}

/*************************************************************
 * DEL_SHM: 
 *************************************************************/
void DEL_SHM(tIPC_ID shmid)
{
    shmctl(shmid, IPC_RMID, (struct shmid_ds *)0);
}

/*************************************************************
 * quit0: 
 * 
 * Used to free - shm, sema
 *************************************************************/
void quit0()
{
}
