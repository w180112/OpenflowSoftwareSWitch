/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OS_TIMER.C

  Designed by Dennis Tseng on Apr 16,'01
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include        "common.h"

extern char     *SYS_UP_CTIME();
int 			TMR_forkTask();

#ifdef UNIX_VERSION
static key_t    tmr_semkey=0x01;
static key_t    tmr_shmkey=0x02;
static key_t    tmr_msgkey=0x03;
tIPC_ID        	tmr_semid,tmr_shmid;
tIPC_ID        	tmrQ;
#endif

#ifdef VXW_VERSION
SEM_ID          tmrExclSem;
#endif

void            os_tmr();
void 			OSTMR_FreeIpcs();
int             tmrTid;
tTMR_OBJ        tmrObj; /* must be defined behind above key & id */

/* testing ...
void INIT_TMR()
{
    int    ccb;
  
    tmrObj.InitTmrCbs();
    tmrObj.StartTmr(&ccb, 3, "Tmr:test", 10);
    tmrObj.StartTmr(&ccb, 3, 1701, "Tmr:test", 10);
    tmrObj.StartTmr(&ccb, 3, NULL, 1701, "Tmr:test", 10);
}
*/

/*************************************************************
 * tmrInit
 *
 *************************************************************/
int tmrInit()
{
	if (OSTMR_InitTmrCbs() == ERROR){
		  return ERROR;
	}
	return TMR_forkTask(); /* Must be put behind InitTmrCbs() */
}

/*************************************************************
 * tmrExit()
 *
 * - Be should be called ahead of any application.
 *************************************************************/
void tmrExit()
{
    /*
    ** Kill task ahead of killing ipc resource, 
    ** otherwise, task will waiting an error queue 
    ** or sources 
    */
    kill(tmrTid,SIGINT);
    OSTMR_FreeIpcs();
}

/*********************************************************************
 * timer
 *
 * purpose : compares the delta time of head node of timer list
 *           and invokes corresponding event handler if time out.
 **********************************************************************/
/*-----------------------------------------
 * wakeup : used to wakeup timer task when 
 * new timer added.
 *----------------------------------------*/
void wakeup()
{
#   ifdef UNIX_VERSION  
    signal(SIGUSR1, (SIG_FUNCPTR)wakeup);
#   endif 
} 

/*-----------------------------------------
 * FreeIpcs : 
 *
 * purpose : - free ipc caused by tmr task
 *           - kill timer task
 *----------------------------------------*/
void OSTMR_FreeIpcs()
{
#   ifdef UNIX_VERSION  
    semctl(tmr_semid,0,IPC_RMID,(struct semid_ds*)NULL);
    shmctl(tmr_shmid,IPC_RMID,(struct shmid_ds*)NULL);
    msgctl(tmrQ,IPC_RMID,(struct msqid_ds*)NULL);
#   endif 
}

/*-----------------------------------------
 * TMR_sigExit
 *
 * purpose : kill timer task
 *----------------------------------------*/
void TMR_sigExit()
{
    exit(0);
}

/*--------------------------------------------------------------------
 * InitTmrCbs : 
 *
 * purpose: get a new semaphore, a share memory and initialize all 
 *          tmr control blocks in this share memory.
 * input  : p - tmr block
 * return : 
 *-------------------------------------------------------------------*/
STATUS OSTMR_InitTmrCbs()
{
  U16  i;

  #ifdef UNIX_VERSION
    /* For unix process inherentance reason,
    Signal must be put ahead of "fork()" a timer task */
    signal(SIGUSR1, (SIG_FUNCPTR)wakeup);
    GET_SEM(&tmr_semid,tmr_semkey,1); 
    GET_MSGQ(&tmrQ,tmr_msgkey);
    tmrObj.shm = (tTMR_CBs*)GET_SHM(&tmr_shmid, tmr_shmkey, sizeof(tTMR_CBs));  
    if (tmrObj.shm == NULL)  return ERROR;
  #elif defined(VXW_VERSION)
    tmrExclSem = semMCreate(SEM_Q_PRIORITY|SEM_INVERSION_SAFE|SEM_DELETE_SAFE);
    tmrObj.shm = (tTMR_CBs*)OS_MALLOC(sizeof(tTMR_CBs),CODE_LOCATION);
  #endif
  
    tmrObj.shm->tmrHdr = NULL;
    for(i=0; i<MAX_TMR_CB_NUM; i++){
        tmrObj.shm->tcb[i].idle = TRUE;
    }
    return TRUE;
}

/*-----------------------------------------
 * TMR_forkTask
 *----------------------------------------*/
int TMR_forkTask()
{ 
    if ((tmrTid=fork()) == 0){
        os_tmr();
    }
    return tmrTid;   
}
 
/*==============================================================
 * os_tmr : 
 *=============================================================*/ 
void os_tmr(void)
{
    #ifdef  VXW_VERSION
      LOCAL SEM_ID      delaySem;
      U64               bgn_time=0,end_time=0;
    #elif               defined(UNIX_VERSION)
      struct timeval    first,second;
      struct timezone   temp;
      time_t            bgn_time=0,end_time=0;
    #else 
      #error            Compiler-Option! What OS version ? VxWorks or UNIX.
    #endif
    
    tIPC_ID            	Que;
    pTMR_CB             cur;
    tIPC_PRIM			prim;
    tMBUF  				mbuf;
    int                 delta; /* Do not declare to be U32, since negative check needed */


    #if defined(VXW_VERSION)
    delaySem = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
    tickSet(0);
    #endif
  
    signal(SIGINT, (SIG_FUNCPTR)TMR_sigExit);
    for(;;){
        #ifdef UNIX_VERSION   
          p(tmr_semid);
        #elif defined(VXW_VERSION)    
          semTake(tmrExclSem, WAIT_FOREVER);
        #else 
          #error Compiler-Option! What OS version ? VxWorks or UNIX.
        #endif
  
        if (OSTMR_IsTmrListEmpty()){
            #ifdef UNIX_VERSION     
              v(tmr_semid);
              pause();
              p(tmr_semid);
              gettimeofday(&first, &temp);
              bgn_time = first.tv_usec;
            #elif defined(VXW_VERSION)
              semGive(tmrExclSem);
              taskSuspend(SELFTASK);
              bgn_time = tickGet(); /* time counting is beginning ... */
            #else 
              #error Compiler-Option! What OS version ? VxWorks or UNIX.
            #endif            
        }
        
        #if 0
        if (tmrObj.shm->tmrHdr->timeleft < 0){
            printf("timer> [?] *** ERROR *** timer(): tmr(%s)'s timeleft(%d) < 0\07\n",
            tmrObj.shm->tmrHdr->tmr_name,tmrObj.shm->tmrHdr->timeleft);
            tmrObj.shm->tmrHdr->timeleft = 0;
        }
        #endif
        
        #ifdef UNIX_VERSION        
          v(tmr_semid);
        #elif defined(VXW_VERSION)        
          semGive(tmrExclSem);
        #else 
          #error Compiler-Option! What OS version ? VxWorks or UNIX.
        #endif        
        
        /*
        * - We added delay() in 'for loop' to protect infinite loop when
        *   suspended task is resumed.
        *
        * - The worst case that the task can't be resumed when it is still
        *   in delay() is impossible to be happen because it is either case -
        *   either task is suspend(timer list=NULL) or delay(timer list!=NULL)
        *
        * - Can't just use delay(tmrHdr->timeleft) and hope everything works.
        *   This will result in the difficulty when the new timer want to be
        *   inserted into timer-list, since the real tmrHdr->timeleft is unknown
        *   at that time. New timer which will be added by other task can't measure
        *   the timeleft of tmrHdr controlled by the timer task.
        *   So, granulation of time slice is necessary.
        *
        *   e.g. The error code is :
        *   delay(tmrHdr->timeleft);
        *   execute(tmrHdr->handler);
        */
        #ifdef UNIX_VERSION
          usleep(SEC/2);
          gettimeofday(&second, &temp);
          end_time = second.tv_usec;
        #elif defined(VXW_VERSION)
          semTake(delaySem,SEC/2); 
          end_time = tickGet(); /* time counting is end. */
        #else 
          #error Compiler-Option! What OS version ? VxWorks or UNIX.
        #endif

        delta = (end_time - bgn_time);
        if (delta < 0)  delta = SEC/2; /* to protect delta from circular around number */

        if (delta > SEC){
            printf("*** ERROR *** tmr delta[%d] is too big!\07\n",delta);
            delta = SEC; /* default */
        }
        bgn_time = end_time; /* time counting is beginning ... */

        /* testing ... 
        p(tmr_semid);
        //OSTMR_PrintTmr(tmrObj.shm->tmrHdr);
        printf("%s> delta[%d],timeleft[%d]\n",
        tmrObj.shm->tmrHdr->tmr_name, delta, tmrObj.shm->tmrHdr->timeleft); 
        v(tmr_semid);
        */
        
        /* 
         * For example :
         *
         * delta=200
         *  ___     ___     ___ 
         * |200|-->| 0 |-->|100|-->|||
         *  ---     ---     ---
         */
        for(;;){
            #ifdef UNIX_VERSION         
              p(tmr_semid);
            #elif defined(VXW_VERSION)          
              semTake(tmrExclSem, WAIT_FOREVER);
            #else 
              #error Compiler-Option! What OS version ? VxWorks or UNIX.
            #endif
          
            if (OSTMR_IsTmrListEmpty() || tmrObj.shm->tmrHdr->timeleft > delta){
                #ifdef UNIX_VERSION       
                  v(tmr_semid);
                #elif defined(VXW_VERSION)                  
                  semGive(tmrExclSem);
                #else 
                  #error Compiler-Option! What OS version ? VxWorks or UNIX.
                #endif    
                break;
            }
      
            cur = tmrObj.shm->tmrHdr;
            tmrObj.shm->tmrHdr = tmrObj.shm->tmrHdr->next;
            delta -= cur->timeleft;
            if (delta < 0)  delta = 0; /* Added on 9/18/2000 */

            prim.type = IPC_EV_TYPE_TMR;
            prim.evt = cur->event;
            prim.ccb = cur->ccb;
            
            Que = cur->Que;
            OSTMR_FreeTmrCb(cur);
            
            #if defined(VXW_VERSION)            
              semGive(tmrExclSem); /* must release sema before sending mail */
              msgQSend(Que,(char*)&mail,sizeof(mail),WAIT_FOREVER,MSG_PRI_NORMAL);
            #elif defined(UNIX_VERSION)
              v(tmr_semid); 
              /*
              [method-1] important! v() must be ahead of SND_MSG(),
                otherwise msgq will be full because other task(s) will
                continueously start a new timer.
              [method-2] use usleep(10) to slow down inserting into queue
              */
              
              memcpy(mbuf.mtext,(U8*)&prim,sizeof(prim));
              mbuf.DA = IPC_DEF_USR;
              if (SND_MSGQT2(Que, &mbuf) == ERROR){
                  printf("os_tmr: SND_MSGQ error !!!\n");
              }
              
              #if 0
              {
                struct msqid_ds stat;
                msgctl(tmrQ, IPC_STAT, &stat);
                printf("qbytes=%d qnum=%d\n",stat.msg_qbytes, stat.msg_qnum);
              }
              #endif
            #else 
              #error Compiler-Option! What OS version ? VxWorks or UNIX.
            #endif          
        } /* for */

        /*
         * e.g. Init Delta=250; after "while", delta=250-200-0=50
         *                then /---<--tmrHdr(timeleft=100-50) after 'while'
         *  ___    ___    ___ /
         * |200|->| 0 |->|100|->||
         *  ---    ---    ---
         *
         * e.g. Init delta=150; after "while", delta=150-200=0
         *   then /<--tmrHdr(timeleft=200-150) after 'while'
         *       /
         *  ___/   ___    ___
         * |200|->| 0 |->|100|->||
         *  ---    ---    ---
         */
        #ifdef UNIX_VERSION         
          p(tmr_semid);
        #elif defined(VXW_VERSION)        
          semTake(tmrExclSem, WAIT_FOREVER);
        #else 
          #error Compiler-Option! What OS version ? VxWorks or UNIX.
        #endif          

        if (!OSTMR_IsTmrListEmpty())
            tmrObj.shm->tmrHdr->timeleft -= delta; /* left delta */
            
        #ifdef UNIX_VERSION            
          v(tmr_semid);
        #elif defined(VXW_VERSION)        
          semGive(tmrExclSem);
        #else 
          #error Compiler-Option! What OS version ? VxWorks or UNIX.
        #endif          
    } /* for */
}

/**********************************************************************
 * StartTmr : used for IGMP
 *
 * input   : Que - message queue
 *           ccb - who owns this timer. Could be ccb, lcb or
 *                 conn/pty/ilmi/link
 *           delay - new inserted timer in tick unit(ps: 100 ticks=1 sec)
 *           name - char string for test or mng
 *           event - response event while expiration.
 **********************************************************************/
void OSTMR_StartTmr(tIPC_ID Que, void *ccb, U32 delay, char *name, U16 event)
{
    pTMR_CB  tcb,cur,prev;

    if (!ccb)  return;
    
    OSTMR_StopXtmr(ccb, event);
    tcb = (pTMR_CB)OSTMR_MallocTmrCb();
    if (!tcb)  return; 
    
    tcb->Que = Que;
    tcb->ccb = ccb;
    tcb->timeleft = delay;
    strcpy(tcb->tmr_name,name);
    tcb->event = event;
    tcb->next = NULL;

    #ifdef UNIX_VERSION
      p(tmr_semid);
    #elif defined(VXW_VERSION)            
      semTake(tmrExclSem, WAIT_FOREVER);
    #endif

    if (tmrObj.shm->tmrHdr == NULL){
        tmrObj.shm->tmrHdr = tcb;
        
        #ifdef _PRN_TMR_LIST    
          OSTMR_PrintTmr(tcb);
        #endif    
    
        #ifdef UNIX_VERSION            
          v(tmr_semid);
        #elif defined(VXW_VERSION)        
          semGive(tmrExclSem);
        #endif              
    
        #ifdef UNIX_VERSION            
          kill(tmrTid,SIGUSR1);
        #elif defined(VXW_VERSION)        
          taskResume(tmrTid);
        #endif
        
        return;
    }
    else{
        for(prev=NULL,cur=tmrObj.shm->tmrHdr; cur; prev=cur,cur=cur->next){
            if (tcb->timeleft < cur->timeleft)  break;
            tcb->timeleft -= cur->timeleft;
        }
        tcb->next = cur;
        /* cur could be NULL or a time-node immediately behind tcb-node */

        if (prev) /* init prev must be NULL in above 'for' loop */
            prev->next = tcb; /* initial tcb->timeleft <= tmrHdr->timeleft */
        else
            tmrObj.shm->tmrHdr = tcb; /* initial tcb->timeleft > tmrHdr->timeleft */

        if (cur)
            cur->timeleft -= tcb->timeleft; /* to make total time unchanged */
            
        #ifdef _PRN_TMR_LIST    
        OSTMR_PrintTmr(tcb);
        #endif    
    
        #ifdef UNIX_VERSION            
          v(tmr_semid);
        #elif defined(VXW_VERSION)        
          semGive(tmrExclSem);
        #endif              
    }    
}

/*********************************************************************
 * StopTmrs()
 *
 * purpose : delete all timer(s) related to specific vcc in list
 * input   : ccb - conn/pty/ilmi/link
 **********************************************************************/
void  OSTMR_StopTmrs(void *ccb)
{
    pTMR_CB  prev,cur,tmp;

    #ifdef UNIX_VERSION
      p(tmr_semid);
    #elif defined(VXW_VERSION)            
      semTake(tmrExclSem, WAIT_FOREVER);
    #endif
    
    for(prev=NULL,cur=tmrObj.shm->tmrHdr; cur;){
        if (cur->ccb != ccb){
            prev = cur;
            cur = cur->next;
            continue;
        }

    	#ifdef _PRN_TMR_LIST
    	OSTMR_PrintTmr(cur);
    	#endif
    
    	if (prev){ /* delete middle/tail */
        	prev->next = cur->next;
    	}
    	else{ /* delete header */
        	tmrObj.shm->tmrHdr = cur->next;
    	}

    	if (cur->next){
        	cur->next->timeleft += cur->timeleft;
		}
		
        tmp = cur;
        cur = cur->next;
        
        OSTMR_FreeTmrCb(tmp); 
        //OSTMR_PrintTmr(NULL);
    } /* for */

    #ifdef UNIX_VERSION            
      v(tmr_semid);
    #elif defined(VXW_VERSION)        
      semGive(tmrExclSem);
    #endif                
}

/*********************************************************************
 * StopXtmr()
 *
 * purpose : delete all timer(s) related to specific RESTART handler in
 *           timer-list.
 *           Because global cr might invoked 2 timers (T316 & T317)
 *           simultaneously, we adopt this procdure while not use
 *           STOP_TMR() to discriminate between these 2 timers.
 *
 * input   : ccb - conn/pty/ilmi/link,
 *           handler - void (*handler)()
 * return  : time spent
 **********************************************************************/
void OSTMR_StopXtmr(void *ccb, U16 event)
{
    pTMR_CB  prev,cur;

    #ifdef UNIX_VERSION
      p(tmr_semid);
    #elif defined(VXW_VERSION)            
      semTake(tmrExclSem, WAIT_FOREVER);
    #endif
  
    for(prev=NULL,cur=tmrObj.shm->tmrHdr; cur; prev=cur,cur=cur->next)
        if (cur->ccb == ccb && event == cur->event)
            break;

    if (cur == NULL){
        #ifdef UNIX_VERSION
          v(tmr_semid);
        #elif defined(VXW_VERSION)
          semGive(tmrExclSem);
        #endif
        return;
    }

    /*----------------- have found --------------------*/
    /* timespent = now - cur->start_time; */

    #ifdef _PRN_TMR_LIST
    OSTMR_PrintTmr(cur);
    #endif

    if (prev) /* delete middle/tail */
        prev->next = cur->next;
    else{ /* delete header */
        tmrObj.shm->tmrHdr = cur->next;
    }
    
    /* test ...
    if (OSTMR_IsTmrListEmpty())  printf("stop_tmr(): Timer list is empty !!!\n");
    else  printf("stop_tmr(): Timer list is NOT empty !!!\n");
    */    

    if (cur->next)
        cur->next->timeleft += cur->timeleft;

    OSTMR_FreeTmrCb(cur); 
    //OSTMR_PrintTmr(NULL);
    
    #ifdef UNIX_VERSION            
      v(tmr_semid);
    #elif defined(VXW_VERSION)        
      semGive(tmrExclSem);
    #endif                  
}

/**********************************************************************
 * IsTmrListEmpty()
 *
 * purpose : check whether the timer-list is empty or not.
 * input   : ccb - conn/pty/ilmi/link rather than tmr ccb
 *           event - timeout event
 * return  : TRUE/FALSE
 **********************************************************************/
BOOL  OSTMR_IsTmrListEmpty()
{
    if (tmrObj.shm->tmrHdr == NULL)
        return TRUE;
    else
        return FALSE;
}
  
/**********************************************************************
 * IsTmrExist()
 *
 * purpose : check whether the specific timer in timer-list or not.
 * input   : ccb - conn/pty/ilmi/link rather than tmr ccb
 *           event - timeout event
 * return  : TRUE/FALSE
 **********************************************************************/
BOOL  OSTMR_IsTmrExist(void *ccb, U16 event)
{
    pTMR_CB  cur;

    #ifdef UNIX_VERSION
      p(tmr_semid);
    #elif defined(VXW_VERSION)            
      semTake(tmrExclSem, WAIT_FOREVER);
    #endif
  
    for(cur=tmrObj.shm->tmrHdr; cur; cur=cur->next){
        if (cur->ccb != ccb)
            continue;
            
        if (event == 0){ /* event is don't care */
            #ifdef UNIX_VERSION            
              v(tmr_semid);
            #elif defined(VXW_VERSION)        
              semGive(tmrExclSem);
            #endif                
            return TRUE;
        }
        
        if (event == cur->event){
            #ifdef UNIX_VERSION            
              v(tmr_semid);
            #elif defined(VXW_VERSION)        
              semGive(tmrExclSem);
            #endif                
            return TRUE;
        }
    }

    #ifdef UNIX_VERSION            
      v(tmr_semid);
    #elif defined(VXW_VERSION)        
      semGive(tmrExclSem);
    #endif                
  
    return FALSE;
}
  
/*--------------------------------------------------------------------
 * MallocTmrCb : 
 *
 * input  : 
 * return : 
 *-------------------------------------------------------------------*/
pTMR_CB  OSTMR_MallocTmrCb()
{
    pTMR_CB     pTcb;
    U16         i;
     
    #ifdef UNIX_VERSION    
      p(tmr_semid);
    #elif defined(VXW_VERSION)
      semTake(tmrExclSem, WAIT_FOREVER);
    #endif
    
    for(i=0; i<MAX_TMR_CB_NUM; i++){ 
        if (tmrObj.shm->tcb[i].idle){
            pTcb = &tmrObj.shm->tcb[i];
            tmrObj.shm->tcb[i].idle = FALSE;
            
            #ifdef UNIX_VERSION
              v(tmr_semid);
            #elif defined(VXW_VERSION)
              semGive(tmrExclSem);
            #endif
        
            return pTcb;
        }
    }
    printf("TMR_MALLOC(): not available memory!\n");
            
    #ifdef UNIX_VERSION
      v(tmr_semid);
    #elif defined(VXW_VERSION)
      semGive(tmrExclSem);
    #endif
        
    return NULL;
    
    /*
    static U16  ns=0;
    for(i=ns;;){ 
        if (tmrObj.shm->tcb[i].idle){
            pTcb = &tmrObj.shm->tcb[i];
            tmrObj.shm->tcb[i].idle = FALSE;
            ns = ++i;
            
            #ifdef UNIX_VERSION
        v(tmr_semid);
        #elif defined(VXW_VERSION)
        semGive(tmrExclSem);
        #endif
        
            return pTcb;
        }
        
        if (i == MAX_TMR_CB_NUM)
            i = 0;
            
        if (i == ns){
            printf("TMR_MALLOC(): not available memory!\n");
            
            #ifdef UNIX_VERSION
            v(tmr_semid);
            #elif defined(VXW_VERSION)
        semGive(tmrExclSem);
        #endif
        
        return NULL;
        }
    }*/
}

/*--------------------------------------------------------------------
 * FreeTmrCb() : 
 *
 * input  : p - tmr block
 * return : 
 *-------------------------------------------------------------------*/
void OSTMR_FreeTmrCb(pTMR_CB p)
{
    p->idle = TRUE;
    
    /*
  U16  i;
  
  for(i=0; i<MAX_TMR_CB_NUM; i++){
        if (p == &tmrObj.shm->tcb[i]){
            tmrObj.shm->tcb[i].idle = TRUE;
            return;
        }
    }
    printf("FREE_tcb(): can't find mem to free!\n");
    */
}

/*********************************************************************
 * PrintTmr(): 
 *     print out all timers for all L2TP tunnels
 *
 * input:
 *     s - comment e.g. "+"
 *     tmr - tmr ccb
 *********************************************************************/
void OSTMR_PrintTmr(pTMR_CB tmr)
{
    pTMR_CB cur;
  
    if (!tmrObj.shm->tmrHdr){
      	printf("[timer] null\n");
      	return;
    }
      
    for(cur=tmrObj.shm->tmrHdr; cur; cur=cur->next){
        printf("<%s,%d>",cur->tmr_name, cur->timeleft);
    }
}

#ifdef VXW_VERSION
/*--------------------------------------------------------------------
 * SYS_UP_CTIME() : calculate the total ticks since system starts up
 *
 * return : char string of total time elapses
 *-------------------------------------------------------------------*/
char *SYS_UP_CTIME()
{
    static char str[80];
    char        tmp[10];
    U64         totalTicks,secs,mins,hours;
    U16         days,hour,min,sec,tick;

    
    totalTicks = tickGet();
    
    tick = totalTicks % Hz;
    secs = totalTicks / Hz;

    sec = secs % 60;
    mins = secs / 60;

    min = mins % 60;
    hours = mins / 60;

    hour = hours % 24;
    days = hours / 24;

    if (days == 0)
        sprintf(str,"dd:");
    else
        sprintf(str,"%02d:",days);

    if (hour == 0 && days == 0)
        sprintf(tmp,"hh:");
    else
        sprintf(tmp,"%02d:",hour);
    strcat(str,tmp);

    if (min == 0 && hour == 0 && days == 0)
        sprintf(tmp,"mm:");
    else
        sprintf(tmp,"%02d:",min);
    strcat(str,tmp);

    sprintf(tmp,"%02d.%02d",sec,tick);
    strcat(str,tmp);

    /*
    sprintf(str,"%02d:%02d:%02d:%02d",days,hour,min,sec);
    */

    return str;
}
#endif

#ifdef UNIX_VERSION
/*--------------------------------------------------------------------
 * SYS_UP_CTIME() : calculate the total ticks since system starts up
 *
 * note   : ctime() defined in <time.h>
 *          This "C" function is located in "t.cpp", so if ".c" file
 *          wants to call this func., extern "C" must be included in "t.h"
 * return : char string(FULL format) of total time elapses
 *-------------------------------------------------------------------*/
char *SYS_UP_CTIME()
{
  long  timeval;
  
  time(&timeval);
  return ctime(&timeval);
}

/*--------------------------------------------------------------------
 * SYS_UP_TIME() : convert ticks to char string[dd:hh:mm:xx]
 *
 * input  : 
 * return : char string(DAY + TIME) after convertion.
 *-------------------------------------------------------------------*/
char *SYS_UP_TIME()
{
    static char sTime[80];
    char      *cp,sTm[80],sDay[10];
  
    cp = SYS_UP_CTIME();
    strcpy(sTime,cp);
    cp = sTime;
    GetStrTok(&cp," "); /* weekday */
    GetStrTok(&cp," "); /* month */
    strcpy(sDay,GetStrTok(&cp," ")); /* day */
    strcpy(sTm,GetStrTok(&cp," ")); /* time */
  
    if (strcmp(sDay,"") == 0)
        strcpy(sTime,"dd");
    else
        strcpy(sTime,sDay);
  
    strcat(sTime,":");
    strcat(sTime,sTm);
    return sTime;
}
#endif

/*--------------------------------------------------------------------
 * _2CTIME() : convert ticks to char string[dd:hh:mm:xx]
 *
 * input  : ticks
 * return : char string after convertion.
 *-------------------------------------------------------------------*/
char *_2CTIME(U64 ticks)
{
    static char str[80];
    char        tmp[10];
    U64         secs,mins,hours;
    U16         days,hour,min,sec;


    secs = ticks / HZ;

    sec = secs % 60;
    mins = secs / 60;

    min = mins % 60;
    hours = mins / 60;

    hour = hours % 24;
    days = hours / 24;

    if (days == 0)
        sprintf(str,"dd:");
    else
        sprintf(str,"%02d:",days);

    if (hour == 0 && days == 0)
        sprintf(tmp,"hh:");
    else
        sprintf(tmp,"%02d:",hour);
    strcat(str,tmp);

    if (min == 0 && hour == 0 && days == 0)
        sprintf(tmp,"mm:");
    else
        sprintf(tmp,"%02d:",min);
    strcat(str,tmp);

    sprintf(tmp,"%02d",sec);
    strcat(str,tmp);

    /*
    sprintf(str,"%02d:%02d:%02d:%02d",days,hour,min,sec);
    */

    return str;
}

/*--------------------------------------------------------------------
 * _SEC2CTIME() : convert ticks to char string[dd:hh:mm:xx]
 *
 * input  : ticks
 * return : char string after convertion.
 *-------------------------------------------------------------------*/
void _SEC2CTIME(U64 secs, char str[])
{
    char    tmp[10];
    U64     mins,hours;
    U16     days,hour,min,sec;
    
    sec = secs % 60;
    mins = secs / 60;

    min = mins % 60;
    hours = mins / 60;

    hour = hours % 24;
    days = hours / 24;

    if (days == 0)
        sprintf(str,"dd:");
    else
        sprintf(str,"%02d:",days);

    if (hour == 0 && days == 0)
        sprintf(tmp,"hh:");
    else
        sprintf(tmp,"%02d:",hour);
    strcat(str,tmp);

    if (min == 0 && hour == 0 && days == 0)
        sprintf(tmp,"mm:");
    else
        sprintf(tmp,"%02d:",min);
    strcat(str,tmp);

    sprintf(tmp,"%02d",sec);
    strcat(str,tmp);

    /*
    sprintf(str,"%02d:%02d:%02d:%02d",days,hour,min,sec);
    */
}

/*--------------------------------------------------------------------
 * _SEC2STR_TIME() : convert ticks to char string[dd:hh:mm:xx]
 *
 * input  : secs
 * return : char string after convertion.
 *-------------------------------------------------------------------*/
void _SEC2STR_TIME(U64 secs, char str[])
{
    char    tmp[10];
    U64     mins,hours;
    U16     days,hour,min,sec;
    
    sec = secs % 60;
    mins = secs / 60;

    min = mins % 60;
    hours = mins / 60;

    hour = hours % 24;
    days = hours / 24;

    if (days == 0){
        str[0] = 0;
    }
    else if (days == 1){
        sprintf(str,"%d day ",days);
    }
    else{
        sprintf(str,"%d days ",days);
    }

    if (hour == 0 && days == 0){
        tmp[0] = 0;
    }
    else if (hour == 0 || hour == 1){
        sprintf(tmp,"%d hour ",hour);
    }
    else{
        sprintf(tmp,"%d hours ",hour);
    }
    strcat(str,tmp);

    if (min == 0 && hour == 0 && days == 0){
        tmp[0] = 0;
    }
    else if (min == 0 || min == 1){
        sprintf(tmp,"%d min ",min);
    }
    else{
        sprintf(tmp,"%d mins ",min);
    }
    strcat(str,tmp);

    if (sec == 0 || sec == 1){
        sprintf(tmp,"%d sec",sec);
    }
    else{
        sprintf(tmp,"%d secs",sec);
    }
    strcat(str,tmp);
}
