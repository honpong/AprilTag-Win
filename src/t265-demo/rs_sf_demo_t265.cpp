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
#define DEFAULT_PATH "C:\\temp\\data\\"
#define GET_CAPTURE_DISPLAY_IMAGE(src) src.one_image()
#define COLOR_STREAM_REQUEST {}
#else
#define PATH_SEPARATER '/'
//#define DEFAULT_PATH (std::string(getenv("HOME"))+"/temp/shapefit/1/")
#define DEFAULT_PATH (std::string(getenv("HOME"))+"/Desktop/temp/data/")
#define GET_CAPTURE_DISPLAY_IMAGE(src) src.images()
#define COLOR_STREAM_REQUEST {if(app.color_request()){ g_replace_color = !g_replace_color; pipe.reset(true); }}

#endif
#define DEFAULT_CAMERA_JSON default_camera_json
#define STREAM_REQUEST(l) (rs_sf_stream_request{l,-1,-1,g_ir_fps,g_color_fps,g_replace_color})
#define VERSION_STRING "v1.3"

int g_ir_fps        = 60;
int g_color_fps     = 15;
int g_accel_dec     = 1;
int g_gyro_dec      = 1;
int g_tablet_screen = 0;
bool g_print_cmd_pose    = false;
bool g_replay_once       = false;
bool g_write_rgb_pose    = false;
std::string g_pose_path = "C:\\temp\\t265-capture\\";

int scn_width  = 800;
int scn_height = 600;

cv::Size size_fisheye(848, 800);
cv::Size size_screen() { return cv::Size(scn_width, scn_height); }
cv::Size size_button() { return cv::Size(scn_width / 5, scn_height / 10); }
cv::Rect win_exit() { return cv::Rect(scn_width - size_button().width, scn_height - size_button().height, size_button().width, size_button().height); }
cv::Rect win_capture() { return cv::Rect(win_exit().x, win_exit().y - size_button().height, size_button().width, size_button().height); }
cv::Rect win_fisheye() { return cv::Rect(scn_width - size_fisheye.width / 4, 0, size_fisheye.width / 4, size_fisheye.height / 4); }
cv::Rect win_text() { return cv::Rect(0, 0, scn_width - win_fisheye().width, scn_height); }

std::vector<std::string>& operator<<(std::vector<std::string>& buf, const std::string& msg) { buf.emplace_back(msg); return buf; }

bool g_t265 = true;

struct app_data {
	bool exit_request = false;
	bool capture_request = false;
} g_app_data;

void run()
{
    int camera_id = 0;
    std::string window_name = "hello camera";
	cv::namedWindow(window_name);

    std::map<int, int> counters;
    std::map<int, std::string> stream_names;
    std::mutex mutex;

	//rs2::context ctx;
	//auto d = ctx.query_devices();
	//bool t265_available = (d.size() > 0);
	//if (t265_available) { printf("device available\n"); }
	bool t265_available = g_t265;
	std::string folder_path = g_pose_path;
	std::ofstream index_file;
	
    for(g_app_data.exit_request=false; !g_app_data.exit_request; )
	{
        cv::VideoCapture cap(camera_id);
		cap.set(CV_CAP_PROP_FRAME_WIDTH, 3200);
		cap.set(CV_CAP_PROP_FRAME_HEIGHT, 2400);
        
        // Define frame callback
        // The callback is executed on a sensor thread and can be called simultaneously from multiple sensors
        // Therefore any modification to common memory should be done under lock
		/*
        auto callback = [&](const rs2::frame& frame)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (rs2::frameset fs = frame.as<rs2::frameset>())
            {
                // With callbacks, all synchronized stream will arrive in a single frameset
                for (const rs2::frame& f : fs)
                    counters[f.get_profile().unique_id()]++;
            }
            else
            {
                // Stream that bypass synchronization (such as IMU) will produce single frames
                counters[frame.get_profile().unique_id()]++;
            }
        };
		*/
        
		std::unique_ptr<rs2::pipeline> pipe;
		rs2::pipeline_profile profiles;
		if (t265_available) 
		{
			pipe = std::make_unique<rs2::pipeline>();
			profiles = pipe->start();
			
			//Collect the enabled streams names
			for (auto p : profiles.get_streams()) {
				stream_names[p.unique_id()] = p.stream_name();
				printf("%s\n", p.stream_name().c_str());
			}
		}
        
        for(bool switch_request=false; !switch_request && !g_app_data.exit_request;)
        {
            cv::Mat img, screen_img;
            cv::Mat cvIr;
			std::vector<std::string> scn_msg;
			rs2::frame p;

            if(cap.isOpened()){
                cap >> img;
				cv::resize(img, screen_img, size_screen(), 0, 0, CV_INTER_NN);
				scn_msg << "RGB cam id:" + std::to_string(camera_id) + "  w:" + std::to_string(img.cols) + " h:" + std::to_string(img.rows);
            }
            else{
				screen_img = cv::Mat(size_screen(), CV_8UC3);
				screen_img.setTo(0);
            }
			scn_msg << "press 0-5 to switch RGB cam";
            
			if (t265_available) {
				rs2::frameset fs = pipe->wait_for_frames();
				if (fs) {
					auto f = fs.first_or_default(RS2_STREAM_FISHEYE);
					cv::Mat simg;
					if (f) {
						auto vf = f.as<rs2::video_frame>();
						cv::Mat cvvf(vf.get_height(), vf.get_width(), CV_8UC1, (void*)vf.get_data());
						cv::resize(cvvf, simg, cv::Size(), 0.25, 0.25, CV_INTER_NN);
						cv::cvtColor(simg, screen_img(win_fisheye()), CV_GRAY2RGB, 3);
					}

					p =  fs.first_or_default(RS2_STREAM_POSE);
					if (p)
					{
						auto pf = p.as<rs2::pose_frame>();
						if (pf)
						{
							const auto print = [&scn_msg](const std::string& conf, const rs2_pose& p) {
								std::stringstream ss;
								scn_msg << "T265 Confidence: " + conf;
								ss << std::fixed << std::right << std::setprecision(3) << std::setw(6);
								ss << p.translation.x << "," << p.translation.y << "," << p.translation.z;
								scn_msg << "Translation: " + ss.str();
								std::stringstream sr;
								sr << std::fixed << std::right << std::setprecision(3) << std::setw(6);
								sr << p.rotation.w << "," << p.rotation.x << "," << p.rotation.y << "," << p.rotation.z;
								scn_msg << "Rotation: " + sr.str();
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

			for (int i = 0; i < (int)scn_msg.size(); ++i) {
				cv::putText(screen_img, scn_msg[i], cv::Point(win_text().x + 10, win_text().y + 20 * (1+i)), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
			}
			cv::rectangle(screen_img, win_exit(), cv::Scalar(255, 255, 255));
			cv::putText(screen_img, "  EXIT", cv::Point(win_exit().x, win_exit().y + win_exit().height / 2), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
			cv::rectangle(screen_img, win_capture(), cv::Scalar(255, 255, 255));
			cv::putText(screen_img, "  CAPTURE", cv::Point(win_capture().x, win_capture().y + win_capture().height / 2), CV_FONT_HERSHEY_DUPLEX, 0.5, cv::Scalar(255, 255, 255));
            cv::imshow(window_name, screen_img);
            switch(cv::waitKey(1))
            {
                case 'q': case 27: return;
				case '0': if (camera_id != 0) { camera_id = 0; switch_request = true; break; }
				case '1': if (camera_id != 1) { camera_id = 1; switch_request = true; break; }
				case '2': if (camera_id != 2) { camera_id = 2; switch_request = true; break; }
				case '3': if (camera_id != 3) { camera_id = 3; switch_request = true; break; }
				case '4': if (camera_id != 4) { camera_id = 4; switch_request = true; break; }
				case '5': if (camera_id != 5) { camera_id = 5; switch_request = true; break; }
                default: break;
            }

			if (g_app_data.capture_request)
			{
				auto t = std::time(NULL);
				auto tm = *std::localtime(&t);
				std::stringstream ss;
				ss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");

				std::string filename = "rgb_" + ss.str() + ".jpg";
				std::cout << "write file :" << filename << std::endl;
				cv::imwrite(folder_path + filename, img, { CV_IMWRITE_JPEG_QUALITY, 100 });
				
				if (!index_file.is_open()) {
					index_file.open(folder_path + "pose.txt", std::ios_base::out | std::ios_base::trunc);
				}
				index_file << filename;

				if (p) {
					auto pf = p.as<rs2::pose_frame>();
					if (pf) {
						auto pd = pf.get_pose_data();
						index_file << "," << pd.translation.x << "," << pd.translation.y << "," << pd.translation.z;
						index_file << "," << pd.rotation.w << "," << pd.rotation.x << "," << pd.rotation.y << "," << pd.rotation.z;
					}
				}

				index_file << std::endl;
				g_app_data.capture_request = false; //handled capture request
			}

			cv::setMouseCallback(window_name, [](int event, int x, int y, int flags, void* userdata) {
				switch (event) {
				case cv::EVENT_LBUTTONUP: 
					if (win_exit().contains(cv::Point(x, y))) { g_app_data.exit_request = true; }
					if (win_capture().contains(cv::Point(x, y))) { g_app_data.capture_request = true; }
				default: break;
				}
			}, &g_app_data);
        }
    }
}

int main(int argc, char* argv[])
{
    bool is_live = true, is_capture = false, is_replay = false; int laser_option = 0;
    std::string data_path = DEFAULT_PATH, camera_json_path = "."; //camera.json location
    std::vector<int> capture_size = { 640,480 };

    for (int i = 1; i < argc; ++i) {
        if      (!strcmp(argv[i], "--live"))            { is_live = true; is_replay = false; g_replay_once = false;}
		else if (!strcmp(argv[i], "--no_tm2"))          { g_t265 = false; }
        else if (!strcmp(argv[i], "--capture"))         { is_capture = true; is_live = false; }
        else if (!strcmp(argv[i], "--path"))            { data_path = camera_json_path = argv[++i]; }
        else if (!strcmp(argv[i], "--replay"))          { is_replay = true; is_live = false; }
        else if (!strcmp(argv[i], "--replay_once"))     { is_replay = true; is_live = false; g_replay_once = true;}
        else if (!strcmp(argv[i], "--hd"))              { capture_size = { 1280,720 }; }
        else if (!strcmp(argv[i], "--qhd"))             { capture_size = { 640,360 }; }
        else if (!strcmp(argv[i], "--vga"))             { capture_size = { 640,480 }; }
        else {
            printf("usages:\n t265-demo \n");
            return 0;
        }
    }
    if (data_path.back() != '\\' && data_path.back() != '/'){ data_path.push_back(PATH_SEPARATER); }
    if (camera_json_path.back() != '\\' && camera_json_path.back() != '/'){ camera_json_path.push_back(PATH_SEPARATER); }
    if (g_pose_path.back() != '\\' && g_pose_path.back() != '/'){ g_pose_path.push_back(PATH_SEPARATER); }
    
    run();
    return 0;
}
