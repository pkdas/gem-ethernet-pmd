#!/usr/bin/env python3
import sys
import os
import argparse
import subprocess
import glob
import shutil
import re
import time
import datetime
import fileinput
import base64
import json
import copy

# DPDK config
# sa out 1 cipher_algo null auth_algo null mode transport
# sa in 4001 cipher_algo null auth_algo null mode transport

# sp ipv4 out esp protect 1 pri 1 dst 198.18.1.1/32 sport 0:65535 dport 0:65535
# sp ipv4 in esp protect 4001 pri 4001 dst 198.19.0.1/32 sport 0:65535 dport 0:65535

# rt ipv4 dst 198.19.0.0/16 port 0 
# rt ipv4 dst 198.18.0.0/16 port 1 

#-------------
# inbound: dst 198.19.0.0 egress port 0  spi 4xxx
# outbound: dst 198.18.0.0 egress port 1 spi xxx1
# 
#-------------

# VPP config
#  

# spd0 - inbound/outbound 

# if from dst-ip find port find spd0
#    ipsec policy add spd <yyy> priority <pri> action protect sa <sa> remote-ip-range <dst>

# ipsec policy add spd 0 priority 1 inbound action protect sa 1 local-ip-range 1.1.1.2-1.1.1.2 remote-ip-range 5.5.5.2-5.5.5.2  

rt_map={}
vpp_sa_fname=''
vpp_sp_fname=''
vpp_ipsec_cmdx=''
cmd = ' add '

def parse_port_config(dpdk_fname):
  dpdk_file  = open(dpdk_fname, 'r')
  for line in dpdk_file:
    line = line.strip()
    if line:
       if line[0] == '#':
         continue
       if line[0] == 'r' and line[1] == 't': 
           tokens = line.split()
           #rt ipv4 dst 198.19.0.0/16 port 0
           dst_ip = tokens[3]
           dst_ip_subnet = dst_ip.split(".0.0")[0]
           port = tokens[5] 
           rt_map[dst_ip_subnet] = port

  #print(rt_map)
  dpdk_file.close()

def parse_sa_sp_config(dpdk_fname):
  global vpp_sa_fname, vpp_sp_fname
  vpp_sa_fname = "vpp_sa_"+dpdk_fname
  vpp_sp_fname = "vpp_sp_"+dpdk_fname
  vpp_sa_file  = open(vpp_sa_fname, 'w')
  vpp_sp_file  = open(vpp_sp_fname, 'w')
  dpdk_file  = open(dpdk_fname, 'r')
  sa_lines = []
  sp_lines = []
  sa_count = 0
  sp_count = 0
  for line in dpdk_file:
    if line.strip():
       if line[0] == '#':
         continue
       tokens = line.split()
       if tokens[0] == 'sa': 
          sa_idx = tokens[2]
          spi = tokens[2]
          sa_count +=1
          #ipsec sa add 1 spi 1 esp crypto-alg none
          line = "vppctl ipsec sa"+cmd+sa_idx+" spi "+spi+" esp crypto-alg none no-algo-no-drop\nsleep 0.05\n"
          sa_lines.append(line)
       if tokens[0] == 'sp': 
          sp_count += 1
          # dpdk secgw: sp ipv4 out esp protect 1 pri 1 dst 198.18.1.1/32 sport 0:65535 dport 0:65535
          # vpp: ipsec policy add spd 0 priority 1 inbound action protect sa 1 local-ip-range 1.1.1.2-1.1.1.2 remote-ip-range 5.5.5.2-5.5.5.2  
          try:
            dst_ip = tokens[tokens.index("dst")+1].split('.')
            dst_ip_subnet = dst_ip[0]+'.'+dst_ip[1]
            dst_ip_addr = dst_ip[0]+'.'+dst_ip[1]+'.'+dst_ip[2]+'.'+dst_ip[3]
            dst_ip_addr = dst_ip_addr.split("/")[0]
            dst_ip_range = dst_ip_addr+"-"+dst_ip_addr
            port = rt_map[dst_ip_subnet]
            priority = tokens[tokens.index("pri")+1]
            sa = tokens[tokens.index("protect")+1]
            dir = "outbound"
            if "in" in tokens[2]:
                dir = "inbound"

            match_ip_range = 'remote-ip-range'
            if dir == 'inbound':
              match_ip_range = 'local-ip-range'
            line = "vppctl ipsec policy"+cmd+"spd 0 priority "+priority+" "+dir+" action protect sa "+sa+" "+match_ip_range+" "+dst_ip_range+"\nsleep 0.05\n"
            sp_lines.append(line)
          except:
            print("Error parsing SP")
            sys.exit(-1) 

  vpp_sa_file.writelines(sa_lines)
  vpp_sp_file.writelines(sp_lines)
  vpp_sa_file.close()
  vpp_sp_file.close()
  dpdk_file.close()

if __name__ == '__main__':
  sys.stderr = sys.stdout
  numargs = len(sys.argv)
  
  if numargs < 3:
     print("Usage : "+sys.argv[0]+" <dpdk_fname> <ipsec_cmd_fname>")
     sys.exit(0)

  if numargs == 4:
    cmd=' del '

  dpdk_fname = sys.argv[1]
  ipsec_cmd_fname = sys.argv[2]
  parse_port_config(dpdk_fname)
  parse_sa_sp_config(dpdk_fname)

  f1=open(ipsec_cmd_fname, "a+")
  with open(vpp_sa_fname, "r") as f:
    f1.write(f.read());
  with open(vpp_sp_fname, "r") as f:
    f1.write(f.read());
  f1.close()
