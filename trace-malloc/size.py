#!/usr/bin/python

from scipy.stats import invgauss
from pylab import plot, savefig, title
import numpy as np
import sys

dist = invgauss
MAX_SIZE = 2048
BINS = 128

def read_values(stream):
    values = []
    for line in stream:
        words = line.split()
        if len(words) < 3 or words[2] != "malloc":
            continue
        size = int(words[3])
        if size > MAX_SIZE:
            continue
        values.append(size)
    return values

data = read_values(sys.stdin)
params = dist.fit(data)
values, borders = np.histogram(data, BINS, normed=True)
fitted = dist.pdf(borders[:-1], *params)
title('Allocation size distribution')
plot(borders[:-1], fitted, 'r')
plot(borders[:-1], values, 'b')
savefig('allocsize.eps', format="eps", dpi=1000)
print 'paramters for', dist.name, params
