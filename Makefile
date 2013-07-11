#BUILD_CAMERA:=TRUE

############################ DEFINITIONS ###########################

SWIG := swig
WXGLADE := wxglade
CC := gcc
CXX := g++
PYTHON_CFLAGS := `python-config --includes` -I/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/numpy/core/include
PYTHON_LDFLAGS := `python-config --libs`
# -ftree-vectorize -ffast-math 
CPPFLAGS := -Wall -pthread -g -Icor -fPIC -march=core2 -mfpmath=sse -O0 -D DEBUG $(PYTHON_CFLAGS) -I/opt/local/include -I/opt/local/include/FTGL -I/opt/local/include/freetype2
CFLAGS := -std=gnu99
LDFLAGS := -pthread -lpthread --warn-unresolved-symbols -O0 $(PYTHON_LDFLAGS) -framework Accelerate -framework OpenGL -framework GLUT -L/opt/local/lib -lftgl -lfreetype
SWIGFLAGS := -Wall

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

%.cpp: %.ipp
	$(SWIG) -python -c++ -threads $(SWIGFLAGS) -o $@ $<

%.c: %.i
	$(SWIG) -python -threads $(SWIGFLAGS) -o $@ $<

%.py: %.wxg
	$(WXGLADE) -g python -o $@ $<

%.i.d: %.i
	@set -e; rm -f $@; \
        $(SWIG) -MM $< > $@.$$$$; \
        sed 's,\($*\)_wrap\.c[ :]*,\1.c $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

%.ipp.d: %.ipp
	@set -e; rm -f $@; \
        $(SWIG) -c++ -MM $< > $@.$$$$; \
        sed 's,\($*\)_wrap\.cxx[ :]*,\1.cpp $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

%.d: %.c
	@set -e; rm -f $@; \
        $(CC) $(CFLAGS) $(CPPFLAGS) -MP -MM $< -MF $@.$$$$ -MT $@; \
        sed 's,\($*\)\.d[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

%.dpp: %.cpp
	@set -e; rm -f $@; \
        $(CXX) $(CXXFLAGS) $(CPPFLAGS) -MP -MM $< -MF $@.$$$$ -MT $@; \
        sed 's,\($*\)\.dpp[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

%.so: %.o
	$(CC) -shared -undefined suppress -flat_namespace $(LDFLAGS) $(PYTHON_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

############################## COR ############################

d := cor

SUBDIRS := $(SUBDIRS) $(d)

COR_SOURCES := $(addprefix $(d)/, packet.c mapbuffer.c inbuffer.c timestamp.c plugins.c _cor.c dispatch.c debug.c)
$(d)/_cor.o: CPPFLAGS := -Wall -pthread -g -Icor -fPIC $(PYTHON_CFLAGS) -march=nocona -mfpmath=sse -fno-strict-aliasing -ffast-math -O3 -framework vecLib -I/opt/local/include

$(d)/_cor.so: $(COR_SOURCES:.c=.o)

TARGETS := $(TARGETS) $(d)/_cor.so
CLEAN := $(CLEAN) $(d)/cor.py
SECONDARY := $(SECONDARY) $(d)/_cor.c
SOURCES := $(SOURCES) $(COR_SOURCES)
DEPS := $(DEPS) $(d)/_cor.i.d

############################## NUMERICS ############################
d := numerics

SUBDIRS := $(SUBDIRS) $(d)

NUMERICS_CXX_SOURCES := $(addprefix $(d)/, vec4.cpp matrix.cpp kalman.cpp _numerics.cpp)

$(d)/_numerics.so: LDFLAGS := $(LDFLAGS) -lm -lcblas -latlas -llapack -lstdc++ -framework vecLib
#$(d)/_numerics.so: LD := gfortran
$(d)/_numerics.so: $(NUMERICS_CXX_SOURCES:.cpp=.o)

TARGETS := $(TARGETS) $(d)/_numerics.so
CLEAN := $(CLEAN) $(d)/numerics.py
SECONDARY := $(SECONDARY) $(d)/_numerics.cpp $(d)/_numerics.py
CXX_SOURCES := $(CXX_SOURCES) $(NUMERICS_CXX_SOURCES)
DEPS := $(DEPS) $(d)/_numerics.ipp.d

############################## FILTER ############################

d := filter
SUBDIRS := $(SUBDIRS) $(d)

FILTER_CXX_SOURCES := $(addprefix $(d)/, _filter.cpp filter.cpp model.cpp observation.cpp filter_setup.cpp detector_fast.cpp tracker_fast.cpp tracker_pyramid.cpp fast_detector/fast_9.cpp)
$(d)/_filter.so: CC := $(CXX)
$(d)/_filter.so: LDFLAGS := $(LDFLAGS) `pkg-config --libs opencv`
$(d)/_filter.so: $(FILTER_CXX_SOURCES:.cpp=.o)

TARGETS := $(TARGETS) $(d)/_filter.so
CLEAN := $(CLEAN) $(d)/filter.py
SECONDARY := $(SECONDARY) $(d)/_filter.cpp
CXX_SOURCES := $(CXX_SOURCES) $(FILTER_CXX_SOURCES)
DEPS := $(DEPS) $(d)/_filter.ipp.d

############################## RENDERABLE ############################

d := renderable
SUBDIRS := $(SUBDIRS) $(d)

RENDERABLE_SOURCES := $(addprefix $(d)/, _renderable.cpp renderable.cpp)
$(d)/_renderable.so: CC := $(CXX)
$(d)/_renderable.so: LDFLAGS := $(LDFLAGS)
$(d)/_renderable.so: $(RENDERABLE_SOURCES:.cpp=.o)

TARGETS := $(TARGETS) $(d)/_renderable.so
CLEAN := $(CLEAN) $(d)/renderable.py
SECONDARY := $(SECONDARY) $(d)/_renderable.cpp
CXX_SOURCES := $(CXX_SOURCES) $(RENDERABLE_SOURCES)
DEPS := $(DEPS) $(d)/_renderable.ipp.d

##################################################################

DEPS := $(DEPS) $(SOURCES:.c=.d) $(CXX_SOURCES:.cpp=.dpp)

.SECONDARY: $(SECONDARY)

all: $(TARGETS)

clean:
	rm -f $(CLEAN) $(TARGETS) $(SECONDARY)
	rm -f $(addsuffix /*.[od], $(SUBDIRS))
	rm -f $(addsuffix /*.dpp, $(SUBDIRS))
	rm -f $(addsuffix /*.dpp.*, $(SUBDIRS))
	rm -f $(addsuffix /*.d.*, $(SUBDIRS))
	rm -f $(addsuffix /*.pyc, $(SUBDIRS))

include $(DEPS)
