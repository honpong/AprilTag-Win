#ifndef __BYTEORDER_H__
#define __BYTEORDER_H__

#include <endian.h>
#include <byteswap.h>
#if BYTE_ORDER == BIG_ENDIAN
#define from_le16(x) bswap_16(x);
#define   to_le16(x) bswap_16(x);
#define from_le32(x) bswap_32(x);
#define   to_le32(x) bswap_32(x);
#define from_be16(x) (x)
#define   to_be16(x) (x)
#define from_be32(x) (x)
#define   to_be32(x) (x)
#elif BYTE_ORDER == LITTLE_ENDIAN
#define from_le16(x) (x)
#define   to_le16(x) (x)
#define from_be16(x) bswap_16(x)
#define   to_be16(x) bswap_16(x)
#define from_le32(x) (x)
#define   to_le32(x) (x)
#define from_be32(x) bswap_32(x)
#define   to_be32(x) bswap_32(x)
#else
#error what endian are we?
#endif

#endif//__BYTEORDER_H__
