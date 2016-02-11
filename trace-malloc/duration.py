#!/usr/bin/python

from collections import defaultdict
from scipy.stats import invgauss
from pylab import plot, savefig, title
import numpy as np
import sys

MAX_DURATION=1000000
BINS=1000

def alloc(used, words):
    def _alloc(used, pid, stamp, addr):
        used[pid][addr] = stamp

    if len(words) < 5:
        return

    pid, time, addr = int(words[0]), int(words[1]), int(words[4])

    if addr == 0:
        return

    _alloc(used, pid, time, addr)

def free(time, used, words):
    def _free(time, used, pid, addr, stamp):
        addrmap = used[pid]
        if addr not in addrmap:
            return
        alloc_stamp = addrmap[addr]
        del addrmap[addr]
        duration = stamp - alloc_stamp
        if duration > MAX_DURATION:
            time.append(stamp - alloc_stamp)

    if len(words) < 4:
        return

    pid, stamp, addr = int(words[0]), int(words[1]), int(words[3])
    _free(time, used, pid, addr, stamp)

def read_values(stream):
    time, used = [], defaultdict(dict)
    for line in sys.stdin:
        words = line.split()
        if len(words) < 3:
            continue
        if words[2] == "malloc":
            alloc(used, words)
        elif words[2] == "free":
            free(time, used, words)
    return time

data = read_values(sys.stdin)
params = invgauss.fit(data)
values, borders = np.histogram(data, BINS, normed=True)
fitted = invgauss.pdf(borders[:-1], *params)
title('Object lifetime distribution')
plot(borders[:-1], fitted, 'r')
plot(borders[:-1], values, 'b')
savefig('lifetime.eps', format="eps", dpi=1000)
