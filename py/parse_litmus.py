#!/usr/bin/env python

import sys
import re

def parse_litmus(lines):
    thread_names = []
    threads = {}
    mem = {}
    assign = {}
    other = []
    res = { 'thread_names': thread_names, 'threads': threads, 'vars': mem, 'const': assign, 'other': other }

    arch_re = re.compile(r'^(\S+)\s+(\S+).*')
    thread_re = re.compile(r'.*;\s*$')
    other_re = re.compile(r'\s*[~]?exists\s*\(.*')
    params_re = re.compile(r'-=-=-')
    var_start = re.compile(r'.*{\s*')
    var_end = re.compile(r'.*}\s*')
    var_dt = re.compile(r'(\S+)\s*=\s*(\S+)\s*;')
    var_def = re.compile(r'([^= ;]+) +([^= ;]+)\s*;')
    is_int = re.compile(r'(0x[0-9a-fA-F]+|[0-9]+)')
    is_th_assignment = re.compile(r'(\d+):(\S+)')

    variables = 0
    thread_ln = 0
    for line in lines:
        line = line.replace("\n", '')
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
                else:
                    assign[v] = name
        elif (variables == 2) and thread_re.match(line) and not other_re.match(line):
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
        elif variables >= 2:
            other.append(line)

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
    for name in sorted(test['vars'].keys()):
        spec = test['vars'][name]
        lines.append('%s %s;' % (spec['type'], name))
        for assignment in sorted(spec['assign']):
            lines.append(':'.join(assignment) + ' = ' + name + ';')
    for assignment in sorted(test['const'].keys()):
        lines.append(assignment + ' = ' + test['const'][assignment] + ';')
    lines.append('}')
    lines.append(thread_lines(threads, line_width))
    for i in range(n_thread_lines):
        strs = []
        for n in threads:
            if i < len(test['threads'][n]):
                strs.append(test['threads'][n][i])
            else:
                strs.append('')
        lines.append(thread_lines(strs, line_width))
    lines.append('')
    lines.extend(test['other'])

    return lines

def ins_parts(ins):
    res = []
    for i in ins:
        parts = re.split(r'[ ,\[\]]+', i)
        for p in parts:
            if len(p) > 0:
                res.append(p)
    return res

def arm_registers(ins):
    parts = ins_parts(ins)
    reg_re = re.compile(r'([rRwWxX]\d{1,2})')
    regs = {}
    for p in parts:
        if reg_re.match(p):
            regs[int(p[1:])] = True
    return regs

def assign_register(regs):
    for i in range(30):
        if i not in regs:
            regs[i] = True
            return 'X%d' % i
    assert False

def rewrite_ins(ins, subs):
    res = []
    for i in ins:
        for (sub, val) in subs.items():
            i = i.replace(sub, val)
        res.append(i)
    return res

def rewrite_defs(test):
    names = {}
    tidx = {}

    is_int = re.compile(r'(0x\d+|\d+)')
    const_check = []
    
    # look through constants for things out of place
    for (sub, var) in test['const'].items():
        if sub[0] == '%':
            if var not in names:
                names[var] = {}
            names[var][sub] = None
        else:
            const_check.append(sub)
            if not is_int.match(var):
                if var not in names:
                    names[var] = {}

    # create any varibles required
    for var in names.keys():
        if var not in test['vars']:
            test['vars'][var] = { 'type': 'uint64_t', 'assign': [] }

    # generate thread indexes
    for (name, idx) in zip(test['thread_names'], range(len(test['thread_names']))):
        tidx[name] = idx

    # check for register and variable assignments marked as constants
    for sub in const_check:
        var = test['const'][sub]
        ps = sub.split(':', 2)[:]
        if len(ps) == 2:
            if ps[0] in tidx:
                ps[0] = str(tidx[ps[0]])
                if ps[1][0] == 'R':
                    ps[1] = 'X' + ps[1][1:]
            del test['const'][sub]
            if var in names:
                test['vars'][var]['assign'].append(tuple(ps))
            else:
                test['const'][':'.join(ps)] = var

    # assign per thread registers to variables
    for (name, ins) in test['threads'].items():
        idx = tidx[name]
        regs = arm_registers(ins)

        _parts = ins_parts(ins)
        parts = {}
        for p in _parts:
            parts[p] = True

        usage = {}
        for var in names.keys():
            for sub in sorted(names[var].keys()):
                if sub in parts:
                    usage[sub] = assign_register(regs)
                    test['vars'][var]['assign'].append((str(idx), usage[sub])) 

        test['threads'][name] = rewrite_ins(ins, usage)

    for var in names.keys():
        for sub in names[var].keys():
            del test['const'][sub]

def arm_to_v8_ins(ins):
    old_reg_re = re.compile(r'[rR](\d{1,2})')
    res = []
    for i in ins:
        parts = re.split(r'(\s+|,|\[|\])', i)
        if len(parts) > 0:
            name = parts[0].upper()
            for pi in range(len(parts)):
                p = parts[pi]
                if old_reg_re.match(p):
                    m = old_reg_re.match(p)
                    parts[pi] = 'X' + m.group(1)

            # add domain for DMB/DSB
            if (name == 'DMB' or name == 'DSB') and (len(parts) == 1):
                parts[0] = parts[0] + ' SY'

            # add missing square braces for STR/LDR
            if (name == 'STR' or name == 'LDR') and (len(parts) == 5):
                if parts[4][0] != '[':
                    parts[4] = '[' + parts[4] + ']'
        
        res.append(''.join(parts))
    return res

def arm_to_v8_other(other):
    reg_rn = re.compile(r'(\d):R(\d{1,2})')
    res = []
    for line in other:
        line = reg_rn.sub('\\1:X\\2', line)
        res.append(line)
    return res

def arm_to_v8(test):
    if test['arch'] == 'ARM':
        rewrite_defs(test)
        for tn in test['threads']:
            test['threads'][tn] = arm_to_v8_ins(test['threads'][tn])
        test['other'] = arm_to_v8_other(test['other'])
        test['arch'] = 'AArch64'

def main(args):
    if len(args) > 0:
        with open(args[0]) as f:
            test = parse_litmus(f.readlines())
    
        arm_to_v8(test)
        
        litmus = serialise_litmus(test)
        print "\n".join(litmus)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main(sys.argv[1:])
    sys.exit(0)
