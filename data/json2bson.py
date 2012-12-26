#!/usr/bin/python3
# Reads JSON from file (or stdin), one record per line. Wrties BSON to stdout

import sys
import bson
import bson.json_util

infile = sys.stdin
outfile = sys.stdout.buffer # needs to be binary
if len(sys.argv) > 1:
    infile = open(sys.argv[1], 'r')

for line in infile:
    b = bson.BSON.encode(bson.json_util.loads(line))
    outfile.write(b)
