#include "stereo_features.h"

vector<sift_match> ubc_match(vector<sift_keypoint> k1, vector<sift_keypoint>k2)
{
    double thresh = 1.5;

    vector<sift_match> matches;
    for(unsigned int i1 = 0; i1 < k1.size(); i1++) {
        sift_match match;
        float best_score = 1e10;
        float second_best_score = 1e10;
        int bestk = -1;
        for(unsigned int i2 = 0; i2 < k2.size(); i2++) {
            double acc = 0;
            for(int bin = 0; bin < 128; bin++)
            {
                float delta = k1[i1].d[bin] - k2[i2].d[bin];
                acc += delta*delta;
            }

            if(acc < best_score) {
                second_best_score = best_score;
                best_score = acc;
                bestk = i2;
            } else if(acc < second_best_score) {
                second_best_score = acc;
            }
        }
        /* Lowe's method: accept the match only if unique. */
        if(second_best_score == 1e10) continue;

        if(thresh * (float) best_score < (float) second_best_score && bestk != -1) {
            match.i1 = i1;
            match.i2 = bestk;
            match.score = best_score;
            matches.push_back(match);
        }
    }
    return matches;
}

vector<sift_keypoint> sift_detect(uint8_t * image, int width, int height, int noctaves, int nlevels, int o_min)
{
    vector<sift_keypoint> keypoints;
    vl_sift_pix * fdata = (vl_sift_pix *)malloc(width*height*sizeof(vl_sift_pix));

    /* convert data type */
    for (int q = 0; q < width * height; ++q) {
        fdata[q] = image[q];
    }

    VlSiftFilt * filt = vl_sift_new(width, height, noctaves, nlevels, o_min);
    /* ...............................................................
     *                                             Process each octave
     * ............................................................ */
    int i     = 0 ;
    int first = 1 ;
    int err;
    while (1) {
        VlSiftKeypoint const *keys = 0 ;
        int                   nkeys ;

        /* calculate the GSS for the next octave .................... */
        if (first) {
            first = 0 ;
            err = vl_sift_process_first_octave (filt, fdata) ;
        } else {
            err = vl_sift_process_next_octave  (filt) ;
        }

        if (err) {
            err = VL_ERR_OK ;
            break ;
        }

        /* run detector ............................................. */
        vl_sift_detect (filt) ;

        keys  = vl_sift_get_keypoints     (filt) ;
        nkeys = vl_sift_get_nkeypoints (filt) ;
        i     = 0 ;

        /* for each keypoint ........................................ */
        for (; i < nkeys ; ++i) {
            double                angles [4] ;
            int                   nangles ;
            VlSiftKeypoint const *k ;

            /* obtain keypoint orientations ........................... */
            k = keys + i ;
            nangles = vl_sift_calc_keypoint_orientations(filt, angles, k) ;

            /* for each orientation ................................... */
            for (int q = 0 ; q < (unsigned) nangles ; ++q) {
                sift_keypoint descr;

                /* compute descriptor (if necessary) */
                vl_sift_calc_keypoint_descriptor(filt, descr.d, k, angles [q]) ;
                descr.pt.x = k->x;
                descr.pt.y = k->y;
                descr.sigma = k->sigma;
                /* possibly should convert to uint8 for matching later */
                /*
                 int l ;
                 for (l = 0 ; l < 128 ; ++l) {
                double x = 512.0 * descr[l] ;
                x = (x < 255.0) ? x : 255.0 ;
                vl_file_meta_put_uint8 (&dsc, (vl_uint8) (x)) ;
                 }
                 */
                keypoints.push_back(descr);
            }
        }
    }
    free(fdata);
    return keypoints;
}
