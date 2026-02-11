#ifndef PLATFORM_H
#define PLATFORM_H

#include <cinttypes>
#include <string>

// #define PACK( __Declaration__ ) __Declaration__
#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#else
#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#else
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif
#endif

#ifdef ESP_PLATFORM
    #include <arpa/inet.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
    #include <WinSock2.h>
#endif

#if defined(_MSC_VER)
    #define SWAP_BYTES_2(x) htons(x)
    #define SWAP_BYTES_4(x) htonl(x)
    #define SWAP_BYTES_8(x) htonll(x)
#else
#if defined(__MINGW32__)
        #define SWAP_BYTES_2(x) htons(x)
        #define SWAP_BYTES_4(x) htonl(x)
        #define SWAP_BYTES_8(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
    #else
        #ifdef ESP_PLATFORM
            #define SWAP_BYTES_2(x) lwip_htons(x)
            #define SWAP_BYTES_4(x) lwip_htonl(x)
            #define SWAP_BYTES_8(x) ((((uint64_t) lwip_htonl(x)) << 32) + lwip_htonl((uint64_t)(x) >> 32))
        #else
            #define SWAP_BYTES_2(x) be16toh(x)
            #define SWAP_BYTES_4(x) be32toh(x)
            #define SWAP_BYTES_8(x) be64toh(x)
        #endif
    #endif
#endif

#if defined(_MSC_VER)
// @see https://stackoverflow.com/questions/2100331/macro-definition-to-determine-big-endian-or-little-endian-machine
    #define LITTLE_ENDIAN 0x41424344UL
    #define BIG_ENDIAN    0x44434241UL
    #define PDP_ENDIAN    0x42414443UL
    #define ENDIAN_ORDER  ('ABCD')
    #define IS_BIG_ENDIAN (ENDIAN_ORDER == BIG_ENDIAN)
#else
    #define IS_BIG_ENDIAN BYTE_ORDER == BIG_ENDIAN
#endif

#if IS_BIG_ENDIAN
#define NTOH2(x) (x)
    #define NTOH4(x) (x)
    #define NTOH8(x) (x)
    #define HTON2(x) (x)
    #define HTON4(x) (x)
    #define HTON8(x) (x)
#else
    #define NTOH2(x) SWAP_BYTES_2(x)
    #define NTOH4(x) SWAP_BYTES_4(x)
    #define NTOH8(x) SWAP_BYTES_8(x)
    #define HTON2(x) SWAP_BYTES_2(x)
    #define HTON4(x) SWAP_BYTES_4(x)
    #define HTON8(x) SWAP_BYTES_8(x)
#endif

#endif
