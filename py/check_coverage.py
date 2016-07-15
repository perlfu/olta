#!/usr/bin/env python

import os, sys
import re
import subprocess

OLTA_PATH = None

def check_and_report(test):
    error_re = re.compile(r'ERROR: (.*)')
    result_re = re.compile(r'--- results')
    end_re = re.compile(r'--- end')
    passed = False
    
    sys.stdout.write(test + ": ")
    sys.stdout.flush()
    try:
        in_results = False
        count = 0
        errors = []

        p = subprocess.Popen(["./" + OLTA_PATH, test], stdout=subprocess.PIPE)
        output = p.stdout.readlines()
        errcode = p.wait()
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
            passed = True
            sys.stdout.write("pass (%d end states)\n" % count)

        elif len(errors) > 0:
            sys.stdout.write("failure (err: %d) - %s\n" % (errcode, errors[0]))
        else:
            sys.stdout.write("failure (err: %d) - unknown\n" % errcode)
    
    except subprocess.CalledProcessError as e:
        sys.stdout.write("failure - unknown\n")
    
    sys.stdout.flush()

    return passed

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
        
        passed = 0
        for test in args:
            if check_and_report(test):
                passed += 1

        if len(args) > 1:
            print '%d tested, %d passed, %d failed' % (len(args), passed, len(args) - passed)
    else:
        print >>sys.stderr, "check_coverage.py <test-1> [<test-2> ...]"
        sys.exit(1)

if __name__ == "__main__":
    main(sys.argv[1:])
    sys.exit(0)
