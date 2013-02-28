#BUILD_CAMERA:=TRUE

############################ DEFINITIONS ###########################

SWIG := swig
WXGLADE := wxglade
CC := gcc
CXX := g++
PYTHON_CFLAGS := `python-config --includes` -I/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/numpy/core/include
PYTHON_LDFLAGS := `python-config --libs`
# -ftree-vectorize -ffast-math 
CPPFLAGS := -Wall -pthread -g -Icor -fPIC -march=core2 -mfpmath=sse -O3 $(PYTHON_CFLAGS) -I/opt/local/include -I/opt/local/include/FTGL -I/opt/local/include/freetype2
CFLAGS := -std=gnu99
LDFLAGS := -pthread -lpthread --warn-unresolved-symbols -O1 $(PYTHON_LDFLAGS) -framework Accelerate -framework OpenGL -framework GLUT -L/opt/local/lib -lftgl -lfreetype
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

############################## SHELL ############################

d := shell

SUBDIRS := $(SUBDIRS) $(d)

SHELL_SOURCES := $(addprefix $(d)/, cor.c sigint.c)

$(d)/cor.o: CFLAGS := $(CFLAGS) -x objective-c
$(d)/cor: cor/_cor.so
$(d)/cor: LDFLAGS := $(LDFLAGS) -framework Cocoa
$(d)/cor: $(SHELL_SOURCES:.c=.o)

TARGETS := $(TARGETS) $(d)/cor
SOURCES := $(SOURCES) $(SHELL_SOURCES)


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

############################ DRIVERS ##########################

d := drivers
SUBDIRS := $(SUBDIRS) $(d)

#explicitly add targets to clean since they aren't always generated

CAMERA_SOURCES := $(addprefix $(d)/, _camera.c camera.c debayer.c)
$(d)/_camera.so: $(CAMERA_SOURCES:.c=.o)
$(d)/_camera.so: LDFLAGS := $(LDFLAGS) -ldc1394
CLEAN := $(CLEAN) $(d)/camera.py $(d)/_camera.so
SECONDARY := $(SECONDARY) $(d)/_camera.c $(d)/_camera.so

ifdef DRIVERS
	BUILD_CAMERA := TRUE
endif

ifdef BUILD_CAMERA
	TARGETS := $(TARGETS) $(d)/_camera.so
	SOURCES := $(SOURCES) $(CAMERA_SOURCES)
	DEPS := $(DEPS) $(d)/_camera.i.d
endif

############################## VIS ############################

d := vis
SUBDIRS := $(SUBDIRS) $(d)
#so that we don't delete this, with its handlers
all: $(d)/vis_gui.py

############################## TRACKER ############################

d := tracker
SUBDIRS := $(SUBDIRS) $(d)

TRACKER_SOURCES := $(addprefix $(d)/, _tracker.c tracker.c)
$(d)/_tracker.so: LDFLAGS := $(LDFLAGS) `pkg-config --libs opencv`
$(d)/_tracker.so: $(TRACKER_SOURCES:.c=.o)

TARGETS := $(TARGETS) $(d)/_tracker.so
CLEAN := $(CLEAN) $(d)/tracker.py
SECONDARY := $(SECONDARY) $(d)/_tracker.c
SOURCES := $(SOURCES) $(TRACKER_SOURCES)
DEPS := $(DEPS) $(d)/_tracker.i.d

############################## CALIBRATION ############################

d := calibration

SUBDIRS := $(SUBDIRS) $(d)

CALIBRATION_SOURCES := $(addprefix $(d)/, _calibration.c calibration.c)
$(d)/_calibration.so: LDFLAGS := $(LDFLAGS) `pkg-config --libs opencv`
$(d)/_calibration.so: $(CALIBRATION_SOURCES:.c=.o)

TARGETS := $(TARGETS) $(d)/_calibration.so
CLEAN := $(CLEAN) $(d)/calibration.py
SECONDARY := $(SECONDARY) $(d)/_calibration.c
SOURCES := $(SOURCES) $(CALIBRATION_SOURCES)
DEPS := $(DEPS) $(d)/_calibration.i.d

############################## FILTER ############################

d := filter
SUBDIRS := $(SUBDIRS) $(d)

FILTER_SOURCES := $(addprefix $(d)/, _filter.cpp filter.cpp model.cpp observation.cpp filter_setup.cpp)
$(d)/_filter.so: CC := $(CXX)
$(d)/_filter.so: LDFLAGS := $(LDFLAGS)
$(d)/_filter.so: $(FILTER_SOURCES:.cpp=.o)

TARGETS := $(TARGETS) $(d)/_filter.so
CLEAN := $(CLEAN) $(d)/filter.py
SECONDARY := $(SECONDARY) $(d)/_filter.cpp
CXX_SOURCES := $(CXX_SOURCES) $(FILTER_SOURCES)
DEPS := $(DEPS) $(d)/_filter.ipp.d

############################## MAPPER ############################

d := mapper
SUBDIRS := $(SUBDIRS) $(d)

MAPPER_SOURCES := $(addprefix $(d)/, _mapper.cpp mapper.cpp)
$(d)/_mapper.so: CC := $(CXX)
$(d)/_mapper.so: LDFLAGS := $(LDFLAGS)
$(d)/_mapper.so: $(MAPPER_SOURCES:.cpp=.o)

TARGETS := $(TARGETS) $(d)/_mapper.so
CLEAN := $(CLEAN) $(d)/mapper.py
SECONDARY := $(SECONDARY) $(d)/_mapper.cpp
CXX_SOURCES := $(CXX_SOURCES) $(MAPPER_SOURCES)
DEPS := $(DEPS) $(d)/_mapper.ipp.d

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

############################## RECOGNITION ############################

d := recognition
SUBDIRS := $(SUBDIRS) $(d)

RECOGNITION_CXX_SOURCES := $(addprefix $(d)/, _recognition.cpp recognition.cpp)
$(d)/_recognition.so: CC := $(CXX)
$(d)/_recognition.so: LDFLAGS := $(LDFLAGS) -Lvlfeat/bin/maci64 -lvl
$(d)/_recognition.so: CPPFLAGS := $(CPPFLAGS) -Ivlfeat
$(d)/_recognition.so: $(RECOGNITION_SOURCES:.c=.o)
$(d)/_recognition.so: $(RECOGNITION_CXX_SOURCES:.cpp=.o)

TARGETS := $(TARGETS) $(d)/_recognition.so
CLEAN := $(CLEAN) $(d)/recognition.py
SECONDARY := $(SECONDARY) $(d)/_recognition.cpp
SOURCES := $(SOURCES) $(RECOGNITION_SOURCES)
CXX_SOURCES := $(CXX_SOURCES) $(RECOGNITION_CXX_SOURCES)
DEPS := $(DEPS) $(d)/_recognition.ipp.d

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
