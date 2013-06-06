/*
void run_tracking(struct filter *f, feature_t *trackedfeats)
{
    struct tracker *t = f->track;
    //feature_t *trackedfeats[f->s.features.size()];
    //are we tracking anything?
    int newindex = 0;
    if(f->s.features.size()) {
        feature_t feats[f->s.features.size()];
        int nfeats = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            feats[nfeats].x = (*fiter)->current[0];
            feats[nfeats].y = (*fiter)->current[1];
            ++nfeats;
        }
        float errors[nfeats];
        char found_features[nfeats];
        
        //track
        cvCalcOpticalFlowPyrLK(t->header1, t->header2, 
                               t->pyramid1, t->pyramid2, 
                               (CvPoint2D32f *)feats, (CvPoint2D32f *)trackedfeats, f->s.features.size(),
                               t->optical_flow_window, t->levels, 
                               found_features, 
                               errors, t->optical_flow_termination_criteria, 
                               (t->pyramidgood?CV_LKFLOW_PYR_A_READY:0) | CV_LKFLOW_INITIAL_GUESSES );
        t->pyramidgood = 1;

        int area = (t->spacing * 2 + 3);
        area = area * area;

        int i = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            if(found_features[i] &&
               trackedfeats[i].x > 0.0 &&
               trackedfeats[i].y > 0.0 &&
               trackedfeats[i].x < t->width-1 &&
               trackedfeats[i].y < t->height-1 &&
               errors[i] / area < t->max_tracking_error) {
                (*fiter)->current[0] = (*fiter)->uncalibrated[0] = trackedfeats[i].x;
                (*fiter)->current[1] = (*fiter)->uncalibrated[2] = trackedfeats[i].y;
            } else {
                trackedfeats[i].x = trackedfeats[i].y = INFINITY;
                (*fiter)->current[0] = (*fiter)->uncalibrated[0] = trackedfeats[i].x;
                (*fiter)->current[1] = (*fiter)->uncalibrated[2] = trackedfeats[i].y;
            }
            ++i;            
        }
    }
}
*/

 /*
void run_local_tracking(struct filter *f, feature_t *trackedfeats)
{
    struct tracker *t = f->track;
    int newindex = 0;
    if(f->s.features.size()) {
        int b = 20;
        int xsize = t->width;
        int ysize = t->height;
        int stride = t->width;
        fast_detector detector(xsize, ysize, stride);
        int index = 0;
        for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
            state_vision_feature *i = *fiter;
            xy bestkp = detector.track(t->im1, t->im2, (*fiter)->current[0], (*fiter)->current[1], 15, 15, 20);

            if(bestkp.x < 0.0 ||
               bestkp.y < 0.0 ||
               bestkp.x >= t->width ||
               bestkp.y >= t->height) {
                bestkp.x = bestkp.y = INFINITY;
            }

            (*fiter)->current[0] = (*fiter)->uncalibrated[0] = trackedfeats[index].x = bestkp.x;
            (*fiter)->current[1] = (*fiter)->uncalibrated[2] = trackedfeats[index].y = bestkp.y;
            ++index;
        }
    }
}
 */
