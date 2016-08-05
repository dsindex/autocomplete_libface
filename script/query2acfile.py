#!/usr/bin/env python
#-*- coding: utf8 -*-

import sys
import os.path
from   optparse import OptionParser
sys.path.append(os.path.abspath('.'))
import time

def open_file(filename, mode) :
	try : fid = open(filename, mode)
	except :
		sys.stderr.write("open_file(), file open error : %s\n" % (filename))
		exit(1)
	else :
		return fid

def close_file(fid) :
	fid.close()

if __name__ == '__main__':
	parser = OptionParser()
	parser.add_option("--verbose", action="store_const", const=1, dest="verbose", help="verbose mode")

	(options, args) = parser.parse_args()
	
	if options.verbose == 1 : VERBOSE = 1

	start_time = time.time()	
	while 1:
		try:
			line = sys.stdin.readline()
		except KeyboardInterrupt:
			break
		if not line:
			break
		line = line.strip()
		tokens = line.split('\t')
		length = len(tokens)
		if length != 2 : continue
		phrase = tokens[0]
		freq = tokens[1]

		key = phrase.replace(' ','').lower()
		snippet = phrase
		print freq + "\t" + key + "\t" + snippet

	duration_time = time.time() - start_time
	sys.stderr.write("duration time = " + str(duration_time) + "\n")
