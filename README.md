EmPOWER: Mobile Networks Operating System
=========================================

### What is EmPOWER?
EmPOWER is a new network operating system designed for heterogeneous mobile networks.

### Top-Level Features
* Supports both LTE and WiFi radio access networks
* Northbound abstractions for a global network view, network graph, and
  application intents.
* REST API and native (Python) API for accessing the Northbound abstractions
* Support for Click-based Lightweight Virtual Networks Functions
* Declarative VNF chaning on precise portion of the flowspace
* Flexible southbound interface supporting WiFi APs LTE eNBs

Checkout out our [website](http://empower.create-net.org/) and our [wiki](https://github.com/5g-empower/empower-runtime/wiki)

This repository is a fork of the EURECOM's OpenAirInterface with the EmPOWER eNB Agent and patches.

The EmPOWER eNB Agent is released under the Apache License, Version 2.0.

The OpenAirInterface is released under the OpenAirInterface Software Alliance license.
 
### Pre-requisites:

 * Linux standard build suite (GCC, LD, AR, etc...)
 * Pthread library, necessary to handle multithreading
 * Protocol Buffers C++ (protoc) along with the C++ runtime (Version >= 3.0.0)
 * pkg-config, a helper tool used when compiling applications and libraries
 * Protobuf-c, Google protocol buffer implementation for C (Version >= 1.2.1)
 * Pre-requisites of OpenAirInterface5G
 * Agent and Protocol library from EmPOWER eNB Agent (EMAge)

### Testbed

This software has been developed and tested on Linux, Ubuntu 14.04 LTS with 3.19.8-031908-lowlatency kernel. Our tested consists of OAI eNB, EPC and HSS running on a single host. But, the process is similar for environment with OAI EPC and HSS on a different host than the OAI eNB.

Hardware:
 * Laptop with Intel® Core™ i7-5600U CPU @ 2.60GHz × 4
 * 16 GB RAM
 * USRP B210 (UHD driver version UHD_003.010.000.000-release)
 * Nexus 5 with sysmoUSIM-SJS1 SIM

Software:
 * Latest OAI eNB code from `develop` branch
 * Latest Openair-cn code from tag `v0.3.2`
 * protoc (Version 3.1.0)
 * protobuf-c (Version 1.2.1)

### Download & Install Instructions

Before downloading the code, please follow the instructions below:
 * Ubuntu 14.04 LTS (32-bit or 64-bit)
 * Kernel setup https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/OpenAirKernelMainSetup
 * Disable C-states from BIOS (or from GRUB)
 * Disable CPU frequency scaling
 * Install low-latency kernel

empower-openairinterface requires protocol (libemproto) and agent (libemagent) library of EmPOWER eNB Agent to be installed before compiling.

Download and install protocol (libemproto) and agent (libemagent) library of EmPOWER eNB Agent:
```
git clone https://github.com/5g-empower/empower-enb-agent.git
cd empower-enb-agent/proto
make
sudo make install
cd ../agent
make
sudo make install
```
Create a configuration file by the name `agent.conf` and placed it in the `/etc/empower` directory. An example configuration file is provided in the folder `conf/empower` of EmPOWER eNB Agent repository.


Download and build empower-openairinterface:
```
https://github.com/5g-empower/empower-openairinterface
```
**Rest of the instructions for building empower-openairinterface is similar to building OpenAirInterface5G, which can be found in the following link https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/HowToConnectCOTSUEwithOAIeNB**

Once empower-openairinterface has been successfully compiled, you are good to go !!!

### Running empower-openairinterface

Modify the configuration file for EmPOWER eNB Agent `agent.conf` to specify the IP address and port number at which EmPOWER (or other SDN Controller) is running.

In order to run empower-openairinterface, follow the instructions mentioned in the section `Running eNB, EPC and HSS` in the following link https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/HowToConnectCOTSUEwithOAIeNB.

The EmPOWER eNB agent can be enabled or disabled by setting or unsetting the `EMAGE_AGENT` flag in `CMakeLists.txt` file `empower-openairinterface/cmake_targets/CMakeLists.txt`.
