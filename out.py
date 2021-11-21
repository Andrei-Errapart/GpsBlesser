#! /usr/bin/python

import io	# read the file.
import serial
import time
import sys
import select

period = 0.1
first_sentence = "$GPGGA"


# 1. Read the file.
lines = io.open("gps.txt").readlines()
print "Read %d lines" % len(lines)

# 2. Open da serial port.
# gpsfd = serial.Serial("/dev/ttyUSB0", 38400, timeout=0.01)
gpsfd = io.open("/dev/ttyUSB0", "wb", 0)

# 3. Start time.
t0 = time.time()
lines_written = 0
while lines_written < len(lines):
	# Write all the stuff to the serial port up to the next GPGGA
	n = 0
	while lines_written<len(lines) and (n==0 or lines[lines_written].find(first_sentence)!=0):
		gpsfd.write(str(lines[lines_written]) + "\r")
		lines_written = lines_written + 1
		n = n + 1

	print "%d" % n
	#sys.stdout.write("%d" % n)
	#sys.stdout.flush()

	# Poll until next period.
	t0 = t0 + period
	stimeout = t0 - time.time()
	while stimeout>0:
		inputready,outputready,exceptready = select.select([], [],[], stimeout)
		stimeout = t0 - time.time()

