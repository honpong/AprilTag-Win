//
//  rs_sf_demo_t265.cpp
//  t265-demo
//
//  Created by Hon Pong (Gary) Ho on 2/28/19.
//

//#include "rs_shapefit.h"
//#include "rs_sf_camera.hpp"
//#include "rs_sf_gl_context.hpp"
//#include "rs_sf_pose_tracker.h"
//#include "d435i_default_json.h"
#include "rs_sf_demo_t265.hpp"

#if defined(WIN32) | defined(WIN64) | defined(_WIN32) | defined(_WIN64)
#define PATH_SEPARATER '\\'
#define DEFAULT_PATH ".\\capture\\" //"C:\\temp\\t265-capture\\"
#define DEFAULT_SCRIPT "t265-insight.bat"
//#define BIN_COMMAND ("START \"SCRIPT\" /WAIT /B " + g_script_name + " " + folder_path).c_str()
//#define SCRIPT_COMMAND ("START \"SCRIPT\" /WAIT /B " + g_script_name + " " + folder_path + " py").c_str()
#define BIN_COMMAND ("CALL " + g_script_name + " " + folder_path).c_str()
#define SCRIPT_COMMAND ("CALL " + g_script_name + " " + folder_path + " py").c_str()

#else
#define PATH_SEPARATER '/'
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/temp/shapefit/1/")
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/Desktop/temp/data/")
#define DEFAULT_PATH "./capture/"
#define DEFAULT_SCRIPT "t265-insight.sh"
#define SCRIPT_COMMAND ("chmod +x " + g_script_name + ";./" + g_script_name + " " + folder_path).c_str()
#define BIN_COMMAND SCRIPT_COMMAND

#endif
#define DEFAULT_CAMERA_JSON default_camera_json
#define STREAM_REQUEST(l) (rs_sf_stream_request{l,-1,-1,g_ir_fps,g_color_fps,g_replace_color})
#define VERSION_STRING "v1.3"

bool        g_t265 = true;
int         g_camera_id = 0;
int         g_auto_capture_interval_s = 1; //seconds
int         g_cam_fisheye = 3;
float       g_cam_velocity_thr = 0.2f;
float       g_cam_prev_dist_thr = 0.5f;
std::string g_str_origin = "[filename,tx,ty,tz,rw,rx,ry,rz,blur_r,blur_g,blur_b] | Origin in WGS84 coordinate: Not provided at command line input.";
std::string g_pose_path = DEFAULT_PATH;
std::string g_script_name = DEFAULT_SCRIPT;

cv::Size size_win_fisheye(848 / 4, 800 / 4);

int scn_width = 684 + size_win_fisheye.width;
int scn_height = 456;
const int num_buttons = 6;

inline cv::Size size_rgb() { return cv::Size(scn_width - size_win_fisheye.width, scn_height); }
inline cv::Size size_screen() { return cv::Size(scn_width, scn_height); }

inline cv::Rect win_rgb() { return cv::Rect(0, 0, size_rgb().width, size_rgb().height); }
inline cv::Rect win_fisheye() { return cv::Rect(win_rgb().width, 0, size_win_fisheye.width, size_win_fisheye.height); }
inline cv::Rect win_text() { return cv::Rect(0, 0, scn_width - win_fisheye().width, scn_height); }
inline cv::Rect win_buttons() { return cv::Rect(win_fisheye().x, win_fisheye().height, win_fisheye().width, win_rgb().height - win_fisheye().height); }

inline cv::Rect win_annotate() { return cv::Rect(0, win_rgb().height * 7 / 8, win_rgb().width / 8, win_rgb().height / 8); }

inline cv::Size size_button() { return cv::Size(win_fisheye().width, (scn_height - win_fisheye().height) / num_buttons); }
inline cv::Rect win_exit() { return cv::Rect(scn_width - size_button().width, scn_height - size_button().height, size_button().width, size_button().height); }
inline cv::Rect win_bin() { return cv::Rect(win_exit().x, win_exit().y - win_exit().height, size_button().width / 2, size_button().height); }
inline cv::Rect win_script() { return cv::Rect(win_bin().x + win_bin().width, win_bin().y, win_bin().width, win_bin().height); }

inline cv::Rect win_capture() { return cv::Rect(win_bin().x, win_bin().y - size_button().height, size_button().width, size_button().height); }
inline cv::Rect win_init() { return cv::Rect(win_capture().x, win_capture().y - size_button().height, size_button().width /2, size_button().height); }
inline cv::Rect win_auto() { return cv::Rect(win_script().x, win_init().y, win_init().width, win_init().height); }
inline cv::Rect win_cam2() { return cv::Rect(win_init().x, win_init().y - size_button().height, size_button().width/2, size_button().height); }
inline cv::Rect win_cam3() { return cv::Rect(win_cam2().x+win_cam2().width,win_cam2().y, win_cam2().width, win_cam2().height); }
inline cv::Rect win_cam0() { return cv::Rect(win_cam2().x, win_cam2().y - win_cam2().height, win_cam2().width, win_cam2().height); }
inline cv::Rect win_cam1() { return cv::Rect(win_cam3().x, win_cam3().y - win_cam2().height, win_cam2().width, win_cam2().height); }

inline cv::Point label(const cv::Rect& rect) { return cv::Point(rect.x, rect.y + rect.height * 2 / 3); }
std::vector<std::string>& operator<<(std::vector<std::string>& buf, const std::string& msg) { buf.emplace_back(msg); return buf; }

const cv::Scalar white(255, 255, 255), dark_gray(64, 64, 64), yellow(0, 255, 255);

struct app_data {
    bool exit_request = false;
	bool bin_request = false;
    bool script_request = false;
    bool capture_request = false;
    bool init_request = false;
    bool auto_request = false;
    bool fisheye_0_request = false;
    bool cam0_request = false;
    bool cam1_request = false;
    bool cam2_request = false;
    bool cam3_request = false;
    bool annotate_mode = false;
    bool apriltag_mode = false;
    bool estimate_blur = true;
    int highlight_exit_button = 0;
	int highlight_bin_button = 0;
    int highlight_script_button = 0;
    int highlight_capture_button = 0;
    int highlight_init_button = 0;

    std::time_t last_capture_time = {};
    std::unique_ptr<std::future<int>> system_thread;

	cv::Mat last_rgb_capture, last_rgb_thumbnail, last_rgb_annotate;
	cv::Mat new_rgb_capture;
    double last_blur_estimate[3];

    std::function<void()> task_record_annotation;

    void set_exit_request() { exit_request = true; highlight_exit_button = 5; }
	void set_bin_request() { bin_request = true; highlight_bin_button = 5; }
    void set_script_request() { script_request = true; highlight_script_button = 5; }
    void set_capture_request() { capture_request = true; highlight_capture_button = 5; }
    void set_init_request() { init_request = true; highlight_init_button = 5; }
    void set_auto_request() { auto_request = !auto_request; }
    void set_fisheye_request() { g_cam_fisheye = 3 - g_cam_fisheye; }
    void set_annotate_request(int x, int y) { 
		std::lock_guard<std::mutex> lk(mutex_clicks); 
		last_annotate_clicks.emplace_back(cv::Point(x, y));
	}
	std::deque<cv::Point> get_annotate_clicks() {
		std::lock_guard<std::mutex> lk(mutex_clicks);
		return last_annotate_clicks;
	}
    
    bool is_highlight_exit_button() { highlight_exit_button = std::max(0, highlight_exit_button - 1); return highlight_exit_button > 0; }
	bool is_highlight_bin_button() { highlight_bin_button = std::max(0, highlight_bin_button - 1); return highlight_bin_button > 0; }
	bool is_highlight_script_button() { highlight_script_button = std::max(0, highlight_script_button - 1); return highlight_script_button > 0; }
	bool is_highlight_capture_button() { highlight_capture_button = std::max(0, highlight_capture_button - 1); return highlight_capture_button > 0; }
    bool is_highlight_init_button() { highlight_init_button = std::max(0, highlight_init_button - 1); return highlight_init_button > 0; }
    bool is_highlight_auto_button() { return auto_request; }

    bool is_annotate() { return annotate_mode && !last_rgb_capture.empty(); }
    bool is_system_run() { return (bool)system_thread || is_annotate(); }

	void annotation_record() {
		if (task_record_annotation) {
			task_record_annotation();
			task_record_annotation = nullptr;
		}
		g_app_data.annotation_clear();
	}
    void annotation_clear() {
        last_annotate_clicks.clear(); last_rgb_annotate = cv::Mat();
    }

private:
	std::mutex mutex_clicks;
	std::deque<cv::Point> last_annotate_clicks;
} g_app_data;

bool is_record_fisheye() { return g_cam_fisheye > 0; }

#include "apriltag.h"
#include "tag36h11.h"
#include "tag36h10.h"
#include "tag36artoolkit.h"
#include "tag25h9.h"
#include "tag25h7.h"
#include "common/getopt.h"

struct apriltag
{
    apriltag() { init(); }
    ~apriltag() { destory(); }

    getopt_t *getopt = nullptr;
    const char* famname = nullptr;
    apriltag_family_t *tf = nullptr;
    apriltag_detector_t *td = nullptr;

    void destory()
    {
        apriltag_detector_destroy(td);
        if (!strcmp(famname, "tag36h11"))
            tag36h11_destroy(tf);
        else if (!strcmp(famname, "tag36h10"))
            tag36h10_destroy(tf);
        else if (!strcmp(famname, "tag36artoolkit"))
            tag36artoolkit_destroy(tf);
        else if (!strcmp(famname, "tag25h9"))
            tag25h9_destroy(tf);
        else if (!strcmp(famname, "tag25h7"))
            tag25h7_destroy(tf);
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
        else if (!strcmp(famname, "tag36h10"))
            tf = tag36h10_create();
        else if (!strcmp(famname, "tag36artoolkit"))
            tf = tag36artoolkit_create();
        else if (!strcmp(famname, "tag25h9"))
            tf = tag25h9_create();
        else if (!strcmp(famname, "tag25h7"))
            tf = tag25h7_create();
        else {
            printf("Unrecognized tag family name. Use e.g. \"tag36h11\".\n");
            exit(-1);
        }
        tf->black_border = getopt_get_int(getopt, "border");

        td = apriltag_detector_create();
        apriltag_detector_add_family(td, tf);
        td->quad_decimate = (float)getopt_get_double(getopt, "decimate");
        td->quad_sigma = (float)getopt_get_double(getopt, "blur");
        td->nthreads = getopt_get_int(getopt, "threads");
        td->debug = getopt_get_bool(getopt, "debug");
        td->refine_edges = getopt_get_bool(getopt, "refine-edges");
        td->refine_decode = getopt_get_bool(getopt, "refine-decode");
        td->refine_pose = getopt_get_bool(getopt, "refine-pose");
    }

    std::string find(cv::Mat& frame, cv::Mat& gray)
    {
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

        zarray_t *detections = apriltag_detector_detect(td, &im);
        
        int num_tag_detected = zarray_size(detections);
        // Draw detection outlines
        for (int i = 0; i < num_tag_detected; i++) {
            apriltag_detection_t *det;
            zarray_get(detections, i, &det);
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
            cv::Size textsize = getTextSize(text, fontface, fontscale, 2,
                &baseline);
            cv::putText(frame, text, cv::Point2d(det->c[0] - textsize.width / 2,
                det->c[1] + textsize.height / 2),
                fontface, fontscale, cv::Scalar(0xff, 0x99, 0), 2);
        }
        zarray_destroy(detections);

        return std::to_string(num_tag_detected) + " tags detected";
    }
};

void run()
{
    int camera_id = g_camera_id;    
    std::map<int, int> counters;
    std::map<int, std::string> stream_names;
    std::mutex mutex;
    
    bool t265_available = g_t265;
    std::string folder_path = g_pose_path;
    std::ofstream index_file, fisheye_calibration_file;
    std::string last_file_written, last_blur_estimate_str;

    std::string window_name = "T265-Insight POC App " + std::string(VERSION_STRING) + " @ " + folder_path;
    cv::namedWindow(window_name);

    for(g_app_data.exit_request=false; !g_app_data.exit_request; g_app_data.annotation_record() )
    {
        struct cv_cam : public rgb_cam, private cv::VideoCapture
        {
            cv_cam(int camera_id) : cv::VideoCapture(camera_id), m_camera_id(camera_id){
                set(CV_CAP_PROP_FRAME_WIDTH, 4096);
                set(CV_CAP_PROP_FRAME_HEIGHT, 2160);

                auto cap_width = get(CV_CAP_PROP_FRAME_WIDTH);
                auto cap_height = get(CV_CAP_PROP_FRAME_HEIGHT);
            }
            bool isOpened() override { return cv::VideoCapture::isOpened(); }
            bool get_image(int& w, int& h, cv::Mat& preview, cv::Mat* original) override {
				try {
					*this >> preview; 
					w = preview.cols; h = preview.rows;
					if (original) { *original = preview; }
				}
                catch (...) { return false; }
                return true;
            }
            std::string name() override { return "cam id " + std::to_string(m_camera_id); }
            int m_camera_id = -1;
        };

        std::unique_ptr<rgb_cam> cap = rgb_cam::create();
        if (!cap->isOpened()) { cap = std::make_unique<cv_cam>(camera_id); }

        struct pose_rec : public rs2_pose {
            void update(const rs2_pose& ref) { *(rs2_pose*)this = ref; }
            void record_capture() { _prev_cap_pose = std::make_shared<pose_rec>(*this); _prev_cap_pose->_prev_cap_pose.reset(); }
            double distance_prev_capture() const {
                return (_prev_cap_pose ? cv::norm(pos() - _prev_cap_pose->pos(), cv::NORM_L2) : -1.0);
            }
            cv::Vec3f pos() const { return cv::Vec3f(translation.x, translation.y, translation.z); }
            cv::Vec3f vel() const { return cv::Vec3f(velocity.x, velocity.y, velocity.z); }
            double speed() const { return cv::norm(vel(), cv::NORM_L2); }
            std::shared_ptr<pose_rec> _prev_cap_pose;
        } current_pose;

        std::unique_ptr<rs2::pipeline> pipe;
		std::deque<rs2_intrinsics> intr;

        auto start_t265_pipe = [&](){
            // Declare RealSense pipeline, encapsulating the actual device and sensors
            try {

				/**
				rs2::context ctx;
				if (ctx.query_devices().size() == 0) {
					t265_available = false;
					return;
				}
				*/

                current_pose = {};
				intr.clear();

                pipe = std::make_unique<rs2::pipeline>();
                // Create a configuration for configuring the pipeline with a non default profile
                rs2::config cfg;
                // Add pose stream
                cfg.enable_stream(RS2_STREAM_FISHEYE, 0);
                cfg.enable_stream(RS2_STREAM_FISHEYE, 1);
                cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);

                // Start pipeline with chosen configuration
                auto profiles = pipe->start();
                auto _streams = profiles.get_streams();

                //Collect the enabled streams names
                for (auto p : profiles.get_streams()) {
                    stream_names[p.unique_id()] = p.stream_name();
                    printf("%s\n", p.stream_name().c_str());

                    auto vp = p.as<rs2::video_stream_profile>();
                    if (vp) {
						intr.emplace_back(vp.get_intrinsics());
						auto fi = intr.back();
                        printf("fisheye fx %.3f fy %.3f, px %.3f py %.3f, size %d %d,", fi.fx, fi.fy, fi.ppx, fi.ppy, fi.width, fi.height);
                        for (auto m : fi.coeffs) {
                            printf("%.3f ", m);
                        }
                        printf("\n");
                    }    
				}
            }
            catch (...) {
                t265_available = false;
            }
        };

        apriltag atag;

        int cap_width = -1;  //cap.get(CV_CAP_PROP_FRAME_WIDTH);
        int cap_height = -1;  //cap.get(CV_CAP_PROP_FRAME_HEIGHT);

        cv::Mat preview_img, screen_img, cvFe0, tag_gray;
        screen_img.create(size_screen(), CV_8UC3);

        for (bool switch_request = false; !switch_request && !g_app_data.exit_request;)
        {
            std::vector<std::string> scn_msg, scn_warn;

            if (cap->isOpened()) {

				cv::Mat* original_img_ptr = nullptr;
				if (g_app_data.capture_request && g_app_data.new_rgb_capture.empty()) {
					original_img_ptr = &g_app_data.new_rgb_capture;
				}
                cap->get_image(cap_width, cap_height, preview_img, original_img_ptr);
               
                scn_height = (int)((scn_width - size_win_fisheye.width)* preview_img.rows / preview_img.cols);

				if (screen_img.rows < scn_height) { screen_img.create(size_screen(), CV_8UC3); }
				screen_img(win_buttons()).setTo(0);

                if (!g_app_data.is_annotate()) {
                    cv::resize(preview_img, screen_img(win_rgb()), size_rgb(), 0, 0, CV_INTER_NN);

                    std::string num_tag_msg = "Press `a` for apriltag";
                    if (g_app_data.apriltag_mode) {
                        cv::cvtColor(screen_img(win_rgb()), tag_gray, CV_RGB2GRAY);
                        num_tag_msg = atag.find(screen_img(win_rgb()), tag_gray);
                    }
                    scn_msg << "RGB " + cap->name() + "  w:" + std::to_string(cap_width) + " h:" + std::to_string(cap_height)
                        + " " + num_tag_msg;
                }
            }
            else {
                screen_img.setTo(0);
            }

            // handle annotation thumbnail
            if (!g_app_data.is_annotate()) {
                if (!g_app_data.last_rgb_thumbnail.empty()) {
                    g_app_data.last_rgb_thumbnail.copyTo(screen_img(win_annotate()));
                }
            }
            else {
                cv::resize(g_app_data.last_rgb_capture, screen_img(win_rgb()), size_rgb(), 0, 0, CV_INTER_NN);
            }
            scn_msg << "Script: " + g_script_name;

            rs2::frame p;
            rs2::frameset fs;

            auto get_fisheye = [](rs2::frameset& fs, int lens) -> cv::Mat {
                if (fs) {
                    auto f = fs.first_or_default(RS2_STREAM_FISHEYE);
                    if (f) {
                        auto vf = f.as<rs2::video_frame>();
                        return cv::Mat(vf.get_height(), vf.get_width(), CV_8UC1, (void*)vf.get_data());
                    }
                }
                return cv::Mat();
            };

			if (t265_available && !pipe) { start_t265_pipe(); }
			
			if (t265_available && pipe)
			{
                try {
                    //fs = pipe->wait_for_frames();
					if (pipe->poll_for_frames(&fs))
					//if(fs)
					{
						cv::Mat cvvf = get_fisheye(fs, 0);
						if (!cvvf.empty()) {
							cv::Mat simg;
							cv::resize(cvvf, simg, cv::Size(), 0.25, 0.25, CV_INTER_NN);
							cv::cvtColor(is_record_fisheye() ? simg : simg * 0.5 + 128, screen_img(win_fisheye()), CV_GRAY2RGB, 3);
						}
						p = fs.first_or_default(RS2_STREAM_POSE);
						if (p)
						{
							auto pf = p.as<rs2::pose_frame>();
							if (pf)
							{
								const auto print = [&scn_msg, &scn_warn, &current_pose](const std::string& conf, const rs2_pose& p) {
									current_pose.update(p);
									std::stringstream ss;
									scn_msg << "T265 Confidence: " + conf + ", Fisheye Capture: " + (is_record_fisheye() ? "ON" : "OFF");
									ss << std::fixed << std::right << std::setprecision(3) << std::setw(6);
									ss << p.translation.x << "," << p.translation.y << "," << p.translation.z;
									scn_msg << "Translation: " + ss.str();
									std::stringstream sr;
									sr << std::fixed << std::right << std::setprecision(3) << std::setw(6);
									sr << p.rotation.w << "," << p.rotation.x << "," << p.rotation.y << "," << p.rotation.z;
									scn_msg << "Rotation: " + sr.str();
									std::stringstream sv;
									sv << std::fixed << std::right << std::setprecision(3) << std::setw(6);
									sv << current_pose.speed();
									if (current_pose._prev_cap_pose) { sv << ",  " << current_pose.distance_prev_capture() << " meter away last capture"; }
									scn_msg << "Velocity: " + sv.str();

									scn_warn << (current_pose.speed() > g_cam_velocity_thr ? " SLOW DOWN !!!" : "");
									scn_warn << (current_pose.distance_prev_capture() > g_cam_prev_dist_thr ? "TOO FAR FROM LAST CAPTURE!" : "");
								};

								auto pd = pf.get_pose_data();
								switch (pd.tracker_confidence) {
								case 1: print("Low", pd); break;
								case 2: print("Medium", pd); break;
								case 3: print("High", pd); break;
								default: scn_msg << "T265 Tracking Failed";
								}
							}
						}
					}
                }
                catch (...) { 
					t265_available = false; 
					p = nullptr;
					fs = {};
				}
            }
            if (!last_file_written.empty()) {
                scn_msg << "Last write:" + last_file_written;
                if (g_app_data.estimate_blur) { scn_msg << "Blur Est: " + last_blur_estimate_str; }
            }

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            if (g_app_data.is_annotate()) {
                scn_msg.clear();
                scn_msg << "Annotation Mode";
                scn_warn.clear();

				std::vector<cv::Point> pts; pts.reserve(200);
				for (const cv::Point click : g_app_data.get_annotate_clicks()) {
                    //cv::circle(screen_img, click, 30, dark_gray, 2);
					pts.clear();
					auto delta = (int)(max(1.0, 180.0 * M_1_PI / 25));
					cv::ellipse2Poly(click, cv::Size(25, 25), 0, 0, 359, delta, pts);
                    cv::polylines(screen_img, pts, false, yellow, 2);
                }
                cv::resize(screen_img(win_rgb()), g_app_data.last_rgb_annotate, cv::Size(), 1, 1, CV_INTER_NN);

               cv::rectangle(screen_img, win_annotate(), white, 1);
               cv::putText(screen_img, "   DONE", label(win_annotate()), CV_FONT_HERSHEY_DUPLEX, 0.5, white, 2);
            }
			else {
				// complete previous annotations
				if (!g_app_data.last_rgb_annotate.empty()) {
					
					g_app_data.task_record_annotation = [&, last_cap_time = g_app_data.last_capture_time, annotate_img = g_app_data.last_rgb_annotate.clone(), clicks = g_app_data.get_annotate_clicks()](){

						std::stringstream ss;
						ss << std::put_time(std::localtime(&last_cap_time), "%Y_%m_%d_%H_%M_%S");

						auto time_str = ss.str();
						std::string original_filename = "rgb_" + time_str;
						std::string annotated_filename = "rgb_annotated_" + time_str;

						double xfactor = (double)cap_width / annotate_img.cols;
						double yfactor = (double)cap_height / annotate_img.rows;

						Json::Value json_root, coord;
						json_root["type"] = "Feature";
						json_root["geometry"]["type"] = "Polygon";

						std::vector<cv::Point> pts; pts.reserve(200);
						for (int c = 0, r = 25; c < (int)clicks.size(); ++c) {
							pts.clear();
							auto delta = (int)(max(1.0, 180.0 * M_1_PI / r));
							cv::ellipse2Poly(clicks[c], cv::Size(r, r), 0, 0, 360, delta, pts);
							for (int p = 0; p < (int)pts.size(); ++p) {
                                if (pts[p].x < annotate_img.cols && pts[p].y < annotate_img.rows) {
                                    coord[c][p][0] = pts[p].x * xfactor;
                                    coord[c][p][1] = (annotate_img.rows - pts[p].y) * yfactor;
                                }
							}
						}
						json_root["geometry"]["coordinates"] = coord;
						json_root["properties"]["comment"] = "From RealSight/Insight POC " VERSION_STRING;
						json_root["properties"]["image"] = (original_filename + ".jpg").c_str();
						json_root["properties"]["name"] = "circle(s)";
						json_root["properties"]["color"][0] = (int)yellow[2];
						json_root["properties"]["color"][1] = (int)yellow[1];
						json_root["properties"]["color"][2] = (int)yellow[0];

						std::stringstream timestr;
						timestr << std::put_time(std::localtime(&last_cap_time), "%Y-%m-%dT:%H:%M:%S.000Z");
						json_root["properties"]["created"] = timestr.str();

						auto anno_img_path = folder_path + annotated_filename;
						auto geo_json_path = folder_path + original_filename;
						auto anno_img = annotate_img;
						auto jroot = json_root;

						//std::async(std::launch::async, [anno_img_path = folder_path + annotated_filename, 
						//                            geo_json_path = folder_path + original_filename,
						//                            anno_img = annotate_img, jroot = json_root]()
						{
							try {
								cv::imwrite(anno_img_path + ".jpg", anno_img, { CV_IMWRITE_JPEG_QUALITY, 100 });

								std::ofstream annotation_file;
								annotation_file.open(geo_json_path + ".geojson", std::ios_base::out | std::ios_base::trunc);
								Json::StyledStreamWriter writer;
								writer.write(annotation_file, jroot);
							}
							catch (...) {
								printf("WARNING: error in writing geojson file.");
							}
						}
						//);
					};

					g_app_data.annotation_clear();
				}

				if (g_app_data.is_system_run() && !g_app_data.capture_request && g_app_data.new_rgb_capture.empty()) { //while not is_annotate()
					scn_warn << "Tagging/Uploading ... ";
					scn_warn << "           Please wait";
				}
			}
            //////////////////////////////////////////////////////////////////////////////////////////////////////

            for (int i = 0; i < (int)scn_msg.size(); ++i) {
                cv::putText(screen_img, scn_msg[i], cv::Point(win_text().x + 10, win_text().y + 20 * (1 + i)), CV_FONT_HERSHEY_DUPLEX, 0.5, white);
            }

            for (int j = 0; j < (int)scn_warn.size(); ++j) {
                cv::putText(screen_img, scn_warn[j], cv::Point(win_rgb().x + 50, win_rgb().y + win_rgb().height / 2 + 50 * j), CV_FONT_HERSHEY_DUPLEX, 1.25, yellow, 2);
            }
            
            auto label_color = [](bool flag) { return flag ? white : dark_gray; };
            auto is_button_on = [&]() { return !g_app_data.is_system_run(); };
            cv::rectangle(screen_img, win_exit(), white, g_app_data.is_highlight_exit_button() ? 3 : 1);
            cv::putText(screen_img, "  EXIT", label(win_exit()), CV_FONT_HERSHEY_DUPLEX, 0.5, label_color(is_button_on()));
            cv::rectangle(screen_img, win_bin(), white, g_app_data.is_highlight_bin_button() ? 3 : 1);
            cv::putText(screen_img, "  UPLOAD", label(win_bin()), CV_FONT_HERSHEY_DUPLEX, 0.5, label_color(is_button_on()));
			cv::rectangle(screen_img, win_script(), white, g_app_data.is_highlight_script_button() ? 3 : 1);
            cv::putText(screen_img, " CALL SCRIPT", label(win_script()), CV_FONT_HERSHEY_DUPLEX, 0.45, label_color(is_button_on()));
            cv::rectangle(screen_img, win_capture(), white, g_app_data.is_highlight_capture_button() ? 3 : 1);
            cv::putText(screen_img, "  CAPTURE", label(win_capture()), CV_FONT_HERSHEY_DUPLEX, 0.5, label_color(!g_app_data.auto_request && is_button_on()));
            cv::rectangle(screen_img, win_init(), white, g_app_data.is_highlight_init_button() ? 3 : 1);
            cv::putText(screen_img, " INIT NORTH", label(win_init()), CV_FONT_HERSHEY_DUPLEX, 0.5, label_color(is_button_on()));
            cv::rectangle(screen_img, win_auto(), white, g_app_data.is_highlight_auto_button() ? 3 : 1);
            cv::putText(screen_img, "  AUTO " + std::to_string(g_auto_capture_interval_s) + "s", label(win_auto()), CV_FONT_HERSHEY_DUPLEX, 0.5, label_color(is_button_on()||g_app_data.auto_request));
            cv::rectangle(screen_img, win_cam3(), white, camera_id == 3 ? 3 : 1);
            cv::putText(screen_img, "  CAM 3", label(win_cam3()), CV_FONT_HERSHEY_DUPLEX, 0.5, white);
            cv::rectangle(screen_img, win_cam2(), white, camera_id == 2 ? 3 : 1);
            cv::putText(screen_img, "  CAM 2", label(win_cam2()), CV_FONT_HERSHEY_DUPLEX, 0.5, white);
            cv::rectangle(screen_img, win_cam1(), white, camera_id == 1 ? 3 : 1);
            cv::putText(screen_img, "  CAM 1", label(win_cam1()), CV_FONT_HERSHEY_DUPLEX, 0.5, white);
            cv::rectangle(screen_img, win_cam0(), white, camera_id == 0 ? 3 : 1);
            cv::putText(screen_img, "  CAM 0", label(win_cam0()), CV_FONT_HERSHEY_DUPLEX, 0.5, white);
            
            //////////////////////////////////////////////////////////////////////////////////////
            if (g_app_data.init_request)
            {
                g_app_data.annotation_record();

                pipe = nullptr;
				t265_available = g_t265;
                if (index_file.is_open()) { index_file.close(); }
                g_app_data.init_request = false;
            }

            auto t = std::time(NULL);
            if (g_app_data.auto_request && g_app_data.new_rgb_capture.empty())
            {
                auto current_time = std::chrono::system_clock::from_time_t(t);
                auto last_time = std::chrono::system_clock::from_time_t(g_app_data.last_capture_time);
                if (std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time).count() >= g_auto_capture_interval_s * 1000)
                {
                    g_app_data.set_capture_request();
                }
            }
            if (g_app_data.capture_request && !g_app_data.is_system_run() && !g_app_data.new_rgb_capture.empty())
			{
				g_app_data.annotation_record();

				g_app_data.last_capture_time = t;
				current_pose.record_capture();

				g_app_data.system_thread = std::make_unique<std::future<int>>(std::async(std::launch::async, 
					[&, cp = current_pose, is_pose_available = (p && fs), cvfisheye = get_fisheye(fs,0).clone()]() -> int 
				{
					auto tm = *std::localtime(&g_app_data.last_capture_time);
					std::stringstream ss;
					ss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");

					std::string filename = "rgb_" + ss.str() + ".jpg";
					last_file_written = folder_path + filename;

					if (g_app_data.new_rgb_capture.channels() == 1) {
						cv::cvtColor(g_app_data.new_rgb_capture, g_app_data.last_rgb_capture, CV_BayerBG2BGR, 3);
					}
					else {
						g_app_data.last_rgb_capture = g_app_data.new_rgb_capture.clone();
					}

					cv::imwrite(last_file_written, g_app_data.last_rgb_capture, { CV_IMWRITE_JPEG_QUALITY, 100 });
					cv::resize(g_app_data.last_rgb_capture, g_app_data.last_rgb_thumbnail, win_annotate().size(), 0, 0, CV_INTER_NN);

					if (!index_file.is_open()) {
						index_file.open(folder_path + "pose.txt", std::ios_base::out | std::ios_base::trunc);
						index_file << g_str_origin << std::endl;
					}

					index_file << filename;

                    if (!is_pose_available) 
                    {
                        index_file << "," << tm.tm_sec/60.0f << "," << tm.tm_min/60.0f << "," << 0;
                        index_file << "," << 0 << "," << 0 << "," << 0 << "," << 0;
                    }
                    else
                    {
                        index_file << "," << cp.translation.x << "," << cp.translation.y << "," << cp.translation.z;
                        index_file << "," << cp.rotation.w << "," << cp.rotation.x << "," << cp.rotation.y << "," << cp.rotation.z;

						if (is_record_fisheye() && !cvfisheye.empty()) {

							std::string fisheye_0_filename = "fe0_" + ss.str() + ".jpg";
							index_file << "," << fisheye_0_filename;
							cv::imwrite(folder_path + fisheye_0_filename, cvfisheye, { CV_IMWRITE_JPEG_QUALITY, 100 });
							last_file_written = folder_path + "fe0_+" + filename;

							if (!fisheye_calibration_file.is_open())
							{
								auto intr0 = intr[0];
								Json::Value json_intr;
								json_intr["fx"] = intr0.fx;
								json_intr["fy"] = intr0.fy;
								json_intr["ppx"] = intr0.ppx;
								json_intr["ppy"] = intr0.ppy;
								json_intr["height"] = intr0.height;
								json_intr["width"] = intr0.width;
								json_intr["model"] = intr0.model;
								json_intr["model_name"] = rs2_distortion_to_string(intr0.model);
								for (const auto& c : intr0.coeffs) {
									json_intr["coeff"].append(c);
								}
								Json::Value json_root;
								json_root["device"]["serial_numer"] = pipe->get_active_profile().get_device().get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
								json_root["fisheye"][0]["intrinsics"] = json_intr;

								try {
									printf("INFO: writing fisheye calibration file");
									fisheye_calibration_file.open(folder_path + "fisheye_calibration.txt", std::ios_base::out | std::ios_base::trunc);
									Json::StyledStreamWriter writer;
									writer.write(fisheye_calibration_file, json_root);
								}
								catch (...) {
									printf("WARNING: error in writing fisheye calibration file.");
								}
							}
						}
					}

                    // blur metric
                    if (g_app_data.estimate_blur) {
                        cv::Mat temp_mat;
                        std::vector<cv::Mat> RGB_mats;
                        cv::split(g_app_data.last_rgb_capture, RGB_mats);                       
                        double blur_estimate[3];
                        for (int c = 0; c < 3; ++c) {
                            cv::Laplacian(RGB_mats[c], temp_mat, CV_64F); //going to cause allocation
                            cv::Scalar mean, std;
                            cv::meanStdDev(temp_mat, mean, std);
                            g_app_data.last_blur_estimate[c] = blur_estimate[c] = std.val[0];
                        }
                        index_file << "," << blur_estimate[0] << "," << blur_estimate[1] << "," << blur_estimate[2];

                        std::stringstream ss;
                        ss << std::fixed << std::left << std::setprecision(1) <<
                            g_app_data.last_blur_estimate[0] << ", " <<
                            g_app_data.last_blur_estimate[1] << ", " <<
                            g_app_data.last_blur_estimate[2];
                        last_blur_estimate_str = ss.str();
                    }

					index_file << std::endl;
					g_app_data.new_rgb_capture = cv::Mat();
					g_app_data.capture_request = false; //capture request handled
					g_app_data.system_thread.reset();

					return 0;
				}));
            }
            if (g_app_data.script_request && !g_app_data.is_system_run()) {

				g_app_data.auto_request = g_app_data.capture_request = false;
				g_app_data.new_rgb_capture = cv::Mat();
				g_app_data.annotation_record();

                g_app_data.system_thread = std::make_unique<std::future<int>>(std::async(std::launch::async, [&]() -> int {
                    auto rtn = system(SCRIPT_COMMAND);
                    g_app_data.system_thread.reset();
                    return rtn;
                }));
                g_app_data.auto_request = false;
                g_app_data.script_request = false; //script process request handled
            }
			if (g_app_data.bin_request && !g_app_data.is_system_run()) {

				g_app_data.auto_request = g_app_data.capture_request = false;
				g_app_data.new_rgb_capture = cv::Mat(); 
				g_app_data.annotation_record();

                g_app_data.system_thread = std::make_unique<std::future<int>>(std::async(std::launch::async, [&]() -> int {
                    auto rtn = system(BIN_COMMAND);
                    g_app_data.system_thread.reset();
                    return rtn;
                }));
                g_app_data.auto_request = false;
				g_app_data.bin_request = false; // exe/bin process request handled
			}

            if (g_app_data.cam0_request && camera_id != 0) { camera_id = 0; switch_request = true; }
            else if (g_app_data.cam1_request && camera_id != 1) { camera_id = 1; switch_request = true; }
            else if (g_app_data.cam2_request && camera_id != 2) { camera_id = 2; switch_request = true; }
            else if (g_app_data.cam3_request && camera_id != 3) { camera_id = 3; switch_request = true; }
            g_app_data.cam0_request = false;
            g_app_data.cam1_request = false;
            g_app_data.cam2_request = false;
            g_app_data.cam3_request = false;
            
            cv::imshow(window_name, screen_img.clone());
            switch (cv::waitKey(1))
            {
                case 'q': case 27: return;
                case 'a': g_app_data.apriltag_mode = !g_app_data.apriltag_mode; break;
                case '0': if (camera_id != 0) { camera_id = 0; switch_request = true; break; }
                case '1': if (camera_id != 1) { camera_id = 1; switch_request = true; break; }
                case '2': if (camera_id != 2) { camera_id = 2; switch_request = true; break; }
                case '3': if (camera_id != 3) { camera_id = 3; switch_request = true; break; }
                case '4': if (camera_id != 4) { camera_id = 4; switch_request = true; break; }
                case '5': if (camera_id != 5) { camera_id = 5; switch_request = true; break; }
                default: break;
            }
            
            cv::setMouseCallback(window_name, [](int event, int x, int y, int flags, void* userdata) {
                auto is_button_on = [&]() { return !g_app_data.is_system_run(); };
				auto is_auto_on = [&]() { return is_button_on() || g_app_data.auto_request; };
				auto is_anno_on = [&]() { return !g_app_data.system_thread && !g_app_data.auto_request; };

                switch (event) {
                    case cv::EVENT_LBUTTONUP:
						if (win_annotate().contains(cv::Point(x, y))) { if (is_anno_on()) { g_app_data.annotate_mode = !g_app_data.annotate_mode; } }
                        else {
                            if (g_app_data.is_annotate()) {
                                g_app_data.set_annotate_request(x, y);
                            }
                            else {
                                if (win_exit().contains(cv::Point(x, y))) { if (is_button_on()) { g_app_data.set_exit_request(); } }
                                if (win_bin().contains(cv::Point(x, y))) { if (is_button_on()) { g_app_data.set_bin_request(); } }
                                if (win_script().contains(cv::Point(x, y))) { if (is_button_on()) { g_app_data.set_script_request(); } }
                                if (win_rgb().contains(cv::Point(x,y)) || 
                                    win_capture().contains(cv::Point(x, y))) { if (is_button_on() && !g_app_data.auto_request) { g_app_data.set_capture_request(); } }
                                if (win_init().contains(cv::Point(x, y))) { if (is_button_on()) { g_app_data.set_init_request(); } }
                                if (win_auto().contains(cv::Point(x, y))) { if (is_auto_on()) { g_app_data.set_auto_request(); } }
                                if (win_fisheye().contains(cv::Point(x, y))) { if (is_button_on()) { g_app_data.set_fisheye_request(); } }
                                if (win_cam0().contains(cv::Point(x, y))) { g_app_data.cam0_request = true; }
                                if (win_cam1().contains(cv::Point(x, y))) { g_app_data.cam1_request = true; }
                                if (win_cam2().contains(cv::Point(x, y))) { g_app_data.cam2_request = true; }
                                if (win_cam3().contains(cv::Point(x, y))) { g_app_data.cam3_request = true; }
                            }
                        }
                    default: break;
                }
            }, &g_app_data);
        }
    }
}


void spinnaker_test()
{
	auto cap = rgb_cam::create();

	cv::Mat src, display;
	int w, h;
	for (;cap->isOpened();)
	{
		if (cap->get_image(w, h, display, nullptr))
		{
			//cv::resize(src, display, cv::Size(), 0.1, 0.1, CV_INTER_NN);
			cv::imshow("test", display);
		}
		switch (cv::waitKey(1)) {
		case 'q': case 27: return;
		}
	}
}


int main(int argc, char* argv[])
{
	if (false)
	{
		spinnaker_test();
		return 0;
	}

    bool is_live = true, is_capture = false, is_replay = false; int laser_option = 0;
    std::string data_path = DEFAULT_PATH, camera_json_path = "."; //camera.json location
    std::vector<int> capture_size = { 640,480 };
    
    for (int i = 1; i < argc; ++i) {
        if      (!strcmp(argv[i], "--live"))            { is_live = true; is_replay = false; }
        else if (!strcmp(argv[i], "--no_t265"))         { g_t265 = false; }
        else if (!strcmp(argv[i], "--origin"))          { g_str_origin = argv[++i]; }
        else if (!strcmp(argv[i], "--path"))            { g_pose_path = argv[++i]; }
        else if (!strcmp(argv[i], "--cam"))             { g_camera_id = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--script"))          { g_script_name = argv[++i]; }
        else if (!strcmp(argv[i], "--interval"))        { g_auto_capture_interval_s = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--fisheye"))         { g_cam_fisheye = 3; }
        else if (!strcmp(argv[i], "--apriltag"))        { g_app_data.apriltag_mode = true; }
        else if (!strcmp(argv[i], "--no_blur_est"))     { g_app_data.estimate_blur = false; }
        else {
            printf("usages:\n t265-demo.exe [--no_t265][--origin STR][--cam ID][--interval SEC][--script FILENAME][--path OUTPUT_PATH]\n");
            printf("\n");
            printf("--no_tm2  : Capture RGB only without t265 connected.\n");
            printf("--origin  : STR will be added to the first line of output pose.txt.   \n");
            printf("--cam     : ID set the initial camera ID, default 0. Rear-facing tablet cam usually has ID=1\n");
            printf("--interval: SEC number of seconds per auto captured image, default is 2 seconds. \n");
            printf("--script  : FILENAME of an external script, default script is t265-insight.bat \n");
            printf("--path    : OUTPUT_PATH to the capture files, default is the .\\capture\\.\n\n");
            return 0;
        }
    }
    if (data_path.back() != '\\' && data_path.back() != '/'){ data_path.push_back(PATH_SEPARATER); }
    if (camera_json_path.back() != '\\' && camera_json_path.back() != '/'){ camera_json_path.push_back(PATH_SEPARATER); }
    if (g_pose_path.back() != '\\' && g_pose_path.back() != '/'){ g_pose_path.push_back(PATH_SEPARATER); }
    
    run();
    return 0;
}
