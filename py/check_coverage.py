#!/usr/bin/env python

import os, sys
import re
import subprocess

OLTA_PATH = None

def check_and_report(test):
    result_re = re.compile(r'--- results')
    end_re = re.compile(r'--- end')
    
    sys.stdout.write(test + ": ")
    sys.stdout.flush()
    try:
        in_results = False
        count = 0

        output = subprocess.check_output(["./" + OLTA_PATH, test])
        for line in output.split("\n"):
            if result_re.match(line):
                in_results = True
            elif end_re.match(line)
                in_results = False
            elif in_results:
                count += 1

        sys.stdout.write("pass (%d end states)\n" % count)
    except subprocess.CalledProcessError as e:
        sys.stdout.write("failured\n")
    
    sys.stdout.flush()

def main(args):
    global OLTA_PATH
    
    if len(args) > 0:
        for cp in ['olta/olta', '../olta/olta', 'olta']:
            if os.path.exists(cp) and os.path.isfile(cp):
                OLTA_PATH = cp
                break

        if not OLTA_PATH:
            print >>sys.stderr, "unable to find olta binary"
            sys.exit(1)
        
        for test in args:
            check_and_report(test)
    else:
        print >>sys.stderr, "check_coverage.py <test-1> [<test-2> ...]"
        sys.exit(1)

if __name__ == "__main__":
    main(sys.argv[1:])
    sys.exit(0)
