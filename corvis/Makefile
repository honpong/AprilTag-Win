#BUILD_CAMERA:=TRUE

############################ DEFINITIONS ###########################

SWIG := swig
WXGLADE := wxglade
CC := gcc
CXX := g++
PYTHON_CFLAGS := $(shell python-config --includes) -I$(shell python -c 'import numpy; print numpy.__path__[0] + "/core/include"')
PYTHON_LDFLAGS := $(shell python-config --libs)
# -ftree-vectorize -ffast-math 
CPPFLAGS := -Wall -g -fPIC -march=core2 -mfpmath=sse -O3 -D DEBUG $(PYTHON_CFLAGS) \
        $(patsubst %,-I%/include,$(wildcard $(shell brew --prefix boost 2>/dev/null) /opt/local)) \
        $(shell pkg-config ftgl --cflags) -Isrc/cor -Wdeprecated-writable-strings
CFLAGS := -std=gnu99
LDFLAGS := -pthread -lpthread --warn-unresolved-symbols -O3 $(PYTHON_LDFLAGS) -framework Accelerate -framework OpenGL -framework GLUT $(shell pkg-config ftgl --libs)
SWIGFLAGS := -w403,362,389,503,401,462,472,314,509,204

####################### RULES ######################################

all:

.PHONY: all clean

Q=$(if $V,,@)
_CC := $(CC)
_CXX := $(CXX)
_SWIG := $(SWIG)
_AS := $(AS)
_WXGLADE := $(WXGLADE)

%.o: override CC=$(Q)printf "%32s %s\n" "Compiling" $<; $(_CC)
%.o: override CXX=$(Q)printf "%32s %s\n" "Compiling" $<; $(_CXX)
%.d: override CC=printf "%32s %s\n" "Checking dependencies" $<; $(_CC)
%.c: override SWIG=$(Q)printf "%32s %s\n" "SWIG wrapping" $(@); $(_SWIG)
%.cpp: override SWIG=$(Q)printf "%32s %s\n" "SWIG wrapping" $(@); $(_SWIG)
%.py: override WXGLADE=$(Q)printf "%32s %s\n" "Generating GUI" $<; $(_WXGLADE)
%.i.d: override SWIG=printf "%32s %s\n" "Checking dependencies" $<; $(_SWIG)
%.o: override AS=$(Q)printf "%32s %s\n" "Assembling" $<; $(_AS)
%.so: override CC=$(Q)printf "%32s %s\n" "Linking" $(@); $(_CC)
cor/cor:override CC=$(Q)printf "%32s %s\n" "Linking" $(@); $(_CC)

#%.cpp:
#	@mkdir -p $(@D)
#	$(SWIG) -python -c++ -threads $(SWIGFLAGS) -o $@ $<

#%.c:
#	@mkdir -p $(@D)
#	$(SWIG) -python -threads $(SWIGFLAGS) -o $@ $<

#%.cpp: %.ipp
#	@mkdir -p $(@D)
#	$(SWIG) -python -c++ -threads $(SWIGFLAGS) -o $@ $<

#%.c: %.i
#	@mkdir -p $(@D)
#	$(SWIG) -python -threads $(SWIGFLAGS) -o $@ $<

%.py: %.wxg
	@mkdir -p $(@D)
	$(WXGLADE) -g python -o $@ $<

build/%.i.d: swig/%.i
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(SWIG) -Isrc/cor -MM $< > $@.$$$$; \
        sed 's,\($*\)_wrap\.c[ :]*,\1.c $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

build/%.ipp.d: swig/%.ipp
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(SWIG) -c++ -MM -Isrc/filter -Isrc/numerics $< > $@.$$$$; \
        sed 's,\($*\)_wrap\.cxx[ :]*,\1.cpp $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

build/%.d: src/%.c
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(CC) $(CFLAGS) $(CPPFLAGS) -MP -MM $< -MF $@.$$$$ -MT $@; \
        sed 's,\($*\)\.d[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

build/%.dpp: src/%.cpp
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(CXX) $(CXXFLAGS) $(CPPFLAGS) -MP -MM $< -MF $@.$$$$ -MT $@; \
        sed 's,\($*\)\.dpp[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

build/%.d: build/%.c
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(CC) $(CFLAGS) $(CPPFLAGS) -MP -MM $< -MF $@.$$$$ -MT $@; \
        sed 's,\($*\)\.d[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

build/test/%.dpp: test/%.cpp
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(CXX) $(CXXFLAGS) $(CPPFLAGS) -MP -MM $< -MF $@.$$$$ -MT $@; \
        sed 's,\($*\)\.dpp[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

build/%.dpp: build/%.cpp
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(CXX) $(CXXFLAGS) $(CPPFLAGS) -MP -MM $< -MF $@.$$$$ -MT $@; \
        sed 's,\($*\)\.dpp[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

build/test/%.dcc: ../gtest/%.cc
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(CXX) $(CXXFLAGS) $(CPPFLAGS) -MP -MM $< -MF $@.$$$$ -MT $@; \
        sed 's,\($*\)\.dpp[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

%.so:
	@mkdir -p $(@D)
	$(CC) -shared -undefined suppress -flat_namespace $(LDFLAGS) $(PYTHON_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

build/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

%/__init__.py:
	@mkdir -p $(@D)
	touch $@


############################## COR ############################

d := cor

COR_SOURCES := $(addprefix src/$(d)/, packet.c mapbuffer.c inbuffer.c timestamp.c plugins.c dispatch.c debug.c) build/$(d)/_cor.c

build/$(d)/_cor.c: swig/_cor.i
	@mkdir -p $(@D)
	@mkdir -p bin/cor
	$(SWIG) -python -threads $(SWIGFLAGS) -Isrc/cor -o $@ -outdir bin/cor $<

bin/$(d)/_cor.so: $(patsubst src/%,build/%,$(COR_SOURCES:.c=.o))

TARGETS := $(TARGETS) bin/$(d)/_cor.so bin/$(d)/__init__.py
SOURCES := $(SOURCES) $(COR_SOURCES) build/cor/_cor.c
DEPS := $(DEPS) build/_cor.i.d

############################## NUMERICS ############################
d := numerics

NUMERICS_CXX_SOURCES := $(addprefix src/$(d)/, vec4.cpp matrix.cpp kalman.cpp) build/$(d)/_numerics.cpp

build/$(d)/_numerics.cpp: swig/_numerics.ipp
	@mkdir -p $(@D)
	@mkdir -p bin/numerics
	$(SWIG) -python -c++ -threads $(SWIGFLAGS) -Isrc/numerics -o $@ -outdir bin/numerics $<

bin/$(d)/_numerics.so: LDFLAGS := $(LDFLAGS) -lm -lcblas -llapack -lstdc++
bin/$(d)/_numerics.so: $(patsubst src/%,build/%,$(NUMERICS_CXX_SOURCES:.cpp=.o))

TARGETS := $(TARGETS) bin/$(d)/_numerics.so bin/$(d)/__init__.py
CXX_SOURCES := $(CXX_SOURCES) $(NUMERICS_CXX_SOURCES)
DEPS := $(DEPS) build/_numerics.ipp.d

############################## FILTER ############################

d := filter

FILTER_CXX_SOURCES := $(addprefix src/$(d)/, filter.cpp state_vision.cpp state_motion.cpp state.cpp observation.cpp filter_setup.cpp fast_detector/fast_9.cpp scaled_mask.cpp camera_control_interface_dummy.cpp) build/$(d)/_filter.cpp

FILTER_SOURCES := $(addprefix src/$(d)/, device_parameters.c)

CPPFLAGS := $(CPPFLAGS) -Isrc/filter
build/$(d)/_filter.cpp: swig/_filter.ipp
	@mkdir -p $(@D)
	@mkdir -p bin/filter
	$(SWIG) -python -c++ -threads $(SWIGFLAGS) -Isrc/filter -o $@ -outdir bin/filter $<

#bin/$(d)/_filter.so: LDFLAGS := $(LDFLAGS) `pkg-config --libs opencv`
bin/$(d)/_filter.so:$(patsubst src/%,build/%,$(FILTER_CXX_SOURCES:.cpp=.o)) $(patsubst src/%,build/%,$(FILTER_SOURCES:.c=.o))

TARGETS := $(TARGETS) bin/$(d)/_filter.so bin/$(d)/__init__.py
CXX_SOURCES := $(CXX_SOURCES) $(FILTER_CXX_SOURCES)
SOURCES := $(SOURCES) $(FILTER_SOURCES)
DEPS := $(DEPS) build/_filter.ipp.d


############################## STEREO ############################

d := stereo

STEREO_CXX_SOURCES := $(addprefix src/$(d)/, stereo.cpp stereo_mesh.cpp camera.cpp fundamental.cpp stereo_features.cpp)

STEREO_SOURCES :=

CPPFLAGS := $(CPPFLAGS) -Isrc/stereo -Isrc/filter -Isrc/qhull/src/libqhull/ -Isrc/vlfeat-0.9.18/vl -Isrc/libDAI-0.3.1/include

bin/$(d)/_stereo.so:$(patsubst src/%,build/%,$(STEREO_CXX_SOURCES:.cpp=.o)) $(patsubst src/%,build/%,$(STEREO_SOURCES:.c=.o)) bin/libDAI-0.3.1/_libDAI.so bin/vlfeat-0.9.18/_vl.so

TARGETS := $(TARGETS) bin/$(d)/_stereo.so bin/$(d)/__init__.py
CXX_SOURCES := $(CXX_SOURCES) $(STEREO_CXX_SOURCES)
SOURCES := $(SOURCES) $(STEREO_SOURCES)

############################## QHull ############################
d := qhull

QHULL_C_SOURCES := $(addprefix src/$(d)/src/libqhull/, geom.c geom2.c global.c io.c libqhull.c mem.c merge.c poly.c poly2.c qset.c random.c rboxlib.c stat.c user.c usermem.c userprintf.c userprintf_rbox.c)

CPPFLAGS := $(CPPFLAGS)

bin/$(d)/_qhull.so:$(patsubst src/%,build/%,$(QHULL_C_SOURCES:.c=.o))

TARGETS := $(TARGETS) bin/$(d)/_qhull.so
SOURCES := $(SOURCES) $(QHULL_C_SOURCES)

############################## DAI ############################
d := libDAI-0.3.1

DAI_CXX_SOURCES := $(addprefix src/$(d)/src/, alldai.cpp graph.cpp regiongraph.cpp emalg.cpp bipgraph.cpp evidence.cpp io.cpp trwbp.cpp bp.cpp exactinf.cpp util.cpp bp_dual.cpp exceptions.cpp varset.cpp factor.cpp weightedgraph.cpp clustergraph.cpp factorgraph.cpp dag.cpp)

CPPFLAGS := $(CPPFLAGS) -Isrc/$(d)/include -I/usr/local/include -DMACOSX -DDAI_WITH_BP -DDAI_WITH_TRWBP

bin/$(d)/_libDAI.so:$(patsubst src/%,build/%,$(DAI_CXX_SOURCES:.cpp=.o))

TARGETS := $(TARGETS) bin/$(d)/_libDAI.so
CXX_SOURCES := $(CXX_SOURCES) $(DAI_CXX_SOURCES)

############################## VLFEAT ############################
d := vlfeat-0.9.18
 
VL_C_SOURCES := $(addprefix src/$(d)/, vl/aib.c vl/array.c vl/covdet.c vl/dsift.c vl/fisher.c vl/generic.c vl/getopt_long.c vl/gmm.c vl/hikmeans.c vl/hog.c vl/homkermap.c vl/host.c vl/ikmeans.c vl/imopv.c vl/imopv_sse2.c vl/kdtree.c vl/kmeans.c vl/lbp.c vl/liop.c vl/mathop.c vl/mathop_avx.c vl/mathop_sse2.c vl/mser.c vl/pgm.c vl/quickshift.c vl/random.c vl/rodrigues.c vl/scalespace.c vl/sift.c vl/slic.c vl/stringop.c vl/svm.c vl/svmdataset.c vl/vlad.c)

CPPFLAGS := $(CPPFLAGS)

bin/$(d)/_vl.so:$(patsubst src/%,build/%,$(VL_C_SOURCES:.c=.o))

TARGETS := $(TARGETS) bin/$(d)/_vl.so
SOURCES := $(SOURCES) $(VL_C_SOURCES)

############################## TEST ############################

d := test

TEST_CXX_SOURCES := $(addprefix $(d)/, test.cpp numerics/vec4.cpp stereo/stereo.cpp stereo/camera.cpp numerics/quaternion.cpp)

CPPFLAGS := $(CPPFLAGS) -I../ -Isrc/numerics -Itest
LDFLAGS := $(LDFLAGS) -lstdc++

build/$(d)/%.o: ../gtest/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

build/$(d)/%.o: $(d)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

bin/test: build/$(d)/gtest_main.o build/$(d)/gtest-all.o $(patsubst test/%,build/test/%,$(TEST_CXX_SOURCES:.cpp=.o)) $(addprefix build/numerics/, vec4.o matrix.o) $(addprefix build/filter/, fast_detector/fast_9.o scaled_mask.o) bin/stereo/_stereo.so bin/libDAI-0.3.1/_libDAI.so bin/qhull/_qhull.so
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(PYTHON_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

TARGETS := $(TARGETS) bin/test
#CXX_SOURCES := $(CXX_SOURCES) $(TEST_CXX_SOURCES)

##################################################################

TARGETS := $(TARGETS) bin/__init__.py

DEPS := $(DEPS) $(patsubst src/%,build/%, $(SOURCES:.c=.d) $(CXX_SOURCES:.cpp=.dpp)) build/test/gtest_main.dcc build/test/gtest-all.dcc $(patsubst test/%,build/test/%, $(TEST_CXX_SOURCES:.cpp=.dpp))

all: $(TARGETS)

clean:
	rm -rf build bin

-include $(DEPS)
