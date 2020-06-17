#include "mailbox.h"

/*********************************************************
 * mailbox:
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
STATUS mailbox(struct rte_ring *ring, U8 *mail, uint16_t n)
{
    return rte_ring_enqueue_burst(ring, (void **)&mail, n, NULL)==0 ? FALSE : TRUE;
}