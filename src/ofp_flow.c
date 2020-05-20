/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFP_FLOW.C :

  Designed by THE on MAY 20, 2020
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#include "ofp_flow.h"
#include "ofpd.h"

flow_table_t flow_table[OFP_FABLE_SIZE];
tuple_table_t tuple_table[OFP_FABLE_SIZE];

STATUS add_huge_flow_table(flowmod_info_t flowmod_info, uint8_t tuple_mask[])
{
    uint16_t new_index;// = (tuple_mask[0] * 1000 + tuple_mask[1] * 100 + tuple_mask[2] * 10 + tuple_mask[3]) % OFP_FABLE_SIZE;
    uint16_t flow_index;

    for(flow_index=0; flow_table[flow_index].is_exist==TRUE; flow_index++);
    //fill flow table
    for(int i=0;; i++) {
        if (tuple_table[i].is_exist == FALSE) {
            tuple_table[new_index].is_exist = TRUE;
            memcpy(tuple_table[new_index].match_bits, tuple_mask, 4);
            new_index = i;
            break;
        }
        if (BYTES_CMP(tuple_table[i].match_bits, tuple_mask, 4) == TRUE) {
            new_index = i;
            break;
        }
    }
    struct flow_entry_list **cur;
    for(cur=&(tuple_table[new_index].list); *cur!= NULL; cur=&(*cur)->next);
    struct flow_entry_list *new_node = (struct flow_entry_list *)malloc(sizeof(struct flow_entry_list));
    *cur = new_node;
    new_node->entry_id = flow_index;
    new_node->next = NULL;

    return TRUE;
}

void init_tables(void) {
    memset(flow_table, 0, sizeof(flow_table_t) * 20);
    memset(tuple_table, 0, sizeof(tuple_table_t) * 20);
}
