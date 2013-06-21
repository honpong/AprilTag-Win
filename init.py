import sys, os
import getopt
import ctypes

KB = 1024
MB = 1024*KB

oldflags = sys.getdlopenflags()
sys.setdlopenflags(ctypes.RTLD_GLOBAL | oldflags)
sys.path.extend(['cor/'])
import cor
sys.path.extend(['numerics/'])
import numerics
sys.setdlopenflags(oldflags)

try:
    opts, args = getopt.getopt(sys.argv[1:], 'vf:', ['vis', 'file='])
except getopt.GetoptError:
    print """Usage: 
-v, --vis: run with visualization
-f, --file <filename>: execute a file instead of starting a shell
"""

runvis = False
runfile = False
for opt, arg in opts:
    sys.argv.remove(opt)
    if(arg is not ''):
        sys.argv.remove(arg)
    if opt in ('-v', '--vis'):
        runvis = True
    if opt in ('-f', '--file'):
        runfile = True
        runfile_filename = arg

if len(args) >= 1:
    execfile(args[0])
