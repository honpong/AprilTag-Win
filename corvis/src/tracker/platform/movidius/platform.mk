MAKEDEPEND = yes

# SLAM_PREFIX must be defined before including this makefile
SLAM_PLATFORM_PREFIX := $(SLAM_PREFIX)/corvis/src/tracker/platform/movidius
SLAM_SOURCES := $(addprefix $(SLAM_PREFIX)/corvis/src/tracker/, \
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
SLAM_SOURCES += $(SLAM_PREFIX)/corvis/src/feature/descriptor/orb_descriptor.cpp
SLAM_SOURCES += $(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.cpp)

SLAM_C_SOURCES := $(addprefix $(SLAM_PREFIX)/ThirdParty/vlfeat-0.9.18/vl/, \
    liop.c \
    imopv.c \
    kmeans.c \
    generic.c \
    mathop.c \
)
SLAM_C_SOURCES += $(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.c)

#Commented out to prevent loading vocabulary in TM2 project
SLAM_SOURCES   += $(SLAM_PREFIX)/corvis/src/vocabulary/resource.cpp
SLAM_C_SOURCES += $(SLAM_PREFIX)/corvis/src/vocabulary/voc.c

$(SLAM_PREFIX)/corvis/src/vocabulary/voc.c: \
$(SLAM_PREFIX)/corvis/src/vocabulary/voc.bin

SLAM_HEADERS := \
	$(wildcard $(SLAM_PREFIX)/corvis/src/tracker/*.h) \
	$(wildcard $(SLAM_PREFIX)/ThirdParty/DBoW2/include/*.h) \
	$(wildcard $(SLAM_PREFIX)/corvis/src/feature/detector/*.h) \
	$(wildcard $(SLAM_PREFIX)/corvis/src/feature/descriptor/*.h) \
	$(wildcard $(SLAM_PLATFORM_PREFIX)/*.h) \
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
	-I$(SLAM_PREFIX)/corvis/src/vocabulary \
	-I$(SLAM_PREFIX)/ThirdParty/DBoW2/include \
	-isystem$(SLAM_PREFIX)/ThirdParty/eigen \
	-isystem$(SLAM_PREFIX)/ThirdParty/spdlog/include \
	-isystem$(SLAM_PREFIX)/ThirdParty/rapidjson/include/ \
	-Wno-unused-function \
	-Wno-unused-variable \
	-Wno-unused-parameter \
	-Wno-sign-compare \
	-fno-strict-aliasing \
	-DVOC_DIR='"$(SLAM_PREFIX)/corvis/src/vocabulary/"' \
	-DRAPIDJSON_64BIT=1 \
	-DEIGEN_STACK_ALLOCATION_LIMIT=1024 \
	-MD \
	-finstrument-functions-exclude-file-list=eigen \
	-finstrument-functions-exclude-function-list=operator,iterator,from_row,to_col,get_stride,size,cols,rows,std

SLAM_CPPOPT  := -std=gnu++11 -fpermissive -fno-exceptions -Wno-reorder -Wno-missing-field-initializers -fno-strict-aliasing

SLAM_SHAVE_CCOPT := -nostdinc -Wno-c++11-extensions -Wno-literal-range -fno-strict-aliasing -fno-exceptions -Iinclude -Ieigen -Ileon \
   -I$(SLAM_PREFIX)/corvis/src/feature/descriptor \
   -I$(SLAM_PREFIX)/corvis/src/feature/detector \
   -I$(SLAM_PREFIX)/corvis/src/feature/tracker \
   -I$(SLAM_PLATFORM_PREFIX)/shared \
   -I$(SLAM_PLATFORM_PREFIX)/leon

SLAM_SHAVE_CPPOPT := -std=c++11 -Wno-c++11-extensions -Wno-literal-range -fno-strict-aliasing -fno-exceptions -Iinclude -Ieigen -Ileon -I$(SLAM_PLATFORM_PREFIX)/leon/ -I$(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)/common/moviCompile/include/c++ -mno-replace-jmp-with-bra-peephole

SHAVE_SEARCH_PATH = $(SLAM_PLATFORM_PREFIX)/shave

ENTRYPOINTS_cvrt = fast_detect fast_track
ENTRYPOINTS_stereo_initialize = stereo_match
ENTRYPOINTS_project_covariance = vision_project_motion_covariance vision_project_observation_covariance1 vision_project_observation_covariance
ENTRYPOINTS_blis = startSGEMM startSGEMMTRSM_LL startSGEMMTRSM_LU startSGEMMTRSM_RU startSGEMMTRSM_RL
SHAVES_IDX_cvrt               = 0 1 2 3 4 5 6 7
SHAVES_IDX_stereo_initialize  = 0 1 2 3
SHAVES_IDX_project_covariance = 0 1 2 3
SHAVES_IDX_blis               = 0 1 2 3

SHAVE_CPP_AUTOSTAT_SOURCES_cvrt += $(SHAVE_SEARCH_PATH)/stereo_initialize/common_shave.cpp

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
MVCCOPT += -I$(SLAM_PLATFORM_PREFIX)/shave/blis

MVCCOPT += -I$(SLAM_PLATFORM_PREFIX)/shared
CCOPT   += -I$(SLAM_PLATFORM_PREFIX)/shared

SLAM_CCOPT += -DBLIS_VERSION_STRING=\"0.1.0-34\" -DLEON_USE_REAL_NUMBERS_ONLY
SLAM_CCOPT += $(addprefix -I, $(sort $(dir $(shell find $(MV_COMMON_BASE)/components/BLIS/leon -name "*.h"))))
SLAM_C_SOURCES +=             $(sort       $(shell find $(MV_COMMON_BASE)/components/BLIS/leon -name "*.c"))
