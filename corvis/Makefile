############################ DEFINITIONS ###########################

CC := gcc
CXX := g++
# -ftree-vectorize -ffast-math 
CPPFLAGS = $(_CPPFLAGS) -Wall -g -fPIC -march=core2 -mfpmath=sse -O3 -D DEBUG
_CPPFLAGS := \
        $(patsubst %,-isystem %/include,$(wildcard $(shell brew --prefix boost mathgl gsl 2>/dev/null) /opt/local)) \
        -Isrc/cor -Wdeprecated-writable-strings
CXXFLAGS := -std=c++1y
CFLAGS := -std=gnu99
LDFLAGS := -lpthread --warn-unresolved-symbols -O3 \
        -framework Accelerate \
        $(patsubst %,-L%/lib,$(wildcard $(shell brew --prefix boost mathgl gsl 2>/dev/null) /opt/local)) \

CPPFLAGS  += -MP -MMD -MF $@.d

####################### RULES ######################################

all:

.PHONY: all clean

Q=$(if $V,,@)
_CC := $(CC)
_CXX := $(CXX)
_AS := $(AS)
_AR := $(AR)

%.o: override CC=$(Q)printf "%32s %s\n" "Compiling" $<; $(_CC)
%.o: override CXX=$(Q)printf "%32s %s\n" "Compiling" $<; $(_CXX)
%.S: override CC=$(Q)printf "%32s %s\n" "Generating Code" $<; $(_CC)
%.S: override CXX=$(Q)printf "%32s %s\n" "Generating code" $<; $(_CXX)
%.E: override CC=$(Q)printf "%32s %s\n" "Preprocessing" $<; $(_CC)
%.E: override CXX=$(Q)printf "%32s %s\n" "Preprocessing" $<; $(_CXX)
%.o: override AS=$(Q)printf "%32s %s\n" "Assembling" $<; $(_AS)
%.so: override CC=$(Q)printf "%32s %s\n" "Linking" $(@); $(_CC)
%.a: override AR=$(Q)printf "%32s %s\n" "Archiving" $(@); $(_AR)
bin/%:override CC=$(Q)printf "%32s %s\n" "Linking" $(@); $(_CC)

%.a:
	@mkdir -p $(@D)
	$(AR) cr $@ $^

%.so:
	@mkdir -p $(@D)
	$(CC) -shared -undefined suppress -flat_namespace $(LDFLAGS) $^ $(LDLIBS) -o $@

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

############################## COR ############################

COR_CXX_SOURCES := $(addprefix src/cor/, sensor_fusion_queue.cpp platform/shell/sensor_data.cpp platform/shell/sensor_clock.cpp)

lib/cor.a: $(patsubst src/%,build/%,$(COR_CXX_SOURCES:.cpp=.o))

TARGETS := $(TARGETS) lib/cor.a
CXX_SOURCES := $(CXX_SOURCES) $(COR_CXX_SOURCES)

############################## NUMERICS ############################

NUMERICS_CXX_SOURCES := $(addprefix src/numerics/, rotation_vector.cpp vec4.cpp matrix.cpp kalman.cpp)
lib/numerics.a: LDFLAGS := $(LDFLAGS) -lm -lcblas -llapack -lstdc++
lib/numerics.a: $(patsubst src/%,build/%,$(NUMERICS_CXX_SOURCES:.cpp=.o))

TARGETS := $(TARGETS) lib/numerics.a
CXX_SOURCES := $(CXX_SOURCES) $(NUMERICS_CXX_SOURCES)

############################## FILTER ############################

FILTER_CXX_SOURCES := $(addprefix src/filter/, filter.cpp state_vision.cpp state_motion.cpp observation.cpp filter_setup.cpp fast_detector/fast_9.cpp scaled_mask.cpp camera_control_interface_dummy.cpp homography.cpp qr.cpp device_parameters.cpp)

lib/filter.a:$(patsubst src/%,build/%,$(FILTER_CXX_SOURCES:.cpp=.o)) $(patsubst src/%,build/%,$(FILTER_SOURCES:.c=.o))

TARGETS := $(TARGETS) lib/filter.a
CXX_SOURCES := $(CXX_SOURCES) $(FILTER_CXX_SOURCES)

############################## VIS ################################

VIS_CXX_SOURCES := $(addprefix src/vis/, world_state.cpp world_state_render.cpp arcball.cpp offscreen_render.cpp gui.cpp lodepng.cpp gl_util.cpp render.cpp video_render.cpp)
lib/vis.a: LDFLAGS := $(LDFLAGS) -framework OpenGL -lmgl
lib/vis.a: $(patsubst src/%,build/%,$(VIS_CXX_SOURCES:.cpp=.o))

_CPPFLAGS := $(_CPPFLAGS) -Wno-deprecated-declarations

TARGETS := $(TARGETS) lib/vis.a
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

lib/libzxing.a:$(patsubst src/%,build/%,$(LIBZXING_CXX_SOURCES:.cpp=.o)) $(patsubst src/%,build/%,$(LIBZXING_SOURCES:.c=.o))

TARGETS := $(TARGETS) lib/libzxing.a
CXX_SOURCES := $(CXX_SOURCES) $(LIBZXING_CXX_SOURCES)

############################## STEREO ############################

d := stereo

STEREO_CXX_SOURCES := $(addprefix src/$(d)/, stereo.cpp stereo_mesh.cpp camera.cpp fundamental.cpp stereo_features.cpp)

_CPPFLAGS := $(_CPPFLAGS) -Isrc/stereo -Isrc/filter -Isrc/qhull/src/libqhull/ -Isrc/vlfeat-0.9.18/vl -Isrc/libDAI-0.3.1/include

lib/stereo.a:$(patsubst src/%,build/%,$(STEREO_CXX_SOURCES:.cpp=.o)) lib/libDAI.a lib/libvlfeat.a lib/libqhull.a

TARGETS := $(TARGETS) lib/stereo.a
CXX_SOURCES := $(CXX_SOURCES) $(STEREO_CXX_SOURCES)

############################## QHull ############################
d := qhull

QHULL_C_SOURCES := $(addprefix src/$(d)/src/libqhull/, geom.c geom2.c global.c io.c libqhull.c mem.c merge.c poly.c poly2.c qset.c random.c rboxlib.c stat.c user.c usermem.c userprintf.c userprintf_rbox.c)

_CPPFLAGS := $(_CPPFLAGS)

lib/libqhull.a:$(patsubst src/%,build/%,$(QHULL_C_SOURCES:.c=.o))

TARGETS := $(TARGETS) lib/libqhull.a
SOURCES := $(SOURCES) $(QHULL_C_SOURCES)

############################## DAI ############################
d := libDAI-0.3.1

DAI_CXX_SOURCES := $(addprefix src/$(d)/src/, alldai.cpp graph.cpp regiongraph.cpp emalg.cpp bipgraph.cpp evidence.cpp io.cpp trwbp.cpp bp.cpp exactinf.cpp daialg.cpp properties.cpp util.cpp bp_dual.cpp exceptions.cpp varset.cpp factor.cpp weightedgraph.cpp clustergraph.cpp factorgraph.cpp dag.cpp)

_CPPFLAGS := $(_CPPFLAGS) -Isrc/$(d)/include -I/usr/local/include -DMACOSX -DDAI_WITH_BP -DDAI_WITH_TRWBP

lib/libDAI.a:$(patsubst src/%,build/%,$(DAI_CXX_SOURCES:.cpp=.o))

TARGETS := $(TARGETS) lib/libDAI.a
CXX_SOURCES := $(CXX_SOURCES) $(DAI_CXX_SOURCES)

############################## VLFEAT ############################
d := vlfeat-0.9.18
 
VL_C_SOURCES := $(addprefix src/$(d)/, vl/aib.c vl/array.c vl/covdet.c vl/dsift.c vl/fisher.c vl/generic.c vl/getopt_long.c vl/gmm.c vl/hikmeans.c vl/hog.c vl/homkermap.c vl/host.c vl/ikmeans.c vl/imopv.c vl/imopv_sse2.c vl/kdtree.c vl/kmeans.c vl/lbp.c vl/liop.c vl/mathop.c vl/mathop_avx.c vl/mathop_sse2.c vl/mser.c vl/pgm.c vl/quickshift.c vl/random.c vl/rodrigues.c vl/scalespace.c vl/sift.c vl/slic.c vl/stringop.c vl/svm.c vl/svmdataset.c vl/vlad.c)

_CPPFLAGS := $(_CPPFLAGS)

lib/libvlfeat.a:$(patsubst src/%,build/%,$(VL_C_SOURCES:.c=.o))

TARGETS := $(TARGETS) lib/libvlfeat.a
SOURCES := $(SOURCES) $(VL_C_SOURCES)

############################## libglad ############################
GLAD_SOURCES := $(addprefix src/glfw/deps/, glad.c)

lib/libglad.a: $(patsubst src/%,build/%,$(GLAD_SOURCES:.c=.o))

SOURCES := $(SOURCES) $(GLAD_SOURCES)
TARGETS := $(TARGETS) lib/libglad.a

############################## libglfw3 ############################
lib/libglfw3.a:
	@mkdir -p build/glfw/
	@cd build/glfw/ && cmake ../../src/glfw/ -DCMAKE_INSTALL_PREFIX=../../
	@cmake --build build/glfw/ --target glfw
	@cmake -P build/glfw/cmake_install.cmake

TARGETS := $(TARGETS) lib/libglfw3.a

############################## MEASURE ############################

MEASURE_CXX_SOURCES := src/tools/measure.cpp src/filter/replay.cpp $(FILTER_CXX_SOURCES) $(NUMERICS_CXX_SOURCES) $(COR_CXX_SOURCES) $(VIS_CXX_SOURCES)
SOURCES := $(SOURCES)
CXX_SOURCES := $(CXX_SOURCES) $(MEASURE_CXX_SOURCES)

LDFLAGS := $(LDFLAGS) -lstdc++ -lmgl

_CPPFLAGS := $(_CPPFLAGS) -Iinclude -Isrc/glfw/deps/

bin/measure: $(patsubst src/%,build/%,$(MEASURE_CXX_SOURCES:.cpp=.o)) lib/libglfw3.a lib/libglad.a lib/filter.a lib/numerics.a lib/cor.a lib/libzxing.a
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(shell pkg-config lib/pkgconfig/glfw3.pc --static --libs) $^ $(LDLIBS) -o $@

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

bin/test: build/$(d)/gtest_main.o build/$(d)/gtest-all.o $(patsubst test/%,build/test/%,$(TEST_CXX_SOURCES:.cpp=.o)) lib/filter.a lib/stereo.a lib/numerics.a lib/cor.a lib/libzxing.a lib/libDAI.a lib/libvlfeat.a lib/libqhull.a
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

TARGETS := $(TARGETS) bin/test
#CXX_SOURCES := $(CXX_SOURCES) $(TEST_CXX_SOURCES)

##################################################################

DEPS := $(DEPS) $(patsubst src/%,build/%, $(SOURCES:.c=.o.d) $(CXX_SOURCES:.cpp=.o.d)) build/test/gtest_main.o.d build/test/gtest-all.o.d $(patsubst test/%,build/test/%, $(TEST_CXX_SOURCES:.cpp=.o.d))

all: $(TARGETS)

clean:
	rm -rf build bin lib include

-include $(DEPS)
