# OpenFlow protocol software switch implementation

[![BSD license](https://img.shields.io/badge/License-BSD-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Build Status](https://travis-ci.org/w180112/OpenflowSoftwareSWitch.svg?branch=dpdk)](https://travis-ci.org/w180112/OpenflowSoftwareSWitch)

This is a small OpenFlow protocol based software switch implementation in C.

## System required:

1. Linux OS
2. RAM size larger than 2 GiB
3. x86 system or ARM system with little endian
4. DPDK and compatible NIC

## How to use:

Git clone this repository

	# git clone https://github.com/w180112/OpenflowSoftwareSWitch.git

Type

	# cd OpenflowSoftwareSWitch && git checkout dpdk

Run

	# make

to compile

Then

	# ./osw <DPDK eal options> -- <OpenFlow NIC name> <SDN controller ip>

Once OpenFlow connection is established, a simple console interface will show

	OSW>

Try to type

	OSW> show flows

to get flow table.

If the prompt is missing, just press Enter.

To remove the binary files

	# make clean

## Note:

1. This implementation is under OpenFlow protocol 1.3
2. In Flowmod, I only implementation Flowmod type - "add flow" so far.

## Test environment:

1. CentOS 8.1 and AMD EPYC 7401P, 256GB ram server
2. Can connect to Ryu SDN controller with simple_switch_13.py application

## TODO:

1. implementation Flowmod type - "delete flow" and flow timeout.
2. add port configuration

## Changelog 2020/07/11

1. Bug fix.
2. Split flow table into control plane and date plane, when flow rule missed in data plane but matched in control plane, control plane will send the matched flow rule to data plane, otherwise control plane will send a packet_in packet to OpenFlow controller. 