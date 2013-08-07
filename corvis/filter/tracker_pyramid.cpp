#include "tracker_pyramid.h"

//#include <opencv2/core/core_c.h>
#include <opencv/cv.h>

struct cv_pyramid_tracker {
    int width, height, stride;
    int spacing, levels;
    CvSize optical_flow_window;
    CvTermCriteria optical_flow_termination_criteria;

    //state
    feature_vector_t features;
    uint8_t * last_im1;

    feature_t *calibrated;

    //storage
    IplImage *header1, *header2;
    IplImage *eig_image, *temp_image;
    IplImage *mask;
    CvMat *pyramid1, *pyramid2;
} t;

void tracker_pyramid_destroy()
{
    return;
    cvReleaseImageHeader(&t.header1);
    cvReleaseImageHeader(&t.header2);
    cvReleaseImage(&t.mask);
    cvReleaseImage(&t.eig_image);
    cvReleaseImage(&t.temp_image);
    cvReleaseMat(&t.pyramid1);
    cvReleaseMat(&t.pyramid2);
}

void tracker_pyramid_init(int width, int height, int stride)
{
    CvSize size = cvSize(width, height);
    t.width = width;
    t.height = height;
    t.stride = stride;
    t.header1 = cvCreateImageHeader(size, IPL_DEPTH_8U, 1);
    t.header2 = cvCreateImageHeader(size, IPL_DEPTH_8U, 1);
    t.optical_flow_window = cvSize(10,10);
    int max_iter = 100;
    double epsilon = 1;
    /* TODO: what are reasonable values for max_iter and epsilon */
    t.optical_flow_termination_criteria = cvTermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, max_iter, epsilon);
    t.levels = 2;

    
    t.mask = cvCreateImage(size, IPL_DEPTH_8U, 1);
    //turn all selections off by default
    cvSet(t.mask, cvScalarAll(0), NULL);
    
    t.eig_image = cvCreateImage(size, IPL_DEPTH_32F, 1);
    t.temp_image = cvCreateImage(size, IPL_DEPTH_32F, 1);

    int pyrsize = (t.width+8)*t.height/3;
    t.pyramid1 = cvCreateMat(pyrsize, 1, CV_8UC1);
    
    t.pyramid2 = cvCreateMat(pyrsize, 1, CV_8UC1);
}

void tracker_pyramid_precalculate(uint8_t * im1, uint8_t * im2)
{
    cvSetData(t.header1, im1, t.stride);
    cvSetData(t.header2, im2, t.stride);


    cvCalcOpticalFlowPyrLK(t.header1, t.header2, 
                       t.pyramid1, t.pyramid2, 
                       NULL, NULL, 0,
                       t.optical_flow_window, t.levels, 
                       NULL, 
                       NULL, t.optical_flow_termination_criteria, 
                       0);
}

feature_t tracker_pyramid_track(uint8_t * im1, uint8_t * im2, int currentx, int currenty, int x1, int y1, int x2, int y2)
{
    if (t.last_im1 != im1) {
        tracker_pyramid_precalculate(im1, im2);
        t.last_im1 = im1;
    }

    int nfeats = 1;
    float errors[nfeats];
    char found_features[nfeats];
    feature_t current, next;
    current.x = next.x = currentx;
    current.y = next.y = currenty;

    fprintf(stderr, "Current: %f %f\n", current.x, current.y);
    fprintf(stderr, "Next: %f %f\n", next.x, next.y);

    //track
    cvCalcOpticalFlowPyrLK(t.header1, t.header2, 
                           t.pyramid1, t.pyramid2, 
                           (CvPoint2D32f *)&current, (CvPoint2D32f *)&next, 1,
                           t.optical_flow_window, t.levels, 
                           found_features, 
                           errors, t.optical_flow_termination_criteria, 
                           (CV_LKFLOW_PYR_A_READY | CV_LKFLOW_PYR_B_READY | CV_LKFLOW_INITIAL_GUESSES ));

    fprintf(stderr, "Found:\n");
    for(int i = 0; i < nfeats; i++) 
        fprintf(stderr, "found %d error %f next %f %f\n", found_features[i], errors[i], next.x, next.y);
   
    return next; 
}

