/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  UTIL.H
    the common utilities of all files are saved in this file.
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

extern char *GetStrTok(char **cpp, char *delimiters);
extern int  get_local_ip(U8 *ip, char *sif);
extern int  set_local_ip(char *ip_str, char *sif);

extern U8	*DECODE_U16(U16 *val, U8 *mp);
extern U8	*DECODE_U24(U32 *val, U8 *mp);
extern U8	*DECODE_U32(U32 *val, U8 *mp);

extern U8	*ENCODE_U16(U8 *mp, U16 val);
extern U8	*ENCODE_U24(U8 *mp, U32 val);
extern U8	*ENCODE_U32(U8 *mp, U32 val);

extern void PRINT_MESSAGE(unsigned char*, int);
extern int ethernet_interface(const char *const name, int *const index, int *const speed, int *const duplex);
extern U16 	ADD_CARRY_FOR_CHKSUM(U32 sum);
extern U16 	CHECK_SUM(U32);
extern STATUS BYTES_CMP(U8 *var1, U8 *var2, U32 len);
extern int32_t hash_func(unsigned char *key, size_t len);
extern int abs_int32(int *x);

#ifndef   UINT64_C
#define   UINT64_C(value) __CONCAT(value,ULL)
#endif

#ifdef __cplusplus
}
#endif

#endif
