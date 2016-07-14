#!/usr/bin/env python

import os, sys
import re
import subprocess

OLTA_PATH = None

def check_and_report(test):
    error_re = re.compile(r'ERROR: (.*)')
    result_re = re.compile(r'--- results')
    end_re = re.compile(r'--- end')
    
    sys.stdout.write(test + ": ")
    sys.stdout.flush()
    try:
        in_results = False
        count = 0
        errors = []

        p = subprocess.Popen(["./" + OLTA_PATH, test], stdout=subprocess.PIPE)
        output = p.stdout.readlines()
        errcode = p.returncode
        for line in output:
            if error_re.match(line):
                m = error_re.match(line)
                errors.append(m.group(1))
            elif result_re.match(line):
                in_results = True
            elif end_re.match(line):
                in_results = False
            elif in_results:
                count += 1

        if errcode == 0:
            sys.stdout.write("pass (%d end states)\n" % count)
        else:
            sys.stdout.write("failure - " + errors[0] + "\n")
    except subprocess.CalledProcessError as e:
        sys.stdout.write("failure - unknown\n")
    
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
