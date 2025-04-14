#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
  
#include "gemu.h"

// device should be disabled(tx/rx disable) before setting mac address 
int gemu_set_mac_addr(gemu *instance, MAC_ADDR mac_addr, u8 index)
{
    u32 mac_addr_32;
    void *gem_base_addr = (void *)instance->config.base_addr;

    /* Set the MAC bits [31:0] in BOT */
    mac_addr_32 = mac_addr[0];
    mac_addr_32 |= ((u32)(mac_addr[1]) << 8U);
    mac_addr_32 |= ((u32)(mac_addr[2]) << 16U);
    mac_addr_32 |= ((u32)(mac_addr[3]) << 24U);
    gemu_write_reg(gem_base_addr, ((u32)GEM_LADDR1L_OFFSET + ((u32)index * (u32)8)), mac_addr_32);

    /* There are reserved bits in TOP so don't affect them */
    mac_addr_32 = gemu_read_reg(gem_base_addr, ((u32)GEM_LADDR1H_OFFSET + ((u32)index * (u32)8)));

    mac_addr_32 &= (u32)(~GEM_LADDR_MACH_MASK);

    /* Set MAC bits [47:32] in TOP */
    mac_addr_32 |= (u32)(mac_addr[4]);
    mac_addr_32 |= (u32)(mac_addr[5] << 8U);

    gemu_write_reg(gem_base_addr, ((u32)GEM_LADDR1H_OFFSET + ((u32)index * (u32)8)), mac_addr_32);

    return 0;
}

void gemu_get_mac_addr(gemu *instance, MAC_ADDR mac_addr, u8 index)
{
    u32 mac_addr_32;
    void *gem_base_addr = (void *)instance->config.base_addr;

    mac_addr_32 = gemu_read_reg(gem_base_addr, ((u32)GEM_LADDR1L_OFFSET + ((u32)index * (u32)8)));
    mac_addr[0] = (u8) mac_addr_32;
    mac_addr[1] = (u8) (mac_addr_32 >> 8U);
    mac_addr[2] = (u8) (mac_addr_32 >> 16U);
    mac_addr[3] = (u8) (mac_addr_32 >> 24U);

    /* Read MAC bits [47:32] in TOP */
    mac_addr_32 = gemu_read_reg(gem_base_addr, ((u32)GEM_LADDR1H_OFFSET + ((u32)index * (u32)8)));
    mac_addr[4] = (u8) mac_addr_32;
    mac_addr[5] = (u8) (mac_addr_32 >> 8U);
}

void gemu_hw_reset_x(gemu *instance) 
{
    void *base_addr = (void *)instance->config.base_addr; 
    u32 regval;

    /* Disable the interrupts  */
    gemu_write_reg(base_addr,GEM_IDR_OFFSET,0x0U);

    /* Stop transmission,disable loopback and Stop tx and Rx engines */
    regval = gemu_read_reg(base_addr,GEM_NWCTRL_OFFSET);
    regval &= ~((u32)GEM_NWCTRL_TXEN_MASK|
                (u32)GEM_NWCTRL_RXEN_MASK|
                (u32)GEM_NWCTRL_HALTTX_MASK|
                (u32)GEM_NWCTRL_LOOPEN_MASK);
    /* Clear the statistic registers, flush the packets in DPRAM*/
    regval |= (GEM_NWCTRL_STATCLR_MASK|
                GEM_NWCTRL_FLUSH_DPRAM_MASK);
    gemu_write_reg(base_addr,GEM_NWCTRL_OFFSET,regval);
    /* Clear the interrupt status */
    gemu_write_reg(base_addr,GEM_ISR_OFFSET,GEM_IXR_ALL_MASK);
    /* Clear the tx status */
    gemu_write_reg(base_addr,GEM_TXSR_OFFSET,(GEM_TXSR_ERROR_MASK|
                          (u32)GEM_TXSR_TXCOMPL_MASK|
                        (u32)GEM_TXSR_TXGO_MASK));
    /* Clear the rx status */
    gemu_write_reg(base_addr,GEM_RXSR_OFFSET, GEM_RXSR_FRAMERX_MASK);
    /* Clear the tx base addr */
    gemu_write_reg(base_addr,GEM_TXQBASE_OFFSET,0x0U);
    /* Clear the rx base addr */
    gemu_write_reg(base_addr,GEM_RXQBASE_OFFSET,0x0U);
    /* Update the network config register with reset value */
    gemu_write_reg(base_addr,GEM_NWCFG_OFFSET,GEM_NWCFG_RESET_MASK);
    /* Update the hash addr registers with reset value */
    gemu_write_reg(base_addr,GEM_HASHL_OFFSET,0x0U);
    gemu_write_reg(base_addr,GEM_HASHH_OFFSET,0x0U);
}

u32 gemu_get_qx_offset(gemu_qx_reg_offset reg, u8 queue)
{
    u32 map[GEM_REG_END][GEM_MAX_QUEUES] = {
        { GEM_TXQBASE_OFFSET, GEM_TXQ1BASE_OFFSET }, /* TXQIBASE */
        { GEM_RXQBASE_OFFSET, GEM_RXQ1BASE_OFFSET }, /* RXQIBASE  */
        { 0, GEM_DMA_RXQ1_BUFSIZE_OFFSET }, /* DMA_RXQI_BUFSIZE */
        { GEM_ISR_OFFSET, GEM_INTQ1_STS_OFFSET }, /* INTQI_STS */
        { 0, GEM_INTQ1_IER_OFFSET },  /* INTQI_IER */
        { 0, GEM_INTQ1_IDR_OFFSET }}; /* INTQI_IDR */

    assert(reg < GEM_REG_END);
    assert(queue < GEM_MAX_QUEUES);

    return map[reg][queue];
}

