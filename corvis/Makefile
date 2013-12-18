#BUILD_CAMERA:=TRUE

############################ DEFINITIONS ###########################

SWIG := swig
WXGLADE := wxglade
CC := gcc
CXX := g++
PYTHON_CFLAGS := `python-config --includes` -I/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/numpy/core/include
PYTHON_LDFLAGS := `python-config --libs`
# -ftree-vectorize -ffast-math 
CPPFLAGS := -Wall -pthread -g -fPIC -march=core2 -mfpmath=sse -O0 -D DEBUG $(PYTHON_CFLAGS) -I/opt/local/include -I/opt/local/include/FTGL -I/opt/local/include/freetype2 -Isrc/cor
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
#%.c: override SWIG=$(Q)printf "%32s %s\n" "SWIG wrapping" $(@); $(_SWIG)
%.cpp: override SWIG=$(Q)printf "%32s %s\n" "SWIG wrapping" $(@); $(_SWIG)
%.py: override WXGLADE=$(Q)printf "%32s %s\n" "Generating GUI" $<; $(_WXGLADE)
%.i.d: override SWIG=printf "%32s %s\n" "Checking dependencies" $<; $(_SWIG)
%.o: override AS=$(Q)printf "%32s %s\n" "Assembling" $<; $(_AS)
#%.so: override CC=$(Q)printf "%32s %s\n" "Linking" $(@); $(_CC)
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

%.i.d: %.i
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(SWIG) -MM $< > $@.$$$$; \
        sed 's,\($*\)_wrap\.c[ :]*,\1.c $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

%.ipp.d: %.ipp
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(SWIG) -c++ -MM $< > $@.$$$$; \
        sed 's,\($*\)_wrap\.cxx[ :]*,\1.cpp $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

%.d: %.c
	@mkdir -p $(@D)
	@set -e; rm -f $@; \
        $(CC) $(CFLAGS) $(CPPFLAGS) -MP -MM $< -MF $@.$$$$ -MT $@; \
        sed 's,\($*\)\.d[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
        rm -f $@.$$$$

%.dpp: %.cpp
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

COR_SOURCES := $(addprefix src/$(d)/, packet.c mapbuffer.c inbuffer.c timestamp.c plugins.c dispatch.c debug.c)

build/$(d)/_cor.o: CPPFLAGS := $(CPPFLAGS) -Isrc/$(d)

build/$(d)/_cor.c: swig/_cor.i
	@mkdir -p $(@D)
	@mkdir -p bin/cor
	$(SWIG) -python -threads $(SWIGFLAGS) -Isrc/cor -o $@ -outdir bin/cor $<

bin/$(d)/_cor.so: $(patsubst src/%,build/%,$(COR_SOURCES:.c=.o)) build/$(d)/_cor.o

TARGETS := $(TARGETS) bin/$(d)/_cor.so bin/$(d)/__init__.py

############################## NUMERICS ############################
d := numerics

NUMERICS_CXX_SOURCES := $(addprefix src/$(d)/, vec4.cpp matrix.cpp kalman.cpp) build/$(d)/_numerics.cpp

build/$(d)/_numerics.cpp: swig/_numerics.ipp
	@mkdir -p $(@D)
	@mkdir -p bin/numerics
	$(SWIG) -python -c++ -threads $(SWIGFLAGS) -Isrc/numerics -o $@ -outdir bin/numerics $<

build/$(d)/%.o: src/$(d)/%.cpp
build/$(d)/_numerics.o: CPPFLAGS := $(CPPFLAGS) -Isrc/$(d)

bin/$(d)/_numerics.so: LDFLAGS := $(LDFLAGS) -lm -lcblas -llapack -lstdc++ -framework vecLib
bin/$(d)/_numerics.so: $(patsubst src/%,build/%,$(NUMERICS_CXX_SOURCES:.cpp=.o))

TARGETS := $(TARGETS) bin/$(d)/_numerics.so bin/$(d)/__init__.py

############################## FILTER ############################

d := filter

FILTER_CXX_SOURCES := $(addprefix src/$(d)/, filter.cpp model.cpp observation.cpp filter_setup.cpp fast_detector/fast_9.cpp fast_detector/fast_10.cpp fast_detector/fast_11.cpp fast_detector/fast_12.cpp) build/$(d)/_filter.cpp
build/$(d)/_filter.cpp: swig/_filter.ipp
	@mkdir -p $(@D)
	@mkdir -p bin/filter
	$(SWIG) -python -c++ -threads $(SWIGFLAGS) -Isrc/filter -o $@ -outdir bin/filter $<

build/$(d)/%.o: src/$(d)/%.cpp
build/$(d)/_filter.o: CPPFLAGS := $(CPPFLAGS) -Isrc/$(d)

#bin/$(d)/_filter.so: LDFLAGS := $(LDFLAGS) `pkg-config --libs opencv`
bin/$(d)/_filter.so:$(patsubst src/%,build/%,$(FILTER_CXX_SOURCES:.cpp=.o))

TARGETS := $(TARGETS) bin/$(d)/_filter.so bin/$(d)/__init__.py

##################################################################

TARGETS := $(TARGETS) bin/__init__.py

all: $(TARGETS)

clean:
	rm -rf build bin

