/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2013-2014 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include "projection.h"
#include "pxcsession.h"
#include "pxccapture.h"
#include "pxcmetadata.h"
#include "main.h"

#define MAX_LOCAL_DEPTH_VALUE 2000

Projection::Projection(PXCSession *session, PXCCapture::Device *device, PXCImage::ImageInfo *dinfo, PXCImage::ImageInfo *cinfo) {
	/* retrieve the invalid depth pixel values */
	invalid_value = device->QueryDepthLowConfidenceValue();

	projection = device->CreateProjection();

	int size=dinfo->width*dinfo->height;
	dcords=new PXCPoint3DF32[size];
	ccords=new PXCPointF32[size];
	if (dcords) {
		for (int y = 0, k = 0; y < (int)dinfo->height; y++)
			for (int x = 0; x < (int)dinfo->width; x++, k++)
				dcords[k].x = (pxcF32)x, dcords[k].y = (pxcF32)y;
	}

	uvmap=new PXCPointF32[dinfo->width*dinfo->height];
	invuvmap=new PXCPointF32[cinfo->width*cinfo->height];
}

Projection::~Projection(void) {
	if (invuvmap) delete[] invuvmap;
	if (uvmap) delete[] uvmap;
	if (projection) projection->Release();
	if (ccords) delete [] ccords;
	if (dcords) delete [] dcords;
}

void Projection::PlotXY(pxcBYTE *cpixels, int xx, int yy, int cwidth, int cheight, int dots, int color) {
	if (xx < 0 || xx >= cwidth || yy < 0 || yy >= cheight) return;

	int lyy = yy * cwidth;
	int xxm1 = (xx > 0 ? xx - 1 : xx), xxp1 = (xx < (int)cwidth - 1 ? xx + 1 : xx);
	int lyym1 = yy > 0 ? lyy - cwidth : lyy, lyyp1 = yy < (int)cheight - 1 ? lyy + cwidth : lyy;

	if (dots >= 9)  /* 9 dots */
	{
		cpixels[(lyym1 + xxm1) * 4 + color] = 0xFF;
		cpixels[(lyym1 + xxp1) * 4 + color] = 0xFF;
		cpixels[(lyyp1 + xxm1) * 4 + color] = 0xFF;
		cpixels[(lyyp1 + xxp1) * 4 + color] = 0xFF;
	}
	if (dots >= 5)  /* 5 dots */
	{
		cpixels[(lyym1 + xx) * 4 + color] = 0xFF;
		cpixels[(lyy + xxm1) * 4 + color] = 0xFF;
		cpixels[(lyy + xxp1) * 4 + color] = 0xFF;
		cpixels[(lyyp1 + xx) * 4 + color] = 0xFF;
	}
	cpixels[(lyy + xx) * 4 + color] = 0xFF; /* 1 dot */
}

void Projection::DepthToColorByUVMAP(PXCImage *color, PXCImage *depth, int dots) {
    if ( !color || !depth ) return;
	PXCImage::ImageInfo cinfo=color->QueryInfo();

	/* Retrieve the color pixels */
	PXCImage::ImageData cdata;
	pxcStatus sts = color->AcquireAccess(PXCImage::ACCESS_READ,PXCImage::PIXEL_FORMAT_RGB32,&cdata); 
	if (sts>=PXC_STATUS_NO_ERROR) {
		int cwidth = cdata.pitches[0]/sizeof(int); /* aligned width */
		pxcBYTE *cpixels=cdata.planes[0];

		PXCImage::ImageInfo dinfo=depth->QueryInfo();

		/* Retrieve the depth pixels and uvmap */
		PXCImage::ImageData ddata;
		if (depth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_DEPTH, &ddata)>=PXC_STATUS_NO_ERROR) {
			pxcU16 *dpixels=(pxcU16*)ddata.planes[0];
			int   dpitch = ddata.pitches[0]/sizeof(pxcU16); /* aligned width */

			sts = projection->QueryUVMap(depth, uvmap);
            if(sts>=PXC_STATUS_NO_ERROR) {
			    int uvpitch = depth->QueryInfo().width;

			    /* Draw dots onto the color pixels */
			    for (int y = 0; y < (int)dinfo.height; y++) {
				    for (int x = 0; x < (int)dinfo.width; x++) {
					    pxcU16 d = dpixels[y*dpitch+x];
					    if (d == invalid_value) continue; // no mapping based on unreliable depth values

					    float uvx = uvmap[y*uvpitch+x].x;
					    float uvy = uvmap[y*uvpitch+x].y;
					    int xx = (int)(uvx * cwidth + 0.5f), yy = (int)(uvy * cinfo.height + 0.5f);
					    if (xx >= 0 && xx < cwidth && yy >= 0 && yy < cinfo.height)
					    {
						    PlotXY(cpixels, xx, yy, cwidth, cinfo.height, dots, color?1:0);
					    }
				    }
			    }
            }
			depth->ReleaseAccess(&ddata);
		}
		color->ReleaseAccess(&cdata);
	}
}

void Projection::DepthToColorByFunction(PXCImage *color, PXCImage *depth, int dots) {
	if ( !color || !depth || !projection || !ccords || !dcords) return;
	/* Retrieve the color pixels */
	PXCImage::ImageInfo cinfo=color->QueryInfo();

	PXCImage::ImageData cdata;
	pxcStatus sts = color->AcquireAccess(PXCImage::ACCESS_READ,PXCImage::PIXEL_FORMAT_RGB32,&cdata); 
	if (sts>=PXC_STATUS_NO_ERROR) {
		int cwidth = cdata.pitches[0]/sizeof(int); /* aligned width */
		pxcBYTE *cpixels=cdata.planes[0];

		/* Retrieve the depth pixels and uvmap */
		PXCImage::ImageInfo dinfo=depth->QueryInfo();

		PXCImage::ImageData ddata;
		if (depth->AcquireAccess(PXCImage::ACCESS_READ,PXCImage::PIXEL_FORMAT_DEPTH,&ddata)>=PXC_STATUS_NO_ERROR) {
			pxcU16 *dpixels=(pxcU16*)ddata.planes[0];
			int   dpitch = ddata.pitches[0]/sizeof(pxcU16); /* aligned width */

			/* Projection Calculation */
			for (int y = 0; y < (int)dinfo.height; y++) {
				for (int x = 0; x < (int)dinfo.width; x++) {
					pxcU16 d = dpixels[y*dpitch+x];
					dcords[y*dinfo.width+x].z = (pxcF32)d;
				}
			}
			projection->MapDepthToColor(dinfo.width*dinfo.height,dcords,ccords);

			/* Draw dots onto the color pixels */
			for (int y = 0; y < (int)dinfo.height; y++) {
				for (int x = 0; x < (int)dinfo.width; x++) {
					pxcU16 d = dpixels[y*dpitch+x];
					if (d == invalid_value) continue; // no mapping based on unreliable depth values

					int xx = (int)ccords[y*dinfo.width+x].x;
					int yy = (int)ccords[y*dinfo.width+x].y;
					PlotXY(cpixels, xx, yy, cwidth, (int)cinfo.height, dots, color?2:0);
				}
			}
			depth->ReleaseAccess(&ddata);
		}
		color->ReleaseAccess(&cdata);
	}
}

void Projection::ColorToDepthByInvUVMAP(PXCImage *color, PXCImage *depth, int dots) {
    if ( !color || !depth ) return;
    PXCImage::ImageInfo cinfo=color?color->QueryInfo():depth->QueryInfo();

    /* Retrieve the color pixels */
    PXCImage::ImageData cdata;
    pxcStatus sts = color?color->AcquireAccess(PXCImage::ACCESS_READ,PXCImage::PIXEL_FORMAT_RGB32,&cdata):depth->AcquireAccess(PXCImage::ACCESS_READ,PXCImage::PIXEL_FORMAT_DEPTH,&cdata); 
    if (sts>=PXC_STATUS_NO_ERROR) {
        if (!color) {
            depth->ReleaseAccess(&cdata);
            return;
        }

        PXCImage::ImageInfo dinfo=depth->QueryInfo();

        if(dots >= 9)
        { // A sample for CreateDepthImageMappedToColor output visualization
            PXCImage::ImageData d2cDat;
            PXCImage* d2c = projection->CreateDepthImageMappedToColor(depth, color);
            if(!d2c) {
                if (color) color->ReleaseAccess(&cdata);
                return;
            }
            if(d2c->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_DEPTH, &d2cDat) >= PXC_STATUS_NO_ERROR)
            {
                for (int y = 0; y < cinfo.height; y++) {
                    pxcU16 *d = (pxcU16*)(d2cDat.planes[0] + y * d2cDat.pitches[0]);
                    pxcBYTE *cpixels = cdata.planes[0] + y * cdata.pitches[0];
                    for (int x = 0; x < cinfo.width; x++) {
                        if (d[x] == invalid_value) continue; // no mapping based on unreliable depth values
                        cpixels[x * 4] = 0xFF;
                    }
                }
                d2c->ReleaseAccess(&d2cDat);
            }
            d2c->Release();
            if (color) color->ReleaseAccess(&cdata);
            return;
        }

        /* Retrieve the depth pixels and Color To Depth (Inverse) UVMap */
        PXCImage::ImageData ddata;
        if (depth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_DEPTH, &ddata)>=PXC_STATUS_NO_ERROR) {
            sts = projection->QueryInvUVMap(depth, invuvmap);
            if (sts>=PXC_STATUS_NO_ERROR) {
                pxcU16 *dpixels=(pxcU16*)ddata.planes[0];
                int   dpitch = ddata.pitches[0]/sizeof(pxcU16);
                if( dots > 1 ) { // If Depth data is valid just set a blue pixel
                    /* Draw dots onto the color pixels */
                    for (int y = 0; y < cinfo.height; y++) {
                        pxcBYTE *cpixels = cdata.planes[0] + y * cdata.pitches[0];
                        for (int x = 0; x < cinfo.width; x++) {
                            int xx = (int)(invuvmap[y*cinfo.width+x].x * dinfo.width);
                            int yy = (int)(invuvmap[y*cinfo.width+x].y * dinfo.height);
                            if (xx >= 0 && yy >= 0) {
                                pxcU16 d = dpixels[yy*dpitch+xx];
                                if (d == invalid_value) continue; // no mapping based on unreliable depth values
                                cpixels[x * 4] = 0xFF;
                            }
                        }
                    }
                } else { // If Depth data is valid just set a blue pixel with briteness depends on Depth value
                    int depth_hist[MAX_LOCAL_DEPTH_VALUE];
                    memset(depth_hist, 0, sizeof(depth_hist));
                    int num_depth_points = 0;
                    for (int y = 0; y < (int)dinfo.height; y++) {
                        for (int x = 0; x < (int)dinfo.width; x++) {
                            pxcU16 d = dpixels[y*dpitch+x];
                            if (d == invalid_value || d >= MAX_LOCAL_DEPTH_VALUE) continue; // no mapping based on unreliable depth values
                            depth_hist[d]++;
                            num_depth_points++;
                        }
                    }
                    if (num_depth_points) {
                        for (int i = 1; i < MAX_LOCAL_DEPTH_VALUE; i++) depth_hist[i] += depth_hist[i - 1];
                        for (int i = 1; i < MAX_LOCAL_DEPTH_VALUE; i++) {
                            depth_hist[i] = 255 - (unsigned int)(255.0 * depth_hist[i] / num_depth_points);
                        }

                        /* Draw dots onto the color pixels */
                        for (int y = 0; y < cinfo.height; y++) {
                            pxcBYTE *cpixels = cdata.planes[0] + y * cdata.pitches[0];
                            for (int x = 0; x < cinfo.width; x++) {
                                int xx = (int)(invuvmap[y*cinfo.width+x].x * dinfo.width);
                                int yy = (int)(invuvmap[y*cinfo.width+x].y * dinfo.height);
                                if (xx >= 0 && yy >= 0) {
                                    pxcU16 d = dpixels[yy*dpitch+xx];
                                    if (d == invalid_value || d >= MAX_LOCAL_DEPTH_VALUE) continue; // no mapping based on unreliable depth values
                                    cpixels[x * 4] = depth_hist[d];
                                }
                            }
                        }
                    }
                }
                depth->ReleaseAccess(&ddata);
            }
            if (color) color->ReleaseAccess(&cdata);
        }
    }
}
