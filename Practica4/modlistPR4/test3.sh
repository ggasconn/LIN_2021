#!/bin/bash

while (true); do
	echo add 1 > /proc/modlist
	cat /proc/modlist
	echo remove 1 > /proc/modlist
done
