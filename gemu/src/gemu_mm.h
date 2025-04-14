#ifndef __GEMU_MM_H__
#define __GEMU_MM_H__

#include "gemu_types.h"

typedef u64 phys_addr_t;
typedef uint64_t rte_iova_t;

typedef enum gemu_buf_state
{
    GEMU_BUF_FREE = 0xA0A0,
    GEMU_BUF_ALLOCED,
    GEMU_BUF_IN_TX_BD,
    GEMU_BUF_IN_RX_BD,
    GEMU_BUF_IN_PROCESS, 
} gemu_buf_state;

typedef enum gemu_buf_type
{
    GEMU_BUF_TYPE_RX = 0xA0A0,
    GEMU_BUF_TYPE_TX = 0xA0B0,
} gemu_buf_type;

typedef struct  __attribute__((packed)) gemu_buf_hdr
{
    struct gemu     *gemu;
    gemu_buf_state  state;
    u32      pkt_len;
    UINTPTR  phys_buf_addr; 
    u32      buf_len;
    u32      buf_type;
    u32      pad[8];
}gemu_buf_hdr;

SIZE_ASSERT(gemu_buf_hdr, 64);

typedef struct gemu_buf
{
    gemu_buf_hdr buf_hdr;
    u8           data[1];    
} gemu_buf;

typedef struct buf_addr
{
    uint64_t virt_addr; 
    uint64_t phys_addr;
}buf_addr;

typedef struct bumem_stack 
{
    buf_addr   *bufs;
    int        num_bufs;
    int        cur_count;
    u32        bufs_popped;
    u32        bufs_pushed;
}bufmem_stack;

#define RTE_BAD_IOVA ((rte_iova_t)-1)

phys_addr_t rte_mem_virt2phy(volatile const void *virtaddr);

#define BUF_START_FROM_BD_BUF_ADDR(bd_buf_addr) ((bd_buf_addr)-sizeof(gemu_buf_hdr))
#define BUF_START_TO_BD_BUF_ADDR(buf_addr) ((buf_addr)+sizeof(gemu_buf_hdr)) 

void gemu_mm_init();
void *gemu_bdmem_alloc(u32 memsize);
void  gemu_bdmem_free(void *p, u32 memsize);


#endif // __GEMU_MM_H__
