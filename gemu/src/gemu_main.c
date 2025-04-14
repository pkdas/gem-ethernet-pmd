#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#define __USE_GNU
#include <pthread.h>

#include "gemu.h"
#include "gemu_mm.h"
#include "gemu_net_hdr.h"

static gemu gemu_dev_list[GEM_MAX_DEVICES] = {0};

static void *gem_ref_ctrl_base;

static gemu_hbd gemu_rx_hbds[GEM_MAX_DEVICES][GEMU_MAX_HBDS+4]; 
static gemu_hbd gemu_tx_hbds[GEM_MAX_DEVICES][GEMU_MAX_HBDS+4]; 

static u32 _pagesize;

static void gemu_init_rx_bds(gemu *gemu);
static void gemu_init_tx_bds(gemu *gemu);
static void gemu_disable(gemu *gemu);
static int gemu_tx_status(gemu *gemu);

static ALWAYS_INLINE void gemu_dump_bytes(u8 *data, int num_bytes)
{
#ifdef GEMU_DEBUG
    for (int i=0; i < num_bytes; i++)
    {
        gemu_log("0x%0x ", data[i]);
        if (i && ((i+1) % 16)==0)
        {
            gemu_log("\n");
        }
    }
    gemu_log("\n");
#endif
}

static ALWAYS_INLINE void gemu_init_hbds()
{
    memset(gemu_rx_hbds, 0, sizeof(gemu_rx_hbds));
    memset(gemu_tx_hbds, 0, sizeof(gemu_tx_hbds));
}

static ALWAYS_INLINE void gemu_dump_rx_desc(volatile gemu_desc *desc)
{
#ifdef GEMU_DEBUG
    volatile gemu_bd_ex *bd=&desc->bd;
    volatile gemu_bd *_bd=&desc->_bd;

    gemu_log("addr_lo 0x%0x status 0x%0x addr_hi 0x%0x w3 0x%0x wrap 0x%0x\n", 
            bd->addr_lo, bd->status, bd->addr_hi, (*_bd)[3], ((*_bd)[0] & GEM_RX_DESC_WRAP));

#endif
    // RX desc WRAP word0 bit 1 0x2
}

static ALWAYS_INLINE void gemu_dump_tx_desc(volatile gemu_desc *desc)
{
#ifdef GEMU_DEBUG
    volatile gemu_bd_ex *bd=&desc->bd;
    volatile gemu_bd *_bd=&desc->_bd;

    gemu_log("addr_lo 0x%0x status 0x%0x addr_hi 0x%0x w3 0x%0x wrap 0x%0x\n", 
            bd->addr_lo, bd->status, bd->addr_hi, (*_bd)[3], ((*_bd)[1] & GEM_TX_DESC_WRAP));

#endif
    // TX desc WRAP word1 bit 30 0x40000000
}

static ALWAYS_INLINE int gemu_is_rx_desc_wrap(volatile gemu_desc *rx_desc)
{
    volatile gemu_bd *_bd=&rx_desc->_bd;

    if ((*_bd)[0] & GEM_RX_DESC_WRAP)
    {
        return 1;
    }

    return 0;
}

static ALWAYS_INLINE int gemu_is_tx_desc_wrap(volatile gemu_desc *tx_desc)
{
    volatile gemu_bd *_bd=&tx_desc->_bd;

    if ((*_bd)[1] & GEM_TX_DESC_WRAP)
    {
        return 1;
    }

    return 0;
}

static ALWAYS_INLINE void gemu_dump_rx_descs(gemu *gemu, int count)
{
#ifdef GEMU_DEBUG
    volatile gemu_desc *rx_desc = (gemu_desc *)gemu->rx_bd_mem;
    u32 rx_desc_count=1;

    gemu_log("!!!!%s Dumping Rx-Descriptor list..\n", gemu->config.name); 
    gemu_log("________________________________\n"); 
    while (1)
    {
        gemu_log("RX-%d >>", rx_desc_count);
        gemu_dump_rx_desc(rx_desc);

        if (gemu_is_rx_desc_wrap(rx_desc))
        {
             gemu_log("RX_desc WRAP found rx-desc-count %d\n", rx_desc_count);
             break;
        }

        if (rx_desc_count == count)
        {
             gemu_log("Dumped rx-desc count %d\n", count);
             break;
        }
        rx_desc_count++;
        rx_desc++;

    }

    if (!count)
    {
        gemu_assert(rx_desc_count == gemu->rx_bd_count);
    }
#endif
}

static ALWAYS_INLINE void gemu_dump_tx_descs(gemu *gemu, int count)
{
#ifdef GEMU_DEBUG
    volatile gemu_desc *tx_desc = (gemu_desc *)gemu->tx_bd_mem;
    u32 tx_desc_count=1;

    gemu_log("!!!!%s Dumping Tx-Descriptor list..\n", gemu->config.name); 
    gemu_log("________________________________\n"); 
    while (1)
    {
        gemu_log("TX-%d >>", tx_desc_count);
        gemu_dump_tx_desc(tx_desc);

        if (gemu_is_tx_desc_wrap(tx_desc))
        {
            gemu_log("TX_desc WRAP found tx-desc-count %d\n", tx_desc_count);
            break;
        }

        if (tx_desc_count == count)
        {
             gemu_log("Dumped tx-desc count %d\n", count);
             break;
        }

        tx_desc_count++;
        tx_desc++;
    }

    if (!count)
    {
        gemu_assert(tx_desc_count == gemu->tx_bd_count);
    }
#endif
}

static ALWAYS_INLINE void gemu_dump_desc(gemu *gemu)
{
    gemu_dump_rx_descs(gemu, 0);
    gemu_dump_tx_descs(gemu, 0);
}

static ALWAYS_INLINE int gemu_is_q_empty(u32 pi, u32 ci)
{
    return (pi==ci);
}

static ALWAYS_INLINE int gemu_num_entries_in_q(u32 pi, u32 ci, u32 max_entries)
{
    int num_entries = 0;

    if (pi >= ci)
    {
        num_entries = pi - ci;  
    }else
    {
        num_entries = max_entries + pi - ci;
    }

    gemu_assert(num_entries <= max_entries);

    return num_entries;
}

static ALWAYS_INLINE int gemu_is_q_full(u32 pi, u32 ci, u32 count)
{
    return (gemu_num_entries_in_q(pi, ci, count) == count);
}

static ALWAYS_INLINE int gemu_is_tx_q_full(gemu *gemu)
{
    return (gemu_is_q_full(gemu->tx_pi, gemu->tx_ci, gemu->tx_bd_count));
}

static ALWAYS_INLINE int gemu_is_rx_q_full(gemu *gemu)
{
    return (gemu_is_q_full(gemu->rx_pi, gemu->rx_ci, gemu->rx_bd_count)); 
}

static ALWAYS_INLINE u32 increment_index(u32 val, u32 count)
{
    if (++val > count)
        val = 0;

    return val;
}

static ALWAYS_INLINE int gemu_tx_pkt(gemu *gemu, gemu_buf *tx_pkt)
{
    // current pi
    // obtain tx_desc and set buf pointers
    struct gemu *src_gemu = tx_pkt->buf_hdr.gemu;

    volatile gemu_bd *tx_bd=0;
    gemu_hbd *tx_hbd=0;

    if (gemu_is_tx_q_full(gemu))
    {
        gemu->stats.tx_queue_full++;
        return -1;
    }

    // pointing to invalid slot past the last entry, update to correct index to 0
    if (gemu->tx_pi == gemu->tx_bd_count)
    {
        gemu->tx_pi = 0;
    }

    gemu_assert(gemu->tx_pi < gemu->tx_bd_count);

    tx_bd = (volatile gemu_bd *)&gemu->tx_bd_mem[gemu->tx_pi];
    tx_hbd = &gemu->tx_hbds[gemu->tx_pi];
    gemu_assert(tx_hbd->bd_id == gemu->tx_pi);

    gemu_assert(tx_bd);
    // paranoia checks - remove FIXME PK
    // gemu_assert(rte_mem_virt2phy(tx_pkt) == tx_pkt->buf_hdr.phys_buf_addr);
    gemu_assert((tx_pkt->buf_hdr.state == GEMU_BUF_ALLOCED) || (tx_pkt->buf_hdr.state == GEMU_BUF_IN_PROCESS));

    gemu_bd_set_addr_tx(tx_bd, BUF_START_TO_BD_BUF_ADDR(tx_pkt->buf_hdr.phys_buf_addr));

    gemu_assert((sizeof(gemu_buf_hdr)+tx_pkt->buf_hdr.pkt_len) <= tx_pkt->buf_hdr.buf_len);
    gemu_log("TX packet length %d\n", tx_pkt->buf_hdr.pkt_len);
    gemu_dump_bytes(tx_pkt->data, 24);
    
    gemu_bd_set_length(tx_bd, (tx_pkt->buf_hdr.pkt_len));
    gemu_bd_clear_tx_used(tx_bd);
    gemu_bd_set_last(tx_bd);

    // update bufpool_id in the tx_bd hbd_data
    volatile gemu_desc *tx_desc = (volatile gemu_desc *)tx_bd;
    tx_desc->bd.bufpool_id = src_gemu->config.device_id;

    gemu_assert(tx_hbd->desc == (gemu_desc *)tx_bd);

    tx_hbd->buf = (UINTPTR)tx_pkt;
    tx_hbd->bufpool_id = src_gemu->config.device_id;
    tx_hbd->phys_buf_addr = tx_pkt->buf_hdr.phys_buf_addr; 

    tx_pkt->buf_hdr.state = GEMU_BUF_IN_TX_BD;

    gemu->tx_pi = increment_index(gemu->tx_pi, gemu->tx_bd_count);

    gemu_log("tx_desc addr_lo 0x%0x status 0x%0x addr_hi 0x%0x bd-id 0x%0x bufpool 0x%0x\n", 
            tx_desc->bd.addr_lo, tx_desc->bd.status, tx_desc->bd.addr_hi, tx_desc->bd.bd_id, tx_desc->bd.bufpool_id);

    u32 reg_ctrl = gemu_read_reg(gemu->config.base_addr, GEM_NWCTRL_OFFSET);

    return 0;
}

static ALWAYS_INLINE int gemu_tx_burst(gemu *gemu, gemu_buf **tx_pkts, uint16_t nb_pkts)
{
    int i;
    int tx_err_count = 0;
    int tx_count = 0;

    for (i=0; i < nb_pkts; i++)
    {
        if (gemu_tx_pkt(gemu, tx_pkts[i]) == -1)
        {
            gemu->stats.tx_err++;
            tx_err_count++;
            //break;
            gemu_assert(!"GEM tx error");
        }else
        {
            tx_count++;
        }
    }

    if (tx_count) 
    {
        gemu_log(">>>>....GEM%d transmit pkt count %d error %d\n", 
                gemu->config.device_id, tx_count, tx_err_count);
        rte_wmb();
        gemu_transmit(gemu);
        gemu->stats.tx_start++;
    }

    gemu->stats.tx_pkts += tx_count;
    return tx_count;
}

// NOTE: buffers are alloc/dealloced from the same gemu device buf pool

static ALWAYS_INLINE void gemu_complete_tx_bd(gemu *gemu, struct gemu *gemu_buf, gemu_bd *tx_bd, gemu_hbd *tx_hbd)
{
    // return buf
    // reset tx_bd buf_addr
    // clear tx_bd status 

    gemu->stats.tx_complete++;

    volatile gemu_desc *desc = (gemu_desc *)tx_bd;
    u32 tx_bd_status = desc->bd.status;
    u32 bd_id = desc->bd.bd_id;
    u32 bufpool_id = desc->bd.bufpool_id;

    gemu_assert(desc->bd.status & (GEM_TXBUF_LAST_MASK));

    // bd and hbd are in sync
    gemu_assert(bd_id == tx_hbd->bd_id);
    gemu_assert(tx_hbd->bufpool_id == bufpool_id);

    UINTPTR phys_tx_buf_addr;
    gemu_assert(tx_hbd->buf);
    phys_tx_buf_addr = gemu_bd_get_tx_buf_addr(tx_bd); 

    gemu_assert(tx_hbd->phys_buf_addr == BUF_START_FROM_BD_BUF_ADDR(phys_tx_buf_addr));

    // all check success - clean up
    gemu_buf_hdr *buf_hdr = (gemu_buf_hdr *)tx_hbd->buf;
    gemu_assert(buf_hdr->state == GEMU_BUF_IN_TX_BD);

    bufmem_stack_push(buf_hdr->gemu, tx_hbd->buf, tx_hbd->phys_buf_addr);

    if (buf_hdr->gemu == gemu)
    {
        gemu->stats.bufs_pushed_tx_complete++;
    } else
    {
        buf_hdr->gemu->stats.bufs_pushed_fwd_complete++;
    }

    tx_hbd->buf = 0;
    gemu_bd_set_addr_tx(tx_bd, 0);

    // clear USED and LAST bits
    u32 status_before = *(volatile u32 *)&desc->bd.status;
    *(volatile u32 *)&desc->bd.status = GEM_TXBUF_USED_MASK | (status_before & GEM_TXBUF_WRAP_MASK); 

    rte_wmb();

    gemu_log("%s complete-tx-bd id=%d, buffer returned to %s pool \n", gemu->config.name, bd_id, buf_hdr->gemu->config.name);
    u32 status_after = *(volatile u32 *)&desc->bd.status;
    gemu_log("%s complete-tx-bd status before 0x%0x status after 0x%0x\n", 
           gemu->config.name, status_before, status_after);
}

static ALWAYS_INLINE void gemu_complete_rx_bd(gemu *gemu, gemu_bd *rx_bd, gemu_hbd *rx_hbd)
{
    UINTPTR virt_buf_addr, phys_buf_addr;
    virt_buf_addr = phys_buf_addr = 0;
    bufmem_stack_pop(gemu, &virt_buf_addr, &phys_buf_addr);
    gemu->stats.bufs_popped_rx_refill++;

    gemu_assert(virt_buf_addr);
    gemu_assert(phys_buf_addr);
    gemu_buf_hdr *hdr = (gemu_buf_hdr *)virt_buf_addr;
    gemu_assert(hdr->gemu == gemu);

    volatile gemu_desc *desc = (gemu_desc *)rx_bd;
    u32 rx_bd_addr_lo=*(volatile u32 *)&desc->bd.addr_lo;
    u32 bd_id=desc->bd.bd_id;
    u32 bufpool_id=desc->bd.bufpool_id;

    // bd and hbd are in sync
    gemu_assert(bd_id == rx_hbd->bd_id);
    gemu_assert(rx_hbd->bufpool_id == gemu->config.device_id);
    gemu_assert(rx_hbd->bufpool_id == bufpool_id);

    // verify that USED bit is set
    gemu_assert(rx_bd_addr_lo & GEM_RXBUF_NEW_MASK);
    gemu_assert(desc->bd.status & (GEM_RXBUF_EOF_MASK | GEM_RXBUF_SOF_MASK));

    // if this is the last BD, verify WRAP bit
    if (rx_hbd->bd_id == (gemu->rx_bd_count-1))
    {
        gemu_assert(rx_bd_addr_lo & GEM_RXBUF_WRAP_MASK);
    }
    
    gemu_log("rx_bd %p virt_buf_addr %p phys_buf_addr %p\n", (void *)rx_bd, (void *)virt_buf_addr, (void *)phys_buf_addr);
    gemu_bd_set_addr_rx(rx_bd, BUF_START_TO_BD_BUF_ADDR(phys_buf_addr));
    // no need to set bufpool id or bd_id for rx
    rte_wmb();

    rx_hbd->buf = virt_buf_addr;
    ((gemu_buf_hdr *)rx_hbd->buf)->state = GEMU_BUF_IN_RX_BD;

    u32 addr_lo_before = *(volatile u32 *)&desc->bd.addr_lo;

    // clear the USED bit - since we have updated the rx_buf in the desc
    *(volatile u32 *)&desc->bd.addr_lo &= ~GEM_RXBUF_NEW_MASK;
}


// acting on behalf of GEM

// tx-q
// - pi is updated by gemu tx request by applicaiton
// - ci is updated by GEM by updating the BD tx completion status
 
// look at tx-complete flag in current TX-BD update ci
static ALWAYS_INLINE u32 gemu_update_tx_ci(gemu *gemu)
{
#define TX_BD_MAX_READ 1 

    u32 tx_bd_status[TX_BD_MAX_READ] = {0};
    u32 curr_ci = gemu->tx_ci;   

    // if there is any pending tx completion poll tx-bd status

    // last one is an empty slot, need to to skip check if we increment
    u32 tx_q_len =  gemu_num_entries_in_q(gemu->tx_pi, gemu->tx_ci, gemu->tx_bd_count);

    if (tx_q_len == 0) 
    {
        return curr_ci;
    } 

    if (tx_q_len > gemu->stats.max_tx_q_len)
    {
        gemu->stats.max_tx_q_len = tx_q_len;
    }
    if (tx_q_len > 16)
    {
        gemu->stats.tx_q_len_16++;
    }

    gemu->stats.tx_bd_poll++;

    if (curr_ci == gemu->tx_bd_count)
    {
        // this is a special case, pointing to unused slot at index tx_bd_count
        curr_ci = 0; 
    }

#if 0
    if (tx_q_len > GEMU_TX_VECTOR_SIZE)
    {
        tx_q_len = GEMU_TX_VECTOR_SIZE;
        gemu->stats.tx_q_len_max++;
    } 
#endif

    int ci_count = 0;
    while (1)
    {
        int updated=0;
#if 0
        if (likely(curr_ci+TX_BD_MAX_READ <= tx_q_len) && 
            likely((curr_ci+TX_BD_MAX_READ) < gemu->tx_bd_count))
        {
            int i = 0;
            while (i < TX_BD_MAX_READ)
            {
                tx_bd_status[i] = *(volatile u32 *)&(gemu->tx_bd_mem[curr_ci+i].bd.status);
                i++;
            }

            i=TX_BD_MAX_READ;
            while (i && !(tx_bd_status[i-1] & GEM_TXBUF_USED_MASK))
            {
                i--; 
            }     
            if (i > 1)
            {
                gemu->stats.tx_ci_burst_complete++;
            }
            if (i == 1)
            {
                gemu->stats.tx_ci_single_complete++;
            }
            if (i)
            {
                gemu_assert(tx_bd_status[i-1] & GEM_TXBUF_LAST_MASK);
                updated = 1;
            }
            curr_ci += i;
            tx_q_len -= i;
            ci_count += i;
        } else
#endif
        {
            tx_bd_status[0] = *(volatile u32 *)&gemu->tx_bd_mem[curr_ci].bd.status;
            if (tx_bd_status[0] & GEM_TXBUF_USED_MASK) 
            {
                gemu->stats.tx_ci_single_complete++;
                updated = 1;
                gemu_assert(tx_bd_status[0] & GEM_TXBUF_LAST_MASK);

                if (curr_ci++ == (gemu->tx_bd_count-1))
                {
                    // last entry - check wrap bit for consistency
                    gemu_assert(tx_bd_status[0] & GEM_TXBUF_WRAP_MASK);
                }
                tx_q_len -= 1;
                ci_count += 1;
            }
        }

        if ((!updated) || (!tx_q_len))
        {
            break;
        }

    }

    if (gemu->tx_ci != curr_ci)
    {
        gemu_log("%s updated tx ci curr_ci %d prev_ci %d\n", gemu->config.name, curr_ci, gemu->tx_ci);
        gemu->stats.tx_ci_updated++;
    }

    gemu->stats.tx_ci_count += ci_count;

    if (!tx_q_len)
    {
        gemu_tx_status(gemu);
        gemu->stats.tx_ci_updated_all++;
    }

    gemu->tx_ci = curr_ci;
    return (curr_ci);
}

// rx-q
// - pi is updated by GEM by updating the BD rx completion status
// - ci is updated by gemu by applicaiton rx processing

// look at rx-complete flag in current RX-BD and update pi
static ALWAYS_INLINE u32 gemu_update_rx_pi(gemu *gemu)
{
#define RX_BD_MAX_READ 1
    u32 rx_bd_addr_lo[RX_BD_MAX_READ] = {0};
    u32 curr_pi = gemu->rx_pi;   

    u32 rx_q_len =  gemu_num_entries_in_q(gemu->rx_pi, gemu->rx_ci, gemu->rx_bd_count);
    gemu_assert(rx_q_len <= gemu->rx_bd_count);
    // rx_queue full no update
    if (rx_q_len == gemu->rx_bd_count)
    {
        gemu_log("update_rx_pi - queue full pi=%d ci=%d\n", curr_pi, gemu->rx_ci);
        gemu->stats.rx_pi_no_update_q_full++;
        return curr_pi;
    }

    if (curr_pi == gemu->rx_bd_count)
    {
        gemu_log("update_rx_pi - resetting rx_pi to 0\n");
        curr_pi = 0; 
        gemu->rx_pi = curr_pi;
        gemu->stats.rx_pi_wrap++;
    }

    int pi_count = 0;
    int updated = 0;
    while (1)
    {
        updated=0;
#if 0
        if (likely((curr_pi+RX_BD_MAX_READ-1) < gemu->rx_bd_count)  && 
            ((rx_q_len+RX_BD_MAX_READ) <= gemu->rx_bd_count) &&
            ((pi_count+RX_BD_MAX_READ) <= GEMU_RX_VECTOR_SIZE))
        {
            gemu_assert((curr_pi+RX_BD_MAX_READ-1) < gemu->rx_bd_count);
            int i = 0;
            while (i < RX_BD_MAX_READ)
            {
                rx_bd_addr_lo[i] = *(volatile u32 *)&(gemu->rx_bd_mem[curr_pi+i].bd.addr_lo);
                i++;
            }

            i=RX_BD_MAX_READ;
            while (i && !(rx_bd_addr_lo[i-1] & GEM_RXBUF_NEW_MASK))
            {
                i--; 
            }     
            if (i > 1)
            {
                gemu->stats.rx_pi_burst_read++;
            }
            if (i == 1)
            {
                gemu->stats.rx_pi_single_read++;
            }
            if (i)
            {
                updated = 1;
                gemu_log("rx-pi updated by i %d\n", i);
            }
            curr_pi += i;
            pi_count += i;
        } else
#endif
        {
            gemu_assert(curr_pi < gemu->rx_bd_count);
            rx_bd_addr_lo[0] = *(volatile u32 *)&(gemu->rx_bd_mem[curr_pi].bd.addr_lo);
            if (rx_bd_addr_lo[0] & GEM_RXBUF_NEW_MASK)
            { 
                gemu->stats.rx_pi_single_read++;
                updated = 1;
                gemu_log("rx-pi updated by 1 only\n");
                pi_count += 1;
                if (curr_pi++ == (gemu->rx_bd_count-1))
                {
                    // last entry - check wrap bit for consistency
                    gemu_assert(rx_bd_addr_lo[0] & GEM_RXBUF_WRAP_MASK);
                }
            }
        }

        if (!updated)
        {
            break;
        } else
        {
            gemu->stats.rx_pi_updated++;
        }

        gemu_assert(pi_count <= GEMU_RX_VECTOR_SIZE);
        if (pi_count == GEMU_RX_VECTOR_SIZE)
        {
            gemu->stats.rx_pi_update_max++;
            break;
        }
        gemu_assert((rx_q_len + pi_count) <= gemu->rx_bd_count);

        // rx-q full
        if (rx_q_len + pi_count == gemu->rx_bd_count)
        {
            gemu->stats.rx_pi_update_q_full++;
            break;
        }

        if (curr_pi == gemu->rx_bd_count)
        {
            gemu_log("update_rx_pi - B - resetting rx_pi to 0\n");
            curr_pi = 0; 
            gemu->stats.rx_pi_wrap++;
        }
    }

    if (gemu->rx_pi != curr_pi)
    {
        gemu_log("%s updated pi curr_pi %d prev_pi %d\n", gemu->config.name, curr_pi, gemu->rx_pi);
    } else
    {
        //gemu_log("%s rx-pi is not updated\n", gemu->config.name);
    }

    gemu->stats.rx_pi_count += pi_count;
    gemu->rx_pi = curr_pi;
    return (curr_pi);
}

static ALWAYS_INLINE LONG gemu_tx_complete(gemu *gemu)
{
    LONG status = GEMU_SUCCESS;

    u32 curr_ci = gemu->tx_ci;
    u32 prev_ci = gemu->tx_prev_ci;

    if (prev_ci == curr_ci)
    {
        return status;
    }

    if (prev_ci == gemu->tx_bd_count)
    {
        prev_ci = 0; 
    }
   
    while (prev_ci != curr_ci)
    {
        volatile gemu_desc *desc = (volatile gemu_desc *)&gemu->tx_bd_mem[prev_ci];
        gemu_hbd  *hbd = &gemu->tx_hbds[prev_ci];
        u32 w0 = *(volatile u32 *)&desc->bd.addr_lo;
        u32 hw0 = *(volatile u32 *)&hbd->desc->bd.addr_lo;
     
        rte_rmb();

        gemu_assert(desc);
        gemu_assert(hbd);
        gemu_assert(hbd->desc->bd.bd_id == prev_ci);
        gemu_assert(hbd->desc->bd.addr_lo == desc->bd.addr_lo);
        gemu_assert(hbd->desc->bd.addr_hi == desc->bd.addr_hi);
        gemu_assert(hbd->desc->bd.bd_id == desc->bd.bd_id);

        gemu_buf_hdr *buf_hdr = (gemu_buf_hdr *)hbd->buf;
        gemu_assert(buf_hdr);
        gemu_assert(buf_hdr->gemu);
        gemu_assert(buf_hdr->gemu->config.device_id == desc->bd.bufpool_id);
        u64 phys_bd_buf_addr = BUF_START_FROM_BD_BUF_ADDR((u64)desc->bd.addr_lo | ((u64)(desc->bd.addr_hi) << 32));
        gemu_assert(buf_hdr->phys_buf_addr == phys_bd_buf_addr); 

        gemu_complete_tx_bd(gemu, buf_hdr->gemu, (gemu_bd *)desc, hbd);  
        prev_ci = increment_index(prev_ci, gemu->tx_bd_count);
    }

    if (gemu->tx_prev_ci != prev_ci)
    {
        gemu_log("%s update tx_prev_ci, new-tx-prev_ci=%d old-tx-prev_ci=%d\n", gemu->config.name, prev_ci, gemu->tx_prev_ci);
    }

    gemu->tx_prev_ci = prev_ci;
  
    return (status);
}

static ALWAYS_INLINE LONG gemu_rx_complete(gemu *gemu)
{
    LONG status = GEMU_SUCCESS;

    u32 curr_pi = gemu->rx_pi;
    u32 curr_ci = gemu->rx_ci;

    u32 rx_q_len =  gemu_num_entries_in_q(gemu->rx_pi, gemu->rx_ci, gemu->rx_bd_count);
    gemu_assert(rx_q_len <= gemu->rx_bd_count);

    if (!rx_q_len)
    {
        //gemu_log("update_rx_pi - queue empty ci=%d\n", curr_ci);
        return status;
    }

    if (rx_q_len > gemu->stats.max_rx_q_len)
    {
        gemu->stats.max_rx_q_len = rx_q_len;
    }
    if (rx_q_len > 16)
    {
        gemu->stats.rx_q_len_16++;
    }

    if (curr_ci == gemu->rx_bd_count)
    {
        gemu_log("rx_complete - resetting rx_ci to 0\n");
        curr_ci = 0; 
        gemu->rx_ci = curr_ci;
        gemu->stats.rx_ci_wrap++;
    }

    int ci_count = 0;
    while (curr_ci != curr_pi)
    {
        gemu_assert(curr_ci < gemu->rx_bd_count);
        volatile gemu_desc *desc = (volatile gemu_desc *)&gemu->rx_bd_mem[curr_ci];
        gemu_hbd  *hbd = &gemu->rx_hbds[curr_ci];
        u32 w0 = *(volatile u32 *)&desc->bd.addr_lo;
        u32 hw0 = *(volatile u32 *)&hbd->desc->bd.addr_lo;
     
        gemu_assert(desc);
        gemu_assert(hbd);
        gemu_assert(hbd->desc->bd.bd_id == curr_ci);
        gemu_assert((hbd->desc->bd.addr_lo & GEM_RXBUF_ADD_MASK) == (desc->bd.addr_lo & GEM_RXBUF_ADD_MASK));
        gemu_assert(hbd->desc->bd.addr_hi == desc->bd.addr_hi);
        gemu_assert(hbd->desc->bd.bd_id == desc->bd.bd_id);

        gemu_buf_hdr *buf_hdr = (gemu_buf_hdr *)hbd->buf;
        gemu_assert(buf_hdr);
        gemu_assert(buf_hdr->gemu);
        gemu_assert(buf_hdr->gemu->config.device_id == desc->bd.bufpool_id);
        u64 phys_bd_buf_addr = BUF_START_FROM_BD_BUF_ADDR((u64)(desc->bd.addr_lo & GEM_RXBUF_ADD_MASK) | \
                                                          ((u64)(desc->bd.addr_hi) << 32));
        gemu_assert(buf_hdr->phys_buf_addr == phys_bd_buf_addr); 

        gemu_assert(gemu->rx_vector_size < GEMU_RX_VECTOR_SIZE);
        gemu_dump_rx_desc(desc);

        buf_hdr->pkt_len = gemu_bd_get_length(desc);
        buf_hdr->state = GEMU_BUF_IN_PROCESS; 

        gemu->rx_vector[gemu->rx_vector_size++] = (gemu_buf *)buf_hdr;

        gemu_complete_rx_bd(buf_hdr->gemu, (gemu_bd *)desc, hbd);  

        ci_count++;
        curr_ci++;

        if (ci_count == rx_q_len)
        {
            gemu->stats.rx_ci_all++;
            break;
        }

        if (curr_ci == gemu->rx_bd_count)
        {
            gemu_log("rx_complete - resetting rx_ci to 0\n");
            curr_ci = 0; 
            gemu->stats.rx_ci_wrap++;
        }
    }

    gemu_assert(ci_count == rx_q_len);

    if (gemu->rx_ci != curr_ci)
    {
        gemu_log("%s update ci, curr_ci=%d prev_ci=%d\n", gemu->config.name, curr_ci, gemu->rx_ci);
        gemu->stats.rx_ci_updated++;
    }

    gemu->rx_ci = curr_ci;

    return (status);
}

// returns number of pkts received
u32 gemu_rx_burst(gemu *gemu, UINTPTR pkt_virt_addr[], u32 pkt_len[], u32 rx_count)
{
    return 0;
}

LONG gemu_configure_queue_ptrs(gemu *gemu)
{
    // receive q 0x18 lo32
    gemu_write_reg(gemu->config.base_addr, 0x00000018, (u32)gemu->rx_hbds[0].phys_desc & 0xFFFFFFFF);
    // transmit q 0x1C lo32
    gemu_write_reg(gemu->config.base_addr, 0x0000001C, (u32)gemu->tx_hbds[0].phys_desc & 0xFFFFFFFF);

    // receive q  0x4D4 hi32
    gemu_write_reg(gemu->config.base_addr, 0x000004D4, (u32)((gemu->rx_hbds[0].phys_desc >> 32) & 0xFFFFFFFF));
    // transmit q  0x4C8 hi32
    gemu_write_reg(gemu->config.base_addr, 0x000004C8, ((u32)(gemu->tx_hbds[0].phys_desc >> 32) & 0xFFFFFFFF));

    // transmit q1 0x440
    gemu_write_reg(gemu->config.base_addr, 0x00000440, 0); 
    // receive q1 0x480
    gemu_write_reg(gemu->config.base_addr, 0x00000480, 0); 
    // dma rxbuf q1 0x4A0
    gemu_write_reg(gemu->config.base_addr, 0x000004A0, 0); 
}

LONG gemu_dev_setup(gemu *gemu) 
{
    LONG status;
    gemu_config *config = &gemu->config;

    gemu_assert (config->base_addr);

    gemu_assert (!gemu->rx_bd_mem);
    gemu_assert (!gemu->tx_bd_mem);

    /* Allocate Rx and Tx BD space each */
    gemu->rx_bd_count = gemu->config.rx_bd_count;
    gemu->tx_bd_count = gemu->config.tx_bd_count;

    gemu->rx_bd_memsize = gemu->rx_bd_count * sizeof (gemu_bd);
    gemu->tx_bd_memsize = gemu->tx_bd_count * sizeof (gemu_bd);

    void *rx_bd_mem = (gemu_desc *)gemu_bdmem_alloc(gemu->rx_bd_memsize); 
    void *tx_bd_mem = (gemu_desc *)gemu_bdmem_alloc(gemu->tx_bd_memsize); 

    assert(rx_bd_mem);
    assert(tx_bd_mem);

    gemu->phys_rx_bd_mem = rte_mem_virt2phy(rx_bd_mem); 
    gemu->phys_tx_bd_mem = rte_mem_virt2phy(tx_bd_mem); 

    assert(gemu->phys_rx_bd_mem != RTE_BAD_IOVA);
    assert(gemu->phys_tx_bd_mem != RTE_BAD_IOVA);

    gemu->rx_bd_mem = rx_bd_mem;
    gemu->tx_bd_mem = tx_bd_mem;

    gemu_log("BD mem allocated, rx_bd_count %d rx_bd_memsz %d, tx_bd_count %d tx_bd_memsz %d rx_bd_mem %p tx_bd_mem %p\n", 
            gemu->rx_bd_count, gemu->rx_bd_memsize, gemu->tx_bd_count, gemu->tx_bd_memsize, gemu->rx_bd_mem, gemu->tx_bd_mem);
    gemu_assert(gemu->tx_bd_mem);
    gemu_assert(gemu->rx_bd_mem);

    gemu_log("RX-ptr base physaddr %p TX-ptr base physaddr %p\n", (void *)gemu->phys_rx_bd_mem, (void *)gemu->phys_tx_bd_mem);

    // Host BD initialization
    gemu->rx_hbds = gemu_rx_hbds[gemu->config.device_id];
    gemu->tx_hbds = gemu_tx_hbds[gemu->config.device_id];

    gemu->rx_pi = 0;
    gemu->rx_ci = 0;

    gemu->tx_pi = 0;
    gemu->tx_ci = 0;

    gemu->rx_prev_pi = 0;
    gemu->rx_prev_ci = 0;

    gemu->tx_prev_pi = 0;
    gemu->tx_prev_ci = 0;

    // allocate buffers
    gemu_bufmem_alloc(gemu);
    gemu_assert(gemu->bufmem_start);
    gemu_assert(gemu->bufmem_end);

    // config
    gemu_disable(gemu);
    gemu_init_rx_bds(gemu);
    gemu_init_tx_bds(gemu);

    gemu_configure_queue_ptrs(gemu);
    
    return GEMU_SUCCESS;
}

void gemu_dev_configure(gemu *gemu, int device_id, UINTPTR base_virt_addr)
{
    LONG status;
    gemu_config *config;
    gemu_bd bd_template;
    u16 i;

    if (!gemu->config.base_addr)
    {
        gemu_assert(base_virt_addr);
        config = gemu_lookup_config(device_id);
        memcpy(&gemu->config, config, sizeof(gemu->config));
        gemu->config.base_addr = base_virt_addr;
    }

    // gemu_read_desc(gemu);
    // exit(0);

    // start from clean state
    // gemu_hw_reset(gemu); PK FIXME
    gemu_dev_setup(gemu);
}

// initialize all rx_bds 
void gemu_init_rx_bds(gemu *gemu)
{
    gemu_bd *rx_bd = (gemu_bd *)gemu->rx_bd_mem;
    UINTPTR phys_rx_bd = gemu->phys_rx_bd_mem;

    gemu_assert(rx_bd);

    int i;
    for (i=0; i < gemu->rx_bd_count; i++)
    {
        UINTPTR virt_buf_addr, phys_buf_addr;
        virt_buf_addr = phys_buf_addr = 0;
        bufmem_stack_pop(gemu, &virt_buf_addr, &phys_buf_addr);  
        gemu_assert(virt_buf_addr);
        gemu_assert(phys_buf_addr);

        memset(rx_bd, 0, sizeof(*rx_bd));

        gemu_log("rx_bd %p virt_buf_addr %p phys_buf_addr %p\n", (void *)rx_bd, (void *)virt_buf_addr, (void *)phys_buf_addr);
        gemu_bd_set_addr_rx(rx_bd, BUF_START_TO_BD_BUF_ADDR(phys_buf_addr));
       
        // upper16 bit buf_pool_id (gemu device_id)
        // lower16 bit bd_id (index i)
        // u32 hbd_data = (gemu->config.device_id << 16) | i;
        // gemu_bd_set_hbd_data(rx_bd, hbd_data);
        gemu_desc *desc = (gemu_desc *) rx_bd;
        desc->bd.bd_id = i;
        desc->bd.bufpool_id = gemu->config.device_id;
        gemu_log("rx_bd w0=0x%x w1=0x%x w2=0x%x w3=0x%x\n", (*rx_bd)[0], (*rx_bd)[1], (*rx_bd)[2], (*rx_bd)[3]);

        if (i == gemu->rx_bd_count-1)
        {
            gemu_desc *rx_desc = (gemu_desc *)rx_bd;
            u32 w0 = rx_desc->bd.addr_lo | GEM_RXBUF_WRAP_MASK;
            rx_desc->bd.addr_lo = w0;
        }
        
        UINTPTR phys_desc_rx_buf_addr = gemu_bd_get_rx_buf_addr(rx_bd);
        gemu_assert(phys_buf_addr == BUF_START_FROM_BD_BUF_ADDR(phys_desc_rx_buf_addr));

        gemu_hbd *rx_hbd = gemu->rx_hbds+i;
        memset(rx_hbd, 0, sizeof(*rx_hbd));

        rx_hbd->desc = (gemu_desc *)rx_bd;
        rx_hbd->phys_desc = phys_rx_bd;
        rx_hbd->buf = virt_buf_addr;
        rx_hbd->bd_id = i;
        rx_hbd->type = GEMU_HBD_RX;
        rx_hbd->bufpool_id = gemu->config.device_id; 
        ((gemu_buf_hdr *)rx_hbd->buf)->state = GEMU_BUF_IN_RX_BD;

        rx_bd++;
        phys_rx_bd += sizeof(*rx_bd);
    }

    gemu_hbd *rx_hbd = &gemu->rx_hbds[i];
    memset(rx_hbd, 0, sizeof(*rx_hbd));
}

void gemu_init_tx_bds(gemu *gemu)
{
    gemu_bd *tx_bd = (gemu_bd *)gemu->tx_bd_mem;
    UINTPTR phys_tx_bd = gemu->phys_tx_bd_mem;

    gemu_assert(tx_bd);

    int i;
    for (i=0; i < gemu->tx_bd_count; i++)
    {
        memset(tx_bd, 0, sizeof(*tx_bd));
        gemu_desc *tx_desc = (gemu_desc *)tx_bd;
        tx_desc->bd.status = GEM_TXBUF_USED_MASK;
        tx_desc->bd.bd_id = i;
        if (i == gemu->tx_bd_count-1)
        {
            gemu_desc *tx_desc = (gemu_desc *)tx_bd;
            u32 w1 = tx_desc->bd.status | GEM_TXBUF_WRAP_MASK;
            tx_desc->bd.status = w1;
        }

        // do we set bufpool_id on tx?
        u32 hbd_data = i;
        gemu_bd_set_hbd_data(tx_bd, hbd_data);

        gemu_hbd *tx_hbd = gemu->tx_hbds+i;
        memset(tx_hbd, 0, sizeof(*tx_hbd));

        tx_hbd->desc = (gemu_desc *)tx_bd;
        tx_hbd->phys_desc = phys_tx_bd; 
        tx_hbd->bd_id = i;
        tx_hbd->type = GEMU_HBD_TX;

        tx_bd++;
        phys_tx_bd += sizeof(*tx_bd);

        gemu_log("tx_bd w0=0x%x w1=0x%x w2=0x%x w3=0x%x\n", (*tx_bd)[0], (*tx_bd)[1], (*tx_bd)[2], (*tx_bd)[3]);
    }

    gemu_hbd *tx_hbd = &gemu->tx_hbds[i];
    memset(tx_hbd, 0, sizeof(*tx_hbd));
}
static ALWAYS_INLINE int gemu_process_arp_pkt(gemu *gemu, u8 *eth_frame, u32 len)
{
    arp_hdr *arp = (arp_hdr *)(eth_frame + sizeof(ether_hdr));
    ether_hdr *eth_hdr = (ether_hdr *)eth_frame;
    gemu_buf_hdr * buf_hdr = (gemu_buf_hdr *)(eth_frame - sizeof(gemu_buf_hdr)); 
    int ret = 1; 

    if (ntohs(arp->arp_op) == ARP_OP_REQUEST)
    {
        gemu->stats.arp_req_recvd++;
        if (arp->arp_data.arp_tip == (*(u32 *)gemu->config.ip4_addr))
        {
             gemu->stats.arp_req_host_ip_recvd++;

             // send ARP response
             arp->arp_op = htons(ARP_OP_REPLY);
             memcpy(&arp->arp_data.arp_tha, gemu->config.mac_addr, 
                    sizeof(arp->arp_data.arp_tha));

             arp->arp_data.arp_sip = *(u32 *)gemu->config.ip4_addr;
             memcpy(&arp->arp_data.arp_sha, gemu->config.mac_addr, 
                    sizeof(arp->arp_data.arp_sha));

             memcpy(&eth_hdr->d_addr, &eth_hdr->s_addr, sizeof(eth_hdr->d_addr));
             memcpy(&eth_hdr->s_addr, gemu->config.mac_addr, sizeof(eth_hdr->d_addr));
             gemu_assert(gemu->tx_vector_size < GEMU_TX_VECTOR_SIZE);
             gemu->tx_vector[gemu->tx_vector_size++] = (gemu_buf *)buf_hdr; // zero copy

             gemu->stats.arp_reply_sent++;
             gemu->stats.tx_added++; 
             ret = 0;
        }
    }

    return ret;
}

static ALWAYS_INLINE int gemu_process_ip_pkt(gemu *gemu, u8 *eth_frame, u32 len)
{
    ip4_hdr *ip = (ip4_hdr *)(eth_frame + sizeof(ether_hdr));
    int dest_port = -1; 
    struct gemu *dest_gemu; 
    gemu_buf_hdr * buf_hdr = (gemu_buf_hdr *)(eth_frame - sizeof(gemu_buf_hdr)); 
    int ret = 1; 
    
    gemu_assert(ip->version_ihl == 0x45);

    if (ip->next_proto_id == IPPROTO_TCP)
    {
        gemu->stats.tcp_pkts_rxed++;
        dest_port = (gemu->config.device_id==0) ?  1 : 0; 
        gemu->stats.tcp_pkts_fwded++;
    }

    if (ip->next_proto_id == IPPROTO_UDP)
    {
        gemu->stats.udp_pkts_rxed++;
        dest_port = (gemu->config.device_id==0) ?  1 : 0; 
        gemu->stats.udp_pkts_fwded++;
    }

    if (ip->next_proto_id == IPPROTO_ICMP)
    {
        gemu->stats.icmp_pkts_rxed++;

        icmp_hdr *icmp = (icmp_hdr *)(ip+1); 

        if (icmp->icmp_type == IP_ICMP_ECHO_REQUEST)
        {
            if (ip->dst_addr == (*(u32 *)gemu->config.ip4_addr))
            {
                gemu->stats.icmp_echo_requests++;
                icmp->icmp_type = IP_ICMP_ECHO_REPLY;

                dest_port = gemu->config.device_id;

                gemu->stats.icmp_echo_replies++;

                // swap dst and src mac 
                // swap dst and src ip
                u32 dst_ip_addr = ip->dst_addr;
                ip->dst_addr = ip->src_addr;
                ip->src_addr = dst_ip_addr; 
                ip->hdr_checksum = 0;
                ip->packet_id = gemu->tx_pkt_id++;
            } else
            {
                dest_port = (gemu->config.device_id==0) ?  1 : 0; 
                gemu->stats.icmp_echo_req_fwded++;
            }
        } else if (icmp->icmp_type == IP_ICMP_ECHO_REPLY)
        {
            dest_port = (gemu->config.device_id==0) ?  1 : 0;
            gemu->stats.icmp_echo_reply_fwded++;

        } else
        {
            gemu_assert(!"Unexpected ICMP packet type");
        }
    }

    if (dest_port == -1)
    {
        gemu->stats.rx_pkts_tossed++;
        return ret;
    }

    gemu_assert((dest_port == 0) || (dest_port == 1));
    dest_gemu = &gemu_dev_list[dest_port]; 

    ether_hdr *eth_hdr = (ether_hdr *)eth_frame;

    if (dest_port != gemu->config.device_id)
    {
         // forwarding 
         memcpy(&eth_hdr->d_addr, dest_gemu->config.next_hop_mac, sizeof(eth_hdr->d_addr)); 
         gemu->stats.ip_pkts_fwded++;
    } else
    {
         memcpy(&eth_hdr->d_addr, &eth_hdr->s_addr, sizeof(eth_hdr->d_addr));
    }
    memcpy(&eth_hdr->s_addr, dest_gemu->config.mac_addr, sizeof(eth_hdr->d_addr));
    gemu_assert(dest_gemu->tx_vector_size < GEMU_TX_VECTOR_SIZE);

    dest_gemu->tx_vector[dest_gemu->tx_vector_size++] = (gemu_buf *)buf_hdr; // zero copy
    ret = 0;

    dest_gemu->stats.tx_added++; 
    if (gemu != dest_gemu)
    {
        dest_gemu->stats.fwded_tx_added++;
    }
    return ret; 
}

static ALWAYS_INLINE void gemu_process_rx_pkt(gemu *gemu, gemu_buf_hdr *buf_hdr)
{
    u32 len = buf_hdr->pkt_len; 
    u8 *eth_frame = (u8 *)buf_hdr + sizeof(gemu_buf_hdr); 
    ether_hdr *eth_hdr = (ether_hdr *)eth_frame;

    if (eth_hdr->ether_type == 0x0608)
    {
        gemu->stats.rx_arp_pkts++;
        if (gemu_process_arp_pkt(gemu, eth_frame, len) != 0)
        {
            gemu->stats.rx_bufs_freed++;
            bufmem_stack_push(gemu, (UINTPTR)buf_hdr, buf_hdr->phys_buf_addr);
            gemu->stats.bufs_pushed_tossed_arp_pkts++;
        }
        return;
    }

    // 0x0800 big-endian
    if (eth_hdr->ether_type == 0x0008)
    {
        gemu->stats.rx_ip_pkts++;
        if (gemu_process_ip_pkt(gemu, eth_frame, len) != 0)
        {
            gemu->stats.rx_bufs_freed++;
            bufmem_stack_push(gemu, (UINTPTR)buf_hdr, buf_hdr->phys_buf_addr);
            gemu->stats.bufs_pushed_tossed_ip_pkts++;
        }
        return;
    } 
    gemu->stats.rx_nonip_pkts++;
    gemu->stats.rx_bufs_freed++;

    bufmem_stack_push(gemu, (UINTPTR)buf_hdr, buf_hdr->phys_buf_addr);
    gemu->stats.bufs_pushed_tossed_non_ip_pkts++;
}

static ALWAYS_INLINE void gemu_process_rx_pkts(gemu *gemu)
{
    for (int i=0; i < gemu->rx_vector_size; i++)
    {
        gemu_buf_hdr *buf_hdr = (gemu_buf_hdr *)gemu->rx_vector[i];

        gemu_assert(buf_hdr->gemu == gemu);
        gemu_process_rx_pkt(gemu, buf_hdr);
    }
}

static ALWAYS_INLINE void gemu_process_tx_pkts(gemu *gemu)
{
    gemu_tx_burst(gemu, gemu->tx_vector, gemu->tx_vector_size);

    return;
}

static void gemu_dev_stop(gemu *gemu)
{
    //gemu_hw_stop(gemu);

    gemu_bdmem_free((void *)gemu->rx_bd_mem, gemu->rx_bd_memsize);
    gemu_bdmem_free((void *)gemu->tx_bd_mem, gemu->tx_bd_memsize);
    gemu->rx_bd_mem = 0;
    gemu->tx_bd_mem = 0;

    gemu_bufmem_free(gemu);
}


static ALWAYS_INLINE void  gemu_refill_rx_bufs(gemu *gemu)
{
}

void gemu_dump_app_stats(gemu *gemu)
{
    printf("\t tx_added         %d\n", gemu->stats.tx_added);
    printf("\t tx_pkts          %d\n", gemu->stats.tx_pkts);
    printf("\t tx_start         %d\n", gemu->stats.tx_start);
    printf("\t tx_queueue_ful   %d\n", gemu->stats.tx_queue_full);
    printf("\t rx_ip_pkts       %d\n", gemu->stats.rx_ip_pkts);
    printf("\t rx_nonip_pkts    %d\n", gemu->stats.rx_nonip_pkts);
    printf("\t rx_bufs_freed    %d\n", gemu->stats.rx_bufs_freed);
    printf("\t tcp_pkts_rxed    %d\n", gemu->stats.tcp_pkts_rxed);
    printf("\t udp_pkts_rxed    %d\n", gemu->stats.udp_pkts_rxed);
    printf("\t icmp_pkts_rxed   %d\n", gemu->stats.icmp_pkts_rxed);

    printf("\t icmp_echo_reqs   %d\n", gemu->stats.icmp_echo_requests);
    printf("\t icmp_echo_repl   %d\n", gemu->stats.icmp_echo_replies);

    printf("\t tcp_pkts_fwded   %d\n", gemu->stats.tcp_pkts_fwded);
    printf("\t udp_pkts_fwded   %d\n", gemu->stats.udp_pkts_fwded);
    printf("\t icmp_echo_req_fwded    %d\n", gemu->stats.icmp_echo_req_fwded);
    printf("\t icmp_echo_reply_fwded  %d\n", gemu->stats.icmp_echo_reply_fwded);

    printf("\t rx_pkts_tossed   %d\n", gemu->stats.rx_pkts_tossed);

}

static ALWAYS_INLINE void gemu_dump_stats(gemu *gemu)
{
    printf("GEMU device id %d\n", gemu->config.device_id);

    u32 val = gemu_read_reg(gemu->config.base_addr, GEM_TXCNT_OFFSET);
    printf("Total Packets sent: %d\n", val);

    val = gemu_read_reg(gemu->config.base_addr, GEM_RXCNT_OFFSET);
    printf("Total Packets received: %d\n", val);
}

static ALWAYS_INLINE void gemu_err_handler(gemu *gemu)
{
    u32 reg_isr;
    u32 reg_sr;
    u32 reg_ctrl;
    u32 reg_qi_isr[32] = {0};        /* Max queues possible */
    u8 i = 0;

    reg_isr = gemu_read_reg(gemu->config.base_addr, GEM_ISR_OFFSET);

    for (i=0; i < gemu->max_queues; i++)
    {
        reg_qi_isr[i] = gemu_read_reg(gemu->config.base_addr, gemu_get_qx_offset(INTQI_STS, i));
    }

    gemu_write_reg(gemu->config.base_addr, GEM_ISR_OFFSET, reg_qi_isr[0]);

    for(i=0; i < gemu->max_queues;  i++)
    {
        if ( reg_qi_isr[i]  & GEM_INTQISR_RXCOMPL_MASK ) 
        {
            gemu_assert(!"GEM_INTQISR_RXCOMPL_MASK should be disabled!!!!");

        }

    }
    for(i=0; i < gemu->max_queues;  i++)
    {
        if ( reg_qi_isr[i]  & GEM_INTQISR_TXCOMPL_MASK ) {
            gemu_assert(!"GEM_INTQISR_RXCOMPL_MASK should be disabled!!!!");

        }
    }

    if ((reg_isr & GEM_IXR_RX_ERR_MASK) != 0x00000000U) 
    {
        reg_sr = gemu_read_reg(gemu->config.base_addr, GEM_RXSR_OFFSET);
        gemu_write_reg(gemu->config.base_addr, GEM_RXSR_OFFSET, reg_sr);

        /* Fix for CR # 692702. Write to bit 18 of net_ctrl
         * register to flush a packet out of Rx SRAM upon
         * an error for receive buffer not available. */
        if ((reg_isr & GEM_IXR_RXUSED_MASK) != 0x00000000U) 
        {
            reg_ctrl = gemu_read_reg(gemu->config.base_addr, GEM_NWCTRL_OFFSET);
            reg_ctrl |= (u32)GEM_NWCTRL_FLUSH_DPRAM_MASK;
            gemu_write_reg(gemu->config.base_addr, GEM_NWCTRL_OFFSET, reg_ctrl);
        }

    }

    /* When GEM_IXR_TXCOMPL_MASK is flagged, GEM_IXR_TXUSED_MASK
    * will be gemu_asserted the same time.
    * Have to distinguish this bit to handle the real error condition.
    */
    /* Transmit Q1,2,.. error conditions interrupt */
    for(i=1; i < gemu->max_queues;  i++)
    {
        if(((reg_qi_isr[i] & GEM_INTQSR_TXERR_MASK) != 0x00000000U) &&
           ((reg_qi_isr[i] & GEM_INTQISR_TXCOMPL_MASK) != 0x00000000U)) 
        {
            /* Clear Interrupt Q1 status register */
            gemu_write_reg(gemu->config.base_addr, gemu_get_qx_offset(INTQI_STS, i), reg_qi_isr[i]); 
        }
    }

    /* Transmit error conditions interrupt */
    if (((reg_isr & GEM_IXR_TX_ERR_MASK) != 0x00000000U) &&
        ((!(reg_isr & GEM_IXR_TXCOMPL_MASK)) != 0x00000000U)) 
    {
        reg_sr = gemu_read_reg(gemu->config.base_addr, GEM_TXSR_OFFSET);
        gemu_write_reg(gemu->config.base_addr, GEM_TXSR_OFFSET, reg_sr);
    }

}

static ALWAYS_INLINE void gemu_handle_error(gemu *gemu)
{
#ifdef GEMU_DEBUG
    u32 val = gemu_read_reg(gemu->config.base_addr, GEM_ISR_OFFSET);

    if ((val & GEM_IXR_TX_ERR_MASK) || (val & GEM_IXR_RX_ERR_MASK))
    {
        gemu_log("GEM device %d error ISR     : 0x%0x\n", gemu->config.device_id, val);
        gemu_err_handler(gemu);
    }
#endif
}

static void gemu_enable(gemu *gemu)
{
    gemu_log("Enabling %s\n", gemu->config.name);
    gemu_assert(!gemu->started);

    gemu_write_reg(gemu->config.base_addr, GEM_NWCFG_OFFSET, 0x012E0452);

    gemu_write_reg(gemu->config.base_addr, GEM_DMACR_OFFSET, 0x40180F10); 

    gemu_write_reg(gemu->config.base_addr, GEM_ISR_OFFSET, GEM_IXR_ALL_MASK);

    gemu_write_reg(gemu->config.base_addr, GEM_NWCTRL_OFFSET, (GEM_NWCTRL_RXEN_MASK | GEM_NWCTRL_TXEN_MASK));
    gemu->started = 1;

    gemu_log("Wait 5 sec..\n");
    sleep(5);
    gemu_log("Enabling %s complete\n", gemu->config.name);
}

static void gemu_disable(gemu *gemu)
{
    gemu_log("Disabling %s\n", gemu->config.name);
    gemu_write_reg(gemu->config.base_addr, GEM_IDR_OFFSET, GEM_IXR_ALL_MASK);
    gemu_write_reg(gemu->config.base_addr, GEM_ISR_OFFSET, GEM_IXR_ALL_MASK);
    gemu_write_reg(gemu->config.base_addr, GEM_NWCTRL_OFFSET, 0);
    gemu->started = 0;

    gemu_log("Wait 5 sec..\n");
    sleep(5);
    gemu_log("Disabling %s complete\n", gemu->config.name);
}

static int gemu_tx_status(gemu *gemu)
{
    u32 tx_status = gemu_read_reg(gemu->config.base_addr, GEM_TXSR_OFFSET); 
    
    if (tx_status)
    {
        gemu_log("!!!!!!!!TX status 0x%0x\n", tx_status);
    }
 
    if (tx_status & GEM_TXSR_TXGO_MASK)
    {
        gemu->stats.tx_status_go++;
        return 1;
    }

    if (tx_status & GEM_TXSR_TXCOMPL_MASK)
    {
        gemu_log("!!!!!!!!TX complete 0x%0x\n", tx_status);
        gemu_write_reg(gemu->config.base_addr, GEM_TXSR_OFFSET, tx_status);
        gemu->stats.tx_status_complete++;
        return 0;
    }

    if (tx_status & GEM_TXSR_ERROR_MASK)
    {
        gemu_log("!!!!!!!!TX error 0x%0x\n", tx_status);
        gemu->stats.tx_status_err++;

        //gemu_assert(!"!!!!!!!!TX error");
    }

    return 1;
}

void gemu_rx_status(gemu *gemu)
{
    u32 rx_status = gemu_read_reg(gemu->config.base_addr, GEM_RXSR_OFFSET);
    if (rx_status & GEM_RXSR_FRAMERX_MASK)
    {
        // clear rx-status 
        gemu_write_reg(gemu->config.base_addr, GEM_RXSR_OFFSET, rx_status);
    }
}

void gemu_clear_error(gemu *gemu)
{
    u32 rx_status = gemu_read_reg(gemu->config.base_addr, GEM_RXSR_OFFSET);
    u32 tx_status = gemu_read_reg(gemu->config.base_addr, GEM_TXSR_OFFSET);

    rte_rmb();

    if (rx_status & GEM_RXSR_ERROR_MASK)
    {
        gemu_write_reg(gemu->config.base_addr, GEM_RXSR_OFFSET, rx_status);
    }

    if (tx_status & GEM_TXSR_ERROR_MASK)
    {
        gemu_write_reg(gemu->config.base_addr, GEM_TXSR_OFFSET, tx_status);
    }
}

void gemu_set_affinity(int core)
{
    pthread_t thread;
    cpu_set_t cpuset;
    int s;

    thread = pthread_self();

    // Zero out the cpuset
    CPU_ZERO(&cpuset);

    // Add CPU "core" to the cpuset
    CPU_SET(core, &cpuset);

    // Set the affinity of the thread to the cpuset
    s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0) {
        perror("pthread_setaffinity_np");
        exit(EXIT_FAILURE);
    }

    // Verify the affinity mask
    s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (s != 0) {
        perror("pthread_getaffinity_np");
        exit(EXIT_FAILURE);
    }

    printf("Thread %ld is running on CPU(s): ", (long)thread);
    for (int j = 0; j < CPU_SETSIZE; j++) {
        if (CPU_ISSET(j, &cpuset)) {
            printf("%d ", j);
        }
    }
}

int main(int argc, char **argv)
{
    LONG status;
    
    setvbuf(stdout, NULL, _IONBF, 0);

    int memfd;
    volatile void *gem0_mapped_base, *gem0_base_addr; 
    volatile void *gem1_mapped_base, *gem1_base_addr; 
    off_t gem0_dev_base = ZYNQMP_GEM_0_BASEADDR; 
    off_t gem1_dev_base = ZYNQMP_GEM_1_BASEADDR;
    gemu *gemu0= &gemu_dev_list[0];
    gemu *gemu1= &gemu_dev_list[1];
    
    memfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (memfd == -1) 
    {
        gemu_err("Can't open /dev/mem. %s\n", strerror(errno));
        exit(0);
    }

    gemu_log("/dev/mem opened.\n"); 
    _pagesize = getpagesize();

    gemu_init_hbds();

    gem0_mapped_base = mmap(0, GEM_DEV_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 
                            gem0_dev_base & ~(off_t)(_pagesize - 1));
    if (gem0_mapped_base == (void *) -1) 
    {
        gemu_err("Can't map the GEM0 memory IO regs to user space. %s\n", strerror(errno));
        exit(0);
    }
    gemu_log("GEM0 Memory mapped at addr %p.\n", gem0_mapped_base); 
 
    gem1_mapped_base = mmap(0, GEM_DEV_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 
                            gem1_dev_base & ~(off_t)(_pagesize - 1));
    if (gem1_mapped_base == (void *) -1) 
    {
        gemu_err("Can't map the GEM1 memory IO regs to user space. %s\n", strerror(errno));
        exit(0);
    }
    gemu_log("GEM1 Memory mapped at addr %p.\n", gem1_mapped_base); 

    gem0_base_addr = gem0_mapped_base + (gem0_dev_base & GEM_DEV_MAP_MASK);
    gem1_base_addr = gem1_mapped_base + (gem1_dev_base & GEM_DEV_MAP_MASK);

    gemu_log("GEM0 base addr %p GEM1 base addr %p\n", gem0_base_addr, gem1_base_addr); 

    gemu_mm_init(memfd);

    gemu_dev_configure(gemu0, 0, (UINTPTR)gem0_base_addr);
    gemu_dev_configure(gemu1, 1, (UINTPTR)gem1_base_addr);

    gemu_clear_error(gemu0);
    gemu_clear_error(gemu1);

    gemu_enable(gemu0);
    gemu_enable(gemu1);

    gemu_dump_desc(gemu0);
    gemu_dump_desc(gemu1);

    gemu_set_affinity(3);

    // main loop for tx/rx poll
    gemu_err("\nXXXX Main poll loop begin...\n");
    while (1)
    {
        gemu_rx_status(gemu0);

        gemu0->rx_vector_size = gemu0->tx_vector_size = 0;
        gemu1->rx_vector_size = gemu1->tx_vector_size = 0;

        gemu_update_rx_pi(gemu0);
        gemu_update_rx_pi(gemu1);
        gemu_rx_complete(gemu0);
        gemu_rx_complete(gemu1);
        gemu_process_rx_pkts(gemu0);
        gemu_process_rx_pkts(gemu1);
        gemu_process_tx_pkts(gemu0);
        gemu_process_tx_pkts(gemu1);
        gemu_refill_rx_bufs(gemu0);
        gemu_refill_rx_bufs(gemu1);

        gemu_update_tx_ci(gemu0);
        gemu_tx_complete(gemu0);
        gemu_update_tx_ci(gemu1);
        gemu_tx_complete(gemu1);

        // any error handling methods- or do in the manin thread
        gemu_handle_error(gemu0);
        gemu_handle_error(gemu1);
    }

    gemu_dump_stats(gemu0);
    gemu_dump_stats(gemu1);

    close(memfd);

    printf("Exiting GEMU...\n");
    return GEMU_SUCCESS;
}
