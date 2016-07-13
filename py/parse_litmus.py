#!/usr/bin/env python

import sys
import re

def parse_litmus(lines):
    thread_names = []
    threads = {}
    mem = {}
    assign = {}
    res = { 'thread_names': thread_names, 'threads': threads, 'vars': mem, 'const': assign }

    arch_re = re.compile(r'^(\S+)\s+(\S+).*')
    thread_re = re.compile(r'.*;\s*$')
    params_re = re.compile(r'-=-=-')
    var_start = re.compile(r'.*{\s*')
    var_end = re.compile(r'.*}\s*')
    var_dt = re.compile(r'(\S+)\s*=\s*(\S+)\s*;')
    var_def = re.compile(r'([^= ]+) +(\S+)\s*;')
    is_int = re.compile(r'(0x[0-9a-fA-F]+|[0-9]+)')
    is_th_assignment = re.compile(r'(\d+):(\S+)')

    variables = 0
    thread_ln = 0
    for line in lines:
        if ('name' not in res) and arch_re.match(line):
            m = arch_re.match(line)
            res['arch'] = m.group(1)
            res['name'] = m.group(2)
        elif params_re.match(line):
            break
        elif (variables == 0) and var_start.match(line):
            variables = 1
        elif (variables == 1) and var_end.match(line):
            variables = 2
        elif (variables == 1):
            defs = var_def.findall(line)
            vs = var_dt.findall(line)
            for (tp, name) in defs:
                mem[name] = { 'type': tp, 'assign': [] }
            for (v, name) in vs:
                if is_int.match(name):
                    assign[v] = name
                elif name in mem:
                    mem[name]['assign'].append(v.split(':')) 
        elif (variables == 2) and thread_re.match(line):
            thread_ln += 1
            pt = line.replace(';', '').split('|')
            for (i, p) in zip(range(len(pt)), pt):
                p = p.strip()
                if thread_ln == 1:
                    thread_names.append(p)
                    threads[p] = []
                elif len(p) > 0:
                    n = thread_names[i]
                    threads[n].append(p)

    return res

def thread_lines(strs, lws):
    parts = []
    for (s, l) in zip(strs, lws):
        parts.append(('%-' + str(l) + 's') % s)
    return ' ' + ' | '.join(parts) + ' ;'

def serialise_litmus(test):
    n_thread_lines = 0
    threads = test['thread_names']
    line_width = [0] * len(threads)

    for (i, n) in zip(range(len(threads)), threads):
        n_thread_lines = max(n_thread_lines, len(test['threads'][n]))
        line_width[i] = max(map(len, test['threads'][n]))

    lines = []
    lines.append('%s %s' % (test['arch'], test['name']))
    lines.append('{')
    for (name, spec) in test['vars'].items():
        lines.append('%s %s;' % (spec['type'], name))
        for assignment in spec['assign']:
            lines.append(':'.join(assignment) + ' = ' + name + ';')
    for assignment in sorted(test['const'].keys()):
        lines.append(assignment + ' = ' + test['const'][assignment] + ';')
    lines.append('}')
    lines.append('')
    lines.append(thread_lines(threads, line_width))
    for i in range(n_thread_lines):
        strs = []
        for n in threads:
            if i < len(test['threads'][n]):
                strs.append(test['threads'][n][i])
            else:
                strs.append('')
        lines.append(thread_lines(strs, line_width))

    return lines

def main(args):
    if len(args) > 0:
        with open(args[0]) as f:
            r = parse_litmus(f.readlines())
        print r
        
        l = serialise_litmus(r)
        print "\n"
        print "\n".join(l), "\n"
    else:
        sys.exit(1)

if __name__ == "__main__":
    main(sys.argv[1:])
    sys.exit(0)
