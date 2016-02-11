#!/usr/bin/python

from collections import defaultdict
from scipy.stats import invgauss
from pylab import plot, savefig, title
import numpy as np
import sys

MAX_INTERVAL=20000
BINS=100
dist = invgauss

def read_values(stream):
    times = []
    for line in sys.stdin:
        words = line.split()
        if len(words) < 3 or words[2] != "malloc":
            continue
        times.append(int(words[1]))
    times.sort()
    invs = []
    for i, time in enumerate(times[1:]):
        diff = time - times[i]
        if diff > MAX_INTERVAL:
            continue
        invs.append(time - times[i])
    return invs

data = read_values(sys.stdin)
params = dist.fit(data)
values, borders = np.histogram(data, BINS, normed=True)
fitted = dist.pdf(borders[:-1], *params)
title('Request interval distribution')
plot(borders[:-1], fitted, 'r')
plot(borders[:-1], values, 'b')
savefig('interval.eps', format="eps", dpi=1000)
print 'parameters for', dist.name, params
