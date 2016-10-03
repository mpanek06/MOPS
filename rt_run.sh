#!/bin/bash

ifconfig eth0 down
ifconfig eth1 down

rmmod e1000

/usr/local/rtnet/sbin/rtnet -v -c start
