# ------------------------------[ App ]--------------------------------------------#
# Default application name is the same as the folder it is in.
# This can be overridden here if something different is required
#APPNAME ?= $(notdir $(shell pwd))

# Verbose or not, common out for more verbosity
ECHO=@

MV_AUTO_LST ?= no

all: all_binaries

#-------------------------------[ MDK COMMON ]---------------------------------------#
# Set MV_COMMON_BASE relative to mdk directory location (but allow user to override in environment)
#MV_COMMON_BASE:=../../../../common
#MV_AUTO_LST=no

# ------------------------------[ Build overrides ]--------------------------------#
# overrides that need to be set before including generic.mk
#ShaveSymObject   = $(DirAppObjDir)/$(APPNAME).sym.o

MV_SOC_PLATFORM ?= myriad2
MV_SOC_REV      ?= ma2150

LEON_RT_BUILD    = no

DirLDScrCommon = $(MV_COMMON_BASE)/scripts/ld/$(MV_SOC_PLATFORM)memorySections

# Ensure that the we are using the correct rtems libs etc
MV_SOC_OS 	 	 = rtems
MV_SOC_OS_LRT    = rtems
RTEMS_BUILD_NAME = b-prebuilt

MV_USB_PROTOS = protovsc2
# ------------------------------[ Components used ]--------------------------------#

### Custom board component
ComponentList_Intel_LOS = MemLoggerIntel ES0
LEON_COMPONENT_PATHS_LOS_INIT += $(patsubst %,components/%/leon,$(ComponentList_Intel_LOS))
LEON_COMPONENT_PATHS_LOS_INIT += $(patsubst %,components/%/leon/*,$(ComponentList_Intel_LOS))
LEON_COMPONENT_PATHS_LOS_INIT += $(patsubst %,components/%/leon/*/src,$(ComponentList_Intel_LOS))
LEON_COMPONENT_PATHS_LOS_INIT += $(patsubst %,components/%/leon/*/src/*,$(ComponentList_Intel_LOS))
LEON_COMPONENT_PATHS_LOS_INIT += $(patsubst %,components/%/leon/*/include,$(ComponentList_Intel_LOS))
LEON_COMPONENT_PATHS_LOS_INIT += $(patsubst %,components/%/leon/*/include/*,$(ComponentList_Intel_LOS))
LEON_COMPONENT_PATHS_LOS_INIT += $(patsubst %,components/%/leon/*/gpioDefaults,$(ComponentList_Intel_LOS))
LEON_COMPONENT_PATHS_LOS_INIT += $(patsubst %,components/%/leon/*/gpioDefaults/*,$(ComponentList_Intel_LOS))

### Common
ComponentList     += PipePrint

### LOS
ComponentList_LOS +=

### LRT
ComponentList_LRT += 

### SHAVES
ComponentList_SVE +=  

SHAVE_COMPONENTS = yes

LEON_APP_URC_SOURCES +=  $(./*leon/*.urc)

#########################################################################################

# Board revision (for clock freq's)
TM2_BRD_REV  ?= 1

ifeq ($(TM2_BRD_REV), 1)
DdrInitHeaderMvcmd = ddrInit_es1/ddrInitHeaderForMvcmd_ma2150
DdrInitElf = ddrInit_es1/ddrInit_ma2150.elf
endif

TM2_BRD_OPTS = -D'TM2_BRD_REV='$(TM2_BRD_REV)

CCOPT_LRT  += $(TM2_BRD_OPTS)
CPPOPT_LRT += $(TM2_BRD_OPTS)
CCOPT      += $(TM2_BRD_OPTS)
CPPOPT     += $(TM2_BRD_OPTS)

SHAVE_COMPONENTS = yes

CCOPT    += -D'CMX_DATA=__attribute__((section(".cmx.data")))'

#########################################################################################

# ------------------------------[ SLAM ]-------------------------------------------#

SLAM_PREFIX := ../../..
include $(SLAM_PREFIX)/src/tracker/platform/movidius/platform.mk

LEON_APP_CPP_SOURCES += $(SLAM_SOURCES)
LEON_APP_C_SOURCES   += $(SLAM_C_SOURCES)

### Trace
# Set the TRACE buffer size. If you change this value, please make the corresponding change in
# scripts/trace_run_mdbg2.tcl to save the correct file size
TRACE_BUFFER_SIZES=1048576

CCOPT     += -I../common -Icommon -DTRACE_BUFFER_SIZES=$(TRACE_BUFFER_SIZES)
CCOPT_LRT += -I../common -Icommon -DTRACE_BUFFER_SIZES=$(TRACE_BUFFER_SIZES)

LEON_HEADERS     += common/mv_trace.h

LEON_APP_CPP_SOURCES += common/mv_trace.cpp

trace: MVDBG_SCRIPT_OPT += --script $(DirAppScripts)/save_trace_buffers.tcl -D:traceSize="$(TRACE_BUFFER_SIZES)"
trace: debug

### HW Breakpoints code
LEON_HEADERS     += common/hw_breakpoints.h
LEON_HEADERS_LRT += common/hw_breakpoints.h

LEON_RT_CPP_SOURCES  += common/hw_breakpoints.cpp
LEON_APP_CPP_SOURCES += common/hw_breakpoints.cpp

# Application version:
MVCMDOPT   = -mvcmdVersion:v00.70
MVASMOPT  += -a
#MVCC_OPT_LEVEL= -O2

#common headers
LEON_HEADERS +=	$(wildcard ../common/*.h) 
LEON_HEADERS_LRT +=	$(wildcard ../common/*.h) 

# ------------------------------[ Tools ]------------------------------------------#
# Include the generic Makefile
include $(MV_COMMON_BASE)/generic.mk	

include $(SLAM_PREFIX)/src/tracker/platform/movidius/platform_after.mk

#LRT_PROJECT_LIBS += --start-group $(SLAM_LEON_LIBS) --end-group
AllLibs += --start-group $(SLAM_LEON_LIBS) --end-group

# ------------------------------[ USB ]--------------------------------------------#

CCOPT      += $(SLAM_CCOPT)
CPPOPT     += $(SLAM_CPPOPT)
MVCCOPT    += $(SLAM_SHAVE_CCOPT)
MVCCPPOPT  += $(SLAM_SHAVE_CPPOPT)

ASSERT_OPTIONS = -DNDEBUG

CCOPT      += $(ASSERT_OPTIONS)
CPPOPT     += $(ASSERT_OPTIONS)
MVCCOPT    += $(ASSERT_OPTIONS)
MVCCPPOPT  += $(ASSERT_OPTIONS)

CCOPT     += -DDEFAULT_RTEMS_L2C_WAYS=0
CCOPT_LRT += -DDEFAULT_RTEMS_L2C_WAYS=0

CCOPT     += -DPIPEPRINT_NOWAIT_DEBUG
CCOPT_LRT += -DPIPEPRINT_NOWAIT_DEBUG

# DA # MVCCOPT += -mlsu-load-policy=use-only-lsu0
# DA # MVCCOPT += -mlsu-store-policy=use-only-lsu0

MVCCOPT   += -fno-align-labels
MVCCPPOPT += -fno-align-labels

#-------------------------------[ MODULES ]---------------------------------------#
AppModuleList += appConfig

$(svuSippImg).mvlib : $(SippSvuObj)
	@mkdir -p $(dir $@)
	$(ECHO) $(LD) $(MVLIBOPT) $(SippSvuObj) $(CompilerANSILibs) -o $@

$(svuSippImg)_sym.o : $(svuSippImg).shvdcomplete
	$(ECHO) $(OBJCOPY) --prefix-symbols=SS_ --extract-symbol $< $@

$(DirAppObjBase)$(DirAppRoot)/leon/appMemMap.o : $(svuSippImg).shvdlib

# -------------------------------- [ Build Options ] ------------------------------ #

OPT_FLAGS = -O3
CPPOPT += $(OPT_FLAGS) -Wall
CPPOPT_LRT += $(OPT_FLAGS)
CCOPT += $(OPT_FLAGS)
CCOPT_LRT += $(OPT_FLAGS)
LDOPT += -flto

CCOPT_LRT += -DPIPEPRINT_NOWAIT_DEBUG 
CCOPT     += -DPIPEPRINT_NOWAIT_DEBUG -I../common

print-%: ; @echo $* = $($*)
