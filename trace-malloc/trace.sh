#/bin/bash

ltrace -tttf -e malloc+free ${@:1} -e 2>&1 | python parse.py
