#!/bin/bash
sudo rmmod device_driver
make clean
make
insmod device_driver.ko
rm /dev/device0
rm /dev/device1
mknod -m 666 /dev/device0 c 1337 0
mknod -m 666 /dev/device1 c 1337 1