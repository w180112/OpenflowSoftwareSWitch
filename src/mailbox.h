/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
   MAILBOX.H
 
 *\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include <common.h>
#include <rte_ring.h>

extern STATUS mailbox(struct rte_ring *ring, U8 *mail, uint16_t n);

#endif 