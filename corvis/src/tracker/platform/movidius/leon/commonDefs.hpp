/*
 * commonDefs.hpp
 *
 *  Created on: Nov 13, 2016
 *      Author: administrator
 */

#ifndef COMMON_COMMONDEFS_HPP_
#define COMMON_COMMONDEFS_HPP_

#define MAX_CORD 10000
#define PADDING 8
#define MAX_WIDTH 848
#define MAX_HEIGHT 800

struct ShaveFastDetectSettings {

	int imgWidth;
	int imgHeight;
	int winWidth;
	int winHeight;
	int imgStride;
	int x;
	int y;
	int threshold;
	int halfWindow;

	ShaveFastDetectSettings(int _imgWidth,
				int _imgHeight,
				int _winWidth, int _winHeight, int _imgStride, int _x, int _y, int _thresh, int _halfWin) {
		imgWidth = _imgWidth;
		imgHeight = _imgHeight;
		winWidth = _winWidth;
		winHeight = _winHeight;
		imgStride = _imgStride;
		x = _x;
		y = _y;
		threshold = _thresh;
		halfWindow = _halfWin;
	}

	ShaveFastDetectSettings() {
			imgWidth = imgHeight = winWidth = winHeight = imgStride = x =  y = threshold = halfWindow = 0;
		}

};



struct TrackingData {

	unsigned char *patch;
	float x_dx, y_dy, pred_x, pred_y;
};

#endif /* COMMON_COMMONDEFS_HPP_ */
