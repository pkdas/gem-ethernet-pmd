#ifndef __GEMU_H__
#define __GEMU_H__

#include <assert.h>
#include "gemu_log.h"
#include "gemu_types.h"
#include "gemu_bd.h"
#include "gemu_mm.h"

struct bufmem_stack;

// PK FIXME remove these
#ifndef TRUE
#  define TRUE        1U
#endif

#ifndef FALSE
#  define FALSE        0U
#endif

#ifndef NULL
#define NULL        0U
#endif
//-------------

typedef enum {
    GEMU_SUCCESS=0,
    GEMU_FAILURE
} gemu_status;

typedef enum {
    GEM_DEVICE_0 = 0,
    GEM_DEVICE_1,
    GEM_MAX_DEVICES
} GEM_DEVICE_NUMS;

#define ZYNQMP_GEM_0_BASEADDR 0xFF0B0000
#define ZYNQMP_GEM_1_BASEADDR 0xFF0C0000
#define ZYNQMP_GEM_2_BASEADDR 0xFF0D0000
#define ZYNQMP_GEM_3_BASEADDR 0xFF0E0000

#define GEM_DEV_MAP_SIZE 4096
#define GEM_DEV_MAP_MASK (GEM_DEV_MAP_SIZE - 1)
#define GEM_PROMISC_OPTION               0x00000001U
#define GEM_FRAME1536_OPTION             0x00000002U
#define GEM_VLAN_OPTION                  0x00000004U
#define GEM_FLOW_CONTROL_OPTION          0x00000010U
#define GEM_FCS_STRIP_OPTION             0x00000020U
#define GEM_FCS_INSERT_OPTION            0x00000040U
#define GEM_LENTYPE_ERR_OPTION           0x00000080U
#define GEM_TRANSMITTER_ENABLE_OPTION    0x00000100U
#define GEM_RECEIVER_ENABLE_OPTION       0x00000200U
#define GEM_BROADCAST_OPTION             0x00000400U
#define GEM_MULTICAST_OPTION             0x00000800U
#define GEM_RX_CHKSUM_ENABLE_OPTION      0x00001000U
#define GEM_TX_CHKSUM_ENABLE_OPTION      0x00002000U
#define GEM_JUMBO_ENABLE_OPTION          0x00004000U
#define GEM_SGMII_ENABLE_OPTION          0x00008000U

#define GEM_DEFAULT_OPTIONS                     \
    ((u32)GEM_FLOW_CONTROL_OPTION |                  \
     (u32)GEM_FCS_INSERT_OPTION |                    \
     (u32)GEM_FCS_STRIP_OPTION |                     \
     (u32)GEM_BROADCAST_OPTION |                     \
     (u32)GEM_LENTYPE_ERR_OPTION |                   \
     (u32)GEM_TRANSMITTER_ENABLE_OPTION |            \
     (u32)GEM_RECEIVER_ENABLE_OPTION |               \
     (u32)GEM_RX_CHKSUM_ENABLE_OPTION |              \
     (u32)GEM_TX_CHKSUM_ENABLE_OPTION)

#define GEM_MTU             1500U    /* max MTU size of Ethernet frame */

// PK FIXME enable jumbo but use 1500 MTU for now 
// #define GEM_MTU_JUMBO    10240U    /* max MTU size of jumbo frame */
#define GEM_MTU_JUMBO       1500U    /* max MTU size of jumbo frame */
#define GEM_HDR_SIZE        14U    /* size of Ethernet header */
#define GEM_HDR_VLAN_SIZE   18U    /* size of Ethernet header with VLAN */
#define GEM_TRL_SIZE        4U    /* size of Ethernet trailer (FCS) */
#define GEM_MAX_FRAME_SIZE       (GEM_MTU + GEM_HDR_SIZE + GEM_TRL_SIZE)
#define GEM_MAX_VLAN_FRAME_SIZE  (GEM_MTU + GEM_HDR_SIZE +  GEM_HDR_VLAN_SIZE + \
                                       GEM_TRL_SIZE)
#define GEM_MAX_VLAN_FRAME_SIZE_JUMBO  (GEM_MTU_JUMBO + GEM_HDR_SIZE + \
                                        GEM_HDR_VLAN_SIZE + GEM_TRL_SIZE)

/* DMACR Bust length hash defines */
#define GEM_SINGLE_BURST    0x00000001
#define GEM_4BYTE_BURST     0x00000004
#define GEM_8BYTE_BURST     0x00000008
#define GEM_16BYTE_BURST    0x00000010

typedef struct gemu_config{
    char name[64];    
    char ifname[64];
    int device_id; 
    UINTPTR base_addr;
    u8 mac_addr[6];
    u32 rx_bd_count;
    u32 tx_bd_count;
    u8 ip4_addr[4];
    u8 next_hop_mac[6];
} gemu_config;

// 32 bit stats only for now 
typedef struct gemu_stats
{
    u32 rx_pi_updated;
    u32 rx_pi_wrap;
    u32 rx_pi_update_q_full;
    u32 rx_pi_no_update_q_full;
    u32 rx_pi_update_max;
    u32 rx_pi_single_read;
    u32 rx_pi_burst_read;
    u32 rx_ci_wrap;
    u32 rx_ci_updated;
    u32 rx_ci_all;
    u32 rx_pi_count;
    u32 max_rx_q_len;
    u32 rx_q_len_16;

    u32 tx_bd_poll;
    u32 tx_ci_count;
    u32 tx_ci_single_complete;
    u32 tx_ci_burst_complete;
    u32 tx_q_len_max;
    u32 tx_ci_updated;
    u32 tx_ci_updated_all;
    u32 tx_err;
    u32 tx_pkts;
    u32 tx_start;
    u32 tx_queue_full;
    u32 tx_status_go;
    u32 tx_status_complete;
    u32 tx_status_err;
    u32 tx_added;
    u32 fwded_tx_added;
    u32 tx_complete;
    u32 max_tx_q_len;
    u32 tx_q_len_16;

    u32 bufs_popped_rx_refill;
    u32 bufs_pushed_tx_complete;
    u32 bufs_pushed_fwd_complete; 
    u32 bufs_pushed_tossed_arp_pkts; 
    u32 bufs_pushed_tossed_ip_pkts; 
    u32 bufs_pushed_tossed_non_ip_pkts; 

    u32 rx_pkts_tossed;
    u32 tcp_pkts_rxed;
    u32 udp_pkts_rxed;
    u32 icmp_pkts_rxed;

    u32 ip_pkts_fwded;
    u32 tcp_pkts_fwded;
    u32 udp_pkts_fwded;
    u32 icmp_echo_req_fwded;
    u32 icmp_echo_reply_fwded;

    u32 rx_arp_pkts;
    u32 rx_ip_pkts;
    u32 rx_nonip_pkts;
    u32 rx_bufs_freed;

    u32 icmp_echo_requests;
    u32 icmp_echo_replies;
    u32 arp_req_recvd;
    u32 arp_req_host_ip_recvd;
    u32 arp_reply_sent;

} gemu_stats;

typedef struct gemu {
    int started; 
    gemu_config config;    

    volatile gemu_desc *rx_bd_mem;
    volatile u32 *rx_bd_mem_u32;
    UINTPTR phys_rx_bd_mem;
    u32 rx_bd_memsize;
    u32 rx_bd_count; 

    volatile gemu_desc *tx_bd_mem;
    volatile u32 *tx_bd_mem_u32;
    UINTPTR phys_tx_bd_mem;
    u32 tx_bd_memsize;
    u32 tx_bd_count; 

    u32 tx_pkt_id;

    u64 bufmem_start;
    u64 bufmem_end;
    u64 bufmem_size;
    u32 num_bufs;
    u32 buf_size;
    bufmem_stack bufmem_stack;

    gemu_hbd *rx_hbds;
    gemu_hbd *tx_hbds;

    u32    rx_pi;
    u32    rx_ci;
    u32    rx_prev_pi;
    u32    rx_prev_ci;

    u32    tx_pi;
    u32    tx_ci;
    u32    tx_prev_pi;
    u32    tx_prev_ci;

    gemu_buf *rx_vector[GEMU_RX_VECTOR_SIZE];
    gemu_buf *tx_vector[GEMU_TX_VECTOR_SIZE];
    u32     rx_vector_size;
    u32     tx_vector_size;

    u8      peer_mac[6];
    u8      my_mac[6];
    u32     addr_learnt;
 
    u32 version;
    u32 rx_buf_mask;
    u32 max_mtu_size;
    u32 max_frame_size;
    u32 max_vlan_frame_size;
    u32 max_queues;

    // stats
    gemu_stats stats;
} gemu;


#define gemu_int_enable(gemu, mask)                            \
    gemu_write_reg((gemu)->config.base_addr,             \
             GEM_IER_OFFSET,                                     \
             ((mask) & GEM_IXR_ALL_MASK));

#define gemu_int_disable(gemu, mask)                           \
    gemu_write_reg((gemu)->config.base_addr,             \
             GEM_IDR_OFFSET,                                     \
             ((mask) & GEM_IXR_ALL_MASK));

#define gemu_int_qi_enable(gemu, queue, mask)                       \
    gemu_write_reg((gemu)->config.base_addr,             \
             gemu_get_qx_offset(INTQI_IER, queue),        \
             ((mask) & GEM_INTQ_IXR_ALL_MASK));

#define gemu_int_qi_disable(gemu, queue, mask)                      \
    gemu_write_reg((gemu)->config.base_addr,             \
             gemu_get_qx_offset(INTQI_IDR, queue),        \
             ((mask) & GEM_INTQ_IXR_ALL_MASK));

#define gemu_transmit(gemu)                              \
    gemu_write_reg((gemu)->config.base_addr,          \
             GEM_NWCTRL_OFFSET,                                     \
             (gemu_read_reg((gemu)->config.base_addr,          \
                      GEM_NWCTRL_OFFSET) | GEM_NWCTRL_STARTTX_MASK))

#define gemu_is_rx_csum(gemu)                                     \
    ((gemu_read_reg((gemu)->config.base_addr,             \
              GEM_NWCFG_OFFSET) & GEM_NWCFG_RXCHKSUMEN_MASK) != 0U     \
     ? TRUE : FALSE)

#define gemu_is_tx_csum(gemu)                                     \
    ((gemu_read_reg((gemu)->config.base_addr,              \
              GEM_DMACR_OFFSET) & GEM_DMACR_TCPCKSUM_MASK) != 0U       \
     ? TRUE : FALSE)


#define gemu_set_rx_watermark(gemu, High, Low)                     \
    gemu_write_reg((gemu)->config.base_addr,                \
             GEM_RXWATERMARK_OFFSET,                                        \
             (High & GEM_RXWM_HIGH_MASK) |  \
             ((Low << GEM_RXWM_LOW_SHFT_MSK) & GEM_RXWM_LOW_MASK) |)

#define gemu_get_rx_watermark(gemu)                     \
    gemu_read_reg((gemu)->config.base_addr,                \
            GEM_RXWATERMARK_OFFSET)

// Remove FIXME PK
#if 0
#define BIT(n)            (1U << n)
#define SET_BIT(x, n)     (x | BIT(n))
#define GET_BIT(x, n)     ((x >> n) & 1U)
#define CLEAR_BIT(x, n)   (x & (~BIT(n)))
#define GENMASK(h, l)     (((~0U) << (l)) & (~0U >> (sizeof(int) * 8 - 1 - (h))))
#endif

static ALWAYS_INLINE void bufmem_stack_pop(gemu *gemu, uint64_t *virt_addr, uint64_t *phys_addr)
{
    gemu_assert(phys_addr);
    gemu_assert(virt_addr);
    bufmem_stack *p=&gemu->bufmem_stack;

    gemu_assert(p->cur_count > 0);

    int i = --p->cur_count;
    *phys_addr = p->bufs[i].phys_addr;
    *virt_addr = p->bufs[i].virt_addr;

    gemu_assert(*phys_addr);
    gemu_assert(*virt_addr);
    
    p->bufs_popped++;
    gemu_buf_hdr *hdr = (gemu_buf_hdr *)(*virt_addr);
    hdr->state = GEMU_BUF_ALLOCED;
    gemu_log("Allocted buf type 0x%0x current buf count %d ", hdr->buf_type, p->cur_count);
    gemu_log("Popped buf virt_addr %p phys_addr %p device_id %d\n", (void *)p->bufs[i].virt_addr, (void *)p->bufs[i].phys_addr, gemu->config.device_id);
}

static ALWAYS_INLINE void bufmem_stack_push(gemu *gemu, uint64_t virt_addr, uint64_t phys_addr)
{
    bufmem_stack *p=&gemu->bufmem_stack;

    gemu_assert(p->cur_count < p->num_bufs);

    gemu_buf_hdr *hdr = (gemu_buf_hdr *)virt_addr;

    if (hdr->buf_type == GEMU_BUF_TYPE_RX)
    {
        gemu_assert(virt_addr >= gemu->bufmem_start);
        gemu_assert(virt_addr < gemu->bufmem_end);
    }

    p->bufs[p->cur_count].phys_addr = phys_addr;
    p->bufs[p->cur_count].virt_addr = virt_addr;
    p->bufs_pushed++;

    hdr->state = GEMU_BUF_FREE;
    hdr->pkt_len = 0;
    gemu_assert(hdr->gemu == gemu);

    int i = p->cur_count;
    p->cur_count++;
    gemu_log("Returned buf type 0x%0x current buf count %d ", hdr->buf_type, p->cur_count);
    gemu_log("Pushed buf virt_addr %p phys_addr %p device_id %d\n", (void *)p->bufs[i].virt_addr, (void *)p->bufs[i].phys_addr, gemu->config.device_id);
}

static ALWAYS_INLINE void rte_mb() { asm volatile("dmb osh" : : : "memory"); }

static ALWAYS_INLINE void rte_wmb() { asm volatile("dmb oshst" : : : "memory"); }

static ALWAYS_INLINE void rte_rmb() { asm volatile("dmb oshld" : : : "memory"); }

static ALWAYS_INLINE void rte_smp_mb() { asm volatile("dmb ish" : : : "memory"); }

static ALWAYS_INLINE void rte_smp_wmb() { asm volatile("dmb ishst" : : : "memory"); }

static ALWAYS_INLINE void rte_smp_rmb() { asm volatile("dmb ishld" : : : "memory"); }

LONG gemu_cfg_initialize(gemu *gemu, gemu_config *cfg, UINTPTR effective_addr);
void gemu_set_queue_ptr(gemu *gemu, UINTPTR q_ptr, u8 queue_num, u16 direction);

gemu_config *gemu_lookup_config(int device_id);

int gemu_set_mac_addr(gemu *gemu, MAC_ADDR mac_addr, u8 index);
LONG gemu_delete_hash(gemu *gemu, void *mac_addr);
void gemu_get_mac_addr(gemu *gemu, MAC_ADDR mac_addr, u8 index);

LONG gemu_set_hash(gemu *gemu, void *mac_addr);
void gemu_clear_hash(gemu *gemu);
void gemu_get_hash(gemu *gemu, void *mac_addr);

void gemu_hw_reset_x(gemu *gemu);
u32 gemu_get_qx_offset(gemu_qx_reg_offset reg, u8 queue);

void *gemu_mm_alloc(u32 memsize);
void gemu_mm_free(void *p, u32 memsize);

void gemu_bufmem_alloc(gemu *);
void gemu_bufmem_free(gemu *);

#endif // __GEMU_H__
