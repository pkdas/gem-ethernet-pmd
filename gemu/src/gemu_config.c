#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include "gemu.h"

gemu_config gemu_config_table[GEM_MAX_DEVICES] = 
{
    {
        .name = {"gem-0"},
        .ifname = {"eth0"},
        .device_id = 0,
        .base_addr = 0, 
        .mac_addr = { 0xc6, 0x95, 0x88, 0x4b, 0x42, 0x54 }, //  C6:95:88:4B:42:54
        .rx_bd_count = 4096, // BD size 128Bits, 16B, 256*16B = 4KB page size
        .tx_bd_count = 4096,
        .ip4_addr = {0x01, 0x02, 0x03, 0x04 },
        .next_hop_mac = {0xe8, 0xea, 0x6a, 0x06, 0x6a, 0x59 },
    },
    {
        .name = "gem-1",
        .ifname = {"eth1"},
        .device_id = 1,
        .base_addr = 0, 
        .mac_addr = { 0xd6, 0xa6, 0xc1, 0x4e, 0xa1, 0xbd }, // D6:A6:C1:4E:A1:BD 
        .rx_bd_count = 4096,
        .tx_bd_count = 4096,
        .ip4_addr = {0x02, 0x03, 0x04, 0x05 },
        .next_hop_mac = {0xe8, 0xea, 0x6a, 0x06, 0x6a, 0x58 },
    },
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/if_ether.h>  // For ETH_ALEN
#include <sys/ioctl.h>

int gemu_update_eth_mac_addr(const char *ifname, gemu_config *config) 
{
    // Get the interface index
    int if_index = if_nametoindex(ifname);
    if (if_index == 0) {
        perror("Error: if_nametoindex");
        return 1;
    }

    // Allocate memory for the interface address data
    struct ifreq *ifr = (struct ifreq *) malloc(sizeof(struct ifreq));
    if (ifr == NULL) {
        perror("Error: malloc");
        return 1;
    }

    // Create a socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
      perror("Error: socket");
      free(ifr);
      return 1;
    }

    // Populate the ifreq struct with the interface index
    strncpy(ifr->ifr_name, ifname, IFNAMSIZ - 1);
    ifr->ifr_name[IFNAMSIZ - 1] = '\0';

    // Get the interface's hardware address (MAC address)
    if (ioctl(sock, SIOCGIFHWADDR, ifr) == -1) {
        perror("Error: ioctl");
        free(ifr);
        close(sock);
        return 1;
    }

    // Print the MAC address
    unsigned char *mac_address = (unsigned char *)ifr->ifr_hwaddr.sa_data;
    printf("MAC Address of %s: %02X:%02X:%02X:%02X:%02X:%02X\n",
           ifname,
           mac_address[0], mac_address[1], mac_address[2],
           mac_address[3], mac_address[4], mac_address[5]);


    memcpy(config->mac_addr, mac_address, sizeof(config->mac_addr));

    //Cleanup
    free(ifr);
    close(sock);

    return 0;
}

gemu_config *gemu_lookup_config(int device_id)
{
    assert(device_id < GEM_MAX_DEVICES); 
    assert(gemu_config_table[device_id].device_id == device_id);
 
    gemu_config *config = &gemu_config_table[device_id];

    gemu_update_eth_mac_addr(config->ifname, config);
    return (config);
}
