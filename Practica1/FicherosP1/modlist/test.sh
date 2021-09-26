#!/bin/bash

if [ "$EUID" -ne 0 ]; then
	echo -e "ERROR! Script should be run as root!"
	exit -1
fi

make

insmod modlist.ko
if [ $? -ne 0 ]; then
	echo -e "Error loading modlist module!"
	exit -1
fi

cat /proc/modlist

echo -e add 1 > /proc/modlist
echo -e add 2 > /proc/modlist
echo -e add 3 > /proc/modlist
echo -e add 4 > /proc/modlist

cat /proc/modlist
echo -e "\n"

echo -e remove 2 > /proc/modlist

cat /proc/modlist
echo -e "\n"

echo -e cleanup > /proc/modlist

cat /proc/modlist
echo -e "\n"

rmmod modlist

make clean
