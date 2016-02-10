#!/usr/bin/python

import sys
import re

mallocre = re.compile(r"\[pid\s+(\d+)\]\s+(\d+).(\d+)\s+\S+malloc\((\d+)\)\s+=\s+(0x[0-9a-f]+)")
freere = re.compile(r"\[pid\s+(\d+)\]\s+(\d+).(\d+)\s+\S+free\((0x[0-9a-f]+)\)\s+=\s+<void>")

for line in sys.stdin:
    m = mallocre.match(line)
    if m:
        print m.group(1), int(m.group(2)) * 1000000 + int(m.group(3)), "malloc", m.group(4), int(m.group(5), 0)
        continue
    m = freere.match(line)
    if m:
        print m.group(1), int(m.group(2)) * 1000000 + int(m.group(3)), "free", int(m.group(4), 0)
