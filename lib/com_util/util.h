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

#define RTE_STATIC_BSWAP64(v) \
	((((uint64_t)(v) & UINT64_C(0x00000000000000ff)) << 56) | \
	 (((uint64_t)(v) & UINT64_C(0x000000000000ff00)) << 40) | \
	 (((uint64_t)(v) & UINT64_C(0x0000000000ff0000)) << 24) | \
	 (((uint64_t)(v) & UINT64_C(0x00000000ff000000)) <<  8) | \
	 (((uint64_t)(v) & UINT64_C(0x000000ff00000000)) >>  8) | \
	 (((uint64_t)(v) & UINT64_C(0x0000ff0000000000)) >> 24) | \
	 (((uint64_t)(v) & UINT64_C(0x00ff000000000000)) >> 40) | \
	 (((uint64_t)(v) & UINT64_C(0xff00000000000000)) >> 56))

static inline uint64_t rte_constant_bswap64(uint64_t x)
{
	return (uint64_t)RTE_STATIC_BSWAP64(x);
}

#ifdef __x86_64__

static inline uint64_t rte_arch_bswap64(uint64_t _x)
{
	  uint64_t x = _x;
	  __asm__ volatile ("bswap %[x]"
		        : [x] "+r" (x)
		        );
	  return x;
}

#define bitswap64(x) ((uint64_t)(__builtin_constant_p(x) ?		\
				   rte_constant_bswap64(x) :		\
				   rte_arch_bswap64(x)))
#else

#define bitswap64(x) ((uint64_t)(rte_constant_bswap64(x)))

#endif

#ifdef __cplusplus
}
#endif

#endif
