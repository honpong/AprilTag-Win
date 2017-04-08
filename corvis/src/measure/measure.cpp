#include "replay.h"
#include "world_state.h"
#include "offscreen_render.h"
#include "gui.h"
#include "benchmark.h"
#include <iomanip>

#ifdef WIN32
#include <direct.h>
#define mkdir(dir,mode) _mkdir(dir)
#else
#include <sys/stat.h>
#endif

std::string render_filename_from_filename(const char * benchmark_folder, const char * render_folder, const char * filename)
{
    std::string render_filename(filename);
    render_filename = render_filename.substr(strlen(benchmark_folder) + 1);
    std::replace(render_filename.begin(), render_filename.end(), '/', '_');
    return std::string(render_folder) + "/" + render_filename + ".png";
}

int main(int c, char **v)
{
    using std::cerr;
    if (0) { usage:
        cerr << "Usage: " << v[0] << " [--qvga] [--drop-depth] [--realtime] [--async] [--pause] [--pause-at <timestamp_us>][--no-gui] [--no-plots] [--no-fast-path] [--no-video] [--no-main] [--render <file.png>] [(--save | --load) <calibration-json>] [--enable-map] [--save-map <map-json>] [--load-map <map-json>] [--progress] [--incremental-ate] <filename>\n";
        cerr << "       " << v[0] << " [--qvga] [--drop-depth] --benchmark <directory>\n";
        return 1;
    }

    bool realtime = false, start_paused = false, benchmark = false, calibrate = false, zero_bias = false, fast_path = true, async = false, progress = false;
    const char *save = nullptr, *load = nullptr;
    std::string save_map, load_map;
    bool qvga = false, depth = true;
    bool enable_gui = true, show_plots = false, show_video = true, show_depth = true, show_main = true;
    bool enable_map = false;
    bool incremental_ate = false;
    char *filename = nullptr, *rendername = nullptr, *benchmark_output = nullptr, *render_output = nullptr;
    char *pause_at = nullptr;
    for (int i=1; i<c; i++)
        if      (v[i][0] != '-' && !filename) filename = v[i];
        else if (strcmp(v[i], "--no-gui") == 0) enable_gui = false;
        else if (strcmp(v[i], "--realtime") == 0) realtime = true;
        else if (strcmp(v[i], "--async") == 0) async = true;
        else if (strcmp(v[i], "--no-realtime") == 0) realtime = false;
        else if (strcmp(v[i], "--no-fast-path")  == 0) fast_path  = false;
        else if (strcmp(v[i], "--no-plots") == 0) show_plots = false;
        else if (strcmp(v[i], "--no-depth") == 0) show_depth = false;
        else if (strcmp(v[i], "--no-video") == 0) show_video = false;
        else if (strcmp(v[i], "--no-main")  == 0) show_main  = false;
        else if (strcmp(v[i], "--pause")  == 0) start_paused  = true;
        else if (strcmp(v[i], "--pause-at")  == 0 && i+1 < c) pause_at = v[++i];
        else if (strcmp(v[i], "--render") == 0 && i+1 < c) rendername = v[++i];
        else if (strcmp(v[i], "--qvga") == 0) qvga = true;
        else if (strcmp(v[i], "--drop-depth") == 0) depth = false;
        else if (strcmp(v[i], "--save") == 0 && i+1 < c) save = v[++i];
        else if (strcmp(v[i], "--enable-map") == 0) enable_map = true;
        else if (strcmp(v[i], "--save-map") == 0 && i+1 < c) save_map = v[++i];
        else if (strcmp(v[i], "--load-map") == 0 && i+1 < c) load_map = v[++i];
        else if (strcmp(v[i], "--load") == 0 && i+1 < c) load = v[++i];
        else if (strcmp(v[i], "--benchmark") == 0) benchmark = true;
        else if (strcmp(v[i], "--benchmark-output") == 0 && i+1 < c) benchmark_output = v[++i];
        else if (strcmp(v[i], "--render-output") == 0 && i+1 < c) render_output = v[++i];
        else if (strcmp(v[i], "--calibrate") == 0) calibrate = true;
        else if (strcmp(v[i], "--zero-bias") == 0) zero_bias = true;
        else if (strcmp(v[i], "--progress") == 0) progress = true;
        else if (strcmp(v[i], "--incremental-ate") == 0) incremental_ate = true;
        else goto usage;

    if (!filename)
        goto usage;

    auto configure = [&](replay &rp, const char *capture_file) -> bool {
        if(qvga) rp.enable_qvga();
        if(!depth) rp.disable_depth();
        if(realtime) rp.enable_realtime();
        if(enable_map) rp.start_mapping();
        if(fast_path) rp.enable_fast_path();
        if(async) rp.enable_async();

        if(!rp.open(capture_file))
            return false;

        if(pause_at) {
            uint64_t pause_time = 0;
            try {
                pause_time = std::stoull(pause_at);
            } catch (...) {
                cerr << "invalid timestamp: " << pause_at << "\n";
                return false;
            }
            rp.set_pause(pause_time);
        }

        if (load) {
          if(!rp.load_calibration(load)) {
            cerr << "unable to load calibration: " << load << "\n";
            return false;
          }
        } else {
          if(!rp.set_calibration_from_filename(capture_file)) {
            cerr << "calibration not found: " << capture_file << ".json nor calibration.json\n";
            return false;
          }
        }

        if(zero_bias) rp.zero_biases();

        if(!rp.set_reference_from_filename(capture_file) && benchmark) {
            cerr << capture_file << ": unable to find a reference to measure against\n";
            return false;
        }

        return true;
    };

    auto print_results = [&calibrate](replay &rp, struct benchmark_result &res, const char *capture_file) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Reference Straight-line length is " << 100*rp.get_reference_length() << " cm, total path length " << 100*rp.get_reference_path_length() << " cm\n";
        std::cout << "Computed  Straight-line length is " << 100*rp.get_length()           << " cm, total path length " << 100*rp.get_path_length()           << " cm\n";
        std::cout << "Dispatched " << rp.get_packets_dispatched() << " packets " << rp.get_bytes_dispatched()/1.e6 << " Mbytes\n";
        if(res.ate.is_valid())
            res.ate.print_statistics();
        if(rc_getConfidence(rp.tracker) >= rc_E_CONFIDENCE_MEDIUM && calibrate) {
            std::cout << "Updating " << rp.calibration_file << "\n";
            rp.save_calibration(rp.calibration_file);
        }
        else
            std::cout << "Respected " << rp.calibration_file << "\n";
    };

    auto data_callback = [](world_state &ws, replay &rp, bool &first, struct benchmark_result &res, rc_Tracker *tracker, const rc_Data *data) {
        rc_PoseTime current_pose = rc_getPose(tracker, nullptr, nullptr, data->path);
        auto timestamp = sensor_clock::micros_to_tp(current_pose.time_us);
        // get/inerpolate gt at current timestamp
        tpose ref_pose(timestamp);
        bool success = rp.get_reference_pose(timestamp, ref_pose);
        if(success)
            ws.observe_position_gt(sensor_clock::tp_to_micros(ref_pose.t),
                                    ref_pose.G.T.x(), ref_pose.G.T.y(), ref_pose.G.T.z(),
                                    ref_pose.G.Q.w(), ref_pose.G.Q.x(), ref_pose.G.Q.y(), ref_pose.G.Q.z());

        if(data->path == rc_DATA_PATH_SLOW && first && success &&  rp.ground_truth_exists()) {
            first = false;
            //covert current from rc_Pose to pose
            tpose P{timestamp};
            P.G.Q = ::map(current_pose.pose_m.Q.v);
            P.G.T = ::map(current_pose.pose_m.T.v);
            // transform gt trajectory to tracker world frame
            rp.set_relative_pose(timestamp, P);
        }
        if(success) {
            // calculate ate at current position
            v3 T_current = ::map(current_pose.pose_m.T.v);
            res.ate.add_translation(T_current, ref_pose.G.T); // Accumulate information
            // incremental ate ?
            if (res.ate.is_incremental()) {
                res.ate.calculate_statistics(); // This is computationaly expensive
                ws.observe_ate(data->time_us,res.ate.rmse);
            }
        }
        ws.rc_data_callback(tracker, data);
    };

    if (benchmark) {
        enable_gui = false; if (realtime || start_paused) goto usage;

        std::ofstream benchmark_ofstream;
        std::ostream &stream = benchmark_output ? benchmark_ofstream.open(benchmark_output), benchmark_ofstream : std::cout;

        if (render_output)
            mkdir(render_output, 0777);

        benchmark_run(stream, filename,
        [&](const char *capture_file, struct benchmark_result &res) -> bool {
            auto rp_ = std::make_unique<replay>(start_paused); replay &rp = *rp_; // avoid blowing the stack when threaded or on Windows

            if (!configure(rp, capture_file)) return false;

            // clear result variables before processing a new dataset
            res.user_data = nullptr;
            res.ate.set_execution_type(incremental_ate);
            if ( rp.ground_truth_exists() || render_output) {
                world_state * ws = new world_state();
                res.user_data = ws;
                bool first = true;
                rp.set_data_callback([ws,&rp,&first,&res,&data_callback](rc_Tracker * tracker, const rc_Data * data) {
                    data_callback(*ws, rp, first, res, tracker, data);
                });
                if(render_output)
                    res.user_data = ws;
            }

            if (progress) std::cout << "Running  " << capture_file << std::endl;
            rp.start(load_map);
            if (progress) std::cout << "Finished " << capture_file << std::endl;

            //calculate horn's transform and print ATE statistics
            if (!res.ate.is_incremental() && rp.ground_truth_exists()) {
                res.ate.calculate_statistics();
            }

            res.length_cm.reference = 100*rp.get_reference_length();  res.path_length_cm.reference = 100*rp.get_reference_path_length();
            res.length_cm.measured  = 100*rp.get_length();            res.path_length_cm.measured  = 100*rp.get_path_length();

            if (progress) print_results(rp, res, capture_file);

            return true;
        },
        [&](const char *capture_file, struct benchmark_result &res) -> void {
#ifdef HAVE_GLFW
            if(render_output && res.user_data) {
                world_state * ws = (world_state *)res.user_data;
                std::string render_filename = render_filename_from_filename(filename, render_output, capture_file);

                if(!offscreen_render_to_file(render_filename.c_str(), ws))
                    cerr << "Failed to render " << render_filename << "\n";
                delete ws;
            }
#endif
        });
        return 0;
    }

    auto rp_ = std::make_unique<replay>(start_paused); replay &rp = *rp_; // avoid blowing the stack when threaded or on Windows

    if (!configure(rp, filename))
        return 2;

#ifndef HAVE_GLFW
    rp.start(load_map);
#else
    world_state ws;
    benchmark_result res;
    res.ate.set_execution_type(incremental_ate);
    bool first = true;
    if(enable_gui || rendername) {        
        rp.set_data_callback([&ws,&rp,&first,&res,&data_callback](rc_Tracker * tracker, const rc_Data * data) {
            data_callback(ws, rp, first, res, tracker, data);
        });
    }

    if(enable_gui) { // The GUI must be on the main thread
        gui vis(&ws, show_main, show_video, show_depth, show_plots);
        std::thread replay_thread([&](void) { rp.start(load_map); });
        vis.start(&rp);
        rp.stop();
        replay_thread.join();
    } else {
        rp.set_data_callback([&ws,&rp,&first,&res,&data_callback](rc_Tracker * tracker, const rc_Data * data) {
            data_callback(ws, rp, first, res, tracker, data);
        });
        rp.start(load_map);
    }

    if (!res.ate.is_incremental() && rp.ground_truth_exists()) {
        res.ate.calculate_statistics();
    }

    if(rendername && !offscreen_render_to_file(rendername, &ws)) {
        cerr << "Failed to render\n";
        return 1;
    }
    std::cout << ws.get_feature_stats();
#endif

    if (!save_map.empty()) {
        rp.save_map(save_map);
    }

    if (save)
        rp.save_calibration(save);

    std::cout << rc_getTimingStats(rp.tracker);
    print_results(rp,res,filename);
    return 0;
}
