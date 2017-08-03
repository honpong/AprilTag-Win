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
SLAM_PREFIX := ../../../..
SLAM_PLATFORM_PREFIX := $(SLAM_PREFIX)/corvis/src/tracker/platform/movidius
SLAM_SOURCES := $(addprefix $(SLAM_PREFIX)/corvis/src/tracker/, \
    rc_tracker.cpp \
    sensor_fusion_queue.cpp \
    rotation_vector.cpp \
    matrix.cpp \
    kalman.cpp \
    filter.cpp \
    state_vision.cpp \
    state_motion.cpp \
    observation.cpp \
    calibration.cpp \
    calibration_json.cpp \
    sensor_fusion.cpp \
    capture.cpp \
    replay.cpp \
    feature_descriptor.cpp \
    mapper.cpp \
    dictionary.cpp \
    remapper.cpp \
    transformation.cpp \
)
SLAM_SOURCES += $(SLAM_PREFIX)/corvis/src/feature/tracker/feature_tracker.cpp
SLAM_SOURCES += $(SLAM_PREFIX)/corvis/src/feature/tracker/scaled_mask.cpp
SLAM_SOURCES += $(SLAM_PREFIX)/corvis/src/feature/tracker/fast_tracker.cpp
SLAM_SOURCES += $(SLAM_PREFIX)/corvis/src/feature/descriptor/patch_descriptor.cpp
SLAM_SOURCES += $(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.cpp)

SLAM_C_SOURCES := $(addprefix $(SLAM_PREFIX)/ThirdParty/vlfeat-0.9.18/vl/, \
    liop.c \
    imopv.c \
    kmeans.c \
    generic.c \
    mathop.c \
)
SLAM_C_SOURCES += $(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.c)

SLAM_HEADERS := \
	$(wildcard $(SLAM_PREFIX)/corvis/src/tracker/*.h) \
	$(wildcard $(SLAM_PREFIX)/corvis/src/tracker/fast_detector/*.h) \
	$(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.h) \
	$(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.hpp)

SLAM_CCOPT   := \
	-Idummy \
	-I$(SLAM_PLATFORM_PREFIX) \
	-I$(SLAM_PLATFORM_PREFIX)/leon/ \
	-I$(SLAM_PREFIX)/corvis/src/tracker/platform/movidius/ \
	-I$(SLAM_PREFIX)/corvis/src/tracker \
	-I$(SLAM_PREFIX)/corvis/src/feature/descriptor \
	-I$(SLAM_PREFIX)/corvis/src/feature/detector \
	-I$(SLAM_PREFIX)/corvis/src/feature/tracker \
	-I$(SLAM_PREFIX)/ThirdParty/eigen \
	-I$(SLAM_PREFIX)/ThirdParty/spdlog/include \
	-I$(SLAM_PREFIX)/ThirdParty/rapidjson/include/ \
	-Wno-unused-function \
	-Wno-unused-variable \
	-Wno-unused-parameter \
	-Wno-sign-compare \
	-fno-strict-aliasing \
	-DRAPIDJSON_64BIT=1 \
	-DEIGEN_STACK_ALLOCATION_LIMIT=1024 \
	-MD

SLAM_CPPOPT  := -std=gnu++11 -fpermissive -fno-exceptions -Wno-reorder -Wno-missing-field-initializers -fno-strict-aliasing -MD

LEON_APP_CPP_SOURCES += $(SLAM_SOURCES)
LEON_APP_C_SOURCES   += $(SLAM_C_SOURCES)

CCOPT      += $(SLAM_CCOPT)
CPPOPT     += $(SLAM_CPPOPT)

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

### Profiling
# The lines below remove eigen and operator functions off the profiling information
# Please adjust as needed
CCOPT_LRT += -finstrument-functions-exclude-file-list=eigen
CCOPT_LRT += -finstrument-functions-exclude-function-list=operator,iterator,from_row,to_col,get_stride,size,cols,rows,std

# Application version:
MVCMDOPT   = -mvcmdVersion:v00.70
MVASMOPT  += -a
#MVCC_OPT_LEVEL= -O2

SHAVE_APP_LIBS += $(cvrt).mvlib
SHAVE0_APPS += $(cvrt).shv0lib
SHAVE1_APPS += $(cvrt).shv1lib
SHAVE2_APPS += $(cvrt).shv2lib
SHAVE3_APPS += $(cvrt).shv3lib
SHAVE4_APPS += $(blis).shv4lib


PROJECTCLEAN += $(SHAVE_APP_LIBS) $(SHAVE0_APPS) $(SHAVE1_APPS) $(SHAVE2_APPS) $(SHAVE3_APPS) $(SHAVE4_APPS)

cvrt = $(DirAppOutput)/cvrt
blis = $(DirAppOutput)/blis

SHAVE_CPP_SOURCES_cvrt = $(wildcard $(SLAM_PLATFORM_PREFIX)/shave/cvrt/*.cpp)
SHAVE_CPP_SOURCES_cvrt += $(wildcard $(SLAM_PLATFORM_PREFIX)/shave/stereo_initialize/*.cpp)
SHAVE_CPP_SOURCES_cvrt += $(wildcard $(SLAM_PLATFORM_PREFIX)/shave/project_covariance/*.cpp)
SHAVE_ASM_SOURCES_cvrt = $(wildcard $(SLAM_PLATFORM_PREFIX)/shave/cvrt/*.asm)

SHAVE_ASM_SOURCES_cvrt += $(wildcard $(SLAM_PLATFORM_PREFIX)/shave/cholesky/*.asm)

SHAVE_C_SOURCES_cvrt   = $(wildcard $(SLAM_PLATFORM_PREFIX)/shave/cholesky/*.c)

SHAVE_GENASMS_cvrt = \
	$(patsubst %.cpp,$(DirAppObjBase)%.asmgen,$(SHAVE_CPP_SOURCES_cvrt))
SHAVE_GENASMS_cvrt += \
	$(patsubst %.c,$(DirAppObjBase)%.asmgen,$(SHAVE_C_SOURCES_cvrt))

SHAVE_cvrt_OBJS = \
	$(patsubst $(DirAppObjBase)%.asmgen,$(DirAppObjBase)%_shave.o,$(SHAVE_GENASMS_cvrt)) \
	$(patsubst %.asm,$(DirAppObjBase)%_shave.o,$(SHAVE_ASM_SOURCES_cvrt))

# BLIS/GEMM related stuff
VERS_STR            = 0.1.0-34
BLIS_VERSION_STRING = \"$(VERS_STR)\"
VERSION             = BLIS_VERSION_STRING=\"$(VERS_STR)\"

CCOPT   += -DBLIS -DVERSION -DBLIS_VERSION_STRING=\"$(VERS_STR)\" -DLEON_USE_REAL_NUMBERS_ONLY
CCOPT   += -I$(MV_COMMON_BASE)/components/BLIS/leon/config/reference $(BLIS_LEON_HEADERS_I)

#common headers
LEON_HEADERS +=	$(wildcard ../common/*.h) 
LEON_HEADERS_LRT +=	$(wildcard ../common/*.h) 


# ------------------------------[ Tools ]------------------------------------------#
# Include the generic Makefile
include $(MV_COMMON_BASE)/generic.mk	
include $(MV_COMMON_BASE)/components/BLIS/genblislib.mk

#LRT_PROJECT_LIBS += --start-group $(BLIS_LIB_LEON) --end-group
AllLibs += --start-group $(BLIS_LIB_LEON) --end-group

# SHAVE linking rules
cvrt_fns = fast9Detect fast9Track stereo_kp_matching_and_compare vision_project_motion_covariance
cvrt_fns += potrf_ln trsvl_ln trsvl_lt trsvl_lnlt
ENTRYPOINTS_cvrt = -e $(word 1, $(cvrt_fns)) $(foreach ep,$(cvrt_fns),-u $(ep) ) --gc-sections
$(cvrt).mvlib : $(SHAVE_cvrt_OBJS) $(PROJECT_SHAVE_LIBS)
	$(ECHO) $(LD) $(ENTRYPOINTS_cvrt) $(MVLIBOPT) 			\
                      $(SHAVE_cvrt_OBJS) $(PROJECT_SHAVE_LIBS) 		\
                      $(CompilerANSILibs) -o $@

ENTRYPOINTS_blis = -e startSGEMM -u startSGEMMTRSM_LL -u startSGEMMTRSM_LU -u startSGEMMTRSM_RU -u startSGEMMTRSM_RL --gc-sections
$(blis).mvlib : $(BLIS_LIB_SHAVE) $(PROJECT_SHAVE_LIBS)
	$(ECHO) $(LD) $(ENTRYPOINTS_blis) $(MVLIBOPT) 			\
                      $(BLIS_LIB_SHAVE) $(PROJECT_SHAVE_LIBS) 		\
                      $(CompilerANSILibs) -o $@

# ------------------------------[ USB ]--------------------------------------------#
MVCCOPT   += -nostdinc -Wno-c++11-extensions -Wno-literal-range -fno-strict-aliasing -fno-exceptions -Iinclude -Ieigen -Ileon -I$(SLAM_PLATFORM_PREFIX)/leon/ -I$(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)/common/moviCompile/include/c++
MVCCPPOPT += -std=c++11 -Wno-c++11-extensions -Wno-literal-range -fno-strict-aliasing -fno-exceptions -Iinclude -Ieigen -Ileon -I$(SLAM_PLATFORM_PREFIX)/leon/ -I$(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)/common/moviCompile/include/c++

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

-include $(LEON_APP_OBJECTS_REQUIRED:%.o=%.d)
-include $(LEON_RT_APP_OBJ:%.o=%.d)
