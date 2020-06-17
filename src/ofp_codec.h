/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  OFP_CODEC.H

  Designed by THE on SEP 16, 2019
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _OFP_CODEC_H_
#define _OFP_CODEC_H_

#include <ip_codec.h>

/*----------------------------------------------------------
 * The subtype structure of TLV
 *---------------------------------------------------------*/
struct _OFP_TLV {
    U16		len; //=subt+value
	void	*vp; //value pointer
};

typedef struct port_status {
    ofp_port_status_t ofp_port_status;
    struct port_status *next;
}port_status_t;

typedef struct _OFP_TLV	tOFP_TLV; 
typedef U16  *(*DEC_FUNCPTR)(U8*, tOFP_TLV*);
typedef U16  *(*ENC_FUNCPTR)(tOFP_TLV*, U8*);
 
extern  STATUS	OFP_decode_frame(tOFP_MBX *mail, tOFP_PORT *port_ccb);

#endif
