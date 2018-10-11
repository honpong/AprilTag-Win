MAKEDEPEND = yes

# SLAM_PREFIX must be defined before including this makefile
SLAM_PLATFORM_PREFIX := $(SLAM_PREFIX)/src/tracker/platform/movidius
SLAM_SOURCES := $(addprefix $(SLAM_PREFIX)/src/tracker/, \
    rc_tracker.cpp \
    sensor_fusion_queue.cpp \
    rotation_vector.cpp \
    matrix.cpp \
    filter.cpp \
    state_vision.cpp \
    state_motion.cpp \
    observation.cpp \
    calibration.cpp \
    calibration_json.cpp \
    calibration_tm2.cpp \
    sensor_fusion.cpp \
    capture.cpp \
    replay_device.cpp \
    bstream.cpp \
    mapper.cpp \
    remapper.cpp \
    transformation_cov.cpp \
    estimate_horn.cpp \
    estimate_epnp.cpp \
    estimate_fundamental.cpp \
    estimate_3d_point.cpp \
    map_loader.cpp \
)
SLAM_SOURCES += $(SLAM_PREFIX)/src/tracker/platform/shell/sensor_clock.cpp
SLAM_SOURCES += $(SLAM_PREFIX)/src/feature/tracker/feature_tracker.cpp
SLAM_SOURCES += $(SLAM_PREFIX)/src/feature/tracker/scaled_mask.cpp
SLAM_SOURCES += $(SLAM_PREFIX)/src/feature/tracker/fast_tracker.cpp
SLAM_SOURCES += $(SLAM_PREFIX)/src/feature/descriptor/patch_descriptor.cpp
SLAM_SOURCES += $(SLAM_PREFIX)/src/feature/descriptor/orb_descriptor.cpp
SLAM_SOURCES += $(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.cpp)

SLAM_C_SOURCES += $(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.c)

#Commented out to prevent loading vocabulary in TM2 project
SLAM_SOURCES   += $(SLAM_PREFIX)/src/vocabulary/resource.cpp
SLAM_C_SOURCES += $(SLAM_PREFIX)/src/vocabulary/voc.c

VOC_VERSION := $(md5sum $(SLAM_PREFIX)/src/vocabulary/voc_k20_L4_64.bin)

$(SLAM_PREFIX)/src/vocabulary/voc.c: \
$(SLAM_PREFIX)/src/vocabulary/voc_k20_L4_64.bin

SLAM_HEADERS := \
	$(wildcard $(SLAM_PREFIX)/src/tracker/*.h) \
	$(wildcard $(SLAM_PREFIX)/ThirdParty/DBoW2/include/*.h) \
	$(wildcard $(SLAM_PREFIX)/src/feature/detector/*.h) \
	$(wildcard $(SLAM_PREFIX)/src/feature/descriptor/*.h) \
	$(wildcard $(SLAM_PLATFORM_PREFIX)/*.h) \
	$(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.h) \
	$(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.hpp)

SLAM_CCOPT   := \
	-Idummy \
	-I$(SLAM_PLATFORM_PREFIX) \
	-I$(SLAM_PLATFORM_PREFIX)/leon/ \
	-I$(SLAM_PREFIX)/src/tracker/platform \
	-I$(SLAM_PREFIX)/src/tracker/platform/movidius/ \
	-I$(SLAM_PREFIX)/src/tracker \
	-I$(SLAM_PREFIX)/src/feature/descriptor \
	-I$(SLAM_PREFIX)/src/feature/detector \
	-I$(SLAM_PREFIX)/src/feature/tracker \
	-I$(SLAM_PREFIX)/src/vocabulary \
	-I$(SLAM_PREFIX)/ThirdParty/DBoW2/include \
	-isystem$(SLAM_PREFIX)/ThirdParty/eigen \
	-isystem$(SLAM_PREFIX)/ThirdParty/spdlog/include \
	-isystem$(SLAM_PREFIX)/ThirdParty/rapidjson/include/ \
	-Wno-unused-function \
	-Wno-unused-variable \
	-Wno-unused-parameter \
	-Wno-sign-compare \
	-fno-strict-aliasing \
	-DVOC_DIR='"$(SLAM_PREFIX)/src/vocabulary/"' \
	-DVOC_VERSION='"$(VOC_VERSION)"' \
	-DRAPIDJSON_64BIT=1 \
	-DEIGEN_STACK_ALLOCATION_LIMIT=1024 \
	-MD \
	-finstrument-functions-exclude-file-list=eigen \
	-finstrument-functions-exclude-function-list=operator,iterator,from_row,to_col,get_stride,size,cols,rows,std

SLAM_CPPOPT  := -std=gnu++14 -fpermissive -fno-exceptions -Wno-reorder -Wno-missing-field-initializers -fno-strict-aliasing

SLAM_SHAVE_CCOPT := -nostdinc -Wno-c++11-extensions -Wno-literal-range -fno-strict-aliasing -fno-exceptions -Iinclude -Ieigen -Ileon \
   -I$(SLAM_PREFIX)/src/feature/descriptor \
   -I$(SLAM_PREFIX)/src/feature/detector \
   -I$(SLAM_PREFIX)/src/feature/tracker \
   -I$(SLAM_PREFIX)/src/tracker \
   -I$(SLAM_PLATFORM_PREFIX)/shared \
   -I$(SLAM_PLATFORM_PREFIX)/leon \
   -I$(SLAM_PLATFORM_PREFIX)/

SLAM_SHAVE_CPPOPT := -std=c++11 -Wno-c++11-extensions -Wno-literal-range -fno-strict-aliasing -fno-exceptions -Iinclude -Ieigen -Ileon -I$(SLAM_PLATFORM_PREFIX)/leon/ -isystem$(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)/common/moviCompile/include/c++ -isystem$(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)/common/moviCompile/include -mno-replace-jmp-with-bra-peephole

SHAVE_SEARCH_PATH = $(SLAM_PLATFORM_PREFIX)/shave

ENTRYPOINTS_detect = fast_detect
ENTRYPOINTS_track = fast_track
ENTRYPOINTS_stereo_initialize = stereo_match
ENTRYPOINTS_project_covariance = vision_project_motion_covariance vision_project_observation_covariance
ENTRYPOINTS_blis = startSGEMM startSGEMMTRSM_LL startSGEMMTRSM_LU startSGEMMTRSM_RU startSGEMMTRSM_RL
ENTRYPOINTS_orb = compute_descriptors
ENTRYPOINTS_cholesky = potrf_ln
SHAVES_IDX_detect             = 4 5 6 7
SHAVES_IDX_track              = 0 1 2 3
SHAVES_IDX_stereo_initialize  = 0 1 2 3
SHAVES_IDX_project_covariance = 0 1 2 3
SHAVES_IDX_blis               = 0 1 2 3
SHAVES_IDX_orb                = 4 5 6 7
SHAVES_IDX_cholesky           = 0

SHAVE_CPP_AUTOSTAT_SOURCES_detect += $(SHAVE_SEARCH_PATH)/stereo_initialize/common_shave.cpp
#SHAVE_CPP_AUTOSTAT_SOURCES_detect += $(SLAM_PLATFORM_PREFIX)/shared/fast9ScoreCv.cpp
SHAVE_ASM_AUTOSTAT_SOURCES_detect += $(SLAM_PLATFORM_PREFIX)/shared/fast9ScoreCv.asm
SHAVE_CPP_AUTOSTAT_SOURCES_track += $(SHAVE_SEARCH_PATH)/stereo_initialize/common_shave.cpp
SHAVE_ASM_AUTOSTAT_SOURCES_track += $(SLAM_PLATFORM_PREFIX)/shared/fast9ScoreCv.asm

SHAVE_CPP_AUTOSTAT_SOURCES_blis += $(shell realpath --relative-to=$$(pwd) $(wildcard \
  $(MV_COMMON_BASE)/components/kernelLib/LAMA/kernels/sgemm*/shave/src/cpp/*.cpp \
  $(MV_COMMON_BASE)/components/kernelLib/LAMA/kernels/strsm*/shave/src/cpp/*.cpp \
))
SHAVE_C_AUTOSTAT_SOURCES_blis += $(shell realpath --relative-to=$$(pwd) $(wildcard \
  $(MV_COMMON_BASE)/components/BLIS/shave/*.c \
))
SHAVE_ASM_AUTOSTAT_SOURCES_blis += $(shell realpath --relative-to=$$(pwd) $(wildcard \
  $(MV_COMMON_BASE)/components/kernelLib/LAMA/kernels/sgemm*/arch/ma2x5x/shave/src/*.asm \
  $(MV_COMMON_BASE)/components/kernelLib/LAMA/kernels/strsm*/arch/ma2x5x/shave/src/*.asm \
))
MVCCOPT += $(addprefix -I,$(wildcard \
   $(MV_COMMON_BASE)/components/kernelLib/LAMA/kernels/sgemm*/shave/include \
   $(MV_COMMON_BASE)/components/kernelLib/LAMA/kernels/strsm*/shave/include \
   $(MV_COMMON_BASE)/components/kernelLib/MvCV/include \
))
MVCOPT      += -I$(SLAM_PLATFORM_PREFIX)/shave/blis
MVCCOPT     += -I$(SLAM_PLATFORM_PREFIX)/shave/blis
MVCCOPT_LRT += -I$(SLAM_PLATFORM_PREFIX)/shave/blis
MVCCOPT     += -I$(SLAM_PLATFORM_PREFIX)/shared
MVCCOPT_PRT += -I$(SLAM_PLATFORM_PREFIX)/shared
CCOPT       += -I$(SLAM_PLATFORM_PREFIX)/shared
CCOPT_LRT   += -I$(SLAM_PLATFORM_PREFIX)/shared

SLAM_CCOPT += -DBLIS_VERSION_STRING=\"0.1.0-34\" -DLEON_USE_REAL_NUMBERS_ONLY
SLAM_CCOPT += $(addprefix -I, $(sort $(dir $(shell find $(MV_COMMON_BASE)/components/BLIS/leon -name "*.h"))))
SLAM_C_SOURCES +=             $(sort       $(shell find $(MV_COMMON_BASE)/components/BLIS/leon -name "*.c"))
