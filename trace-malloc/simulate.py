#!/usr/bin/python

from scipy.stats import invgauss
import heapq

EVENTS = 1000000
dist = invgauss
size_param = (4.4104723603507887, -2.6088737757264524, 38.771521098316683)
duration_param = (2.4486917249574072, 399004.31314749306, 4145977.1006420013)
interval_param = (1.1323738333275997, 453.39611575820913, 3727.4636141720862)

def gen_events():
    def align_up(value, alignment):
        return alignment * int((value + alignment - 1) / alignment)

    interval = dist.rvs(*interval_param, size=EVENTS)
    duration = dist.rvs(*duration_param, size=EVENTS)
    size = dist.rvs(*size_param, size=EVENTS)

    allocs = []
    time = 0
    for i in xrange(EVENTS):
        allocs.append((i, align_up(size[i], 8), time, time + duration[i]))
        time += interval[i]

    return allocs


def simulate(allocator):
    used = {}
    queue = []
    for i, size, atime, ftime in gen_events():
        heapq.heappush(queue, (atime, i, size, 'A'))
        heapq.heappush(queue, (ftime, i, size, 'F'))

    allocs = 0
    while queue:
        time, i, size, a = heapq.heappop(queue)
        if a == 'A':
            cookie = allocator.alloc(i, size)
            if cookie:
                used[i] = cookie
                allocs += 1
            else:
                allocator.stats()
        else:
            if i not in used:
                continue
            cookie = used[i]
            del used[i]
            allocator.free(i, size, cookie)
        if allocs == EVENTS / 10:
            allocator.stats()
            allocs = 0

class NopAllocator:
    def alloc(self, i, size, **kwargs):
        pass
    def free(self, *args, **kwargs):
        pass
    def stats(self, *args, **kwargs):
        pass

simulate(NopAllocator())
