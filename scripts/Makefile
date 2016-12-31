# The MIT License (MIT)
#
# Copyright (c) 2014-2016 David Medina and Tim Warburton
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

rmSlash = $(patsubst %/,%,$(1))

PROJ_DIR := $(call rmSlash,$(PROJ_DIR))

ifdef OCCA_DIR
  OCCA_DIR := $(call rmSlash,${OCCA_DIR})
endif

ifeq ($(strip $(OCCA_DIR)),)
  OCCA_DIR := $(call rmSlash,$(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/..))
endif

binPath  = $(PROJ_DIR)/bin
libPath  = $(PROJ_DIR)/lib
objPath  = $(PROJ_DIR)/obj
srcPath  = $(PROJ_DIR)/src
incPath  = $(PROJ_DIR)/include
testPath = $(PROJ_DIR)/tests
#=================================================


#---[ Default Variables ]-------------------------
debugEnabled = 0
checkEnabled = 1

osFlags =
flags   =
fFlags  =
cfFlags =

# CXX      : C++ Compiler
# CXXFLAGS : C++ Compiler Flags

# FC       : Fortran Compiler
# FCFLAGS  : Fortran Compiler Flags

# OCCA_INCLUDE_PATH : Extra include paths
# OCCA_LIBRARY_PATH : Extra library paths
#=================================================


#---[ OS Detection ]------------------------------
OCCA_LINUX_OS   = 1
OCCA_OSX_OS     = 2
OCCA_WINDOWS_OS = 4
OCCA_WINUX_OS   = 5 # (OCCA_LINUX_OS | OCCA_WINDOWS_OS)

usingLinux   = 0
usingOSX     = 0
usingWinux   = 0
usingWindows = 0

UNAME = $(shell uname)

ifeq ($(UNAME),Linux)
  usingLinux   = 1
else ifeq ($(UNAME),Darwin)
  usingOSX     = 1
else ifneq (,$(findstring CYGWIN,$(UNAME)))
  usingLinux   = 1
  usingWinux   = 1
else ifneq (,$(findstring MINGW,$(UNAME)))
  usingWinux   = 1
  usingWindows = 1
else
  usingWindows = 1
endif
#=================================================


#---[ Variables ]---------------------------------
ifdef OCCA_DEVELOPER
 ifeq ($(OCCA_DEVELOPER), 1)
  ifeq ($(DEBUG), 0)
     debugEnabled = 0
  else
     debugEnabled = 1
  endif
 else
  ifeq ($(DEBUG), 1)
     debugEnabled = 1
  else
     debugEnabled = 0
  endif
 endif
endif

ifndef CXX
  ifdef OCCA_CXX
    CXX = ${OCCA_CXX}
  else
    CXX = g++
  endif
endif

ifndef FC
  FC = gfortran
endif

ifndef CXXFLAGS
  ifeq ($(DEBUG),1)
    CXXFLAGS = -g
  else
    CXXFLAGS = -O3
  endif
endif

ifndef FCFLAGS
  ifeq ($(DEBUG),1)
    FCFLAGS = -g
  else
    FCFLAGS = -O3
  endif
endif

compiler      = $(CXX)
compilerFlags = $(CXXFLAGS)

fCompiler      = $(FC)
fCompilerFlags = $(FCFLAGS)

links = $(LDFLAGS)
#=================================================


#---[ Paths/Flags/Links ]-------------------------
paths = -I$(OCCA_DIR)/lib -I$(OCCA_DIR)/include -L$(OCCA_DIR)/lib
paths += $(foreach path, $(subst :, ,$(OCCA_INCLUDE_PATH)), -I$(path))
paths += $(foreach path, $(subst :, ,$(OCCA_LIBRARY_PATH)), -L$(path))

ifneq (,$(wildcard ./$(incPath)/*))
  paths += -I./$(incPath)
endif

links += -locca
#=================================================


#---[ Shell Tools ]-------------------------------
ifeq (,$(findstring bash,$(SHELL)))
  SHELL := $(shell which bash)
  ifeq (,$(SHELL))
    $(error Could not find [bash], set SHELL manually with [export SHELL=/path/to/bash] or compile with [make SHELL=/path/to/bash])
  endif
endif

libraryFlagsFor = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; libraryFlags $1)
includeFlagsFor = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; headerFlags  $1)

compilerReleaseFlags = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; compilerReleaseFlags $(compiler))
compilerDebugFlags   = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; compilerDebugFlags   $(compiler))
compilerPicFlag      = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; compilerPicFlag      $(compiler))
compilerSharedFlag   = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; compilerSharedFlag   $(compiler))

fCompilerModuleDirFlag = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; fCompilerModuleDirFlag $(fCompiler))

compilerSupportsOpenMP = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; compilerSupportsOpenMP $(compiler))
compilerOpenMPFlags    = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; compilerOpenMPFlags    $(compiler))

fCompilerSupportsOpenMP = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; compilerSupportsOpenMP $(fCompiler))
fCompilerOpenMPFlags    = $(shell . $(OCCA_DIR)/scripts/shellTools.sh; compilerOpenMPFlags    $(fCompiler))
#=================================================


#---[ Compiler Info ]-----------------------------
debugFlags   = $(call compilerDebugFlags)
releaseFlags = $(call compilerReleaseFlags)
picFlag      = $(call compilerPicFlag)
sharedFlag   = $(call compilerSharedFlag)
fModDirFlag  = $(call fCompilerModuleDirFlag)
#=================================================


#---[ Flags and Libraries ]-----------------------
OCCA_USING_VS := 0

ifeq ($(usingLinux),1)

  ifeq ($(usingWinux),0)
    OCCA_OS := OCCA_LINUX_OS
  else
    OCCA_OS := OCCA_WINUX_OS
  endif

  links += -lm -lrt -ldl

else ifeq ($(usingOSX),1)

	OCCA_OS := OCCA_OSX_OS
  osFlags += -Wno-deprecated-declarations
  links += -framework accelerate -framework CoreServices

else ifeq ($(usingWindows),1)

  ifeq ($(usingWinux),0)
    OCCA_OS := OCCA_WINDOWS_OS
	  OCCA_USING_VS := 1
  else
    OCCA_OS := OCCA_WINDOWS_OS
  endif

  links +=

endif
#=================================================


#---[ Variable Dependencies ]---------------------
openmpEnabled = 0
openclEnabled = 0
cudaEnabled   = 0

ifdef OCCA_OPENMP_ENABLED
  openmpEnabled  = $(OCCA_OPENMP_ENABLED)
  fOpenmpEnabled = $(OCCA_OPENMP_ENABLED)
else
  openmpEnabled  = $(call compilerSupportsOpenMP)
  fOpenmpEnabled = $(call fCompilerSupportsOpenMP)

  ifeq ($(openmpEnabled), 1)
    flags  += $(call compilerOpenMPFlags)
    fFlags += $(call fCompilerOpenMPFlags)
  endif
endif

ifdef OCCA_OPENCL_ENABLED
  openclEnabled = $(OCCA_OPENCL_ENABLED)

  ifeq ($(openclEnabled), 1)
    ifeq ($(usingLinux),1)
      links += -lOpenCL
    else ifeq ($(usingOSX),1)
      links += -framework OpenCL
    endif
  endif
else
  ifeq ($(usingLinux),1)
    openclLibFlags = $(call libraryFlagsFor,OpenCL)
    ifneq (,$(openclLibFlags))

      openclIncFlags = $(call includeFlagsFor,CL/cl.h)
      ifneq (,$(openclIncFlags))
        openclEnabled = 1
        paths += $(openclIncFlags)
        links += $(openclLibFlags)
      endif
    endif
  else ifeq ($(usingOSX),1)
    # OpenCL includes are embedded in the framework
    openclLibFlags = $(call libraryFlagsFor,OpenCL)

    ifneq (,$(openclLibFlags))
      openclEnabled = 1
      links += $(openclLibFlags)
    endif
  endif
endif

ifdef OCCA_CUDA_ENABLED
  cudaEnabled = $(OCCA_CUDA_ENABLED)

  ifeq ($(cudaEnabled), 1)
    ifeq ($(usingLinux),1)
      links += -lcuda
    else ifeq ($(usingOSX),1)
      links += -framework CUDA
    endif
  endif
else
  cudaIncFlags = $(call includeFlagsFor,cuda.h)

  ifneq (,$(cudaIncFlags))

    ifeq ($(usingLinux),1)
      cudaLibFlags = $(call libraryFlagsFor,cuda)
    else ifeq ($(usingOSX),1)
      cudaLibFlags = $(call libraryFlagsFor,CUDA)
    endif

    ifneq (,$(cudaLibFlags))
      cudaEnabled = 1
      paths += $(cudaIncFlags)
      links += $(cudaLibFlags)
    endif
  endif
endif

ifeq ($(debugEnabled), 1)
  OCCA_DEBUG_ENABLED := 1
  cfFlags += $(debugFlags)
else
  OCCA_DEBUG_ENABLED := 0
  cfFlags += $(releaseFlags)
  cfFlags += -DNDEBUG=1
endif

ifeq ($(checkEnabled), 1)
  OCCA_CHECK_ENABLED := 1
else
  OCCA_CHECK_ENABLED := 0
endif

OCCA_OPENMP_ENABLED := $(openmpEnabled)
OCCA_OPENCL_ENABLED := $(openclEnabled)
OCCA_CUDA_ENABLED   := $(cudaEnabled)

flags  += $(cfFlags) $(osFlags)
fFlags += $(cfFlags) $(osFlags)
#=================================================