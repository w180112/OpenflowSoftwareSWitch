# OpenFlow protocol agent implementation

[![BSD license](https://img.shields.io/badge/License-BSD-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

This is a minimal OpenFlow protocol implementation in C.

## System required:

Linux OS

## How to use:

Git clone this repository

	# git clone https://github.com/w180112/ofpagent.git

Type

	# cd ofpagent

Run

	# make

to compile

Then

	# ./ofpagent

To remove the binary files

	# make clean

## Note:

1. This implementation is under OpenFlow protocol 1.3
2. Flowmod and Packetout OF packets are only print out and will do nothing so far.

## Test environment:

1. CentOS 7.6 and AMD Ryzen 2700, 64GB ram desktop
2. Can connect to Ryu/ONOS/OpenDaylight SDN controller
