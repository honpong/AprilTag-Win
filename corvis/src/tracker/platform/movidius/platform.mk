# SLAM_PREFIX must be defined before including this makefile
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
SLAM_SOURCES += $(SLAM_PREFIX)/corvis/src/feature/descriptor/orb_descriptor.cpp
SLAM_SOURCES += $(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.cpp)

SLAM_SOURCES += $(addprefix $(SLAM_PREFIX)/ThirdParty/DBoW2/src/, \
    BowVector.cpp \
    FeatureVector.cpp \
    ScoringObject.cpp \
)

SLAM_C_SOURCES := $(addprefix $(SLAM_PREFIX)/ThirdParty/vlfeat-0.9.18/vl/, \
    liop.c \
    imopv.c \
    kmeans.c \
    generic.c \
    mathop.c \
)
SLAM_C_SOURCES += $(wildcard $(SLAM_PLATFORM_PREFIX)/leon/*.c)

SLAM_C_SOURCES += $(SLAM_PREFIX)/corvis/src/vocabulary/voc.c

SLAM_HEADERS := \
	$(wildcard $(SLAM_PREFIX)/corvis/src/tracker/*.h) \
	$(wildcard $(SLAM_PREFIX)/corvis/src/tracker/fast_detector/*.h) \
	$(wildcard $(SLAM_PREFIX)/ThirdParty/DBoW2/include/*.h) \
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
	-I$(SLAM_PREFIX)/ThirdParty/eigen \
	-I$(SLAM_PREFIX)/ThirdParty/spdlog/include \
	-I$(SLAM_PREFIX)/ThirdParty/rapidjson/include/ \
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

SLAM_CPPOPT  := -std=gnu++11 -fpermissive -fno-exceptions -Wno-reorder -Wno-missing-field-initializers -fno-strict-aliasing -MD

SLAM_SHAVE_CCOPT := -nostdinc -Wno-c++11-extensions -Wno-literal-range -fno-strict-aliasing -fno-exceptions -Iinclude -Ieigen -Ileon -I$(SLAM_PLATFORM_PREFIX)/leon/ -I$(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)/common/moviCompile/include/c++
SLAM_SHAVE_CPPOPT := -std=c++11 -Wno-c++11-extensions -Wno-literal-range -fno-strict-aliasing -fno-exceptions -Iinclude -Ieigen -Ileon -I$(SLAM_PLATFORM_PREFIX)/leon/ -I$(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)/common/moviCompile/include/c++

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

cvrt_fns = fast9Detect fast9Track stereo_kp_matching_and_compare vision_project_motion_covariance
cvrt_fns += potrf_ln trsvl_ln trsvl_lt trsvl_lnlt
ENTRYPOINTS_cvrt = -e $(word 1, $(cvrt_fns)) $(foreach ep,$(cvrt_fns),-u $(ep) ) --gc-sections

ENTRYPOINTS_blis = -e startSGEMM -u startSGEMMTRSM_LL -u startSGEMMTRSM_LU -u startSGEMMTRSM_RU -u startSGEMMTRSM_RL --gc-sections

SHAVE_APP_LIBS += $(cvrt).mvlib
SHAVE0_APPS += $(cvrt).shv0lib
SHAVE1_APPS += $(cvrt).shv1lib
SHAVE2_APPS += $(cvrt).shv2lib
SHAVE3_APPS += $(cvrt).shv3lib
SHAVE4_APPS += $(blis).shv4lib

PROJECTCLEAN += $(SHAVE_APP_LIBS) $(SHAVE0_APPS) $(SHAVE1_APPS) $(SHAVE2_APPS) $(SHAVE3_APPS) $(SHAVE4_APPS)

#include $(MV_COMMON_BASE)/generic.mk	
