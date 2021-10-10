#!/usr/bin/env python

import argparse
import os
import sys

def main():
    ap = argparse.ArgumentParser(description='Convert binary file to C.')
    ap.add_argument('-i', metavar='input-file', help='input file')
    ap.add_argument('-o', metavar='output-file', help='output file')
    args = ap.parse_args()
    process(args.i, args.o)

def process(input, output):
    input, name = (input is None) and (sys.stdin.buffer, "<standard-input>") or (open(input, 'rb'), os.path.basename(input))
    output = (output is None) and sys.stdout or open(output, 'w')
    output.write('// BEGIN "%s"\n' % (name))
    i = 0
    b = input.read(1)
    endl = True
    while len(b) == 1:
        if endl and i > 0 and i % 512 == 0:
            output.write("// %08X:\n" % (i))
        output.write("0x%02X," % (b[0]))
        endl = (i + 1) % 16 == 0
        if endl:
            output.write('\n')
        b = input.read(1)
        i += 1
    if not endl:
        output.write('\n')
    output.write('// END "%s"\n' % (name))

if __name__ == '__main__':
    main()
