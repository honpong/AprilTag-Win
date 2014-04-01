############################ DEFINITIONS ###########################

SWIG := swig
WXGLADE := wxglade
CC := gcc
CXX := g++
PYTHON_CFLAGS := `python-config --includes` -I/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/numpy/core/include
PYTHON_LDFLAGS := `python-config --libs`
# -ftree-vectorize -ffast-math 
CPPFLAGS := -Wall -pthread -g -I../corvis/src -I../corvis/src/cor -fPIC -march=core2 -mfpmath=sse -O0 -D DEBUG $(PYTHON_CFLAGS) -I/opt/local/include -I/opt/local/include/freetype2 -I/opt/local/include/FTGL 
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
