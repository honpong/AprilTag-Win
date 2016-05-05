#include "../filter/replay.h"
#include "../filter/calibration_json.h"
#include "../vis/world_state.h"
#include "../vis/offscreen_render.h"
#include "../vis/gui.h"
#include "benchmark.h"
#include <iomanip>

int main(int c, char **v)
{
    if (0) { usage:
        cerr << "Usage: " << v[0] << " [--qvga] [--drop-depth] [--intel] [--realtime] [--pause] [--pause-at <timestamp_us>][--no-gui] [--no-plots] [--no-video] [--no-main] [--render <file.png>] [(--save | --load) <calibration-json>] [--enable-map] [--save-map <map-json>] [--load-map <map-json>] <filename>\n";
        cerr << "       " << v[0] << " [--qvga] [--drop-depth] [--intel] --benchmark <directory>\n";
        return 1;
    }

    bool realtime = false, start_paused = false, benchmark = false, intel = false, calibrate = false, zero_bias = false;
    const char *save = nullptr, *load = nullptr;
    std::string save_map, load_map;
    bool qvga = false, depth = true;
    bool enable_gui = true, show_plots = false, show_video = true, show_depth = true, show_main = true;
    bool enable_map = false;
    char *filename = nullptr, *rendername = nullptr, *benchmark_output = nullptr;
    char *pause_at = nullptr;
    for (int i=1; i<c; i++)
        if      (v[i][0] != '-' && !filename) filename = v[i];
        else if (strcmp(v[i], "--no-gui") == 0) enable_gui = false;
        else if (strcmp(v[i], "--realtime") == 0) realtime = true;
        else if (strcmp(v[i], "--intel") == 0) intel = true;
        else if (strcmp(v[i], "--no-realtime") == 0) realtime = false;
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
        else if (strcmp(v[i], "--calibrate") == 0) calibrate = true;
        else if (strcmp(v[i], "--zero-bias") == 0) zero_bias = true;
        else goto usage;

    if (!filename)
        goto usage;

    auto configure = [&](replay &rp, const char *capture_file) -> bool {
        if(qvga) rp.enable_qvga();
        if(!depth) rp.disable_depth();
        if(realtime) rp.enable_realtime();
        if(intel) rp.enable_intel();
        if(enable_map) rp.start_mapping();

        if(!rp.open(capture_file))
            return false;

        if(pause_at) {
            uint64_t pause_time = 0;
            try {
                pause_time = stoull(string(pause_at));
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

    auto update_calibration = [](replay &rp, const std::string &file) {
        std::string json;
        if (calibration_serialize(rp.get_device_parameters(), json)) {
            std::ofstream out(file);
            out << json;
        }
    };

    auto print_results = [&calibrate, &update_calibration](replay &rp, const char *capture_file) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Reference Straight-line length is " << 100*rp.get_reference_length() << " cm, total path length " << 100*rp.get_reference_path_length() << " cm\n";
        std::cout << "Computed  Straight-line length is " << 100*rp.get_length()           << " cm, total path length " << 100*rp.get_path_length()           << " cm\n";
        std::cout << "Dispatched " << rp.get_packets_dispatched() << " packets " << rp.get_bytes_dispatched()/1.e6 << " Mbytes\n";
        if      (rp.fusion.sfm.detector_failed)  std::cout << "Detector failed, not updating calibration\n";
        else if (rp.fusion.sfm.tracker_failed)   std::cout << "Tracker failed, not updating calibration\n";
        else if (rp.fusion.sfm.speed_failed)     std::cout << "Too fast, not updating calibration\n";
        else if (rp.fusion.sfm.numeric_failed)   std::cout << "Numeric error, not updating calibration\n";
        else if (rp.fusion.sfm.calibration_bad)  std::cout << "Bad calibration, not updating calibration\n";
        else if (calibrate) { update_calibration(rp, rp.calibration_file); std::cout << "Updating " << rp.calibration_file << "\n"; }
        else                                     std::cout << "Respected " << rp.calibration_file << "\n";
    };

    if (benchmark) {
        enable_gui = false; if (realtime || start_paused) goto usage;

        std::ofstream benchmark_ofstream;
        std::ostream &stream = benchmark_output ? benchmark_ofstream.open(benchmark_output), benchmark_ofstream : std::cout;

        benchmark_run(stream, filename, [&](const char *capture_file, struct benchmark_result &res) -> bool {
            auto rp_ = std::make_unique<replay>(start_paused); replay &rp = *rp_; // avoid blowing the stack when threaded or on Windows

            if (!configure(rp, capture_file)) return false;

            std::cout << "Running  " << capture_file << std::endl;
            rp.start(load_map);
            std::cout << "Finished " << capture_file << std::endl;

            res.length_cm.reference = 100*rp.get_reference_length();  res.path_length_cm.reference = 100*rp.get_reference_path_length();
            res.length_cm.measured  = 100*rp.get_length();            res.path_length_cm.measured  = 100*rp.get_path_length();

            print_results(rp, capture_file);

            return true;
        });
        return 0;
    }

    auto rp_ = std::make_unique<replay>(start_paused); replay &rp = *rp_; // avoid blowing the stack when threaded or on Windows

    if (!configure(rp, filename))
        return 2;

#if defined(ANDROID)
    rp.start(load_map);
#else
    world_state ws;
    if(enable_gui || rendername) {
        rp.set_camera_callback([&](const filter * f, image_gray8 &&d) {
            ws.receive_camera(f, std::move(d));
        });
    }

    if(enable_gui) { // The GUI must be on the main thread
        gui vis(&ws, show_main, show_video, show_depth, show_plots);
        std::thread replay_thread([&](void) { rp.start(load_map); });
        vis.start(&rp);
        rp.stop();
        replay_thread.join();
    } else
        rp.start(load_map);

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
        update_calibration(rp, save);

    print_results(rp, filename);
    return 0;
}
