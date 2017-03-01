#include "rs_sf_util.h"

void set_to_zeros(rs_sf_image* img)
{
    memset(img->data, 0, img->num_char());
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

void draw_planes(rs_sf_image * rgb, const rs_sf_image * map, const rs_sf_image *src, const unsigned char * rgb_table[3], int num_color)
{
    static unsigned char default_r[256] = { 0};
    static unsigned char default_g[256] = { 0};
    static unsigned char default_b[256] = { };
    static unsigned char* default_rgb_table[3] = { default_r,default_g,default_b };
    if (default_rgb_table[0][255] == 0)
    {
        for (int pid = 255; pid >= 0; --pid)
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

    if (src != nullptr && src->byte_per_pixel == 1)
        for (int p = map->num_pixel() - 1; p >= 0; --p)
            rgb->data[p * 3 + 0] = rgb->data[p * 3 + 1] = rgb->data[p * 3 + 2] = src->data[p];
    else if (src != nullptr && src->byte_per_pixel == 3)
        memcpy(rgb->data, src->data, rgb->num_char());

    for (int p = map->num_pixel() - 1; p >= 0; --p) {
        const int pid = map->data[p] - 1;
        if (0 <= pid && pid < num_color) {
            rgb->data[p * 3 + 0] |= rgb_table[0][pid];
            rgb->data[p * 3 + 1] |= rgb_table[1][pid];
            rgb->data[p * 3 + 2] |= rgb_table[2][pid];
        }
    }
}

