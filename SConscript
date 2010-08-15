# -*- python -*-
#
# $Id$
# Authors: James Chiang <jchiang@slac.stanford.edu>
# Version: fitsGenApps-00-01-01

import os
Import('baseEnv')
Import('listFiles')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

progEnv.Tool('fitsGenAppsLib')
if baseEnv['PLATFORM'] == "posix":
    progEnv.Append(CPPDEFINES = 'TRAP_FPE')
makeFT1Bin = progEnv.Program('makeFT1', 'src/makeFT1/makeFT1.cxx')
makeFT2Bin = progEnv.Program('makeFT2', 'src/makeFT2/makeFT2.cxx')
makeFT2aBin = progEnv.Program('makeFT2a', 'src/makeFT2a/makeFT2a.cxx')
egret2FT1Bin = progEnv.Program('egret2FT1', listFiles(['src/egret2FT1/*.cxx']))
convertFT1Bin = progEnv.Program('convertFT1', 'src/convertFT1/convertFT1.cxx')
partitionBin = progEnv.Program('partition', 'src/partition/partition.cxx')
irfTupleBin = progEnv.Program('irfTuple', listFiles(['src/irfTuple/*.cxx']))

progEnv.Tool('registerTargets', package = 'fitsGenApps', 
             binaryCxts = [[makeFT1Bin,progEnv], [makeFT2Bin,progEnv],
                           [makeFT2aBin,progEnv], [egret2FT1Bin, progEnv],
                           [convertFT1Bin,progEnv], [partitionBin,progEnv],
                           [irfTupleBin,progEnv]],
             includes = listFiles(['fitsGenApps/*.h']), 
             pfiles = listFiles(['pfiles/*.par']), recursive = True)

