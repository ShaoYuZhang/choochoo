import os
import sys

f = file('train/cali_data.csv')
avg_col = 6
min_col = 14
max_col = 15

lines = [list(l.strip().split(',')) for l in f if l[0] == 'D' or l[0] == 'A'];

print """#include "CalibrationData.h"\n    
#include <memory.h>\n    
void initVelocity(int* velocity) {       
  int v[15][2] = {"""
for i in xrange(0, 13):
  print "{{{0}, {1}}},".format(lines[i*2][avg_col], lines[i*2+1][avg_col])
print "{{{0}, {1}}}".format(lines[14*2][avg_col], lines[14*2+1][avg_col])
print "};"
print "memcpy_no_overlap_asm((char*)v, (char*)velocity, 2*15*4);"
print "} // end initVelocity\n\n"
  

print """void initStoppingDistance(int* distance) {       
  // 15 speeds each with Acc/Desc .. each with min/max
  int d[15][2][2] = {"""
for i in xrange(0, 13):
  print "{{{{{0}, {1}}}, {{{2}, {3}}} }},".format(
      lines[i*2][min_col], 
      lines[i*2][max_col], 
      lines[i*2+1][min_col],
      lines[i*2+1][max_col])

print "{{{{{0}, {1}}}, {{{2}, {3}}} }},".format(
    lines[14*2][min_col], 
    lines[14*2][max_col], 
    lines[14*2+1][min_col],
    lines[14*2+1][max_col])
print "};"
print "memcpy_no_overlap_asm((char*)d, (char*)distance, 2*2*15*4);"
print "} // end initDistance\n\n"
