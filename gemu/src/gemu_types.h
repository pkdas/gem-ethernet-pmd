#ifndef __GEMU_TYPES_H__
#define __GEMU_TYPES_H__

typedef unsigned char u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;
typedef long          LONG;
typedef unsigned long ULONG;
typedef uint64_t      UINTPTR;
typedef int64_t       INTPTR;
typedef int32_t          s32;

#define CLIB_LOG2_CACHE_LINE_BYTES   6
#define CLIB_CACHE_LINE_BYTES   (1 << CLIB_LOG2_CACHE_LINE_BYTES)
#define CLIB_CACHE_LINE_ALIGN_MARK(mark)   u8 mark[0] __attribute__((aligned(CLIB_CACHE_LINE_BYTES)))
#define CLIB_N_PREFETCHES   16
#define CLIB_PREFETCH_READ   0
#define CLIB_PREFETCH_LOAD   0 /* alias for read */
#define CLIB_PREFETCH_WRITE   1
#define CLIB_PREFETCH_STORE   1 /* alias for write */
#define CLIB_PREFETCH(addr, size, type)

#define MAC_ADDR_LEN  6
typedef u8            MAC_ADDR[MAC_ADDR_LEN];

#define ULONG64_HI_MASK    0xFFFFFFFF00000000U
#define ULONG64_LO_MASK    ~ULONG64_HI_MASK

#ifndef SIZE_ASSERT
#define SIZE_ASSERT( what, howmuch ) \
  typedef char what##_size_wrong_[( !!(sizeof(what) == howmuch) )*2-1 ]
#endif

#define ALWAYS_INLINE inline __attribute__((always_inline))

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/************************** Constant Definitions *****************************/

#define GEM_MAX_MAC_ADDR     4U   /**< Maxmum number of mac addr
                                           supported */
#define GEM_MAX_TYPE_ID      4U   /**< Maxmum number of type id supported */

#define GEM_BD_ALIGNMENT     64U   /**< Minimum buffer descriptor alignment
                                           on the local bus */
#define GEM_RX_BUF_ALIGNMENT 4U   /**< Minimum buffer alignment when using
                                           options that impose alignment
                                           restrictions on the buffer data on
                                           the local bus */

#define GEM_RX_BUF_SIZE 1536U /**< Specify the receive buffer size in
                                       bytes, 64, 128, ... 10240 */
#define GEM_RX_BUF_SIZE_JUMBO 10240U

#define GEM_RX_BUF_UNIT   64U /**< Number of receive buffer bytes as a
                                       unit, this is HW setup */

#define GEM_MAX_HASH_BITS 64U /**< Maximum value for hash bits. 2**6 */

/* Register offset definitions. Unless otherwise noted, register access is
 * 32 bit. Names are self explained here.
 */

#define GEM_NWCTRL_OFFSET        0x00000000U /**< Network Control reg */
#define GEM_NWCFG_OFFSET         0x00000004U /**< Network Config reg */
#define GEM_NWSR_OFFSET          0x00000008U /**< Network Status reg */

#define GEM_DMACR_OFFSET         0x00000010U /**< DMA Control reg */
#define GEM_TXSR_OFFSET          0x00000014U /**< TX Status reg */
#define GEM_RXQBASE_OFFSET       0x00000018U /**< RX Q Base addr reg */
#define GEM_TXQBASE_OFFSET       0x0000001CU /**< TX Q Base addr reg */
#define GEM_RXSR_OFFSET          0x00000020U /**< RX Status reg */

#define GEM_ISR_OFFSET           0x00000024U /**< Interrupt Status reg */
#define GEM_IER_OFFSET           0x00000028U /**< Interrupt Enable reg */
#define GEM_IDR_OFFSET           0x0000002CU /**< Interrupt Disable reg */
#define GEM_IMR_OFFSET           0x00000030U /**< Interrupt Mask reg */

#define GEM_PHYMNTNC_OFFSET      0x00000034U /**< Phy Maintaince reg */
#define GEM_RXPAUSE_OFFSET       0x00000038U /**< RX Pause Time reg */
#define GEM_TXPAUSE_OFFSET       0x0000003CU /**< TX Pause Time reg */

#define GEM_JUMBOMAXLEN_OFFSET   0x00000048U /**< Jumbo max length reg */

#define GEM_RXWATERMARK_OFFSET   0x0000007CU /**< RX watermark reg */

#define GEM_HASHL_OFFSET         0x00000080U /**< Hash Low addr reg */
#define GEM_HASHH_OFFSET         0x00000084U /**< Hash High addr reg */

#define GEM_LADDR1L_OFFSET       0x00000088U /**< Specific1 addr low reg */
#define GEM_LADDR1H_OFFSET       0x0000008CU /**< Specific1 addr high reg */
#define GEM_LADDR2L_OFFSET       0x00000090U /**< Specific2 addr low reg */
#define GEM_LADDR2H_OFFSET       0x00000094U /**< Specific2 addr high reg */
#define GEM_LADDR3L_OFFSET       0x00000098U /**< Specific3 addr low reg */
#define GEM_LADDR3H_OFFSET       0x0000009CU /**< Specific3 addr high reg */
#define GEM_LADDR4L_OFFSET       0x000000A0U /**< Specific4 addr low reg */
#define GEM_LADDR4H_OFFSET       0x000000A4U /**< Specific4 addr high reg */

#define GEM_MATCH1_OFFSET        0x000000A8U /**< Type ID1 Match reg */
#define GEM_MATCH2_OFFSET        0x000000ACU /**< Type ID2 Match reg */
#define GEM_MATCH3_OFFSET        0x000000B0U /**< Type ID3 Match reg */
#define GEM_MATCH4_OFFSET        0x000000B4U /**< Type ID4 Match reg */

#define GEM_STRETCH_OFFSET       0x000000BCU /**< IPG Stretch reg */

#define GEM_OCTTXL_OFFSET        0x00000100U /**< Octects transmitted Low
                                                      reg */
#define GEM_OCTTXH_OFFSET        0x00000104U /**< Octects transmitted High
                                                      reg */

#define GEM_TXCNT_OFFSET         0x00000108U /**< Error-free Frmaes
                                                      transmitted counter */
#define GEM_TXBCCNT_OFFSET       0x0000010CU /**< Error-free Broadcast
                                                      Frames counter*/
#define GEM_TXMCCNT_OFFSET       0x00000110U /**< Error-free Multicast
                                                      Frame counter */
#define GEM_TXPAUSECNT_OFFSET    0x00000114U /**< Pause Frames Transmitted
                                                      Counter */
#define GEM_TX64CNT_OFFSET       0x00000118U /**< Error-free 64 byte Frames
                                                      Transmitted counter */
#define GEM_TX65CNT_OFFSET       0x0000011CU /**< Error-free 65-127 byte
                                                      Frames Transmitted
                                                      counter */
#define GEM_TX128CNT_OFFSET      0x00000120U /**< Error-free 128-255 byte
                                                      Frames Transmitted
                                                      counter*/
#define GEM_TX256CNT_OFFSET      0x00000124U /**< Error-free 256-511 byte
                                                      Frames transmitted
                                                      counter */
#define GEM_TX512CNT_OFFSET      0x00000128U /**< Error-free 512-1023 byte
                                                      Frames transmitted
                                                      counter */
#define GEM_TX1024CNT_OFFSET     0x0000012CU /**< Error-free 1024-1518 byte
                                                      Frames transmitted
                                                      counter */
#define GEM_TX1519CNT_OFFSET     0x00000130U /**< Error-free larger than
                                                      1519 byte Frames
                                                      transmitted counter */
#define GEM_TXURUNCNT_OFFSET     0x00000134U /**< TX under run error
                                                      counter */

#define GEM_SNGLCOLLCNT_OFFSET   0x00000138U /**< Single Collision Frame
                                                      Counter */
#define GEM_MULTICOLLCNT_OFFSET  0x0000013CU /**< Multiple Collision Frame
                                                      Counter */
#define GEM_EXCESSCOLLCNT_OFFSET 0x00000140U /**< Excessive Collision Frame
                                                      Counter */
#define GEM_LATECOLLCNT_OFFSET   0x00000144U /**< Late Collision Frame
                                                      Counter */
#define GEM_TXDEFERCNT_OFFSET    0x00000148U /**< Deferred Transmission
                                                      Frame Counter */
#define GEM_TXCSENSECNT_OFFSET   0x0000014CU /**< Transmit Carrier Sense
                                                      Error Counter */

#define GEM_OCTRXL_OFFSET        0x00000150U /**< Octects Received register
                                                      Low */
#define GEM_OCTRXH_OFFSET        0x00000154U /**< Octects Received register
                                                      High */

#define GEM_RXCNT_OFFSET         0x00000158U /**< Error-free Frames
                                                      Received Counter */
#define GEM_RXBROADCNT_OFFSET    0x0000015CU /**< Error-free Broadcast
                                                      Frames Received Counter */
#define GEM_RXMULTICNT_OFFSET    0x00000160U /**< Error-free Multicast
                                                      Frames Received Counter */
#define GEM_RXPAUSECNT_OFFSET    0x00000164U /**< Pause Frames
                                                      Received Counter */
#define GEM_RX64CNT_OFFSET       0x00000168U /**< Error-free 64 byte Frames
                                                      Received Counter */
#define GEM_RX65CNT_OFFSET       0x0000016CU /**< Error-free 65-127 byte
                                                      Frames Received Counter */
#define GEM_RX128CNT_OFFSET      0x00000170U /**< Error-free 128-255 byte
                                                      Frames Received Counter */
#define GEM_RX256CNT_OFFSET      0x00000174U /**< Error-free 256-512 byte
                                                      Frames Received Counter */
#define GEM_RX512CNT_OFFSET      0x00000178U /**< Error-free 512-1023 byte
                                                      Frames Received Counter */
#define GEM_RX1024CNT_OFFSET     0x0000017CU /**< Error-free 1024-1518 byte
                                                      Frames Received Counter */
#define GEM_RX1519CNT_OFFSET     0x00000180U /**< Error-free 1519-max byte
                                                      Frames Received Counter */
#define GEM_RXUNDRCNT_OFFSET     0x00000184U /**< Undersize Frames Received
                                                      Counter */
#define GEM_RXOVRCNT_OFFSET      0x00000188U /**< Oversize Frames Received
                                                      Counter */
#define GEM_RXJABCNT_OFFSET      0x0000018CU /**< Jabbers Received
                                                      Counter */
#define GEM_RXFCSCNT_OFFSET      0x00000190U /**< Frame Check Sequence
                                                      Error Counter */
#define GEM_RXLENGTHCNT_OFFSET   0x00000194U /**< Length Field Error
                                                      Counter */
#define GEM_RXSYMBCNT_OFFSET     0x00000198U /**< Symbol Error Counter */
#define GEM_RXALIGNCNT_OFFSET    0x0000019CU /**< Alignment Error Counter */
#define GEM_RXRESERRCNT_OFFSET   0x000001A0U /**< Receive Resource Error
                                                      Counter */
#define GEM_RXORCNT_OFFSET       0x000001A4U /**< Receive Overrun Counter */
#define GEM_RXIPCCNT_OFFSET      0x000001A8U /**< IP header Checksum Error
                                                      Counter */
#define GEM_RXTCPCCNT_OFFSET     0x000001ACU /**< TCP Checksum Error
                                                      Counter */
#define GEM_RXUDPCCNT_OFFSET     0x000001B0U /**< UDP Checksum Error
                                                      Counter */
#define GEM_LAST_OFFSET          0x000001B4U /**< Last statistic counter
                              offset, for clearing */

#define GEM_1588_SEC_OFFSET      0x000001D0U /**< 1588 second counter */
#define GEM_1588_NANOSEC_OFFSET  0x000001D4U /**< 1588 nanosecond counter */
#define GEM_1588_ADJ_OFFSET      0x000001D8U /**< 1588 nanosecond
                              adjustment counter */
#define GEM_1588_INC_OFFSET      0x000001DCU /**< 1588 nanosecond
                              increment counter */
#define GEM_PTP_TXSEC_OFFSET     0x000001E0U /**< 1588 PTP transmit second
                              counter */
#define GEM_PTP_TXNANOSEC_OFFSET 0x000001E4U /**< 1588 PTP transmit
                              nanosecond counter */
#define GEM_PTP_RXSEC_OFFSET     0x000001E8U /**< 1588 PTP receive second
                              counter */
#define GEM_PTP_RXNANOSEC_OFFSET 0x000001ECU /**< 1588 PTP receive
                              nanosecond counter */
#define GEM_PTPP_TXSEC_OFFSET    0x000001F0U /**< 1588 PTP peer transmit
                              second counter */
#define GEM_PTPP_TXNANOSEC_OFFSET 0x000001F4U /**< 1588 PTP peer transmit
                              nanosecond counter */
#define GEM_PTPP_RXSEC_OFFSET    0x000001F8U /**< 1588 PTP peer receive
                              second counter */
#define GEM_PTPP_RXNANOSEC_OFFSET 0x000001FCU /**< 1588 PTP peer receive
                              nanosecond counter */
#define GEM_PCS_CONTROL_OFFSET    0x00000200U /** PCS control register */
#define GEM_PCS_STATUS_OFFSET    0x00000204U /** PCS status register */

#define GEM_DCFG6_OFFSET        0x00000294U /** designcfg_debug6 register */

#define GEM_INTQ1_STS_OFFSET     0x00000400U /**< Interrupt Q1 Status
                            reg */
#define GEM_TXQ1BASE_OFFSET         0x00000440U /**< TX Q1 Base addr
                            reg */
#define GEM_RXQ1BASE_OFFSET         0x00000480U /**< RX Q1 Base addr
                            reg */
#define GEM_DMA_RXQ1_BUFSIZE_OFFSET    0x000004A0U /**< RX Q1 DMA buffer size
                            reg */
#define GEM_MSBBUF_TXQBASE_OFFSET  0x000004C8U /**< MSB Buffer TX Q Base
                            reg */
#define GEM_MSBBUF_RXQBASE_OFFSET  0x000004D4U /**< MSB Buffer RX Q Base
                            reg */
#define GEM_SCREEN_TYPE2_REG0       0x00000540U /** Screening Type2 Reg0 **/

#define GEM_INTQ1_IER_OFFSET     0x00000600U /**< Interrupt Q1 Enable
                            reg */
#define GEM_INTQ1_IDR_OFFSET     0x00000620U /**< Interrupt Q1 Disable
                            reg */
#define GEM_INTQ1_IMR_OFFSET     0x00000640U /**< Interrupt Q1 Mask
                            reg */

// Define some bit positions for registers. 

// network control register bit definitions
#define GEM_NWCTRL_FLUSH_DPRAM_MASK    0x00040000U /**< Flush a packet from
                            Rx SRAM */
#define GEM_NWCTRL_ZEROPAUSETX_MASK 0x00000800U /**< Transmit zero quantum
                                                         pause frame */
#define GEM_NWCTRL_PAUSETX_MASK     0x00000800U /**< Transmit pause frame */
#define GEM_NWCTRL_HALTTX_MASK      0x00000400U /**< Halt transmission
                                                         after current frame */
#define GEM_NWCTRL_STARTTX_MASK     0x00000200U /**< Start tx (tx_go) */

#define GEM_NWCTRL_STATWEN_MASK     0x00000080U /**< Enable writing to
                                                         stat counters */
#define GEM_NWCTRL_STATINC_MASK     0x00000040U /**< Increment statistic
                                                         registers */
#define GEM_NWCTRL_STATCLR_MASK     0x00000020U /**< Clear statistic
                                                         registers */
#define GEM_NWCTRL_MDEN_MASK        0x00000010U /**< Enable MDIO port */
#define GEM_NWCTRL_TXEN_MASK        0x00000008U /**< Enable transmit */
#define GEM_NWCTRL_RXEN_MASK        0x00000004U /**< Enable receive */
#define GEM_NWCTRL_LOOPEN_MASK      0x00000002U /**< local loopback */

// network configuration register bit definitions
#define GEM_NWCFG_BADPREAMBEN_MASK 0x20000000U /**< disable rejection of
                                                        non-standard preamble */
#define GEM_NWCFG_IPDSTRETCH_MASK  0x10000000U /**< enable transmit IPG */
#define GEM_NWCFG_SGMIIEN_MASK     0x08000000U /**< SGMII Enable */
#define GEM_NWCFG_FCSIGNORE_MASK   0x04000000U /**< disable rejection of
                                                        FCS error */
#define GEM_NWCFG_HDRXEN_MASK      0x02000000U /**< RX half duplex */
#define GEM_NWCFG_RXCHKSUMEN_MASK  0x01000000U /**< enable RX checksum
                                                        offload */
#define GEM_NWCFG_PAUSECOPYDI_MASK 0x00800000U /**< Do not copy pause
                                                        Frames to memory */
#define GEM_NWCFG_DWIDTH_64_MASK   0x00200000U /**< 64 bit Data bus width */
#define GEM_NWCFG_MDC_SHIFT_MASK   18U       /**< shift bits for MDC */
#define GEM_NWCFG_MDCCLKDIV_MASK   0x001C0000U /**< MDC Mask PCLK divisor */
#define GEM_NWCFG_FCSREM_MASK      0x00020000U /**< Discard FCS from
                                                        received frames */
#define GEM_NWCFG_LENERRDSCRD_MASK 0x00010000U
/**< RX length error discard */
#define GEM_NWCFG_RXOFFS_MASK      0x0000C000U /**< RX buffer offset */
#define GEM_NWCFG_PAUSEEN_MASK     0x00002000U /**< Enable pause RX */
#define GEM_NWCFG_RETRYTESTEN_MASK 0x00001000U /**< Retry test */
#define GEM_NWCFG_XTADDMACHEN_MASK 0x00000200U
/**< External addr match enable */
#define GEM_NWCFG_PCSSEL_MASK      0x00000800U /**< PCS Select */
#define GEM_NWCFG_1000_MASK        0x00000400U /**< 1000 Mbps */
#define GEM_NWCFG_1536RXEN_MASK    0x00000100U /**< Enable 1536 byte
                                                        frames reception */
#define GEM_NWCFG_UCASTHASHEN_MASK 0x00000080U /**< Receive unicast hash
                                                        frames */
#define GEM_NWCFG_MCASTHASHEN_MASK 0x00000040U /**< Receive multicast hash
                                                        frames */
#define GEM_NWCFG_BCASTDI_MASK     0x00000020U /**< Do not receive
                                                        broadcast frames */
#define GEM_NWCFG_COPYALLEN_MASK   0x00000010U /**< Copy all frames */
#define GEM_NWCFG_JUMBO_MASK       0x00000008U /**< Jumbo frames */
#define GEM_NWCFG_NVLANDISC_MASK   0x00000004U /**< Receive only VLAN
                                                        frames */
#define GEM_NWCFG_FDEN_MASK        0x00000002U/**< full duplex */
#define GEM_NWCFG_100_MASK         0x00000001U /**< 100 Mbps */
#define GEM_NWCFG_RESET_MASK       0x00080000U/**< reset value */

// network status register bit definitaions
#define GEM_NWSR_MDIOIDLE_MASK     0x00000004U /**< PHY management idle */
#define GEM_NWSR_MDIO_MASK         0x00000002U /**< Status of mdio_in */


// MAC addr register word 1 mask
#define GEM_LADDR_MACH_MASK        0x0000FFFFU /**< Address bits[47:32]
                                                      bit[31:0] are in BOTTOM */


// DMA control register bit definitions
#define GEM_DMACR_ADDR_WIDTH_64        0x40000000U /**< 64 bit addr bus */
#define GEM_DMACR_TXEXTEND_MASK        0x20000000U /**< Tx Extended desc mode */
#define GEM_DMACR_RXEXTEND_MASK        0x10000000U /**< Rx Extended desc mode */
#define GEM_DMACR_RXBUF_MASK        0x00FF0000U /**< Mask bit for RX buffer
                                                    size */
#define GEM_DMACR_RXBUF_SHIFT         16U    /**< Shift bit for RX buffer
                                                size */
#define GEM_DMACR_TCPCKSUM_MASK        0x00000800U /**< enable/disable TX
                                                        checksum offload */
#define GEM_DMACR_TXSIZE_MASK        0x00000400U /**< TX buffer memory size */
#define GEM_DMACR_RXSIZE_MASK        0x00000300U /**< RX buffer memory size */
#define GEM_DMACR_ENDIAN_MASK        0x00000080U /**< endian configuration */
#define GEM_DMACR_BLENGTH_MASK        0x0000001FU /**< buffer burst length */
#define GEM_DMACR_SINGLE_AHB_BURST    0x00000001U /**< single AHB bursts */
#define GEM_DMACR_INCR4_AHB_BURST    0x00000004U /**< 4 bytes AHB bursts */
#define GEM_DMACR_INCR8_AHB_BURST    0x00000008U /**< 8 bytes AHB bursts */
#define GEM_DMACR_INCR16_AHB_BURST    0x00000010U /**< 16 bytes AHB bursts */

// transmit status register bit definitions
#define GEM_TXSR_HRESPNOK_MASK    0x00000100U /**< Transmit hresp not OK */
#define GEM_TXSR_URUN_MASK        0x00000040U /**< Transmit underrun */
#define GEM_TXSR_TXCOMPL_MASK     0x00000020U /**< Transmit completed OK */
#define GEM_TXSR_BUFEXH_MASK      0x00000010U /**< Transmit buffs exhausted
                                                       mid frame */
#define GEM_TXSR_TXGO_MASK        0x00000008U /**< Status of go flag */
#define GEM_TXSR_RXOVR_MASK       0x00000004U /**< Retry limit exceeded */
#define GEM_TXSR_FRAMERX_MASK     0x00000002U /**< Collision tx frame */
#define GEM_TXSR_USEDREAD_MASK    0x00000001U /**< TX buffer used bit set */

#define GEM_TXSR_ERROR_MASK      ((u32)GEM_TXSR_HRESPNOK_MASK | \
                                       (u32)GEM_TXSR_URUN_MASK | \
                                       (u32)GEM_TXSR_BUFEXH_MASK | \
                                       (u32)GEM_TXSR_RXOVR_MASK | \
                                       (u32)GEM_TXSR_FRAMERX_MASK | \
                                       (u32)GEM_TXSR_USEDREAD_MASK)
// receive status register bit definitions
#define GEM_RXSR_HRESPNOK_MASK    0x00000008U /**< Receive hresp not OK */
#define GEM_RXSR_RXOVR_MASK       0x00000004U /**< Receive overrun */
#define GEM_RXSR_FRAMERX_MASK     0x00000002U /**< Frame received OK */
#define GEM_RXSR_BUFFNA_MASK      0x00000001U /**< RX buffer used bit set */

#define GEM_RXSR_ERROR_MASK      ((u32)GEM_RXSR_HRESPNOK_MASK | \
                                       (u32)GEM_RXSR_RXOVR_MASK | \
                                       (u32)GEM_RXSR_BUFFNA_MASK)

#define GEM_SR_ALL_MASK    0xFFFFFFFFU /**< Mask for full register */

//PCS control register bit definitions
#define GEM_PCS_CON_AUTO_NEG_MASK    0x00001000U /**< Auto-negotiation */

// PCS status register bit definitions
#define GEM_PCS_STATUS_LINK_STATUS_MASK    0x00000004U /**< Link status */

// Interrupt Q1 status register bit definitions
#define GEM_INTQ1SR_TXCOMPL_MASK    0x00000080U /**< Transmit completed OK */
#define GEM_INTQ1SR_TXERR_MASK            0x00000040U /**< Transmit AMBA Error */
#define GEM_INTQ1SR_RXCOMPL_MASK    0x00000002U /**< Receive completed OK */

#define GEM_INTQ1_IXR_ALL_MASK            ((u32)GEM_INTQ1SR_TXCOMPL_MASK | \
                     (u32)GEM_INTQ1SR_TXERR_MASK | \
                                         (u32)GEM_INTQ1SR_RXCOMPL_MASK)

#define GEM_INTQ1_IXR_TX_MASK    ((u32)GEM_INTQ1SR_TXCOMPL_MASK | \
                     (u32)GEM_INTQ1SR_TXERR_MASK)
#define GEM_INTQ1_IXR_RX_MASK    (u32)GEM_INTQ1SR_RXCOMPL_MASK

/* Common Masks for all Queues */
#define GEM_INTQISR_RXCOMPL_MASK    GEM_INTQ1SR_RXCOMPL_MASK
#define GEM_INTQISR_TXCOMPL_MASK    GEM_INTQ1SR_TXCOMPL_MASK
#define GEM_INTQSR_TXERR_MASK            GEM_INTQ1SR_TXERR_MASK
#define GEM_INTQ_IXR_ALL_MASK        GEM_INTQ1_IXR_ALL_MASK


// interrupts bit definitions
// Bits definitions are same in GEM_ISR_OFFSET,
// GEM_IER_OFFSET, GEM_IDR_OFFSET, and GEM_IMR_OFFSET
#define GEM_IXR_PTPPSTX_MASK    0x02000000U /**< PTP Pdelay_resp TXed */
#define GEM_IXR_PTPPDRTX_MASK    0x01000000U /**< PTP Pdelay_req TXed */
#define GEM_IXR_PTPPSRX_MASK    0x00800000U /**< PTP Pdelay_resp RXed */
#define GEM_IXR_PTPPDRRX_MASK    0x00400000U /**< PTP Pdelay_req RXed */

#define GEM_IXR_PTPSTX_MASK        0x00200000U /**< PTP Sync TXed */
#define GEM_IXR_PTPDRTX_MASK    0x00100000U /**< PTP Delay_req TXed */
#define GEM_IXR_PTPSRX_MASK        0x00080000U /**< PTP Sync RXed */
#define GEM_IXR_PTPDRRX_MASK    0x00040000U /**< PTP Delay_req RXed */

#define GEM_IXR_PAUSETX_MASK    0x00004000U    /**< Pause frame transmitted */
#define GEM_IXR_PAUSEZERO_MASK  0x00002000U    /**< Pause time has reached
                                                     zero */
#define GEM_IXR_PAUSENZERO_MASK 0x00001000U    /**< Pause frame received */
#define GEM_IXR_HRESPNOK_MASK   0x00000800U    /**< hresp not ok */
#define GEM_IXR_RXOVR_MASK      0x00000400U    /**< Receive overrun occurred */
#define GEM_IXR_TXCOMPL_MASK    0x00000080U    /**< Frame transmitted ok */
#define GEM_IXR_TXEXH_MASK      0x00000040U    /**< Transmit err occurred or
                                                     no buffers*/
#define GEM_IXR_RETRY_MASK      0x00000020U    /**< Retry limit exceeded */
#define GEM_IXR_URUN_MASK       0x00000010U    /**< Transmit underrun */
#define GEM_IXR_TXUSED_MASK     0x00000008U    /**< Tx buffer used bit read */
#define GEM_IXR_RXUSED_MASK     0x00000004U    /**< Rx buffer used bit read */
#define GEM_IXR_FRAMERX_MASK    0x00000002U    /**< Frame received ok */
#define GEM_IXR_MGMNT_MASK      0x00000001U    /**< PHY management complete */
#define GEM_IXR_ALL_MASK        0x00007FFFU    /**< Everything! */

#define GEM_IXR_TX_ERR_MASK    ((u32)GEM_IXR_TXEXH_MASK |         \
                                     (u32)GEM_IXR_RETRY_MASK |         \
                                     (u32)GEM_IXR_URUN_MASK)


#define GEM_IXR_RX_ERR_MASK    ((u32)GEM_IXR_HRESPNOK_MASK |      \
                                     (u32)GEM_IXR_RXUSED_MASK |        \
                                     (u32)GEM_IXR_RXOVR_MASK)

// PHY Maintenance bit definitions
#define GEM_PHYMNTNC_OP_MASK    0x40020000U    /**< operation mask bits */
#define GEM_PHYMNTNC_OP_R_MASK  0x20000000U    /**< read operation */
#define GEM_PHYMNTNC_OP_W_MASK  0x10000000U    /**< write operation */
#define GEM_PHYMNTNC_ADDR_MASK  0x0F800000U    /**< Address bits */
#define GEM_PHYMNTNC_REG_MASK   0x007C0000U    /**< register bits */
#define GEM_PHYMNTNC_DATA_MASK  0x00000FFFU    /**< data bits */
#define GEM_PHYMNTNC_PHAD_SHFT_MSK   23U    /**< Shift bits for PHYAD */
#define GEM_PHYMNTNC_PREG_SHFT_MSK   18U    /**< Shift bits for PHREG */

// RX watermark bit definitions
#define GEM_RXWM_HIGH_MASK        0x0000FFFFU    /**< RXWM high mask */
#define GEM_RXWM_LOW_MASK        0xFFFF0000U    /**< RXWM low mask */
#define GEM_RXWM_LOW_SHFT_MSK    16U    /**< Shift for RXWM low */

///Screening Type2 bit definitions
#define GEM_CMPA_ENABLE_MASK    0x00040000U    /**< Enable Compare A */
#define GEM_CMPA_MASK        0x0003E000U    /**< Compare reg number */

// Transmit buffer descriptor status words offset
#define GEM_BD_ADDR_OFFSET  0x00000000U /**< word 0/addr of BDs */
#define GEM_BD_STATUS_OFFSET  0x00000004U /**< word 1/status of BDs */
#define GEM_BD_ADDR_HI_OFFSET  0x00000008U /**< word 2/addr of BDs */
#define GEM_BD_ADDR_UNUSED_OFFSET 0x0000000CU 
#define GEM_BD_HBD_DATA_OFFSET GEM_BD_ADDR_UNUSED_OFFSET // w3

/* Transmit buffer descriptor status words bit positions.
 * Transmit buffer descriptor consists of two 32-bit registers,
 * the first - word0 contains a 32-bit addr pointing to the location of
 * the transmit data.
 * The following register - word1, consists of various information to control
 * the gemu transmit process.  After transmit, this is updated with status
 * information, whether the frame was transmitted OK or why it had failed.
 */
#define GEM_TXBUF_USED_MASK  0x80000000U /**< Used bit. */
#define GEM_TXBUF_WRAP_MASK  0x40000000U /**< Wrap bit, last descriptor */
#define GEM_TXBUF_RETRY_MASK 0x20000000U /**< Retry limit exceeded */
#define GEM_TXBUF_URUN_MASK  0x10000000U /**< Transmit underrun occurred */
#define GEM_TXBUF_EXH_MASK   0x08000000U /**< Buffers exhausted */
#define GEM_TXBUF_TCP_MASK   0x04000000U /**< Late collision. */
#define GEM_TXBUF_NOCRC_MASK 0x00010000U /**< No CRC */
#define GEM_TXBUF_LAST_MASK  0x00008000U /**< Last buffer */
#define GEM_TXBUF_LEN_MASK   0x00003FFFU /**< Mask for length field */

/* Receive buffer descriptor status words bit positions.
 * Receive buffer descriptor consists of two 32-bit registers,
 * the first - word0 contains a 32-bit word aligned addr pointing to the
 * addr of the buffer. The lower two bits make up the wrap bit indicating
 * the last descriptor and the ownership bit to indicate it has been used by
 * the XEmacPs.
 * The following register - word1, contains status information regarding why
 * the frame was received (the filter match condition) as well as other
 * useful info.
 */
#define GEM_RXBUF_BCAST_MASK     0x80000000U /**< Broadcast frame */
#define GEM_RXBUF_MULTIHASH_MASK 0x40000000U /**< Multicast hashed frame */
#define GEM_RXBUF_UNIHASH_MASK   0x20000000U /**< Unicast hashed frame */
#define GEM_RXBUF_EXH_MASK       0x08000000U /**< buffer exhausted */
#define GEM_RXBUF_AMATCH_MASK    0x06000000U /**< Specific addr
                                                      matched */
#define GEM_RXBUF_IDFOUND_MASK   0x01000000U /**< Type ID matched */
#define GEM_RXBUF_IDMATCH_MASK   0x00C00000U /**< ID matched mask */
#define GEM_RXBUF_VLAN_MASK      0x00200000U /**< VLAN tagged */
#define GEM_RXBUF_PRI_MASK       0x00100000U /**< Priority tagged */
#define GEM_RXBUF_VPRI_MASK      0x000E0000U /**< Vlan priority */
#define GEM_RXBUF_CFI_MASK       0x00010000U /**< CFI frame */
#define GEM_RXBUF_EOF_MASK       0x00008000U /**< End of frame. */
#define GEM_RXBUF_SOF_MASK       0x00004000U /**< Start of frame. */
#define GEM_RXBUF_LEN_MASK       0x00001FFFU /**< Mask for length field */
#define GEM_RXBUF_LEN_JUMBO_MASK 0x00003FFFU /**< Mask for jumbo length */

#define GEM_RXBUF_WRAP_MASK      0x00000002U /**< Wrap bit, last BD */
#define GEM_RXBUF_NEW_MASK       0x00000001U /**< Used bit.. */
#define GEM_RXBUF_ADD_MASK       0xFFFFFFFCU /**< Mask for addr */

// Queue Registers Offset
#define GEM_MAX_QUEUES          2 

typedef enum { TXQIBASE = 0U, RXQIBASE, DMA_RXQI_BUFSIZE,
           INTQI_STS, INTQI_IER, INTQI_IDR, GEM_REG_END
} gemu_qx_reg_offset;

#ifdef GEMU_DEBUG
#define gemu_assert(cond)  assert(cond)
#else
#define gemu_assert(cond)
#endif

static ALWAYS_INLINE u8 gemu_in8(void *addr)
{
    return *(volatile u8 *) addr;
}

static ALWAYS_INLINE u16 gemu_in16(void *addr)
{
    return *(volatile u16 *) addr;
}

static ALWAYS_INLINE u32 gemu_in32(void *addr)
{
    return *(volatile u32 *) addr;
}

static ALWAYS_INLINE u64 gemu_in64(void *addr)
{
    return *(volatile u64 *) addr;
}

static ALWAYS_INLINE void gemu_out8(void *addr, u8 val)
{
    /* write 8 bit value to specified addr */
    volatile u8 *localaddr = (volatile u8 *)addr;
    *localaddr = val;
}

static ALWAYS_INLINE void gemu_out16(void *addr, u16 val)
{
    /* write 16 bit value to specified addr */
    volatile u16 *localaddr = (volatile u16 *)addr;
    *localaddr = val;
}

static ALWAYS_INLINE void gemu_out32(void *addr, u32 val)
{
    /* write 32 bit value to specified addr */
    volatile u32 *localaddr = (volatile u32 *)addr;
    *localaddr = val;
}

static ALWAYS_INLINE void gemu_out64(void *addr, u64 val)
{
    /* write 64 bit value to specified addr */
    volatile u64 *localaddr = (volatile u64 *)addr;
    *localaddr = val;
}

#define gemu_write_reg(base_addr, reg_offset, data)  \
{ \
  gemu_log("GEM write base_addr %p, offset 0x%0x, val 0x%0x \n", (void *)(base_addr), (u32)(reg_offset), (u32)(data)); \
  gemu_out32((void *)((base_addr) + (u32)(reg_offset)), (u32)(data)); \
}

#define gemu_read_reg(base_addr, reg_offset)            gemu_in32((void *)((base_addr) + (u32)(reg_offset)))


static ALWAYS_INLINE u16 gemu_endian_swap16(u16 data)
{
    return (u16) (((data & 0xFF00U) >> 8U) | ((data & 0x00FFU) << 8U));
}

#endif // __GEMU_TYPES_H
