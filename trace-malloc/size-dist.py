#!/usr/bin/python

from collections import Counter
import argparse
import sys

def is_iterable(obj):
    try:
        _ = iter(obj)
        return True
    except:
        return False

class CheckPositive(argparse.Action):
    def __call__(self, parser, namespace, values, options_string=None):
        if is_iterable(values):
            for value in values:
                if value <= 0:
                    raise ValueError("{} is't positive integer".format(value))
        else:
            if values <= 0:
                raise ValueError("{} is't positive integer".format(values))
        setattr(namespace, self.dest, values)

argparser = argparse.ArgumentParser(description='malloc size histogram')
argparser.add_argument('--bucket', '-b',
                       action=CheckPositive,
                       default='32',
                       nargs='?',
                       type=int,
                       help='Bucket size must be a positive integer')
argparser.add_argument('--limit', '-l',
                       action=CheckPositive,
                       default='50000',
                       nargs='?',
                       type=int,
                       help='allocation size limit, all values above this limit will be dropped')
config = argparser.parse_args()

count = Counter()
bucket, limit = config.bucket, config.limit
for line in sys.stdin:
    words = line.split()
    if len(words) < 3:
        continue
    if words[2] != "malloc":
        continue
    size = int(words[3])
    if size > limit:
        continue
    count[bucket * (size / bucket)] += 1

for size, cnt in sorted(count.items()):
    print size, cnt
