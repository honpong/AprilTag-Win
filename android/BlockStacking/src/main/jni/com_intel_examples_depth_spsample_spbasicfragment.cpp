/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#include <jni.h>

#ifndef NULL
#define NULL   (nullptr)
#endif
#include <android/log.h>
#include <memory.h>
#include <stdlib.h>

const char *TAG = "Native SP Sample";
static const int hist_size = 256 * 256;
static int depth_histogram[hist_size];
static const unsigned int table_nr_colors = 2;
static const unsigned int table_size = (table_nr_colors - 1) * 256 * 3;
static uint8_t depth_colormap[table_size];
void initDepthColorMap(void)
{
	uint8_t colors[table_nr_colors][3] = {
		{255, 0, 0},
		{20, 40, 255}
	};
	for (unsigned int k = 0; k < table_nr_colors - 1; k++) {
		for (int i = 0; i < 256; i++) {
			int j = 3 * (k * 256 + i);
			double a = double(i) / 255;
			depth_colormap[j] = uint8_t((1 - a) * colors[k][0] + a * colors[k + 1][0]);
			depth_colormap[j + 1] = uint8_t((1 - a) * colors[k][1] + a * colors[k + 1][1]);
			depth_colormap[j + 2] = uint8_t((1 - a) * colors[k][2] + a * colors[k + 1][2]);
		}
	}

	depth_colormap[0] = depth_colormap[1] = depth_colormap[2] = 0;
}

static void z16ToRGBA8888(const void *in,
			    void *out,
			    int width, int height, int stride)
{
	const uint16_t *depth = (const uint16_t *)in;
	uint8_t *rgb = (uint8_t *)out;
	int nr_valid = 0;
	unsigned int i;
    int r,c;
    int x;
	static bool init = false;

	if (!init) {
		initDepthColorMap();
		init = true;
	}

	// Build cumulative histogram
	memset(depth_histogram, 0, hist_size * sizeof(int));

    for (r = 0; r < height; r++)
    {
        int base = r*stride/2;
        for (c = 0; c < width; c++)
        {

				uint16_t d = depth[base+c];
				if (d) {
					depth_histogram[d]++;
					nr_valid++;
		    }
        }
    }
	for (i = 1; i < hist_size; i++)
		depth_histogram[i] += depth_histogram[i - 1];

	unsigned int k = table_size / 3 - 1;
	double n = 1.0 / double(nr_valid);
	for (i = 1; i < hist_size; i++)
		depth_histogram[i] = (int)(k * depth_histogram[i] * n);

	// Set rgb values
	i = 0;
	int rgbsize = width*height*4; //in bytes, 32 bits , depth is 16 bits
    for (r = 0; r < height; r++)
    {
        int base = r*stride/2;
        int cbase = r*width*4; //no stride for color
        for (c = 0; c < width; c++)
        {
		    uint16_t d = depth[base+c];
				int v = 3 * depth_histogram[d];
				rgb[cbase + c*4] = depth_colormap[v];
				rgb[cbase + c*4 + 1] = depth_colormap[v + 1];
				rgb[cbase + c*4 + 2] = depth_colormap[v + 2];
				rgb[cbase + c*4 + 3] = 0xff;
        }
	}
}

JNIEXPORT void JNICALL JNI_convertZ16ToRGB
	(JNIEnv* env, jobject thiz,jobject src, jobject dst, int width, int height, int stride)
{
	if ( width*height <= 0 )
	{
		//__android_log_print(ANDROID_LOG_ERROR, "converZ16", "%s","depth buffer size is not valid: %d", width*height);
		return;
	}

	void *depthBuf = env->GetDirectBufferAddress(src);
	void *rgbBuf = env->GetDirectBufferAddress(dst);

	if (depthBuf == NULL)
	{
		//__android_log_print(ANDROID_LOG_ERROR, "converZ16", "%s","Can't convert src direct buffer address");
		return;
	}
	if (rgbBuf == NULL)
	{
		//__android_log_print(ANDROID_LOG_ERROR, "converZ16", "%s", "Can't convert dst direct buffer address");
		return;
	}
	z16ToRGBA8888(depthBuf, rgbBuf, width, height, stride);
	return;
}




//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int gcd(unsigned int n, unsigned int d){
	if (d == 0) return n;
	else return gcd(d, n % d);
}

void frac(int* n, int* d){
	unsigned int res = gcd((unsigned int)(*n), (unsigned int)(*d));
	*n /= res;
	*d /= res;
}

int resize_depth_VGA_2_QVGA(void* inBuf, int w, int h,
		                    void* outBuf){
	unsigned short* input  = static_cast<unsigned short*>(inBuf);
	unsigned short* output = static_cast<unsigned short*>(outBuf);

	int h_offset = 3; // pixels from the top
	int w_offset = 3; // pixels from the side

	int ow = 320; // output width
	int oh = 240; // output height

	for (int i = 0; i < h/2; i++){
		for (int j = 0; j < w/2; j++){
			int in  = 2*(i * w + j); 							// pixel location from VGA image
			int out = ((i + h_offset) * ow) + w_offset + j;		// pixel location of QVGA image

			output[out] = input[in];

		}
	}
	return 1;
}



static JNINativeMethod methodTable[] = {
	{"depthToRGB", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;III)V", (void *)JNI_convertZ16ToRGB},
};

const char* spCoreClassPath = "com/intel/blockstacking/SPBasicFragment";

jint JNI_OnLoad(JavaVM* aVm, void* aReserved)
{
	JNIEnv* env;
	if (aVm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
	{
		//__android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to get JNI_1_6 environment");
		return -1;
	}

	jclass activityClass = env->FindClass(spCoreClassPath);
	if (!activityClass)
	{
		//__android_log_print(ANDROID_LOG_ERROR, TAG, "failed to get %s class reference", spCoreClassPath);
		return -1;
	}

	env->RegisterNatives(activityClass, methodTable, sizeof(methodTable) / sizeof(methodTable[0]));

	return JNI_VERSION_1_6;
}
