# BLIS/GEMM related stuff
VERS_STR            = 0.1.0-34
BLIS_VERSION_STRING = \"$(VERS_STR)\"
VERSION             = BLIS_VERSION_STRING=\"$(VERS_STR)\"

SLAM_CCOPT   += -DBLIS -DVERSION -DBLIS_VERSION_STRING=\"$(VERS_STR)\" -DLEON_USE_REAL_NUMBERS_ONLY

include $(MV_COMMON_BASE)/components/BLIS/genblislib.mk

SLAM_CCOPT   += -I$(MV_COMMON_BASE)/components/BLIS/leon/config/reference $(BLIS_LEON_HEADERS_I)

SLAM_LEON_LIBS := $(BLIS_LIB_LEON)

# SHAVE linking rules
$(cvrt).mvlib : $(SHAVE_cvrt_OBJS) $(PROJECT_SHAVE_LIBS)
	$(ECHO) $(LD) $(ENTRYPOINTS_cvrt) $(MVLIBOPT) 			\
                      $(SHAVE_cvrt_OBJS) $(PROJECT_SHAVE_LIBS) 		\
                      $(CompilerANSILibs) -o $@

$(blis).mvlib : blis_lib $(BLIS_LIB_SHAVE) $(PROJECT_SHAVE_LIBS)
	$(ECHO) $(LD) $(ENTRYPOINTS_blis) $(MVLIBOPT) 			\
                      $(BLIS_LIB_SHAVE) $(PROJECT_SHAVE_LIBS) 		\
                      $(CompilerANSILibs) -o $@
