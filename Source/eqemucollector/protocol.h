#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "../common/types.h"

typedef int8 ETHERNET_ADDRESS [6];

typedef struct ETHERNET_FRAME {
	ETHERNET_ADDRESS	Destination;
	ETHERNET_ADDRESS	Source;
	int16				FrameType;			// in host-order
} ETHERNET_FRAME;

#define	ETHERNET_FRAME_TYPE_IP		0x0800

typedef struct IP_HEADER {
    int8	x;
    int8	tos;
    int16	length;
    int16	identifier;
    int16	fragment;
    int8	ttl;
    int8	protocol;
    int16	cksum;
    int32	src;
    int32	dest;
} IP_HEADER;

#define	IP_HEADER_MINIMUM_LEN	20

typedef struct UDP_HEADER {
	int16	src_port;
	int16	dest_port;
	int16	length;			// including this header
	int16	checksum;
} UDP_HEADER;

#define	UDP_HEADER_LEN			8

#endif

