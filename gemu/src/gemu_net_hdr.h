#ifndef __GEMU_NET_HDR_H__
#define __GEMU_NET_HDR_H__

#include <stdint.h>
#include <stdio.h>

#define ETHER_ADDR_LEN  6 /**< Length of Ethernet address. */
#define ETHER_TYPE_LEN  2 /**< Length of Ethernet type field. */
#define ETHER_CRC_LEN   4 /**< Length of Ethernet CRC. */
#define ETHER_HDR_LEN   \
	(ETHER_ADDR_LEN * 2 + ETHER_TYPE_LEN) /**< Length of Ethernet header. */
#define ETHER_MIN_LEN   64    /**< Minimum frame len, including CRC. */
#define ETHER_MAX_LEN   1518  /**< Maximum frame len, including CRC. */
#define ETHER_MTU       \
	(ETHER_MAX_LEN - ETHER_HDR_LEN - ETHER_CRC_LEN) /**< Ethernet MTU. */

#define ETHER_MAX_VLAN_FRAME_LEN \
	(ETHER_MAX_LEN + 4) /**< Maximum VLAN frame length, including CRC. */

#define ETHER_MAX_JUMBO_FRAME_LEN \
	0x3F00 /**< Maximum Jumbo frame length, including CRC. */

#define ETHER_MAX_VLAN_ID  4095 /**< Maximum VLAN ID. */

#define ETHER_MIN_MTU 68 /**< Minimum MTU for IPv4 packets, see RFC 791. */

struct ether_addr {
	uint8_t addr_bytes[ETHER_ADDR_LEN]; /**< Addr bytes in tx order */
} __attribute__((__packed__));

#define ETHER_LOCAL_ADMIN_ADDR 0x02 /**< Locally assigned Eth. address. */
#define ETHER_GROUP_ADDR       0x01 /**< Multicast or broadcast Eth. address. */

static inline int is_same_ether_addr(const struct ether_addr *ea1,
				     const struct ether_addr *ea2)
{
	int i;
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		if (ea1->addr_bytes[i] != ea2->addr_bytes[i])
			return 0;
	return 1;
}

static inline int is_zero_ether_addr(const struct ether_addr *ea)
{
	int i;
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		if (ea->addr_bytes[i] != 0x00)
			return 0;
	return 1;
}

static inline int is_unicast_ether_addr(const struct ether_addr *ea)
{
	return (ea->addr_bytes[0] & ETHER_GROUP_ADDR) == 0;
}

static inline int is_multicast_ether_addr(const struct ether_addr *ea)
{
	return ea->addr_bytes[0] & ETHER_GROUP_ADDR;
}

static inline int is_broadcast_ether_addr(const struct ether_addr *ea)
{
    u8 bcast_addr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    return (!memcmp(ea, bcast_addr, sizeof(bcast_addr)));
#if 0
	//const unaligned_uint16_t *ea_words = (const unaligned_uint16_t *)ea;
	const uint16_t *ea_words = (const uint16_t *)ea;

	return (ea_words[0] == 0xFFFF && ea_words[1] == 0xFFFF &&
		ea_words[2] == 0xFFFF);
#endif
}

static inline int is_universal_ether_addr(const struct ether_addr *ea)
{
	return (ea->addr_bytes[0] & ETHER_LOCAL_ADMIN_ADDR) == 0;
}

static inline int is_local_admin_ether_addr(const struct ether_addr *ea)
{
	return (ea->addr_bytes[0] & ETHER_LOCAL_ADMIN_ADDR) != 0;
}

static inline int is_valid_assigned_ether_addr(const struct ether_addr *ea)
{
	return is_unicast_ether_addr(ea) && (!is_zero_ether_addr(ea));
}

static inline void ether_addr_copy(const struct ether_addr *ea_from,
				   struct ether_addr *ea_to)
{
	*ea_to = *ea_from;
}

#define ETHER_ADDR_FMT_SIZE         18
static inline void
ether_format_addr(char *buf, uint16_t size,
		  const struct ether_addr *eth_addr)
{
	snprintf(buf, size, "%02X:%02X:%02X:%02X:%02X:%02X",
		 eth_addr->addr_bytes[0],
		 eth_addr->addr_bytes[1],
		 eth_addr->addr_bytes[2],
		 eth_addr->addr_bytes[3],
		 eth_addr->addr_bytes[4],
		 eth_addr->addr_bytes[5]);
}

typedef struct ether_hdr {
	struct ether_addr d_addr; /**< Destination address. */
	struct ether_addr s_addr; /**< Source address. */
	uint16_t ether_type;      /**< Frame type. */
} __attribute__((__packed__)) ether_hdr;

SIZE_ASSERT(ether_hdr, 14);

struct vlan_hdr {
	uint16_t vlan_tci; /**< Priority (3) + CFI (1) + Identifier Code (12) */
	uint16_t eth_proto;/**< Ethernet type of encapsulated frame. */
} __attribute__((__packed__));


/* Ethernet frame types */
#define ETHER_TYPE_IPv4 0x0800 /**< IPv4 Protocol. */
#define ETHER_TYPE_IPv6 0x86DD /**< IPv6 Protocol. */
#define ETHER_TYPE_ARP  0x0806 /**< Arp Protocol. */
#define ETHER_TYPE_RARP 0x8035 /**< Reverse Arp Protocol. */
#define ETHER_TYPE_VLAN 0x8100 /**< IEEE 802.1Q VLAN tagging. */
#define ETHER_TYPE_QINQ 0x88A8 /**< IEEE 802.1ad QinQ tagging. */
#define ETHER_TYPE_1588 0x88F7 /**< IEEE 802.1AS 1588 Precise Time Protocol. */
#define ETHER_TYPE_SLOW 0x8809 /**< Slow protocols (LACP and Marker). */
#define ETHER_TYPE_TEB  0x6558 /**< Transparent Ethernet Bridging. */


typedef struct arp_ipv4 {
	struct ether_addr arp_sha;  /**< sender hardware address */
	uint32_t          arp_sip;  /**< sender IP address */
	struct ether_addr arp_tha;  /**< target hardware address */
	uint32_t          arp_tip;  /**< target IP address */
} __attribute__((__packed__)) arp_ipv4;

typedef struct arp_hdr {
	uint16_t arp_hrd;    /* format of hardware address */
#define ARP_HRD_ETHER     1  /* ARP Ethernet address format */

	uint16_t arp_pro;    /* format of protocol address */
	uint8_t  arp_hln;    /* length of hardware address */
	uint8_t  arp_pln;    /* length of protocol address */
	uint16_t arp_op;     /* ARP opcode (command) */
#define	ARP_OP_REQUEST    1 /* request to resolve address */
#define	ARP_OP_REPLY      2 /* response to previous request */
#define	ARP_OP_REVREQUEST 3 /* request proto addr given hardware */
#define	ARP_OP_REVREPLY   4 /* response giving protocol address */
#define	ARP_OP_INVREQUEST 8 /* request to identify peer */
#define	ARP_OP_INVREPLY   9 /* response identifying peer */

	struct arp_ipv4 arp_data;
} __attribute__((__packed__)) arp_hdr;


typedef struct ip4_hdr {
	uint8_t  version_ihl;		/**< version and header length */
	uint8_t  type_of_service;	/**< type of service */
	uint16_t total_length;		/**< length of packet */
	uint16_t packet_id;		/**< packet ID */
	uint16_t fragment_offset;	/**< fragmentation offset */
	uint8_t  time_to_live;		/**< time to live */
	uint8_t  next_proto_id;		/**< protocol ID */
	uint16_t hdr_checksum;		/**< header checksum */
	uint32_t src_addr;		/**< source address */
	uint32_t dst_addr;		/**< destination address */
} __attribute__((__packed__)) ip4_hdr;

SIZE_ASSERT(ip4_hdr, 20);

typedef struct icmp_hdr {
	uint8_t  icmp_type;   /* ICMP packet type. */
	uint8_t  icmp_code;   /* ICMP packet code. */
	uint16_t icmp_cksum;  /* ICMP packet checksum. */
	uint16_t icmp_ident;  /* ICMP packet identifier. */
	uint16_t icmp_seq_nb; /* ICMP packet sequence number. */
} __attribute__((__packed__)) icmp_hdr ;

SIZE_ASSERT(icmp_hdr, 8);

#define IP_ICMP_ECHO_REPLY   0
#define IP_ICMP_ECHO_REQUEST 8

#endif // __GEMU_NET_HDR_H__
