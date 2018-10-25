/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
//
//  rs_sf_util.hpp
//  algo-core
//
//  Created by Hon Pong (Gary) Ho
//
#include "rs_sf_util.h"

void rs_sf_util_set_to_zeros(rs_sf_image* img)
{
    memset(img->data, 0, img->num_char());
}

void rs_sf_util_convert_to_rgb_image(rs_sf_image * rgb, const rs_sf_image * src)
{
    if (!src) return;
    if (rgb->img_h == src->img_h && rgb->img_w == src->img_w) {
        if (src->byte_per_pixel == 1)
            for (int p = src->num_pixel() - 1, p3 = p * 3; p >= 0; p3 -= 3)
                rgb->data[p3] = rgb->data[p3 + 1] = rgb->data[p3 + 2] = src->data[p--];
        else if (src->byte_per_pixel == 3)
            memcpy(rgb->data, src->data, rgb->num_char());
    }
    else {
        for (int p = rgb->num_pixel() - 1, p3 = p * 3, s, sb = src->byte_per_pixel, s1 = sb / 3, s2 = s1 * 2, w = rgb->img_w, sw = src->img_w, dy = src->img_h / rgb->img_h * sb, dx = sw / w * sb; p >= 0; --p, p3 -= 3)
        {
            rgb->data[p3 + 0] = src->data[s = ((p / w) * dy) * sw + ((p % w) * dx)];
            rgb->data[p3 + 1] = src->data[s + s1];
            rgb->data[p3 + 2] = src->data[s + s2];
        }
    }
}

void rs_sf_util_copy_depth_image(rs_sf_image_depth & dst, const rs_sf_image * src)
{
    dst.frame_id = src->frame_id;
    dst.set_pose(src->cam_pose);
    memcpy(dst.data, src->data, dst.num_char());
}

void rs_sf_util_copy_color_image(rs_sf_image_rgb& dst, const rs_sf_image* src)
{
    dst.frame_id = src->frame_id;
    dst.set_pose(src->cam_pose);
    rs_sf_util_convert_to_rgb_image(&dst, src);
}

void rs_sf_util_draw_plane_ids(rs_sf_image * rgb, const rs_sf_image * map, bool overwrite_rgb, const unsigned char(*rgb_table)[3])
{
    static unsigned char default_rgb_table[MAX_VALID_PID + 2][3] = {};
    if (default_rgb_table[MAX_VALID_PID][0] == 0)
    {
        for (int pid = MAX_VALID_PID; pid >= 0; --pid)
        {
            default_rgb_table[pid][0] = (pid & 0x01) << 7 | (pid & 0x08) << 3 | (pid & 0x40) >> 1;
            default_rgb_table[pid][1] = (pid & 0x02) << 6 | (pid & 0x10) << 2 | (pid & 0x80) >> 2;
            default_rgb_table[pid][2] = (pid & 0x04) << 5 | (pid & 0x20) << 1;
        }
        memset(default_rgb_table[MAX_VALID_PID + 1], 255, 3);
    }
    if (rgb_table == nullptr) { rgb_table = default_rgb_table; }
     
    auto* const map_data = map->data, *const rgb_data = rgb->data;
    if (!overwrite_rgb)
    {
        for (int p = map->num_pixel() - 1, p3 = p * 3; p >= 0; p3 -= 3) {
            const auto& color = rgb_table[map_data[p--]];
            rgb_data[p3 + 0] = (rgb_data[p3 + 0] >> 1) + (color[0] >> 1);
            rgb_data[p3 + 1] = (rgb_data[p3 + 1] >> 1) + (color[1] >> 1);
            rgb_data[p3 + 2] = (rgb_data[p3 + 2] >> 1) + (color[2] >> 1);
        }
    }
    else {
        for (int p = map->num_pixel() - 1, p3 = p * 3; p >= 0; p3 -= 3) {
            memcpy(rgb_data + p3, rgb_table[map_data[p--]], 3);
        }
    }
}

void rs_sf_util_scale_plane_ids(rs_sf_image * map, int max_pid)
{
    max_pid = std::max(1, max_pid);
    for (int p = map->num_pixel() - 1; p >= 0; --p)
        map->data[p] = map->data[p] * MAX_VALID_PID / max_pid;
}

void rs_sf_util_remap_plane_ids(rs_sf_image * map)
{
    for (int p = map->num_pixel() - 1; p >= 0; --p)
        map->data[p] = ((map->data[p] << 4) | (map->data[p] >> 4));
}

void rs_sf_util_draw_line_rgb(rs_sf_image * rgb, const v2& p0, const v2& p1, const b3& color, const int size)
{
    if (p0.array().isInf().any() || p1.array().isInf().any()) return;

    const auto w = rgb->img_w, h = rgb->img_h;
    const auto diff = (p1 - p0);
    const auto dir = diff.normalized();
    std::unordered_map<int, int> line_point;
    for (float s = size * 0.5f, len = std::max(s, diff.norm() - s), hs = s; s <= len; ++s) {
        const auto p = p0 + dir * s;
        const int lx = std::max(0, (int)(p.x() - hs - 1.0f)), ly = std::max(0, (int)(p.y() - hs - 1.0f));
        const int rx = std::min(w, (int)(p.x() + hs + 1.5f)), ry = std::min(h, (int)(p.y() + hs + 1.5f));
        for (int y = ly, px; y < ry; ++y)
            for (int x = lx, yw = y*w; x < rx; ++x)
                if (line_point.find(px = yw + x) == line_point.end()) {
                    const auto r = v2(p0.x() - (float)x, p0.y() - (float)y);
                    const auto d = (r - dir*(r.dot(dir))).norm();
                    line_point[px] = std::max(0, (int)(256 * std::min(1.0f, 1.0f - d + hs)));
                }
    }

    const int c0 = color[0], c1 = color[1], c2 = color[2];
    auto* rgb_data = rgb->data;
    for (const auto& pt : line_point) {
        const auto px = pt.first * 3, beta = pt.second;
        rgb_data[px + 0] += static_cast<unsigned char>(((c0 - rgb_data[px + 0]) * beta) >> 8);
        rgb_data[px + 1] += static_cast<unsigned char>(((c1 - rgb_data[px + 1]) * beta) >> 8);
        rgb_data[px + 2] += static_cast<unsigned char>(((c2 - rgb_data[px + 2]) * beta) >> 8);
    }
}

void rs_sf_util_draw_plane_contours(rs_sf_image * rgb, const pose_t & pose, const rs_sf_intrinsics & camera,
    const rs_sf_plane planes[RS_SF_MAX_PLANE_COUNT], const int pt_per_line)
{
    const b3 plane_wire_color(255, 255, 255);
    pose_t to_cam = pose.invert();
    const int line_width = rs_sf_util_image_to_line_width(rgb);
    for (int pl = 0, next = std::max(1, pt_per_line / 2); pl < RS_SF_MAX_PLANE_COUNT; ++pl) {
        if (planes[pl].plane_id == 0) break;
        auto* pos = planes[pl].pos;
        v2 uv0 = { std::numeric_limits<float>::infinity(),0 }, uvp = uv0;
        for (int np = planes[pl].num_points, p = np - 1, prev = np - next; p >= 0; p -= pt_per_line) {
            const auto cam_pt = to_cam.transform(
                (v3(pos[(p + prev) % np]) + v3(pos[p]) + v3(pos[(p + next) % np]))*(1.0f / 3.0f));
            if (cam_pt.z() < 0.001f) continue;
            const float iz = 1.0f / cam_pt.z();
            const v2 uv(
                ((cam_pt.x() * camera.fx) * iz + camera.ppx),
                ((cam_pt.y() * camera.fy) * iz + camera.ppy));
            if (p == (np - 1)) { uv0 = uv; }
            else { rs_sf_util_draw_line_rgb(rgb, uv, uvp, plane_wire_color, line_width); }
            uvp = uv;
        }
        rs_sf_util_draw_line_rgb(rgb, uv0, uvp, plane_wire_color, line_width);
    }
}

void rs_sf_util_to_box_frame(const rs_sf_box& box, v3 box_frame[12][2])
{
    static const float line_index[][2][3] = {
        {{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f}}, {{-.5f,-.5f,.5f},{.5f,-.5f,.5f}}, {{-.5f,.5f,-.5f},{.5f,.5f,-.5f}}, {{-.5f,.5f,.5f},{.5f,.5f,.5f}},
        {{-.5f,-.5f,-.5f},{-.5f,.5f,-.5f}}, {{-.5f,-.5f,.5f},{-.5f,.5f,.5f}}, {{.5f,-.5f,-.5f},{.5f,.5f,-.5f}}, {{.5f,-.5f,.5f},{.5f,.5f,.5f}},
        {{-.5f,-.5f,-.5f},{-.5f,-.5f,.5f}}, {{-.5f,.5f,-.5f},{-.5f,.5f,.5f}}, {{.5f,-.5f,-.5f},{.5f,-.5f,.5f}}, {{.5f,.5f,-.5f},{.5f,.5f,.5f}} };

    const auto& p0 = box.center, &a0 = box.axis[0], &a1 = box.axis[1], &a2 = box.axis[2];
    for (int l = 0; l < 12; ++l) {
        const auto w0 = line_index[l][0], w1 = line_index[l][1];
        for (int d = 0; d < 3; ++d) {
            box_frame[l][0][d] = p0[d] + a0[d] * w0[0] + a1[d] * w0[1] + a2[d] * w0[2];
            box_frame[l][1][d] = p0[d] + a0[d] * w1[0] + a1[d] * w1[1] + a2[d] * w1[2];
        }
    }
}

void rs_sf_util_draw_boxes(rs_sf_image * rgb, const pose_t& pose, const rs_sf_intrinsics& camera, const std::vector<rs_sf_box>& boxes, const b3& color)
{
    auto proj = [to_cam = pose.invert(), &camera](const v3& pt) {
        const auto pt3d = to_cam.rotation * pt + to_cam.translation;
        return v2{
            (pt3d.x() * camera.fx) / pt3d.z() + camera.ppx,
            (pt3d.y() * camera.fy) / pt3d.z() + camera.ppy };
    };

    v3 box_frame[12][2];
    const int line_width = rs_sf_util_image_to_line_width(rgb);
    for (auto& box : boxes)
    {
        rs_sf_util_to_box_frame(box, box_frame);
        for (const auto& line : box_frame)
            rs_sf_util_draw_line_rgb(rgb, proj(line[0]), proj(line[1]), color, line_width);
    }
}

box_plane rs_sf_util_get_box_plane(const rs_sf_box& box, int pid)
{
    const static float a0[][4] = { { -.5f,-.5f,-.5f,-.5f },{  .5f, .5f, .5f, .5f },  //a0 as normal
                                   { -.5f, .5f, .5f,-.5f },{ -.5f, .5f, .5f,-.5f },  //a1 as normal
                                   { -.5f,-.5f, .5f, .5f },{ -.5f,-.5f, .5f, .5f } };//a2 as normal
    const static float a1[][4] = { { -.5f, .5f, .5f,-.5f },{ -.5f, .5f, .5f,-.5f },  //a0 as normal
                                   { -.5f,-.5f,-.5f,-.5f },{  .5f, .5f, .5f, .5f },  //a1 as normal
                                   { -.5f, .5f, .5f,-.5f },{ -.5f, .5f, .5f,-.5f } };//a2 as normal
    const static float a2[][4] = { { -.5f,-.5f, .5f, .5f },{ -.5f,-.5f, .5f, .5f },  //a0 as normal
                                   { -.5f,-.5f, .5f, .5f },{ -.5f,-.5f, .5f, .5f },  //a1 as normal
                                   { -.5f,-.5f,-.5f,-.5f },{  .5f, .5f, .5f, .5f } };//a2 as normal
    box_plane dst;
    for (int p = 0; p < 4; ++p) //four points per plane
        for (int d = 0; d < 3; ++d) { //each x,y,z dimension in world
            dst[p][d] = box.center[d] + box.axis[0][d] * a0[pid][p] + box.axis[1][d] * a1[pid][p] + box.axis[2][d] * a2[pid][p];
        }
    return dst;
}

void rs_sf_util_raycast_boxes(rs_sf_image * depth, const pose_t& pose, const float depth_unit_in_meter, const rs_sf_intrinsics& camera, const std::vector<rs_sf_box>& boxes)
{
    auto to_cam = pose.invert();
    auto proj = [to_cam = to_cam, &camera](const v3& pt) {
        const auto pt3d = to_cam.rotation * pt + to_cam.translation;
        return v2{
            (pt3d.x() * camera.fx) / pt3d.z() + camera.ppx,
            (pt3d.y() * camera.fy) / pt3d.z() + camera.ppy };
    };

    auto in_plane = [](const v2 pl[4], const i2& pt) {
        for (int i = 0, j = 1, pos_d = 0, neg_d = 0; i < 4; ++i, j = (i + 1) % 4) {
            const auto d = (pt.x() - pl[i].x())*(pl[j].y() - pl[i].y()) - (pt.y() - pl[i].y())*(pl[j].x() - pl[i].x());
            pos_d += ((d > 0) & 0x1);
            neg_d += ((d < 0) & 0x1);
            if (pos_d > 0 && neg_d > 0) return false;
        }
        return true;
    };

    uint16_t* dst_depth = (unsigned short*)(depth->data);
    for (const auto& box : boxes) {        // for each box
        for (int pid = 0; pid < 6; ++pid)  // for each box plane
        {
            // box corners in world coordinates
            const auto box_pt3d = rs_sf_util_get_box_plane(box, pid);
            
            // box corners in depth image coorindates
            const v2 pc[4] = {proj(box_pt3d[0]), proj(box_pt3d[1]), proj(box_pt3d[2]), proj(box_pt3d[3])};

            // plane equation
            const v3 plnor = to_cam.rotation * v3(box.axis[pid / 2]); // plane normal by rs_sf_util_get_box_plane()
            const float pl_c0 = -plnor.dot(to_cam.transform(box_pt3d[0])) / depth_unit_in_meter;// plane intercept

            // box plane ROI
            const auto min_i = std::max((int)(.5f + std::min({ pc[0].x(),pc[1].x(),pc[2].x(),pc[3].x() })), 0);
            const auto max_i = std::min((int)(.5f + std::max({ pc[0].x(),pc[1].x(),pc[2].x(),pc[3].x() })), depth->img_w - 1);
            const auto min_j = std::max((int)(.5f + std::min({ pc[0].y(),pc[1].y(),pc[2].y(),pc[3].y() })), 0);
            const auto max_j = std::min((int)(.5f + std::max({ pc[0].y(),pc[1].y(),pc[2].y(),pc[3].y() })), depth->img_h - 1);

            // generate depth within box plane ROI
            for (int j = min_j; j <= max_j; ++j) {
                for (int i = min_i; i <= max_i; ++i) {
                    if (in_plane(pc, i2(i, j))) {
                        // plane-ray intersection
                        const float z = -pl_c0 / ( 
                            plnor[0] * (i - camera.ppx) / camera.fx +
                            plnor[1] * (j - camera.ppy) / camera.fy + plnor[2]);

                        // update z values if it is at the front
                        if (z > 0) { 
                            auto& dst = dst_depth[j*depth->img_w + i];
                            if (dst == 0 || z < dst) dst = static_cast<uint16_t>(z);
                        }
                    }
                }
            }
        }
    }
}

rs_sf_intrinsics rs_sf_util_match_intrinsics(const rs_sf_image * img, const rs_sf_intrinsics & ref)
{
    rs_sf_intrinsics dst = (img->intrinsics ? *img->intrinsics : ref);
    if (img->img_h != dst.height || img->img_w != dst.width) {
        const float scale_x = ((float)img->img_w / dst.width), scale_y = ((float)img->img_h / dst.height);
        dst.fx *= scale_x; dst.ppx *= scale_x; dst.fy *= scale_y; dst.ppy *= scale_y;
        dst.width = img->img_w; dst.height = img->img_h;
    }
    return dst;
}

void eigen_3x3_real_symmetric(float D[6], float u[3], float v[3][3])
{
    static const float EPSILONZERO = 0.0001f;
    static const float PI = 3.14159265358979f;
    static const float Euclidean[3][3] = { {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
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

std::vector<std::vector<int>> find_contours_in_map_uchar(short * map, const int w, const int h, const int min_len)
{
    const int BACKGROUND = 0;
    std::vector<std::vector<int>> dst;
    dst.reserve(MAX_VALID_PID);
    std::vector<int> parent_targets;
    parent_targets.reserve(w);
    for (int y = 1, i = w + 1, ey = h - 1, ex = w - 1; y < ey; ++y, i += 2, parent_targets.clear()) {
        parent_targets.emplace_back(BACKGROUND);
        for (int x = 1; x < ex; ++x, ++i) {
            const auto current = (map[i] & 0x00ff);
            const auto visited = (map[i] & 0x4000);
            if (current != parent_targets.back() && current != BACKGROUND) //not prev contour
            {
                parent_targets.emplace_back(current);
                if (!visited) {
                    try_follow_border_uchar(dst, map, w, h, i, min_len);
                }
            }
            if (map[i] < 0) { parent_targets.pop_back(); }
        }
    }
    return dst;
}

bool try_follow_border_uchar(std::vector<std::vector<int>>& dst_list, short * map, const int w, const int h, const int _x0, const int min_len)
{
    dst_list.emplace_back();
    auto& dst = dst_list.back();
    dst.reserve(w * h / 4);

    const short target = map[_x0] & 0x00ff;
    const short c_left = target | 0x4000;
    const short c_right = target | 0xc000;
    const int xb[] = { -w,-1,w,1 };             //cross neighbor
    const int xd[] = { -w + 1,-w - 1,w - 1,w + 1 }; //diagonal neigbhor

    int bz0 = -1; // first zero neighbor
    for (int b = 0; b < 4; ++b)
        if ((map[_x0 + xb[b]] & 0x00ff) != target) { bz0 = b; break; }

    for (int x0 = _x0, x1 = -1, bx1, tx1; _x0 != x1; x0 = x1)
    {
        //first non-zero neighbor after first zero neighbor
        if ((map[(x1 = (x0 + xb[(bx1 = (bz0 + 1) % 4)]))] & 0x00ff) == target) {}
        else if ((map[(x1 = (x0 + xb[(bx1 = (bz0 + 2) % 4)]))] & 0x00ff) == target) {}
        else if ((map[(x1 = (x0 + xb[(bx1 = (bz0 + 3) % 4)]))] & 0x00ff) == target) {}
        else { break; }

        if ((map[tx1 = x0 + xd[bx1]] & 0x00ff) == target) { x1 = tx1; bz0 = (bx1 + 2) % 4; }
        else { bz0 = (bx1 + 3) % 4; }

        //dst.emplace_back(x0 % w, x0 / w);
        dst.emplace_back(x0);
        map[x0] = ((map[x0 + 1] & 0x00ff) == target ? c_left : c_right); //shape left: shape right
    }
    if ((int)dst.size() < min_len) { dst_list.pop_back(); return false; }
    return true;
}
