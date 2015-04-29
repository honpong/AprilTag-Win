#BUILD_CAMERA:=TRUE

############################ DEFINITIONS ###########################

SWIG := swig
WXGLADE := wxglade
CC := gcc
CXX := g++
PYTHON_CFLAGS := $(shell python-config --includes) -I$(shell python -c 'import numpy; print numpy.__path__[0] + "/core/include"')
PYTHON_LDFLAGS := $(shell python-config --libs)
# -ftree-vectorize -ffast-math 
CPPFLAGS = $(_CPPFLAGS) -Wall -g -fPIC -march=core2 -mfpmath=sse -O3 -D DEBUG
_CPPFLAGS := \
        $(patsubst %,-I%/include,$(wildcard $(shell brew --prefix boost 2>/dev/null) /opt/local)) \
        $(shell pkg-config ftgl --cflags) -Isrc/cor -Wdeprecated-writable-strings
CXXFLAGS := -std=c++1y
CFLAGS := -std=gnu99
LDFLAGS := -lpthread --warn-unresolved-symbols -O3 -framework Accelerate -framework OpenGL -framework GLUT $(shell pkg-config ftgl --libs)
SWIGFLAGS = -w403,362,389,503,401,462,472,314,509,204

CPPFLAGS  += -MP -MMD -MF $@.d
SWIGFLAGS += -MP -MMD -MF $@.d

####################### RULES ######################################

all:

.PHONY: all clean

Q=$(if $V,,@)
_CC := $(CC)
_CXX := $(CXX)
_SWIG := $(SWIG)
_AS := $(AS)
_AR := $(AR)
_WXGLADE := $(WXGLADE)

%.o: override CC=$(Q)printf "%32s %s\n" "Compiling" $<; $(_CC)
%.o: override CXX=$(Q)printf "%32s %s\n" "Compiling" $<; $(_CXX)
%.S: override CC=$(Q)printf "%32s %s\n" "Generating Code" $<; $(_CC)
%.S: override CXX=$(Q)printf "%32s %s\n" "Generating code" $<; $(_CXX)
%.E: override CC=$(Q)printf "%32s %s\n" "Preprocessing" $<; $(_CC)
%.E: override CXX=$(Q)printf "%32s %s\n" "Preprocessing" $<; $(_CXX)
%.c: override SWIG=$(Q)printf "%32s %s\n" "SWIG Wrapping" $<; $(_SWIG)
%.cpp: override SWIG=$(Q)printf "%32s %s\n" "SWIG Wrapping" $<; $(_SWIG)
%.py: override WXGLADE=$(Q)printf "%32s %s\n" "Generating GUI" $<; $(_WXGLADE)
%.o: override AS=$(Q)printf "%32s %s\n" "Assembling" $<; $(_AS)
%.so: override CC=$(Q)printf "%32s %s\n" "Linking" $(@); $(_CC)
%.a: override AR=$(Q)printf "%32s %s\n" "Archiving" $(@); $(_AR)
bin/%:override CC=$(Q)printf "%32s %s\n" "Linking" $(@); $(_CC)

%.py: %.wxg
	@mkdir -p $(@D)
	$(WXGLADE) -g python -o $@ $<

%.a:
	@mkdir -p $(@D)
	$(AR) cr $@ $^

%.so:
	@mkdir -p $(@D)
	$(CC) -shared -undefined suppress -flat_namespace $(LDFLAGS) $^ $(LDLIBS) -o $@

bin/corvis/_%.so: LDGLAGS += $(PYTHON_LDFLAGS)

build/swig/%.o: CPPFLAGS += $(PYTHON_CFLAGS)

build/swig/_%.c: swig/_%.i
	@mkdir -p $(@D) bin/corvis
	$(SWIG) -python      -threads $(SWIGFLAGS) -o $@ -outdir bin/corvis -Isrc/$(*F) $<

build/swig/_%.cpp: swig/_%.ipp
	@mkdir -p $(@D) bin/corvis
	$(SWIG) -python -c++ -threads $(SWIGFLAGS) -o $@ -outdir bin/corvis -Isrc/$(*F) $<

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

build/%.E: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -E -o $@ $<

build/%.S: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -S -o $@ $<

build/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

build/%.E: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -E -o $@ $<

build/%.S: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -S -o $@ $<

%/__init__.py:
	@mkdir -p $(@D)
	$(Q)touch $@


############################## COR ############################

COR_SOURCES := $(addprefix src/cor/, packet.c mapbuffer.c timestamp.c plugins.c dispatch.c debug.c)
COR_CXX_SOURCES := $(addprefix src/cor/, sensor_fusion_queue.cpp platform/shell/sensor_data.cpp platform/shell/sensor_clock.cpp)

bin/corvis/_cor.so: $(patsubst src/%,build/%,$(COR_SOURCES:.c=.o)) $(patsubst src/%,build/%,$(COR_CXX_SOURCES:.cpp=.o)) build/swig/_cor.o

TARGETS := $(TARGETS) build/swig/_cor.c bin/corvis/_cor.so bin/corvis/__init__.py
SOURCES := $(SOURCES) $(COR_SOURCES)
CXX_SOURCES := $(CXX_SOURCES) $(COR_CXX_SOURCES)
DEPS := $(DEPS) build/swig/_cor.c.d

############################## NUMERICS ############################

NUMERICS_CXX_SOURCES := $(addprefix src/numerics/, rotation_vector.cpp vec4.cpp matrix.cpp kalman.cpp)
bin/corvis/_numerics.so: LDFLAGS := $(LDFLAGS) -lm -lcblas -llapack -lstdc++
bin/corvis/_numerics.so: $(patsubst src/%,build/%,$(NUMERICS_CXX_SOURCES:.cpp=.o)) build/swig/_numerics.o

TARGETS := $(TARGETS) build/swig/_numerics.cpp bin/corvis/_numerics.so bin/corvis/__init__.py
CXX_SOURCES := $(CXX_SOURCES) $(NUMERICS_CXX_SOURCES)
DEPS := $(DEPS) build/swig/_numerics.cpp.d

############################## FILTER ############################

FILTER_CXX_SOURCES := $(addprefix src/filter/, filter.cpp state_vision.cpp state_motion.cpp state.cpp observation.cpp filter_setup.cpp fast_detector/fast_9.cpp scaled_mask.cpp camera_control_interface_dummy.cpp homography.cpp qr.cpp device_parameters.cpp)

FILTER_SOURCES :=

#bin/swig/_filter.so: LDFLAGS := $(LDFLAGS) `pkg-config --libs opencv`
bin/corvis/_filter.so:$(patsubst src/%,build/%,$(FILTER_CXX_SOURCES:.cpp=.o)) $(patsubst src/%,build/%,$(FILTER_SOURCES:.c=.o)) bin/_libzxing.a build/swig/_filter.o

TARGETS := $(TARGETS) build/swig/_filter.cpp bin/corvis/_filter.so bin/corvis/__init__.py
CXX_SOURCES := $(CXX_SOURCES) $(FILTER_CXX_SOURCES)
SOURCES := $(SOURCES) $(FILTER_SOURCES)
DEPS := $(DEPS) build/swig/_filter.cpp.d

############################## VIS ################################

VIS_CXX_SOURCES := $(addprefix src/vis/, world_state.cpp world_state_render.cpp arcball.cpp offscreen_render.cpp lodepng.cpp)
bin/corvis/_vis.so: LDFLAGS := $(LDFLAGS) -framework OpenGL
bin/corvis/_vis.so: $(patsubst src/%,build/%,$(VIS_CXX_SOURCES:.cpp=.o))

_CPPFLAGS := $(_CPPFLAGS) -Wno-deprecated-declarations

TARGETS := $(TARGETS) bin/corvis/_vis.so
CXX_SOURCES := $(CXX_SOURCES) $(VIS_CXX_SOURCES)


############################## LIBZXING ##########################

d := libzxing-cpp

_CPPFLAGS := $(_CPPFLAGS) -Isrc/$(d)/ -DNO_ICONV

LIBZXING_CXX_SOURCES := $(addprefix src/$(d)/zxing/, BarcodeFormat.cpp Binarizer.cpp BinaryBitmap.cpp ChecksumException.cpp DecodeHints.cpp Exception.cpp FormatException.cpp InvertedLuminanceSource.cpp LuminanceSource.cpp MultiFormatReader.cpp Reader.cpp Result.cpp ResultIO.cpp ResultPoint.cpp ResultPointCallback.cpp)

# Common
LIBZXING_CXX_SOURCES := $(LIBZXING_CXX_SOURCES) $(addprefix src/$(d)/zxing/common/, BitArray.cpp BitArrayIO.cpp BitMatrix.cpp BitSource.cpp CharacterSetECI.cpp DecoderResult.cpp DetectorResult.cpp GlobalHistogramBinarizer.cpp GreyscaleLuminanceSource.cpp GreyscaleRotatedLuminanceSource.cpp GridSampler.cpp HybridBinarizer.cpp HybridFastBinarizer.cpp IllegalArgumentException.cpp PerspectiveTransform.cpp Str.cpp StringUtils.cpp detector/MonochromeRectangleDetector.cpp detector/WhiteRectangleDetector.cpp reedsolomon/GenericGF.cpp reedsolomon/GenericGFPoly.cpp reedsolomon/ReedSolomonDecoder.cpp reedsolomon/ReedSolomonException.cpp)

# qrcode
LIBZXING_CXX_SOURCES := $(LIBZXING_CXX_SOURCES) $(addprefix src/$(d)/zxing/qrcode/, ErrorCorrectionLevel.cpp FormatInformation.cpp QRCodeReader.cpp Version.cpp decoder/BitMatrixParser.cpp decoder/DataBlock.cpp decoder/DataMask.cpp decoder/DecodedBitStreamParser.cpp decoder/Decoder.cpp decoder/Mode.cpp detector/AlignmentPattern.cpp detector/AlignmentPatternFinder.cpp detector/Detector.cpp detector/FinderPattern.cpp detector/FinderPatternFinder.cpp detector/FinderPatternInfo.cpp)

# datamatrix
LIBZXING_CXX_SOURCES := $(LIBZXING_CXX_SOURCES) $(addprefix src/$(d)/zxing/datamatrix/, DataMatrixReader.cpp Version.cpp decoder/BitMatrixParser.cpp decoder/DataBlock.cpp decoder/DecodedBitStreamParser.cpp decoder/Decoder.cpp detector/CornerPoint.cpp detector/Detector.cpp detector/DetectorException.cpp)

# pdf417
LIBZXING_CXX_SOURCES := $(LIBZXING_CXX_SOURCES) $(addprefix src/$(d)/zxing/pdf417/, PDF417Reader.cpp decoder/BitMatrixParser.cpp decoder/DecodedBitStreamParser.cpp decoder/Decoder.cpp decoder/ec/ErrorCorrection.cpp decoder/ec/ModulusGF.cpp decoder/ec/ModulusPoly.cpp detector/Detector.cpp detector/LinesSampler.cpp)

# oned
LIBZXING_CXX_SOURCES := $(LIBZXING_CXX_SOURCES) $(addprefix src/$(d)/zxing/oned/, CodaBarReader.cpp Code128Reader.cpp Code39Reader.cpp Code93Reader.cpp EAN13Reader.cpp EAN8Reader.cpp ITFReader.cpp MultiFormatOneDReader.cpp MultiFormatUPCEANReader.cpp OneDReader.cpp OneDResultPoint.cpp UPCAReader.cpp UPCEANReader.cpp UPCEReader.cpp)

# multi
LIBZXING_CXX_SOURCES := $(LIBZXING_CXX_SOURCES) $(addprefix src/$(d)/zxing/multi/, ByQuadrantReader.cpp GenericMultipleBarcodeReader.cpp MultipleBarcodeReader.cpp qrcode/detector/MultiDetector.cpp qrcode/detector/MultiFinderPatternFinder.cpp qrcode/QRCodeMultiReader.cpp)

# multi
LIBZXING_CXX_SOURCES := $(LIBZXING_CXX_SOURCES) $(addprefix src/$(d)/zxing/aztec/, AztecDetectorResult.cpp AztecReader.cpp decoder/Decoder.cpp detector/Detector.cpp)

# bigint
LIBZXING_CXX_SOURCES := $(LIBZXING_CXX_SOURCES) $(addprefix src/$(d)/bigint/, BigIntegerAlgorithms.cpp BigIntegerUtils.cpp BigInteger.cpp BigUnsignedInABase.cpp BigUnsigned.cpp)

LIBZXING_SOURCES :=

bin/_libzxing.a:$(patsubst src/%,build/%,$(LIBZXING_CXX_SOURCES:.cpp=.o)) $(patsubst src/%,build/%,$(LIBZXING_SOURCES:.c=.o))

TARGETS := $(TARGETS) bin/_libzxing.a
CXX_SOURCES := $(CXX_SOURCES) $(LIBZXING_CXX_SOURCES)
SOURCES := $(SOURCES) $(LIBZXING_SOURCES)

############################## STEREO ############################

d := stereo

STEREO_CXX_SOURCES := $(addprefix src/$(d)/, stereo.cpp stereo_mesh.cpp camera.cpp fundamental.cpp stereo_features.cpp)

STEREO_SOURCES :=

_CPPFLAGS := $(_CPPFLAGS) -Isrc/stereo -Isrc/filter -Isrc/qhull/src/libqhull/ -Isrc/vlfeat-0.9.18/vl -Isrc/libDAI-0.3.1/include

bin/_stereo.so:$(patsubst src/%,build/%,$(STEREO_CXX_SOURCES:.cpp=.o)) $(patsubst src/%,build/%,$(STEREO_SOURCES:.c=.o)) bin/_libDAI.so bin/_vl.so

TARGETS := $(TARGETS) bin/_stereo.so bin/corvis/__init__.py
CXX_SOURCES := $(CXX_SOURCES) $(STEREO_CXX_SOURCES)
SOURCES := $(SOURCES) $(STEREO_SOURCES)

############################## QHull ############################
d := qhull

QHULL_C_SOURCES := $(addprefix src/$(d)/src/libqhull/, geom.c geom2.c global.c io.c libqhull.c mem.c merge.c poly.c poly2.c qset.c random.c rboxlib.c stat.c user.c usermem.c userprintf.c userprintf_rbox.c)

_CPPFLAGS := $(_CPPFLAGS)

bin/_qhull.so:$(patsubst src/%,build/%,$(QHULL_C_SOURCES:.c=.o))

TARGETS := $(TARGETS) bin/_qhull.so
SOURCES := $(SOURCES) $(QHULL_C_SOURCES)

############################## DAI ############################
d := libDAI-0.3.1

DAI_CXX_SOURCES := $(addprefix src/$(d)/src/, alldai.cpp graph.cpp regiongraph.cpp emalg.cpp bipgraph.cpp evidence.cpp io.cpp trwbp.cpp bp.cpp exactinf.cpp util.cpp bp_dual.cpp exceptions.cpp varset.cpp factor.cpp weightedgraph.cpp clustergraph.cpp factorgraph.cpp dag.cpp)

_CPPFLAGS := $(_CPPFLAGS) -Isrc/$(d)/include -I/usr/local/include -DMACOSX -DDAI_WITH_BP -DDAI_WITH_TRWBP

bin/_libDAI.so:$(patsubst src/%,build/%,$(DAI_CXX_SOURCES:.cpp=.o))

TARGETS := $(TARGETS) bin/_libDAI.so
CXX_SOURCES := $(CXX_SOURCES) $(DAI_CXX_SOURCES)

############################## VLFEAT ############################
d := vlfeat-0.9.18
 
VL_C_SOURCES := $(addprefix src/$(d)/, vl/aib.c vl/array.c vl/covdet.c vl/dsift.c vl/fisher.c vl/generic.c vl/getopt_long.c vl/gmm.c vl/hikmeans.c vl/hog.c vl/homkermap.c vl/host.c vl/ikmeans.c vl/imopv.c vl/imopv_sse2.c vl/kdtree.c vl/kmeans.c vl/lbp.c vl/liop.c vl/mathop.c vl/mathop_avx.c vl/mathop_sse2.c vl/mser.c vl/pgm.c vl/quickshift.c vl/random.c vl/rodrigues.c vl/scalespace.c vl/sift.c vl/slic.c vl/stringop.c vl/svm.c vl/svmdataset.c vl/vlad.c)

_CPPFLAGS := $(_CPPFLAGS)

bin/_vl.so:$(patsubst src/%,build/%,$(VL_C_SOURCES:.c=.o))

TARGETS := $(TARGETS) bin/_vl.so
SOURCES := $(SOURCES) $(VL_C_SOURCES)

############################## MEASURE ############################

MEASURE_SOURCES := $(FILTER_SOURCES) $(COR_SOURCES)
MEASURE_CXX_SOURCES := src/tools/measure.cpp src/filter/replay.cpp $(FILTER_CXX_SOURCES) $(NUMERICS_CXX_SOURCES) $(COR_CXX_SOURCES) $(VIS_CXX_SOURCES)
SOURCES := $(SOURCES) $(MEASURE_SOURCES)
CXX_SOURCES := $(CXX_SOURCES) $(MEASURE_CXX_SOURCES)

LDFLAGS := $(LDFLAGS) -lstdc++

bin/measure: $(patsubst src/%,build/%,$(MEASURE_SOURCES:.c=.o)) $(patsubst src/%,build/%,$(MEASURE_CXX_SOURCES:.cpp=.o)) bin/_libzxing.a
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(PYTHON_LDFLAGS) $^ $(LDLIBS) -o $@

TARGETS := $(TARGETS) bin/measure
CXX_SOURCES := $(CXX_SOURCES) src/filter/replay.cpp src/tools/measure.cpp

############################## TEST ############################

d := test

TEST_CXX_SOURCES := $(addprefix $(d)/, test.cpp util.cpp numerics/vec4.cpp numerics/rotation_vector.cpp stereo/stereo.cpp stereo/camera.cpp numerics/quaternion.cpp filter/homography.cpp filter/utils.cpp cor/sensor_fusion_queue.cpp)

_CPPFLAGS := $(_CPPFLAGS) -I../ -Isrc/numerics -Itest
LDFLAGS := $(LDFLAGS) -lstdc++

build/$(d)/%.o: ../gtest/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

build/$(d)/%.o: $(d)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

bin/test: build/$(d)/gtest_main.o build/$(d)/gtest-all.o $(patsubst test/%,build/test/%,$(TEST_CXX_SOURCES:.cpp=.o)) $(addprefix build/numerics/, rotation_vector.o vec4.o matrix.o) $(addprefix build/filter/, fast_detector/fast_9.o scaled_mask.o homography.o) $(addprefix build/cor/, sensor_fusion_queue.o platform/shell/sensor_data.o platform/shell/sensor_clock.o) bin/_stereo.so bin/_libDAI.so bin/_qhull.so
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(PYTHON_LDFLAGS) $^ $(LDLIBS) -o $@

TARGETS := $(TARGETS) bin/test
#CXX_SOURCES := $(CXX_SOURCES) $(TEST_CXX_SOURCES)

##################################################################

TARGETS := $(TARGETS) bin/__init__.py

TARGETS := $(TARGETS) __init__.py
__init__.py: bin/corvis/_cor.so bin/corvis/_filter.so bin/corvis/_numerics.so
	$(Q)cd .. && python -c 'import corvis'

DEPS := $(DEPS) $(patsubst src/%,build/%, $(SOURCES:.c=.o.d) $(CXX_SOURCES:.cpp=.o.d)) build/test/gtest_main.o.d build/test/gtest-all.o.d $(patsubst test/%,build/test/%, $(TEST_CXX_SOURCES:.cpp=.o.d))

all: $(TARGETS)

clean:
	rm -rf build bin

-include $(DEPS)
