#!/usr/bin/python
import sys
import subprocess

for arg in sys.argv[1:]:
    if arg == '-o':
        break
    subprocess.call(['llvm-as', arg])
