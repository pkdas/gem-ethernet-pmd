#ifndef __GEMU_BD_H__
#define __GEMU_BD_H__

#include <string.h>

// PK FIXME - we have 64bit addresing #ifdef __aarch64__

/* Minimum BD alignment */
#define GEM_DMABD_MINIMUM_ALIGNMENT  64U

// timestamp capture mode is disabled. 64bit addring enabled
// Every descriptor is 128-bits wide when 64-bit addring is enabled and the descriptor timestamp capture mode is disabled.
#define GEM_BD_NUM_WORDS 4U

typedef u32 gemu_bd[GEM_BD_NUM_WORDS];

typedef struct __attribute__((packed)) gemu_bd_ex 
{
    u32 addr_lo; // w0
    u32 status;  // w1
    u32 addr_hi; // w2
    u16 bd_id;   // w3 unused
    u16 bufpool_id; // ..
} gemu_bd_ex;

typedef union gemu_desc 
{
    gemu_bd _bd;
    gemu_bd_ex bd;
} gemu_desc;

#define GEMU_HBD_TX 0x11
#define GEMU_HBD_RX 0xAA
#define GEMU_MAX_HBDS (8*1024) // max 8K descriptors allowed

#define GEMU_MAX_BDS  GEMU_MAX_HBDS
#define GEMU_RX_QUEUE_SIZE GEMU_MAX_BDS
#define GEMU_TX_QUEUE_SIZE GEMU_MAX_BDS

#define GEM_DESC_MEM_SIZE ((GEMU_MAX_BDS)*sizeof(gemu_desc))

#define GEMU_RX_VECTOR_SIZE   16 
#define GEMU_TX_VECTOR_SIZE   (GEMU_RX_VECTOR_SIZE*4) 

#define GEMU_VECTORS_IN_RX_QUEUE        GEMU_RX_QUEUE_SIZE/GEMU_RX_VECTOR_SIZE
#define GEMU_VECTORS_IN_TX_QUEUE        GEMU_TX_QUEUE_SIZE/GEMU_TX_VECTOR_SIZE 

typedef struct __attribute__((packed)) gemu_hbd
{
    CLIB_CACHE_LINE_ALIGN_MARK(cacheline0);
    u32     bd_id;
    u32     bufpool_id; 
    u32     type; // tx/rx 0x11 for TX 0xAA for RX
    volatile gemu_desc *desc;  // points to the tx/rx BD virt_bd/desc addr
    UINTPTR  phys_desc; //  phys_bd/desc addr
    UINTPTR  buf; // virt addr of the tx/rx buffer in the BD
    UINTPTR phys_buf_addr; // phys addr of tx/rx buffer in the BD
} gemu_hbd; // host-bd

#define GEM_RX_DESC_WRAP 0x2
#define GEM_TX_DESC_WRAP 0x40000000

#define gemu_bd_clear(bd_ptr)                                  \
    memset((bd_ptr), 0, sizeof(gemu_bd))

#define gemu_bd_read(base_addr, offset)             \
    (*(volatile u32 *)((UINTPTR)((void*)(base_addr)) + (u32)(offset)))

#define gemu_bd_write(base_addr, offset, data)              \
    (*(volatile u32 *)((UINTPTR)(void*)(base_addr) + (u32)(offset)) = (u32)(data))

#define gemu_bd_set_hbd_data(bd_ptr, data)                        \
    gemu_bd_write((bd_ptr), GEM_BD_HBD_DATA_OFFSET, data)         \

#define gemu_bd_get_bufpool_id(bd_ptr)                       \
    gemu_bd_read((bd_ptr), GEM_BD_BUFPOOL_ID_OFFSET)         \

#define gemu_bd_set_addr_tx(bd_ptr, addr)                        \
    gemu_bd_write((bd_ptr), GEM_BD_ADDR_OFFSET,        \
            (u32)((addr) & ULONG64_LO_MASK));        \
    gemu_bd_write((bd_ptr), GEM_BD_ADDR_HI_OFFSET,        \
    (u32)(((addr) & ULONG64_HI_MASK) >> 32U));

#define gemu_bd_set_addr_rx(bd_ptr, addr)                        \
    gemu_bd_write((bd_ptr), GEM_BD_ADDR_OFFSET,              \
    ((gemu_bd_read((bd_ptr), GEM_BD_ADDR_OFFSET) &           \
    ~GEM_RXBUF_ADD_MASK) | ((u32)((addr) & ULONG64_LO_MASK))));  \
    gemu_bd_write((bd_ptr), GEM_BD_ADDR_HI_OFFSET,     \
    (u32)(((addr) & ULONG64_HI_MASK) >> 32U));

#define gemu_bd_set_status(bd_ptr, data)                           \
    gemu_bd_write((bd_ptr), GEM_BD_STATUS_OFFSET,              \
    gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) | (data))


#define gemu_bd_get_status(bd_ptr)                                 \
    gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET)


#define gemu_bd_get_tx_buf_addr(bd_ptr)                            \
    ((gemu_bd_read((bd_ptr), GEM_BD_ADDR_OFFSET)) |          \
    ((u64)(gemu_bd_read((bd_ptr), GEM_BD_ADDR_HI_OFFSET)) << 32U))


#define gemu_bd_get_rx_buf_addr(bd_ptr)                            \
    ((gemu_bd_read((bd_ptr), GEM_BD_ADDR_OFFSET) & GEM_RXBUF_ADD_MASK) |          \
    ((u64)(gemu_bd_read((bd_ptr), GEM_BD_ADDR_HI_OFFSET)) << 32U))

#define gemu_bd_set_length(bd_ptr, len_bytes)                       \
    gemu_bd_write((bd_ptr), GEM_BD_STATUS_OFFSET,              \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    ~GEM_TXBUF_LEN_MASK) | (len_bytes)))


#define gemu_bd_get_length(bd_ptr)                                 \
    (gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &            \
    GEM_RXBUF_LEN_MASK)

#define gemu_get_rx_frame_size(instance, bd_ptr)                   \
    (gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &            \
    (instance)->rx_buf_mask)

#define gemu_bd_is_last(bd_ptr)                                    \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_RXBUF_EOF_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_set_last(bd_ptr)                                   \
    (gemu_bd_write((bd_ptr), GEM_BD_STATUS_OFFSET,             \
    gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) |             \
    GEM_TXBUF_LAST_MASK))


#define gemu_bd_clear_last(bd_ptr)                                 \
    (gemu_bd_write((bd_ptr), GEM_BD_STATUS_OFFSET,             \
    gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &             \
    ~GEM_TXBUF_LAST_MASK))


#define gemu_bd_is_rx_wrap(bd_ptr)                                  \
    ((gemu_bd_read((bd_ptr), GEM_BD_ADDR_OFFSET) &           \
    GEM_RXBUF_WRAP_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_tx_wrap(bd_ptr)                                  \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_TXBUF_WRAP_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_clear_rx_new(bd_ptr)                                \
    (gemu_bd_write((bd_ptr), GEM_BD_ADDR_OFFSET,             \
    gemu_bd_read((bd_ptr), GEM_BD_ADDR_OFFSET) &             \
    ~GEM_RXBUF_NEW_MASK))


#define gemu_bd_is_rx_new(bd_ptr)                                   \
    ((gemu_bd_read((bd_ptr), GEM_BD_ADDR_OFFSET) &           \
    GEM_RXBUF_NEW_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_set_tx_used(bd_ptr)                                 \
    (gemu_bd_write((bd_ptr), GEM_BD_STATUS_OFFSET,             \
    gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) |             \
    GEM_TXBUF_USED_MASK))


#define gemu_bd_clear_tx_used(bd_ptr)                               \
    (gemu_bd_write((bd_ptr), GEM_BD_STATUS_OFFSET,             \
    gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &             \
    ~GEM_TXBUF_USED_MASK))


#define gemu_bd_is_tx_used(bd_ptr)                                  \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_TXBUF_USED_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_tx_retry(bd_ptr)                                 \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_TXBUF_RETRY_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_tx_urun(bd_ptr)                                  \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_TXBUF_URUN_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_tx_exh(bd_ptr)                                   \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_TXBUF_EXH_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_set_tx_no_crc(bd_ptr)                                \
    (gemu_bd_write((bd_ptr), GEM_BD_STATUS_OFFSET,             \
    gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) |             \
    GEM_TXBUF_NOCRC_MASK))


#define gemu_bd_clear_tx_no_crc(bd_ptr)                              \
    (gemu_bd_write((bd_ptr), GEM_BD_STATUS_OFFSET,             \
    gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &             \
    ~GEM_TXBUF_NOCRC_MASK))


#define gemu_bd_is_rx_bcast(bd_ptr)                                 \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_RXBUF_BCAST_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_rx_multi_hash(bd_ptr)                             \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_RXBUF_MULTIHASH_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_rx_uni_hash(bd_ptr)                               \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_RXBUF_UNIHASH_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_rx_vlan(bd_ptr)                                  \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_RXBUF_VLAN_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_rx_pri(bd_ptr)                                   \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_RXBUF_PRI_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_rx_cfi(bd_ptr)                                   \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_RXBUF_CFI_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_rx_eof(bd_ptr)                                   \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_RXBUF_EOF_MASK)!=0U ? TRUE : FALSE)


#define gemu_bd_is_rx_sof(bd_ptr)                                   \
    ((gemu_bd_read((bd_ptr), GEM_BD_STATUS_OFFSET) &           \
    GEM_RXBUF_SOF_MASK)!=0U ? TRUE : FALSE)


#endif // __GEMU_BD_H__
