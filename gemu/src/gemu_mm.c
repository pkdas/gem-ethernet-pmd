#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include "gemu_log.h"
#include "gemu.h"
#include "gemu_mm.h"

// Linux defined
#define PFN_MASK_SIZE    8

// User defined
#define GEMU_BUFSIZE  2048

// 2MB huge page default config
#define HPAGE_SIZE_TWOS_POWER 21
#define HPAGE_SIZE  (1 << HPAGE_SIZE_TWOS_POWER)

static int hugepages=1;
static int sys_page_size = 0;
static int huge_page_size = HPAGE_SIZE;

static void gemu_rx_bufmem_alloc(gemu *gemu);

static size_t page_index(void *ptr) 
{
    return (size_t)(ptr) / sys_page_size;
}

static size_t get_pfn(void *ptr) 
{
    size_t pi = page_index(ptr);
    FILE *fp = fopen("/proc/self/pagemap", "rb");

    size_t retval = 0;

    // Each entry in pagemap is 64 bits, i.e, 8 bytes.
    fseek(fp, pi * 8, SEEK_SET);

    uint64_t page_info = 0;
    size_t result = fread(&page_info, 8, 1, fp);
    if (result != 1) 
    {
        gemu_mm_err("Could not read pagemap at entry %ld\n", (pi * 8));
    }

    // check if page is present (bit 63). Otherwise, there is no PFN.
    if (page_info & ((uint64_t)(1) << 63)) 
    {
        uint64_t pfn = page_info & (((uint64_t)(1) << 55) - 1);

        if (pfn == 0) 
        {
            gemu_mm_err("Could not get page frame number. Does the process have the CAP_SYS_ADMIN capability?\n");
        }

        retval= (size_t)(pfn);
    } else 
    {
        gemu_mm_err("Page not present.\n");
    }

    fclose(fp);
    return retval;
}

static size_t get_pflags(void *ptr) 
{
    size_t pfn = get_pfn(ptr);
    if (pfn == 0) 
    {
        return 0;
    }

    FILE *fp = fopen("/proc/kpageflags", "rb");
    fseek(fp, pfn * 8, SEEK_SET);

    uint64_t pflags = 0;
    size_t result = fread(&pflags, 8, 1, fp);
    if (result != 1) 
    {
        gemu_mm_err("Could not read pageflags at entry %ld\n", (pfn * 8));
    }

    fclose(fp);
    return (size_t)(pflags);
}

static bool is_ehp(void *ptr) 
{
    uint64_t flags = get_pflags(ptr);

    return (flags & ((uint64_t)(1) << 17)); // "huge" flag
}

bool is_thp(void *ptr) 
{
    uint64_t flags = get_pflags(ptr);
    return (flags & ((uint64_t)(1) << 22)); // "transparent huge"
}

bool is_huge(void *ptr) { return is_ehp(ptr) || is_thp(ptr); }

phys_addr_t rte_mem_virt2phy(volatile const void *virtaddr)
{
    int fd, retval;
    uint64_t page, physaddr;
    unsigned long virt_pfn;
    off_t offset;


    if (is_ehp((void *)virtaddr))
    {
        gemu_mm_log("virtaddr %p is ehp\n", (void *)virtaddr);
    }

    if (is_thp((void *)virtaddr))
    {
        gemu_mm_log("virtaddr %p is thp\n", (void *)virtaddr);
    }

    if (is_huge((void *)virtaddr))
    {
        gemu_mm_log("virtaddr %p is from huge-pages\n", (void *)virtaddr);
    } else
    {
        gemu_mm_log("virtaddr %p is NOT from huge-pages\n", (void *)virtaddr);
    }

    fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0) 
    {
        gemu_mm_err("%s(): cannot open /proc/self/pagemap: %s\n", __func__, strerror(errno));
        return RTE_BAD_IOVA;
    }

    virt_pfn = (unsigned long)virtaddr / sys_page_size;
    offset = sizeof(uint64_t) * virt_pfn;
    if (lseek(fd, offset, SEEK_SET) == (off_t) -1) 
    {
        gemu_mm_err("%s(): seek error in /proc/self/pagemap: %s\n", __func__, strerror(errno));
        close(fd);
        return RTE_BAD_IOVA;
    }

    retval = read(fd, &page, PFN_MASK_SIZE);
    close(fd);
    if (retval < 0) 
    {
        gemu_mm_err("%s(): cannot read /proc/self/pagemap: %s\n", __func__, strerror(errno));
        return RTE_BAD_IOVA;
    } else if (retval != PFN_MASK_SIZE) 
    {
        gemu_mm_err("%s(): read %d bytes from /proc/self/pagemap\n"
                "but expected %d:",
                __func__, retval, PFN_MASK_SIZE);
        return RTE_BAD_IOVA;
    }

    /*
     * the pfn (page frame number) are bits 0-54 (see
     * pagemap.txt in linux Documentation)
     */
    if ((page & 0x7fffffffffffffULL) == 0)
        return RTE_BAD_IOVA;

    physaddr = ((page & 0x7fffffffffffffULL) * sys_page_size)
                 + ((unsigned long)virtaddr % sys_page_size);

    return physaddr;
}

static int bufmem_stack_init(gemu *gemu, bufmem_stack *p, int num_bufs)
{
    int i = 0;

    if (p==NULL || num_bufs<=0)
    {
        return -1;
    }
    p->num_bufs = num_bufs;
    p->cur_count = 0;
    if (posix_memalign((void**)&p->bufs, sizeof(buf_addr), sizeof(buf_addr)*num_bufs) != 0)
    {
        return -2;
    }

    return 0;
}

static inline void bufmem_stack_dump(gemu *gemu, bufmem_stack *p)
{
    int i = p->cur_count;
    gemu_mm_log("Dumping buf addrs count %d\n", i);
    while (i)
    {
        i--;
        gemu_mm_log("buf virt_addr %p phys_addr %p\n", (void *)p->bufs[i].virt_addr, (void *)p->bufs[i].phys_addr);
    }
}

void gemu_bufmem_alloc(gemu *gemu)
{
   gemu_rx_bufmem_alloc(gemu);
}

static void gemu_rx_bufmem_alloc(gemu *gemu)
{
    int err = 0;
    int i = 0;
    uint64_t  virt_addr = (uint64_t)NULL;
    phys_addr_t phys_addr = 0;

    // add guard bands FIXME PK TODO
    assert(!gemu->bufmem_start);
    assert(!gemu->bufmem_end);

    gemu->num_bufs = gemu->rx_bd_count*8; //gemu->rx_bd_count*2;
    gemu->buf_size = GEMU_BUFSIZE; 
    gemu->bufmem_size = gemu->num_bufs*gemu->buf_size; 

    gemu_mm_log("GEMU bufmem alloc device %d, buf_size %d num_bufs %d bufmem_size %ld\n", 
            gemu->config.device_id, gemu->buf_size, gemu->num_bufs, gemu->bufmem_size);

    int flags = MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE|MAP_LOCKED; 
    if (hugepages)
    {
        flags = flags|MAP_HUGETLB;
        u32 bufmem_size = gemu->bufmem_size;
        bufmem_size = (gemu->bufmem_size / huge_page_size)*huge_page_size + huge_page_size;
        if (bufmem_size != gemu->bufmem_size) 
        {
            bufmem_size += huge_page_size;
        }
        gemu->bufmem_size = bufmem_size;
        printf("!!!!!!!!!!!!bufmem size 0x%0x\n", bufmem_size);
    } 
    
    gemu->bufmem_start = (uint64_t)mmap(NULL, gemu->bufmem_size, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (gemu->bufmem_start == (uint64_t)-1)
    {
        gemu_mm_err("gemu_mem_init mmap failed, err=%d.\n", errno);
        exit(-1);
    }
    gemu->bufmem_end = gemu->bufmem_start + gemu->bufmem_size;

    gemu_mm_log("gemu_mem_init mmap huge-pages success start-va %p, end-va %p memsize %ld\n", 
            (void *)gemu->bufmem_start, (void *)gemu->bufmem_end, gemu->bufmem_size);

    gemu_mm_log("Initializing num_bufs %d of size %d \n", gemu->num_bufs, gemu->buf_size);
    bufmem_stack_init(gemu, &gemu->bufmem_stack, gemu->num_bufs);

    for (int i=0; i < gemu->num_bufs; i++)
    {
        uint64_t virt_buf_addr = gemu->bufmem_start + i*gemu->buf_size;

        gemu_buf_hdr *buf_hdr = (gemu_buf_hdr *)virt_buf_addr;

        // 16 bytes buf_hdr
        buf_hdr->gemu = gemu;
        buf_hdr->pkt_len = 0; // should be filled in on rx or tx
        buf_hdr->buf_len = gemu->buf_size; 
        buf_hdr->buf_type = GEMU_BUF_TYPE_RX;

        uint64_t phys_buf_addr = rte_mem_virt2phy((const void *)virt_buf_addr);
        gemu_mm_log("adding buf num %d virt_addr %p phys_addr %p\n", i, (void *)virt_buf_addr, (void *)phys_buf_addr);
        bufmem_stack_push(gemu, virt_buf_addr, phys_buf_addr);

        buf_hdr->phys_buf_addr = phys_buf_addr;
    }
}

void gemu_bufmem_free(gemu *gemu)
{
    assert(gemu->bufmem_start);
    if (munmap((void *)gemu->bufmem_start, gemu->bufmem_size) != 0)
    {
        gemu_mm_err("gemu_mem_init unmap failed, err=%d.\n", errno);
    }

    gemu->bufmem_start = 0;
    gemu->bufmem_size = 0;
    gemu->bufmem_end = 0;
}


void *gemu_bdmem_alloc(u32 memsize) 
{
    int flags = MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE|MAP_LOCKED; 
    if (memsize > huge_page_size)
    {
	assert(!"memsize bigger than single contiguous page\n");
    }
   
    if (hugepages)
    {
        flags = flags|MAP_HUGETLB;
        memsize = huge_page_size;
    } 
    
    void *p = mmap(NULL, memsize, PROT_READ | PROT_WRITE, flags, -1, 0);
    if ((uint64_t)p == (uint64_t)-1)
    {
        gemu_mm_err("gemu_bdmem_alloc mmap failed, err=%d memsize %d.\n", errno, memsize);
        exit(-1);
    }

    assert(p);

    return p;
}

void  gemu_bdmem_free(void *p, u32 memsize)
{
    assert(p);
    if (hugepages)
    {
        memsize = huge_page_size;
    } 
    assert(munmap(p, memsize) == 0);
    return ;
}

void gemu_mm_init()
{
    sys_page_size = getpagesize();
}
