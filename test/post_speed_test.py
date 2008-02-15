#!/usr/bin/python
# This file is part of Te Tuhi Video Game System.
#
# Copyright (C) 2008 Douglas Bagnall
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import os, sys, re, time

class Trial:
    def __init__(self, lines):
        self.functions = {}
        for line in lines:
            if not line.strip():
                continue
            if line.startswith('placing'):
                self.settings = line
            elif line.startswith('using '):
                self.options = line[6:]
            else:
                
                m = re.search(r'\d+ function calls in ([\d.]+) CPU seconds', line)
                if m:
                    self.whole = float(m.group(1))
                m = re.search(r"\d+\s+([\d\.]+)[\s\d\.]+\{method '(\w+)'", line)
                if m:
                    self.functions[m.group(2)] = float(m.group(1))
                else:#bloody 2.4
                    m = re.search(r"\d+\s+([\d\.]+)[\s\d\.]+:0\((\w+)\)", line)
                    if m:
                        self.functions[m.group(2)] = float(m.group(1))
                    


        self.total = sum(self.functions.values())
        for k,v in self.functions.items():
            #print k,v
            setattr(self, 'fn_' + k, v)



if __name__ == "__main__":
    try:
        fn = sys.argv[1]
    except IndexError:
        fn = '/tmp/test-results.txt'
    f = open(fn)
    sets = []
    s = []
    for line in f:
        if line.startswith('-----'):
            sets.append(s)
            s = []
        else:
            s.append(line)
    sets.append(s)

    trials = [Trial(s) for s in sets if s]
    print [t.__dict__ for t in trials]
    print trials[0].settings
    print time.strftime("%c", time.localtime(os.stat(fn).st_mtime))
    
    for name, key in (('prescan', 'fn_prescan'), 
                      ('scan_zones', 'fn_scan_zones'), 
                      ('total', 'total'), 
                      ('whole process', 'whole'),
                      ): 
        print '\n', name

        #print [t.__dict__ for t in trials if not hasattr(t,'fn_prescan') ]        
        for t in sorted(trials, key=lambda x: getattr(x, key)):
            print "%0.3f  %s" % (getattr(t, key), t.options),
                                 
