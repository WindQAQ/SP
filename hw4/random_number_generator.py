#!/usr/local/bin/python3

import random
import sys

amount = input("number of integer: ")
range_of_int = input("Max integer: ")
outputfile_name = input("output file name: ")

fp = open(outputfile_name, "w")
rand = random.randint
fp.write( str(amount) + "\n")

for _ in range(int(amount)):
	fp.write( str(rand(0, int(range_of_int))) + " ")

fp.close()

