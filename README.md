# gem-ethernet-usermode-pmd (GEM-U)
GEM ethernet Userspace (GEM-U) Poll Mode driver (PMD) for AMD Zynq Ultralscale+ MPSoC

The GEM-U PMD leverages kernel macb driver for PHY initialization and configuration and performs only packet IO as a usermode application. 

The PMD is verified on [AMD Zynq UltraScale+™ MPSoC ZCU102 Evaluation Kit HW platform](https://www.amd.com/en/products/adaptive-socs-and-fpgas/evaluation-boards/ek-u1-zcu102-g.html) 
using the [ZCU102 PS EMIO 1g ethernet design](https://github.com/Xilinx-Wiki-Projects/ZCU102-Ethernet/tree/main/2024.2/ps_mio_eth_1g)
