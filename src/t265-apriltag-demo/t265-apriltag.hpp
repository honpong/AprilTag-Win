#include "apriltag.h"
#include "apriltag_pose.h"
#include "tag36h11.h"
//#include "tag36h10.h"
//#include "tag36artoolkit.h"
#include "tag25h9.h"
//#include "tag25h7.h"
#include "common/getopt.h"
#include <string.h>
#include "librealsense2/rsutil.h"

struct apriltag_detection_undistorted_t : public apriltag_detection_t
{
    apriltag_detection_undistorted_t() : apriltag_detection_t({}) {}
    
    virtual ~apriltag_detection_undistorted_t()
    {
        matd_destroy(this->H);
    }
    
    static void deproject(double pt[2], const rs2_intrinsics& intr, const double px[2])
    {
        float fpt[3], fpx[2] = { (float)px[0], (float)px[1] };
        rs2_deproject_pixel_to_point(fpt, &intr, fpx, 1.0f);
        pt[0] = fpt[0];
        pt[1] = fpt[1];
    }
    
    apriltag_detection_undistorted_t& undistort(const apriltag_detection_t& src, const rs2_intrinsics& intr)
    {
        *(apriltag_detection_t*)this = src;
        deproject(this->c, intr, src.c);
        
        double corr_arr[4][4];
        for(int c=0; c<4; ++c){
            deproject(this->p[c], intr, src.p[c]);
            
            corr_arr[c][0] = (c==0 || c==3) ? -1 : 1;
            corr_arr[c][1] = (c==0 || c==1) ? -1 : 1;
            corr_arr[c][2] = this->p[c][0];
            corr_arr[c][3] = this->p[c][1];
        }
        
        matd_destroy(this->H);
        this->H = homography_compute2(corr_arr);
        return *this;
    }
};

struct apriltag_detection_array
{
    zarray_t *detections = nullptr;
    std::vector<apriltag_detection_undistorted_t> detections_undistorted;
    std::vector<apriltag_pose_t> tag_poses;
    rs2_intrinsics intr;
    
    apriltag_detection_array(apriltag_detector_t* td, cv::Mat& gray, rs2_intrinsics& _intr)
    {
        intr = _intr;
        
        // Make an image_u8_t header for the Mat data
#ifdef _MSC_VER
        image_u8_t im{ gray.cols, gray.rows, gray.cols, gray.data };
#else
        image_u8_t im = { .width = gray.cols,
            .height = gray.rows,
            .stride = gray.cols,
            .buf = gray.data
        };
#endif
        
        detections = apriltag_detector_detect(td, &im);
        undistort_all();
        estimate_tag_poses();
    }
    
    virtual ~apriltag_detection_array()
    {
        zarray_destroy(detections);
    }
    
    int num_detections() const { return zarray_size(detections); }
    
    apriltag_detection_t* get_detection(int i) const {
        apriltag_detection_t *det;
        zarray_get(detections, i, &det);
        return det;
    }
    
    void draw_detections(cv::Mat& frame) const
    {
        int num_tag_detected = num_detections();
        // Draw detection outlines
        for (int i = 0; i < num_tag_detected; i++) {
            apriltag_detection_t *det = get_detection(i);
            
            cv::line(frame, cv::Point2d(det->p[0][0], det->p[0][1]),
                     cv::Point2d(det->p[1][0], det->p[1][1]),
                     cv::Scalar(0, 0xff, 0), 2);
            cv::line(frame, cv::Point2d(det->p[0][0], det->p[0][1]),
                     cv::Point2d(det->p[3][0], det->p[3][1]),
                     cv::Scalar(0, 0, 0xff), 2);
            cv::line(frame, cv::Point2d(det->p[1][0], det->p[1][1]),
                     cv::Point2d(det->p[2][0], det->p[2][1]),
                     cv::Scalar(0xff, 0, 0), 2);
            cv::line(frame, cv::Point2d(det->p[2][0], det->p[2][1]),
                     cv::Point2d(det->p[3][0], det->p[3][1]),
                     cv::Scalar(0xff, 0, 0), 2);
            
            std::stringstream ss;
            ss << det->id;
            cv::String text = ss.str();
            int fontface = cv::FONT_HERSHEY_SCRIPT_SIMPLEX;
            double fontscale = 1.0;
            int baseline;
            cv::Size textsize = cv::getTextSize(text, fontface, fontscale, 2,
                                                &baseline);
            cv::putText(frame, text, cv::Point2d(det->c[0] - textsize.width / 2,
                                                 det->c[1] + textsize.height / 2),
                        fontface, fontscale, cv::Scalar(0xff, 0x99, 0), 2);
            
            std::stringstream pss; pss << "t:";
            pss << matd_get(tag_poses[i].t,0,0) <<
            "," << matd_get(tag_poses[i].t,1,0) <<
            "," << matd_get(tag_poses[i].t,2,0);
            
            cv::putText(frame, pss.str(), cv::Point(30,30), fontface, fontscale, cv::Scalar(255,255,0),1);
        }
    }
    
    void undistort_all()
    {
        const int num_tag_detected = num_detections();
        detections_undistorted.resize(num_tag_detected);
        for(int i=0; i < num_tag_detected; ++i){
            detections_undistorted[i].undistort(*get_detection(i), intr);
        }
    }
    
    void estimate_tag_poses(const double tag_size = 0.05)
    {
        const int num_tag_detected = num_detections();
        tag_poses.resize(num_tag_detected);
        
        // First create an apriltag_detection_info_t struct using your known parameters.
        apriltag_detection_info_t info;
        info.tagsize = tag_size;
        info.fx = intr.fx;
        info.fy = intr.fy;
        info.cx = intr.ppx;
        info.cy = intr.ppy;
        
        for(int i=0; i < num_tag_detected; ++i){
            info.det = get_detection(i);
            
            // Then call estimate_tag_pose.
            double err = estimate_tag_pose(&info, &tag_poses[i]);
        }
    }
};

struct apriltag
{
    apriltag() { init(); }
    ~apriltag() { destory(); }
    
    getopt_t *getopt = nullptr;
    const char* famname = nullptr;
    apriltag_family_t *tf = nullptr;
    apriltag_detector_t *td = nullptr;
    double tag_size = 0.05; //meter
    double fx, fy, ppx, ppy;
    
    void destory()
    {
        apriltag_detector_destroy(td);
        if (!strcmp(famname, "tag36h11"))
            tag36h11_destroy(tf);
        //else if (!strcmp(famname, "tag36h10"))
        //    tag36h10_destroy(tf);
        //else if (!strcmp(famname, "tag36artoolkit"))
        //    tag36artoolkit_destroy(tf);
        else if (!strcmp(famname, "tag25h9"))
            tag25h9_destroy(tf);
        //else if (!strcmp(famname, "tag25h7"))
        //    tag25h7_destroy(tf);
        getopt_destroy(getopt);
        
        getopt = nullptr;
        famname = nullptr;
        tf = nullptr;
        td = nullptr;
    }
    
    void init()
    {
        getopt = getopt_create();
        
        getopt_add_bool(getopt, 'h', "help", 0, "Show this help");
        getopt_add_bool(getopt, 'd', "debug", 0, "Enable debugging output (slow)");
        getopt_add_bool(getopt, 'q', "quiet", 0, "Reduce output");
        getopt_add_string(getopt, 'f', "family", "tag36h11", "Tag family to use");
        getopt_add_int(getopt, '\0', "border", "1", "Set tag family border size");
        getopt_add_int(getopt, 't', "threads", "4", "Use this many CPU threads");
        getopt_add_double(getopt, 'x', "decimate", "1.0", "Decimate input image by this factor");
        getopt_add_double(getopt, 'b', "blur", "0.0", "Apply low-pass blur to input");
        getopt_add_bool(getopt, '0', "refine-edges", 1, "Spend more time trying to align edges of tags");
        getopt_add_bool(getopt, '1', "refine-decode", 0, "Spend more time trying to decode tags");
        getopt_add_bool(getopt, '2', "refine-pose", 0, "Spend more time trying to precisely localize tags");
        
        // Initialize tag detector with options
        famname = getopt_get_string(getopt, "family");
        if (!strcmp(famname, "tag36h11"))
            tf = tag36h11_create();
        //else if (!strcmp(famname, "tag36h10"))
        //    tf = tag36h10_create();
        //else if (!strcmp(famname, "tag36artoolkit"))
        //    tf = tag36artoolkit_create();
        else if (!strcmp(famname, "tag25h9"))
            tf = tag25h9_create();
        //else if (!strcmp(famname, "tag25h7"))
        //    tf = tag25h7_create();
        else {
            printf("Unrecognized tag family name. Use e.g. \"tag36h11\".\n");
            exit(-1);
        }
        //tf->black_border = getopt_get_int(getopt, "border");
        
        td = apriltag_detector_create();
        apriltag_detector_add_family(td, tf);
        td->quad_decimate = (float)getopt_get_double(getopt, "decimate");
        td->quad_sigma = (float)getopt_get_double(getopt, "blur");
        td->nthreads = getopt_get_int(getopt, "threads");
        td->debug = getopt_get_bool(getopt, "debug");
        td->refine_edges = getopt_get_bool(getopt, "refine-edges");
        //td->refine_decode = getopt_get_bool(getopt, "refine-decode");
        //td->refine_pose = getopt_get_bool(getopt, "refine-pose");
    }
    
    std::shared_ptr<apriltag_detection_array> detect(cv::Mat& gray, rs2_intrinsics& intr)
    {
        return std::make_shared<apriltag_detection_array>(td, gray, intr);
    }
    
    void detect_pose(apriltag_detection_t* det)
    {
        // First create an apriltag_detection_info_t struct using your known parameters.
        apriltag_detection_info_t info;
        info.det = det;
        info.tagsize = tag_size;
        info.fx = fx;
        info.fy = fy;
        info.cx = ppx;
        info.cy = ppy;
        
        // Then call estimate_tag_pose.
        apriltag_pose_t pose;
        double err = estimate_tag_pose(&info, &pose);
    }
    
    cv::Mat map_x, map_y;
    cv::Point2f undistort(float mx, float my) const
    {
        float normalized_ux = 0.25f * (mx - map_x.at<float>(cv::Point(map_x.cols/2,map_x.rows/2))) + 0.5f;
        float normalized_uy = 0.25f * (my - map_y.at<float>(cv::Point(map_y.cols/2,map_y.rows/2))) + 0.5f;
        return cv::Point2f(normalized_ux, normalized_uy);
    }
    
    cv::Point2f undistort(int x, int y) const {
        return undistort(map_x.at<float>(cv::Point(x,y)), map_y.at<float>(cv::Point(x,y)));
    }
    
    cv::Mat undistort(cv::Mat& fisheye_frame, rs2_intrinsics& intr)
    {
        float pt[3], px[2];
        
        if(map_x.empty()||map_y.empty())
        {
            map_x.create(intr.height, intr.width, CV_32F);
            map_y.create(intr.height, intr.width, CV_32F);
            for(int y=0; y<intr.height; ++y){
                for(int x=0; x<intr.width; ++x)
                {
                    px[0] = x, px[1] = y;
                    rs2_deproject_pixel_to_point(pt, &intr, px, 1);
                    map_x.at<float>(cv::Point(x,y)) = pt[0];
                    map_y.at<float>(cv::Point(x,y)) = pt[1];
                }
            }
            
            //cv::minMaxLoc(map_x, &min_ux, &max_ux);
            //cv::minMaxLoc(map_y, &min_uy, &max_uy);
            //range_ux = std::ceil(max_ux - min_ux);
            //range_uy = std::ceil(max_uy - min_uy);
        }
        
        int w = map_x.cols, h = map_x.rows;
        const int uw = w, uh = h;
        cv::Mat display(uh, uw, fisheye_frame.type());
        display.setTo(0);
        
        for(int y=h/10, ny=h*9/10; y<ny; ++y){
            for(int x=w/10, nx=w*9/10; x<nx; ++x){
                auto u = undistort(x,y);
                float ux = u.x * uw;
                float uy = u.y * uh;
                if(std::isnan(ux) || std::isnan(uy)){ continue; }
                
                if(fov(ux,uy,uw,uh)){
                    auto& src_color = fisheye_frame.at<unsigned char>(cv::Point(x,y));
                    auto& dst_color = display.at<unsigned char>(cv::Point(ux,uy));
                    if(src_color>dst_color){ dst_color = src_color; }
                }
            }
        }
        
        for(int y=h/10, ny=h*9/10; y<ny; ++y){
            for(int x=w/10, nx=w*9/10; x<nx; ++x){
                auto u = undistort(x,y);
                float ux = u.x * uw;
                float uy = u.y * uh;
                if(std::isnan(ux) || std::isnan(uy)){ continue; }
                
                for(int yy=-1;yy<=1; ++yy){
                    for(int xx=-1; xx<=1; ++xx){
                        float pu[2] = {ux+xx, uy+yy};
                        if(fov(pu[0],pu[1],uw,uh)){
                            auto& src_color = fisheye_frame.at<unsigned char>(cv::Point(x,y));
                            auto& dst_color = display.at<unsigned char>(cv::Point(pu[0],pu[1]));
                            if(src_color>dst_color){ dst_color = src_color; }
                        }
                    }
                }
            }
        }
        //cv::resize(display,display,cv::Size(),2,2);
        return display;
    }
    
    static bool fov(int x, int y, int w, int h) {
        return 0 <= x && x < w && 0 <= y && y < h;
    }
    

};
