#include "stereo.h"
#include "filter.h"

bool debug_state = false;
bool debug_track = false;
bool debug_triangulate = false;
bool debug_F = false;

/* Prints a formatted v4 that can be copied and pasted into Matlab */
void v4_pp(const char * name, v4 vec)
{
    fprintf(stderr, "%s = ", name);
    vec.print();
    fprintf(stderr, ";\n");
}

/* Prints a formatted m4 that can be copied and pasted into Matlab */
void m4_pp(const char * name, m4 mat)
{
    fprintf(stderr, "%s = ", name);
    mat.print();
    fprintf(stderr, ";\n");
}

void write_image(const char * path, uint8_t * image, int width, int height)
{
    FILE * f = fopen(path, "w");
    fprintf(f, "P5 %d %d 255\n", width, height);
    fwrite(image, 1, width*height, f);
    fclose(f);
}

void write_patch(const char * path, uint8_t * image, int stride, int sx, int sy, int ex, int ey)
{
    int width = ex - sx;
    int height = ey - sy;
    uint8_t * patch = (uint8_t *)malloc(sizeof(uint8_t)*width*height);
    for(int y=0; y < height; y++)
    for(int x=0; x < width; x++) {
        patch[x + width*y] = image[sx + x + stride*(sy + y)];
    }
    write_image(path, patch, width, height);
    free(patch);
}


void draw_line(uint8_t * image, int width, int height, int x0, int y0, int x1, int y1)
{
    fprintf(stderr, "drawing a line %d %d %d %d\n", x0, y0, x1, y1);
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
    int err = (dx>dy ? dx : -dy)/2, e2;

    for(;;) {
        image[x0 + y0*width] = 255;
        if (x0==x1 && y0==y1) break;
        e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
    fprintf(stderr, "done drawing a line\n");
}

bool line_endpoints(v4 line, int width, int height, float endpoints[4])
{
    float a = line[0], b = line[1], c = line[2];
    float intersect_y, intersect_x;
    float min_x = 0, max_x = width-1, min_y = 0, max_y = height-1;
    int idx = 0;

    f_t eps = 1e-14;
    // if the line is not vertical
    if(fabs(a) > eps) {
        // intersection with the left edge
        intersect_y = -(a * min_x + c) / b;
        if(intersect_y >= min_y && intersect_y <= max_y) {
            endpoints[idx++] = min_x;
            endpoints[idx++] = intersect_y;
        }

        // intersection with the right edge
        intersect_y = -(a * max_x + c) / b;
        if(intersect_y >= min_y && intersect_y <= max_y) {
            endpoints[idx++] = max_x;
            endpoints[idx++] = intersect_y;
        }
    }

    // if the line is not horizontal
    if(fabs(b) > eps) {
        // intersection with the top edge
        if(idx < 3) {
            intersect_x = -(b * min_y + c) / a;
            if(intersect_x >= min_x && intersect_x <= max_x) {
                endpoints[idx++] = intersect_x;
                endpoints[idx++] = min_y;
            }
        }

        // intersection with the bottom edge
        if(idx < 3) {
            intersect_x = -(b * max_y + c) / a;
            if(intersect_x >= min_x && intersect_x <= max_x) {
                endpoints[idx++] = intersect_x;
                endpoints[idx++] = max_y;
            }
        }
    }

    // if we still haven't found an intersection, return false
    if(idx < 3)
        return false;

    return true;
}

/* Matching functions */
#if 1
#define WINDOW 20
float score_match(const unsigned char *im1, int xsize, int ysize, int stride, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error)
{
    int window = WINDOW;
    int area = (WINDOW*2 + 1) * (WINDOW * 2 + 1);
    
    if(x1 < window || y1 < window || x2 < window || y2 < window || x1 >= xsize - window || x2 >= xsize - window || y1 >= ysize - window || y2 >= ysize - window) return max_error + 1.;

    const unsigned char *p1 = im1 + stride * (y1 - window) + x1;
    const unsigned char *p2 = im2 + stride * (y2 - window) + x2;

    int sum1 = 0, sum2 = 0;
    for(int dy = -window; dy <= window; ++dy, p1+=stride, p2+=stride) {
        for(int dx = -window; dx <= window; ++dx) {
            sum1 += p1[dx];
            sum2 += p2[dx];
        }
    };
    
    float mean1 = sum1 / (float)area;
    float mean2 = sum2 / (float)area;
    
    p1 = im1 + stride * (y1 - window) + x1;
    p2 = im2 + stride * (y2 - window) + x2;
    float top = 0, bottom1 = 0, bottom2 = 0;
    for(int dy = -window; dy <= window; ++dy, p1+=stride, p2+=stride) {
        for(int dx = -window; dx <= window; ++dx) {
            float t1 = (float)p1[dx] - mean1;
            float t2 = (float)p2[dx] - mean2;
            top += t1 * t2;
            bottom1 += (t1 * t1);
            bottom2 += (t2 * t2);
        }
    }
    // constant patches can't be matched
    if(fabs(bottom1) < 1e-15 || fabs(bottom2) < 1e-15)
      return max_error + 1.;

    return -top/sqrtf(bottom1 * bottom2);
}
#else
#define WINDOW 20
float score_match(const unsigned char *im1, int xsize, int ysize, int stride, const int x1, const int y1, const unsigned char *im2, const int x2, const int y2, float max_error)
{
    int window = WINDOW;
    //int area = (WINDOW * 2 + 1) * (WINDOW * 2 + 1) + 3 * 3 + 1;
    int area = (WINDOW * 2 + 1) * (WINDOW * 2 + 1);
    
    if(x1 < window || y1 < window || x2 < window || y2 < window || x1 >= xsize - window || x2 >= xsize - window || y1 >= ysize - window || y2 >= ysize - window) return max_error + 1.;

    const unsigned char *p1 = im1 + stride * (y1 - window) + x1;
    const unsigned char *p2 = im2 + stride * (y2 - window) + x2;
    //int error = abs((short)p1[stride * window] - (short)p2[stride * window]);
    int error = 0;
    int total_max_error = max_error * area;
    for(int dy = -window; dy <= window; ++dy, p1+=stride, p2+=stride) {
        for(int dx = -window; dx <= window; ++dx) {
          error += abs((float)p1[dx] - (float)p2[dx]);
        //if(dy >= -1 && dy <= 1)
        //    error += abs((short)p1[-1]-(short)p2[-1]) + abs((short)p1[0]-(short)p2[0]) + abs((short)p1[1]-(short)p2[1]);
        //if(error >= total_max_error) return max_error + 1;
        }
    }
    return (float)error/(float)area;
}
#endif


float track_line(uint8_t * im1, uint8_t * im2, int width, int height, int currentx, int currenty, int x0, int y0, int x1, int y1, int * bestx, int * besty)
{
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
    int err = (dx>dy ? dx : -dy)/2, e2;

    float bestscore = 300;

    char buffer[80];
    FILE * fp;
    if(debug_track) {
        fprintf(stderr, "track %d %d\n", currentx, currenty);
        write_image("I1.pgm", im1, width, height);
        write_image("I2.pgm", im2, width, height);

        sprintf(buffer, "line_%d_%d_w%d.txt", currentx, currenty, WINDOW);
        fp = fopen(buffer, "w");
        fprintf(fp, "x\ty\tscore\n");
    }

    while(true) {
        float score = score_match(im1, width, height, width, currentx, currenty, im2, x0, y0, 300);
        if(debug_track)
            fprintf(fp, "%d\t%d\t%f\n", x0, y0, score);

        if(score < bestscore) {
          bestscore = score;
          *bestx = x0;
          *besty = y0;
        }

        // move along the line
        if (x0==x1 && y0==y1) break;
        e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
    if(debug_track)
        fclose(fp);

    fprintf(stderr, "best match for %d %d was %d %d with a score of %f\n", currentx, currenty, *bestx, *besty, bestscore);

    if(debug_track) {
        sprintf(buffer, "debug_patchI1_%d_%d.pgm", currentx, currenty);
        write_patch(buffer, im1, width, currentx - WINDOW, currenty - WINDOW, currentx + WINDOW, currenty + WINDOW);
        sprintf(buffer, "debug_patchI2_%d_%d.pgm", currentx, currenty);
        write_patch(buffer, im2, width, *bestx - WINDOW, *besty - WINDOW, *bestx + WINDOW, *besty + WINDOW);
    }

    return bestscore;
}

f_t estimate_kr(v4 point, f_t k1, f_t k2, f_t k3)
{
    f_t r2 = point[0]*point[0] + point[1]*point[1];
    f_t r4 = r2 * r2;
    f_t r6 = r4 * r2;
    f_t kr = 1. + r2 * k1 + r4 * k2 + r6 * k3;
    return kr;
}

v4 calibrate_im_point(v4 normalized_point, float k1, float k2, float k3)
{
    f_t kr;
    v4 calibrated_point = normalized_point;
    //forward calculation - guess calibrated from initial
    kr = estimate_kr(normalized_point, k1, k2, k3);
    calibrated_point[0] /= kr;
    calibrated_point[1] /= kr;
    //backward calculation - use calibrated guess to get new parameters and recompute
    kr = estimate_kr(calibrated_point, k1, k2, k3);
    calibrated_point = normalized_point;
    calibrated_point[0] /= kr;
    calibrated_point[1] /= kr;
    return calibrated_point;
}

v4 project_point(f_t x, f_t y, f_t center_x, f_t center_y, float focal_length)
{
    return v4((x - center_x)/focal_length, (y - center_y)/focal_length, 1, 1);
}

// From http://paulbourke.net/geometry/pointlineplane/lineline.c
bool line_line_intersect(v4 p1, v4 p2, v4 p3, v4 p4, v4 & pa, v4 & pb)
{
    v4 p13,p43,p21;
    double d1343,d4321,d1321,d4343,d2121;
    double numer,denom;

    f_t eps = 1e-14;

    p13 = p1 - p3;
    p43 = p4 - p3;
    if (fabs(p43[0]) < eps && fabs(p43[1]) < eps && fabs(p43[2]) < eps)
      return false;

    p21 = p2 - p1;
    if (fabs(p21[0]) < eps && fabs(p21[1]) < eps && fabs(p21[2]) < eps)
      return false;

    d1343 = sum(p13 * p43); //p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
    d4321 = sum(p43 * p21); //p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
    d1321 = sum(p13 * p21); //p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
    d4343 = sum(p43 * p43); //p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
    d2121 = sum(p21 * p21); //p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

    denom = d2121 * d4343 - d4321 * d4321;
    if (fabs(denom) < eps)
      return false;
    numer = d1343 * d4321 - d1321 * d4343;

    float mua = numer / denom;
    float mub = (d1343 + d4321 * mua) / d4343;

    pa = p1 + mua*p21;
    pb = p3 + mub*p43;
    // Return homogeneous points
    pa[3] = 1; 
    pb[3] = 1;

    return true;
}

#warning estimate_F doesnt agree with eight point F
m4 estimate_F(stereo_state * s1, stereo_state * s2)
{
    m4 R1w = rodrigues(s1->W, NULL);
    m4 R2w = rodrigues(s2->W, NULL);
    m4 dR = transpose(R1w)*R2w;
    m4_pp("dR", dR);
    v4 dT = s2->T - s1->T;

    m4 Rbc = rodrigues(s2->Wc, NULL);
    m4 Rcb = transpose(Rbc);

    dT = Rcb*dT;
    v4_pp("Rcb_dT", dT);

    // E12 is 3x3
    m4 E12 = skew3(dT)*dR;
    m4_pp("E12", dR);

    m4 Kinv;
    Kinv[0][0] = 1./s2->focal_length;
    Kinv[1][1] = 1./s2->focal_length;
    Kinv[0][2] = -s2->center_x/s2->focal_length;
    Kinv[1][2] = -s2->center_y/s2->focal_length;
    Kinv[2][2] = 1;
    Kinv[3][3] = 1;
    m4_pp("Kinv", Kinv);

    m4 F12 = transpose(Kinv)*E12*Kinv;
    m4_pp("F12", F12);

    return F12;
}

// F is from s1 to s2
bool find_correspondence(stereo_state * s1, stereo_state * s2, m4 F, int s1_x, int s1_y, int * s2_x, int * s2_y)
{
    v4 p1 = v4(s1_x, s1_y, 1, 0);

    // p2 should lie on this line
    v4 l1 = p1*transpose(F);
    if(debug_track)
        v4_pp("L1", l1);

    // ground truth sanity check
    // Normalize the line equation so that distances can be computed
    // with the dot product
    //l1 = l1 / sqrt(l1[0]*l1[0] + l1[1]*l1[1]);
    //float d = sum(l1*p2);
    //fprintf(stderr, "distance p1 %f\n", d);

    float endpoints[4];
    if(line_endpoints(l1, s1->width, s1->height, endpoints)) {
        if(debug_track)
            fprintf(stderr, "line endpoints %f %f %f %f\n", endpoints[0], endpoints[1], endpoints[2], endpoints[3]);

        float score = track_line(s1->frame, s2->frame, s1->width, s1->height, p1[0], p1[1],
                                 endpoints[0], endpoints[1], endpoints[2], endpoints[3],
                                 s2_x, s2_y);

        if(debug_track) {
            uint8_t * copy = (uint8_t *)malloc(sizeof(uint8_t) * s2->width * s2->height);
            memcpy(copy, s2->frame, sizeof(uint8_t) * s2->width * s2->height);
            draw_line(copy, s2->width, s2->height, endpoints[0], endpoints[1], endpoints[2], endpoints[3]);
            write_image("epipolar_line.pgm", copy, s2->width, s2->height);
            free(copy);
        }
    }
    else
        fprintf(stderr, "failed to get line endpoints\n");

    return true;
}

m4 eight_point_F(v4 p1[], v4 p2[], int npts)
{
    // normalize points
    v4 center1 = v4(0., 0., 0., 0.);
    v4 center2 = v4(0., 0., 0., 0.);
    float norm1 = 0, norm2 = 0;
    for(int i = 0; i < npts; i++) {
        center1 += p1[i];
        center2 += p2[i];
    }
    center1 /= npts;
    center2 /= npts;
    for(int i = 0; i < npts; i++) {
        norm1 += sum((p1[i] - center1)*(p1[i] - center1));
        norm2 += sum((p2[i] - center1)*(p2[i] - center2));
    }
    norm1 = sqrt(2.*npts / norm1);
    norm2 = sqrt(2.*npts / norm2);

    MAT_TEMP(T1, 3, 3);
    MAT_TEMP(T2, 3, 3);
    T1(0, 0) = norm1; T1(0, 1) = 0;     T1(0, 2) = -norm1*center1[0];
    T1(1, 0) = 0;     T1(1, 1) = norm1; T1(1, 2) = -norm1*center1[1];
    T1(2, 0) = 0;     T1(2, 1) = 0;     T1(2, 2) = 1;


    T2(0, 0) = norm2; T2(0, 1) = 0;     T2(0, 2) = -norm2*center2[0];
    T2(1, 0) = 0;     T2(1, 1) = norm2; T2(1, 2) = -norm2*center2[1];
    T2(2, 0) = 0;     T2(2, 1) = 0;     T2(2, 2) = 1;

    MAT_TEMP(constraints, npts, 9);
    for(int i = 0; i < npts; i++) {
        v4 p1n = (p1[i] - center1) * norm1;
        v4 p2n = (p2[i] - center2) * norm2;
        constraints(i, 0) = p1n[0]*p2n[0];
        constraints(i, 1) = p1n[1]*p2n[0];
        constraints(i, 2) = p2n[0];
        constraints(i, 3) = p1n[0]*p2n[1];
        constraints(i, 4) = p1n[1]*p2n[1];
        constraints(i, 5) = p2n[1];
        constraints(i, 6) = p1n[0];
        constraints(i, 7) = p1n[1];
        constraints(i, 8) = 1;
    }


    // some columns of U are inverted compared to matlab
    MAT_TEMP(U, npts, npts); 
    MAT_TEMP(S, 1, min(npts, 9));
    MAT_TEMP(Vt, 9, 9);

    matrix_svd(constraints, U, S, Vt);

    MAT_TEMP(F, 3, 3);
    F(0, 0) = Vt(8, 0); F(0, 1) = Vt(8, 1); F(0, 2) = Vt(8, 2);
    F(1, 0) = Vt(8, 3); F(1, 1) = Vt(8, 4); F(1, 2) = Vt(8, 5);
    F(2, 0) = Vt(8, 6); F(2, 1) = Vt(8, 7); F(2, 2) = Vt(8, 8);
    m4 initialF;
    for(int i = 0; i < 3; i++)
      for(int j = 0; j < 3; j++)
        initialF[i][j] = F(i,j);

    MAT_TEMP(UF, 3, 3);
    MAT_TEMP(SF, 1, 3);
    MAT_TEMP(VFt, 3, 3);
    matrix_svd(F, UF, SF, VFt);
    MAT_TEMP(SE, 3, 3);
    // Convert S back to a diagonal matrix (matrix_svd fills a vector)
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
        {
            if(i == j)
                SE(i,j) = SF(0,i);
            else
                SE(i,j) = 0;
        }
    SE(2,2) = 0;
    
    // TODO: there is probably a cleaner way to do this:
    // F = transpose(T2) * U * S * Vt * T1;
    MAT_TEMP(temp1, 3, 3);
    MAT_TEMP(temp2, 3, 3);
    matrix_product(temp1, VFt, T1);
    matrix_product(temp2, SE, temp1); 
    matrix_product(temp1, UF, temp2); 
    matrix_product(temp2, T2, temp1, true); // transpose T2
    F = temp2;

    // F = F / norm(F) / sign(F(3,3))
    float Fnorm = 0;
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            Fnorm += F(i,j)*F(i,j);
    Fnorm = sqrt(Fnorm);

    if(F(2,2) < 0)
      Fnorm = -Fnorm;

    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 3; j++)
            F(i,j) = F(i,j) / Fnorm;

    m4 estimatedF;
    for(int i = 0; i < 3; i++)
      for(int j = 0; j < 3; j++)
        estimatedF[i][j] = F(i,j);
    estimatedF[3][3] = 1;

    return estimatedF;
}

void print_stereo_state(stereo_state * s)
{
    fprintf(stderr, "S: %p\n", s);
    fprintf(stderr, "S.features %lu\n", s->features.size());
    //for(list<state_vision_feature>::iterator siter = s->features.begin(); siter != s->features.end(); ++siter) {
    //    state_vision_feature f = *siter;
    //    fprintf(stderr, "%llu: %f %f\n", f.id, f.current[0], f.current[1]);
    //}
    fprintf(stderr, "frame no %d pointer to frame is %p\n", s->frame_number, s->frame);
}

m4 estimate_F_eight_point(stereo_state * s1, stereo_state * s2)
{
    vector<v4> p1;
    vector<v4> p2;

    // This assumes s1->features and s2->features are sorted by id
    for(list<state_vision_feature>::iterator s1iter = s1->features.begin(); s1iter != s1->features.end(); ++s1iter) {
        state_vision_feature f1 = *s1iter;
        for(list<state_vision_feature>::iterator s2iter = s2->features.begin(); s2iter != s2->features.end(); ++s2iter) {
            state_vision_feature f2 = *s2iter;
            if(f1.id == f2.id) {
                p1.push_back(f1.current);
                p2.push_back(f2.current);
            }
        }
    }
    fprintf(stderr, "%lu features are definitely overlapping\n", p1.size());

    m4 F;
    if(p1.size() < 8) {
        fprintf(stderr, "ERROR: Not enough overlapping features to use 8 point\n");
        return F;
    }

    F = eight_point_F(&p1[0], &p2[0], p1.size());

    return F;
}

// Triangulates a point in the world reference frame from two views
v4 triangulate_point(stereo_state * s1, stereo_state * s2, int s1_x, int s1_y, int s2_x, int s2_y)
{
    // s1->T is the translation from the origin to camera frame
    // s1->W is the rotation from the origin to camera frame
    v4 intersection;
    v4 o1_transformed, o2_transformed;
    v4 pa, pb;
    float error;
    bool success;

    v4 p1_calibrated = project_point(s1_x, s1_y, s2->center_x, s2->center_y, s2->focal_length);
    if(debug_triangulate)
        v4_pp("p1_projected", p1_calibrated);
    p1_calibrated = calibrate_im_point(p1_calibrated, s2->k1, s2->k2, s2->k3);
    if(debug_triangulate)
        v4_pp("p1_calibrated", p1_calibrated);
    v4 p2_calibrated = project_point(s2_x, s2_y, s2->center_x, s2->center_y, s2->focal_length);
    if(debug_triangulate)
        v4_pp("p2_projected", p2_calibrated);
    p2_calibrated = calibrate_im_point(p2_calibrated, s2->k1, s2->k2, s2->k3);
    if(debug_triangulate)
        v4_pp("p2_calibrated", p2_calibrated);
    m4 R1w = rodrigues(s1->W, NULL);
    m4 Rbc1 = rodrigues(s2->Wc, NULL);
    m4 R2w = rodrigues(s2->W, NULL);
    m4 Rbc2 = rodrigues(s2->Wc, NULL);

    v4 p1_cal_transformed = R1w*Rbc1*p1_calibrated + R1w * s2->Tc + s1->T;
    v4 p2_cal_transformed = R2w*Rbc2*p2_calibrated + R2w * s2->Tc + s2->T;
    o1_transformed = s1->T;
    o2_transformed = s2->T;
    if(debug_triangulate) {
        v4_pp("o1", o1_transformed);
        v4_pp("o2", o2_transformed);
    }

    success = line_line_intersect(o1_transformed, p1_cal_transformed, o2_transformed, p2_cal_transformed, pa, pb);
    if(!success) {
        fprintf(stderr, "Failed intersect\n");
        return v4(0,0,0,0);
    }
    // pa is the point on the first line closest to the intersection
    // pb is the point on the second line closest to the intersection
    //v4_pp("pa", pa);
    //v4_pp("pb", pb);

    error = norm(pa - pb);
    v4 cam1_intersect = transpose(R1w * Rbc1) * (pa - s2->Tc - s1->T);
    fprintf(stderr, "Lines were %.2fcm from intersecting at a depth of %.2fcm\n", error*100, cam1_intersect[2]*100);

    if(cam1_intersect[2] < 0) {
        fprintf(stderr, "Lines intersected at a negative camera depth, failing\n");
        return v4(0,0,0,0);
    }

    if(error/cam1_intersect[2] > .1) {
        fprintf(stderr, "Error too large, failing\n");
        return v4(0,0,0,0);
    }
    intersection = pa + (pb - pa)/2;
    if(debug_triangulate)
        v4_pp("p1 midpoint", intersection);

    return intersection;
}

m4 stereo_preprocess(stereo_state * s1, stereo_state * s2)
{
    // estimate_F uses R & T directly, does a bad job if motion
    // estimate is poor
    //m4 F = estimate_F(s2, s1);
    //m4_pp("F12", F12);

    // This uses common tracked features between s1 and s2 to
    // bootstrap a F matrix
    // F is from s2 to s1
    m4 Fe = estimate_F_eight_point(s2, s1);
    if(debug_F)
        m4_pp("Fe", Fe);
    return Fe;
}

v4 stereo_triangulate(stereo_state * s1, stereo_state *s2, m4 F, int s2_x1, int s2_y1)
{
    int s1_x1, s1_y1;
    v4 result = v4(0,0,0,0);
    // sets s1_x1,s1_y1 and s1_x2,s1_y2
    bool success = find_correspondence(s2, s1, F, s2_x1, s2_y1, &s1_x1, &s1_y1);
    if(success) {
        v4 intersection = triangulate_point(s1, s2, s1_x1, s1_y1, s2_x1, s2_y1);
        return intersection;
    }

    return result;
}

bool compare_id(state_vision_feature f1, state_vision_feature f2)
{
    return f1.id < f2.id;
}


int intersection_length(list<state_vision_feature> l1, list<state_vision_feature> l2)
{
    l1.sort(compare_id);
    l2.sort(compare_id);
    vector<int> intersection;
    intersection.resize(max(l1.size(),l2.size()));
    vector<int> id1;
    vector<int> id2;
    vector<int>::iterator it;
    
    for(list<state_vision_feature>::iterator fiter = l1.begin(); fiter != l1.end(); ++fiter) {
        id1.push_back((*fiter).id);
    }
    for(list<state_vision_feature>::iterator fiter = l2.begin(); fiter != l2.end(); ++fiter) {
        id2.push_back((*fiter).id);
    }

    it = std::set_intersection(id1.begin(), id1.end(), id2.begin(), id2.end(), intersection.begin());
    int len = (it - intersection.begin());

    return len;
}

bool stereo_should_save_state(struct filter * f, stereo_state s)
{
    list<state_vision_feature> copied_features;
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        copied_features.push_back(state_vision_feature(**fiter));
    }

    int len = intersection_length(s.features, copied_features);
    return len < 15;
}

stereo_state stereo_save_state(struct filter * f, uint8_t * frame)
{
    stereo_state s;
    s.width = f->track.width;
    s.height = f->track.height;
    s.frame_number = f->image_packets;
    s.frame = (uint8_t *)malloc(s.width*s.height*sizeof(uint8_t));
    memcpy(s.frame, frame, s.width*s.height*sizeof(uint8_t)); // TODO copy this?

    fprintf(stderr, "Stereo save state with %lu features\n", f->s.features.size());

    s.T = f->s.T;
    s.W = f->s.W;
    s.Wc = f->s.Wc;
    s.Tc = f->s.Tc;
    s.focal_length = f->s.focal_length;
    s.center_x = f->s.center_x;
    s.center_y = f->s.center_y;
    s.k1 = f->s.k1;
    s.k2 = f->s.k2;
    s.k3 = f->s.k3;

    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        s.features.push_back(state_vision_feature(**fiter));
    }

    if(debug_state) {
        char filename[80];
        sprintf(filename, "output_%d.pgm", s.frame_number);

        fprintf(stderr, "save frame %d\n", s.frame_number);
        write_image(filename, s.frame, s.width, s.height);

        sprintf(filename, "features_%d.txt", s.frame_number);
        FILE * fp = fopen(filename, "w");
        fprintf(fp, "id\tx\ty\n");
        for(list<state_vision_feature>::iterator fiter = s.features.begin(); fiter != s.features.end(); ++fiter) {
          fprintf(fp, "%llu\t%f\t%f\n", (*fiter).id, (*fiter).current[0], (*fiter).current[1]);
        }
        fclose(fp);
    }

    return s;
}

void stereo_free_state(stereo_state s)
{
    if(s.frame) {
        free(s.frame);
        s.frame = NULL;
    }

    s.features.clear();
}
