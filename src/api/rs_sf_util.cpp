#include "rs_sf_util.h"
#include <unordered_map>

void rs_sf_util_set_to_zeros(rs_sf_image* img)
{
    memset(img->data, 0, img->num_char());
}

void rs_sf_util_convert_to_rgb_image(rs_sf_image * rgb, const rs_sf_image * src)
{
    if (!src) return;
    if (src->byte_per_pixel == 1)
        for (int p = src->num_pixel() - 1; p >= 0; --p)
            rgb->data[p * 3 + 0] = rgb->data[p * 3 + 1] = rgb->data[p * 3 + 2] = src->data[p];
    else if (src->byte_per_pixel == 3)
        memcpy(rgb->data, src->data, rgb->num_char());
}

void rs_sf_util_copy_depth_image(rs_sf_image_depth & dst, const rs_sf_image * src)
{
    dst.frame_id = src->frame_id;
    dst.set_pose(src->cam_pose);
    memcpy(dst.data, src->data, dst.num_char());
}

void rs_sf_util_draw_planes(rs_sf_image * rgb, const rs_sf_image * map, const rs_sf_image *src,
	bool overwrite_rgb, const unsigned char * rgb_table[3], int num_color)
{
	static unsigned char default_r[MAX_VALID_PID + 1] = { 0 };
	static unsigned char default_g[MAX_VALID_PID + 1] = { 0 };
	static unsigned char default_b[MAX_VALID_PID + 1] = { 0 };
	static unsigned char* default_rgb_table[3] = { default_r,default_g,default_b };
	if (default_rgb_table[0][MAX_VALID_PID] == 0)
	{
		for (int pid = MAX_VALID_PID; pid >= 0; --pid)
		{
			default_rgb_table[0][pid] = (pid & 0x01) << 7 | (pid & 0x08) << 3 | (pid & 0x40) >> 1;
			default_rgb_table[1][pid] = (pid & 0x02) << 6 | (pid & 0x10) << 2 | (pid & 0x80) >> 2;
			default_rgb_table[2][pid] = (pid & 0x04) << 5 | (pid & 0x20) << 1;
		}
	}

	if (rgb_table == nullptr || num_color <= 0) {
		rgb_table = (const unsigned char**)default_rgb_table;
		num_color = (int)(sizeof(default_r) / sizeof(unsigned char));
	}

	auto* const map_data = map->data, *const rgb_data = rgb->data;
	if (!overwrite_rgb) {
		rs_sf_util_convert_to_rgb_image(rgb, src);
		for (int p = map->num_pixel() - 1, p3 = p * 3; p >= 0; --p, p3 -= 3) {
			const int pid = map_data[p];
			rgb_data[p3 + 0] = (rgb_data[p3 + 0] >> 1) + (rgb_table[0][pid] >> 1);
			rgb_data[p3 + 1] = (rgb_data[p3 + 1] >> 1) + (rgb_table[1][pid] >> 1);
			rgb_data[p3 + 2] = (rgb_data[p3 + 2] >> 1) + (rgb_table[2][pid] >> 1);
		}
	}
	else {
		for (int p = map->num_pixel() - 1, p3 = p * 3; p >= 0; --p, p3 -= 3) {
			const int pid = map_data[p];
			rgb_data[p3 + 0] = rgb_table[0][pid];
			rgb_data[p3 + 1] = rgb_table[1][pid];
			rgb_data[p3 + 2] = rgb_table[2][pid];
		}
	}
}

void rs_sf_util_scale_plane_ids(rs_sf_image * map, int max_pid)
{
    max_pid = std::max(1, max_pid);
    for (int p = map->num_pixel() - 1; p >= 0; --p)
        map->data[p] = map->data[p] * 254 / max_pid;
}

void rs_sf_util_remap_plane_ids(rs_sf_image * map)
{
    for (int p = map->num_pixel() - 1; p >= 0; --p)
        map->data[p] = ((map->data[p] << 4) | (map->data[p] >> 4));
}

void rs_sf_util_draw_line_rgb(rs_sf_image * rgb, v2 p0, v2 p1, const b3& color, const int size)
{
    if (p0.array().isInf().any() || p1.array().isInf().any()) return;
	
    const auto w = rgb->img_w, h = rgb->img_h;
    const auto dir = (p1 - p0).normalized();
    const auto len = (p1 - p0).norm();
    std::unordered_map<int, float> line_point;
    for (float s = 0.0f, hs = size * 0.5f; s < len; ++s) {
        const auto p = p0 + dir * s;
        const int lx = std::max(0, (int)(p.x() - hs - 1)), ly = std::max(0, (int)(p.y() - hs - 1));
        const int rx = std::min(w, (int)(p.x() + hs + 1.5f)), ry = std::min(h, (int)(p.y() + hs + 1.5f));
        for (int y = ly, px; y < ry; ++y)
            for (int x = lx; x < rx; ++x)
                if (line_point.find(px = y*w + x) == line_point.end()) {
                    const auto r = v2(p0.x() - (float)x, p0.y() - (float)y);
                    const auto d = (r - dir*(r.dot(dir))).norm();
                    line_point[px] = std::min(1.0f, std::max(0.0f, d - hs));
                }
    }

    for (const auto& pt : line_point) {
        const auto px = pt.first * 3;
        const auto alpha = pt.second, beta = 1.0f - alpha;
        rgb->data[px + 0] = static_cast<unsigned char>((rgb->data[px + 0] * alpha) + (color[0] * beta));
        rgb->data[px + 1] = static_cast<unsigned char>((rgb->data[px + 1] * alpha) + (color[1] * beta));
        rgb->data[px + 2] = static_cast<unsigned char>((rgb->data[px + 2] * alpha) + (color[2] * beta));
    }
}

void rs_sf_util_to_box_frame(const rs_sf_box& box, v3 box_frame[12][2])
{
    static const int line_index[][2][3] = {
        {{0,0,0},{1,0,0}}, {{0,0,1},{1,0,1}}, {{0,1,0},{1,1,0}}, {{0,1,1},{1,1,1}},
        {{0,0,0},{0,1,0}}, {{0,0,1},{0,1,1}}, {{1,0,0},{1,1,0}}, {{1,0,1},{1,1,1}},
        {{0,0,0},{0,0,1}}, {{0,1,0},{0,1,1}}, {{1,0,0},{1,0,1}}, {{1,1,0},{1,1,1}} };

    const auto& p0 = box.center, &a0 = box.axis[0], &a1 = box.axis[1], &a2 = box.axis[2];
    for (int l = 0; l < 12; ++l) {
        const auto w0 = line_index[l][0], w1 = line_index[l][1];
        for (int d = 0; d < 3; ++d) {
            box_frame[l][0][d] = p0[d] + a0[d] * (w0[0]-0.5f) + a1[d] * (w0[1]-0.5f) + a2[d] * (w0[2]-0.5f);
            box_frame[l][1][d] = p0[d] + a0[d] * (w1[0]-0.5f) + a1[d] * (w1[1]-0.5f) + a2[d] * (w1[2]-0.5f);
        }
    }
}

void rs_sf_util_draw_boxes(rs_sf_image * rgb, const pose_t& pose, const rs_sf_intrinsics& camera, const std::vector<rs_sf_box>& boxes)
{
    auto proj = [to_cam = pose.invert(), cam = camera](const v3& pt) {
        const auto pt3d = to_cam.rotation * pt + to_cam.translation;
        return v2{
            (pt3d.x() * cam.fx) / pt3d.z() + cam.ppx,
            (pt3d.y() * cam.fy) / pt3d.z() + cam.ppy };
    };
    
    v3 box_frame[12][2];
    for (auto& box : boxes)
    {
        rs_sf_util_to_box_frame(box, box_frame);
        for (const auto& line : box_frame)
            rs_sf_util_draw_line_rgb(rgb, proj(line[0]), proj(line[1]), b3{ 255,255,0 });
    }
}

void eigen_3x3_real_symmetric(float D[6], float u[3], float v[3][3])
{
	static const float EPSILONZERO = 0.0001f;
	static const float PI = 3.14159265358979f;
	static const float Euclidean[3][3] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	const float tr = std::max(1.0f, D[0] + D[3] + D[5]);
	const float tt = 1.0f / tr; // scale down factor
	const float d1 = D[0] * tt; // scale down to avoid numerical overflow
	const float d2 = D[1] * tt; // scale down to avoid numerical overflow
	const float d3 = D[2] * tt; // scale down to avoid numerical overflow
	const float d4 = D[3] * tt; // scale down to avoid numerical overflow
	const float d5 = D[4] * tt; // scale down to avoid numerical overflow
	const float d6 = D[5] * tt; // scale down to avoid numerical overflow

								// non-diagonal elements
	const float d2d2 = d2*d2;
	const float d3d3 = d3*d3;
	const float d5d5 = d5*d5;
	const float d2d3 = d2*d3;
	const float d2d5 = d2*d5;
	const float sumNonDiagonal = d2d2 + d3d3 + d5d5;

	// non diagnoal matrix
	if (sumNonDiagonal >= EPSILONZERO)
	{
		// constant coefficient
		const float a0 = d6*d2d2 + d4*d3d3 + d1*d5d5 - d1*d4*d6 - 2 * d2d3*d5;
		// x   coeff
		const float a1 = d1*d4 + d1*d6 + d4*d6 - sumNonDiagonal;
		// x^2 coeff
		const float a2 = d1 + d4 + d6;
		const float a2a2 = a2*a2;
		const float q = (a2a2 - 3 * a1) / 9;
		const float r = (2 * a2*a2a2 - 9 * a2*a1 - 27 * a0) / 54;

		// assume three real roots
		const float sqrtq = std::sqrt(q > 0 ? q : 0);
		const float rdsqq3 = r / (q*sqrtq);
		const float tha = std::acos(std::max(-1.0f, std::min(1.0f, rdsqq3)));

		const float a2div3 = a2 / 3;
		u[0] = 2 * sqrtq * std::cos(tha / 3) + a2div3;
		u[1] = 2 * sqrtq * std::cos((tha + 2 * PI) / 3) + a2div3;
		u[2] = a2 - u[0] - u[1]; // 2 * sqrtq*std::cos((tha + 4 * PI) / 3) + a2div3;
	}
	else // pure diagonal matrix
	{
		u[0] = d1;
		u[1] = d4;
		u[2] = d6;
	}

	//sorting three numbers
	float buf;
	if (u[0] >= u[1]) {
		if (u[1] >= u[2]) {
			//correct order
		}
		else {
			//u[1]<u[2]
			if (u[0] >= u[2]) {
				//u[0] >= u[2] > u[1]
				buf = u[1];
				u[1] = u[2];
				u[2] = buf;
			}
			else {
				//u[2]>u[0]>=u[1]
				buf = u[0];
				u[0] = u[2];
				u[2] = u[1];
				u[1] = buf;
			}
		}
	}
	else { //u[1] > u[0]
		if (u[0] >= u[2]) {
			//u[1] > u[0] >= u[2]
			buf = u[0];
			u[0] = u[1];
			u[1] = buf;
		}
		else {
			//u[2] > u[0]
			if (u[2] > u[1]) {
				//u[2]>u[1]>u[0]
				buf = u[0];
				u[0] = u[2];
				u[2] = buf;
			}
			else {
				//u[1] >= u[2] > u[0]
				buf = u[2];
				u[2] = u[0];
				u[0] = u[1];
				u[1] = buf;
			}
		}
	}

	// first assume all three are distinct
	if (v != nullptr)
	{
		bool vlenZero[3] = { 0 };
		int nZeroVec = 0;
		for (int n = 0; n < 3; ++n)
		{
			// Cramer's rule
			const auto& un = u[n];
			const float v0 = d2d5 - d3*(d4 - un);
			const float v1 = d2d3 - d5*(d1 - un);
			const float v2 = (d1 - un)*(d4 - un) - d2d2;
			// normalize [v0,v1,v2] to form eigenvector n
			const float vlen = std::sqrt(v0*v0 + v1*v1 + v2*v2);
			const bool  bvlenZero = vlen < EPSILONZERO;
			const float divisor = 1.0f / (vlen + (bvlenZero & 0x1));
			v[n][0] = v0*divisor;
			v[n][1] = v1*divisor;
			v[n][2] = v2*divisor;
			vlenZero[n] = bvlenZero;
			// count number of non-zero vectors
			nZeroVec += (bvlenZero & 0x1);
		}

		switch (nZeroVec)
		{
		case 0:
			break;
		case 3: // perfect sphere - pure diagonal matrix 
		{
			if (std::abs(u[0] - d1) < EPSILONZERO)
				v[0][0] = 1;
			else if (std::abs(u[0] - d4) < EPSILONZERO)
				v[0][1] = 1;
			else if (std::abs(u[0] - d6) < EPSILONZERO)
				v[0][2] = 1;

			// continue to case 2;
			vlenZero[0] = false;
		}
		case 2: // symmetric ellipsoid or carry over from perfect sphere
		{
			for (int n = 0; n < 3; ++n)
			{
				//only this vector is non-zero
				if (!vlenZero[n])
				{
					for (int m = 0; m < 3; ++m)
					{
						// if Euclidean[m] is the major axis, fr1 fr2 are the only possible orthogonal plane
						if (std::abs(
							Euclidean[m][0] * v[n][0] +
							Euclidean[m][1] * v[n][1] +
							Euclidean[m][2] * v[n][2]) > EPSILONZERO)
						{
							const int n1 = (n + 1) % 3;
							auto& vn1 = v[n1];
							const auto& fr1 = Euclidean[(m + 1) % 3];
							const auto& fr2 = Euclidean[(m + 2) % 3];
							const auto& un1 = u[n1];

							if (std::abs(un1 * fr1[0] -
								(fr1[0] * d1 + fr1[1] * d2 + fr1[2] * d3)) < EPSILONZERO &&
								std::abs(un1 * fr1[1] -
								(fr1[0] * d2 + fr1[1] * d4 + fr1[2] * d5)) < EPSILONZERO &&
								std::abs(un1 * fr1[2] -
								(fr1[0] * d3 + fr1[1] * d5 + fr1[2] * d6)) < EPSILONZERO)
							{
								// if fr1 match the eigenvalue n1, v[n1]=fr1
								vn1[0] = fr1[0];
								vn1[1] = fr1[1];
								vn1[2] = fr1[2];
							}
							else
							{
								// if fr2 match the eigenvalue n1, v[n1]=fr2
								vn1[0] = fr2[0];
								vn1[1] = fr2[1];
								vn1[2] = fr2[2];
							}

							// match sign of eigenvalue of n1
							if ((vn1[0] * d1 + vn1[1] * d2 + vn1[2] * d3)* un1 < 0 ||
								(vn1[0] * d2 + vn1[1] * d4 + vn1[2] * d5)* un1 < 0 ||
								(vn1[0] * d3 + vn1[1] * d5 + vn1[2] * d6)* un1 < 0)
							{
								vn1[0] *= -1;
								vn1[1] *= -1;
								vn1[2] *= -1;
							}
							// continue to case 1;
							vlenZero[n1] = false;
							n = 3;
							break;
						}
					}
				}
			}
		}
		case 1: // 1 of the eigenvectors is unknown or carry over from symmetric ellipsoid or perfect sphere
		{
			for (int n = 0; n < 3; ++n)
			{
				if (vlenZero[n]) //this vector is known, use vector cross-product to find it
				{
					const auto& x = v[(n + 1) % 3];
					const auto& y = v[(n + 2) % 3];
					auto& vn2 = v[n];
					vn2[0] = (x[1] * y[2] - x[2] * y[1]);
					vn2[1] = (x[2] * y[0] - x[0] * y[2]);
					vn2[2] = (x[0] * y[1] - x[1] * y[0]);

					// match sign of eigenvalue of n
					if ((vn2[0] * d1 + vn2[1] * d2 + vn2[2] * d3)*u[n] < 0 ||
						(vn2[0] * d2 + vn2[1] * d4 + vn2[2] * d5)*u[n] < 0 ||
						(vn2[0] * d3 + vn2[1] * d5 + vn2[2] * d6)*u[n] < 0)
					{
						vn2[0] *= -1;
						vn2[1] *= -1;
						vn2[2] *= -1;
					}
				}
			}
		}
		default:
			break;
		}
	}
	// scale back for original matrix 
	u[0] *= tr;
	u[1] *= tr;
	u[2] *= tr;
}

enum { BACKGROUND = 0, FOREGROUND = 1, CONTOUR_LEFT = 2, CONTOUR_RIGHT = 3 };
std::vector<contour> find_contours_in_binary_img(rs_sf_image* bimg)
{
    std::vector<contour> dst;
    const int W = bimg->img_w, H = bimg->img_h;
    auto* pixel = bimg->data;
    int parent_contour_id = BACKGROUND;
    for (int y = 1, i = W + 1; y < H - 1; ++y, i += 2) {
        for (int x = 1; x < W - 1; ++x, ++i) {
            if (parent_contour_id == BACKGROUND) //outside
            {
                if (pixel[i] == BACKGROUND && pixel[i + 1] == FOREGROUND)
                {
                    dst.push_back(follow_border(pixel, W, H, i + 1));
                    parent_contour_id = (int)dst.size();
                }
            }
            else //inside
            {
                if (pixel[i] == CONTOUR_RIGHT) parent_contour_id = BACKGROUND;
            }
        }
        parent_contour_id = BACKGROUND;
    }
    return dst;
}

contour follow_border(uint8_t* pixel, const int w, const int h, const int _x0)
{
    std::vector<i2> dst;
    dst.reserve(200);

    const int xb[] = { -w,-1,w,1 };             //cross neighbor
    const int xd[] = { -w + 1,-w - 1,w - 1,w + 1 }; //diagonal neigbhor

    int bz0 = -1; // first zero neighbor
    for (int b = 0; b < 4; ++b)
        if (pixel[_x0 + xb[b]] == 0) { bz0 = b; break; }

    for (int x0 = _x0, x1 = -1, bx1, tx1; _x0 != x1; x0 = x1)
    {
        //first non-zero neighbor after first zero neighbor
        if (pixel[(x1 = (x0 + xb[(bx1 = (bz0 + 1) % 4)]))] != 0) {}
        else if (pixel[(x1 = (x0 + xb[(bx1 = (bz0 + 2) % 4)]))] != 0) {}
        else if (pixel[(x1 = (x0 + xb[(bx1 = (bz0 + 3) % 4)]))] != 0) {}
        else { break; }

        if (pixel[tx1 = x0 + xd[bx1]] != 0) { x1 = tx1; bz0 = (bx1 + 2) % 4; }
        else { bz0 = (bx1 + 3) % 4; }

        dst.push_back(i2(x0 % w, x0 / w));
        pixel[x0] = (pixel[x0 + 1] != 0 ? CONTOUR_LEFT : CONTOUR_RIGHT);
    }
    return dst;
}
