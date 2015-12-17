#ifndef PLATFORM_H
#define PLATFORM_H

#include "os_core.h"

#define IF_NAME "wlan0"

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
#ifndef ETH_HLEN
#define ETH_HLEN 14
#endif
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
#ifndef ETH_P_ALL
#define ETH_P_ALL 0x0003
#endif



typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;


#ifdef __CHECKER__
#define __force __attribute__((force))
#define __bitwise __attribute__((bitwise))
#else
#define __force
#define __bitwise
#endif

typedef u16 __bitwise be16;
typedef u16 __bitwise le16;
typedef u32 __bitwise be32;
typedef u32 __bitwise le32;
typedef u64 __bitwise be64;
typedef u64 __bitwise le64;

#ifdef __LITTLE_ENDIAN
#define le_to_host16(n) ((__force u16) (le16) (n))
#define host_to_le16(n) ((__force le16) (u16) (n))
#define be_to_host16(n) bswap_16((__force u16) (be16) (n))
#define host_to_be16(n) ((__force be16) bswap_16((n)))
#define le_to_host32(n) ((__force u32) (le32) (n))
#define host_to_le32(n) ((__force le32) (u32) (n))
#define be_to_host32(n) bswap_32((__force u32) (be32) (n))
#define host_to_be32(n) ((__force be32) bswap_32((n)))
#define le_to_host64(n) ((__force u64) (le64) (n))
#define host_to_le64(n) ((__force le64) (u64) (n))
#define be_to_host64(n) bswap_64((__force u64) (be64) (n))
#define host_to_be64(n) ((__force be64) bswap_64((n)))
#else 
#define le_to_host16(n) bswap_16(n)
#define host_to_le16(n) bswap_16(n)
#define be_to_host16(n) (n)
#define host_to_be16(n) (n)
#define le_to_host32(n) bswap_32(n)
#define host_to_le32(n) bswap_32(n)
#define be_to_host32(n) (n)
#define host_to_be32(n) (n)
#define le_to_host64(n) bswap_64(n)
#define host_to_le64(n) bswap_64(n)
#define be_to_host64(n) (n)
#define host_to_be64(n) (n)
#endif



/* Macros for handling unaligned memory accesses */

static inline u16 WPA_GET_BE16(const u8 *a)
{
	return (a[0] << 8) | a[1];
}

static inline void WPA_PUT_BE16(u8 *a, u16 val)
{
	a[0] = val >> 8;
	a[1] = val & 0xff;
}

static inline u16 WPA_GET_LE16(const u8 *a)
{
	return (a[1] << 8) | a[0];
}

static inline void WPA_PUT_LE16(u8 *a, u16 val)
{
	a[1] = val >> 8;
	a[0] = val & 0xff;
}

static inline u32 WPA_GET_BE24(const u8 *a)
{
	return (a[0] << 16) | (a[1] << 8) | a[2];
}

static inline void WPA_PUT_BE24(u8 *a, u32 val)
{
	a[0] = (val >> 16) & 0xff;
	a[1] = (val >> 8) & 0xff;
	a[2] = val & 0xff;
}

static inline u32 WPA_GET_BE32(const u8 *a)
{
	return (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
}

static inline void WPA_PUT_BE32(u8 *a, u32 val)
{
	a[0] = (val >> 24) & 0xff;
	a[1] = (val >> 16) & 0xff;
	a[2] = (val >> 8) & 0xff;
	a[3] = val & 0xff;
}

static inline u32 WPA_GET_LE32(const u8 *a)
{
	return (a[3] << 24) | (a[2] << 16) | (a[1] << 8) | a[0];
}

static inline void WPA_PUT_LE32(u8 *a, u32 val)
{
	a[3] = (val >> 24) & 0xff;
	a[2] = (val >> 16) & 0xff;
	a[1] = (val >> 8) & 0xff;
	a[0] = val & 0xff;
}

static inline u64 WPA_GET_BE64(const u8 *a)
{
	return (((u64) a[0]) << 56) | (((u64) a[1]) << 48) |
		(((u64) a[2]) << 40) | (((u64) a[3]) << 32) |
		(((u64) a[4]) << 24) | (((u64) a[5]) << 16) |
		(((u64) a[6]) << 8) | ((u64) a[7]);
}

static inline void WPA_PUT_BE64(u8 *a, u64 val)
{
	a[0] = val >> 56;
	a[1] = val >> 48;
	a[2] = val >> 40;
	a[3] = val >> 32;
	a[4] = val >> 24;
	a[5] = val >> 16;
	a[6] = val >> 8;
	a[7] = val & 0xff;
}

static inline u64 WPA_GET_LE64(const u8 *a)
{
	return (((u64) a[7]) << 56) | (((u64) a[6]) << 48) |
		(((u64) a[5]) << 40) | (((u64) a[4]) << 32) |
		(((u64) a[3]) << 24) | (((u64) a[2]) << 16) |
		(((u64) a[1]) << 8) | ((u64) a[0]);
}

static inline void WPA_PUT_LE64(u8 *a, u64 val)
{
	a[7] = val >> 56;
	a[6] = val >> 48;
	a[5] = val >> 40;
	a[4] = val >> 32;
	a[3] = val >> 24;
	a[2] = val >> 16;
	a[1] = val >> 8;
	a[0] = val & 0xff;
}

#define le16_to_cpu		le_to_host16
#define le32_to_cpu		le_to_host32

#define get_unaligned(p)					\
({								\
	struct packed_dummy_struct {				\
		typeof(*(p)) __val;				\
	} __attribute__((packed)) *__ptr = (void *) (p);	\
								\
	__ptr->__val;						\
})
#define get_unaligned_le16(p)	le16_to_cpu(get_unaligned((uint16_t *)(p)))
#define get_unaligned_le32(p)	le32_to_cpu(get_unaligned((uint32_t *)(p)))

#endif /* PLATFORM_H */
