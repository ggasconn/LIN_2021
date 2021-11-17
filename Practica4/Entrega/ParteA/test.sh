#!/bin/bash

###
# Expected behavior:
# Executed on a non SMP-safe enviroment the kernel should terminate the script
# due to a null pointer or bad address error
###

while (true); do
	echo add 1 > /proc/modlist
	cat /proc/modlist
	echo remove 1 > /proc/modlist
done
