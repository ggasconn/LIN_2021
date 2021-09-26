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
echo -e "\n"

echo add 10 > /proc/modlist

cat /proc/modlist 
echo -e "\n"

echo add 4 > /proc/modlist
echo add 4 > /proc/modlist

cat /proc/modlist 
echo -e "\n"

echo add 2 > /proc/modlist
echo add 5 > /proc/modlist

cat /proc/modlist 
echo -e "\n"

echo remove 4 > /proc/modlist 

cat /proc/modlist 
echo -e "\n"

echo cleanup > /proc/modlist 

cat /proc/modlist
echo -e "\n"

rmmod modlist

make clean
