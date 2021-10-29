#!/bin/bash

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

