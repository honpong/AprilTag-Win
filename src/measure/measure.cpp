#include "replay.h"
#include "world_state.h"
#include "offscreen_render.h"
#include "gui.h"
#include "benchmark.h"
#include "rc_compat.h"
#include "gt_generator.h"
#include "file_stream.h"
#ifdef ENABLE_TM2_PLAYBACK
#include "tm2_host_stream.h"
#endif
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

        cerr << "Usage: " << v[0] << " { <filename> [--no-gui] | --benchmark <directory | filename1 [filename2 ...]> [--threads <n>] [--progress] | --benchmark }\n"
             << "   [--qvga] [--qres] [--drop-depth] [--realtime] [--async] [--no-fast-path] [--zero-bias]\n"
             << "   [--trace | --debug | --error | --info | --warn | --none] [--stats]\n"
             << "   [--pause] [--pause-at <timestamp_us>]\n"
             << "   [--skip <seconds>]\n"
             << "   [--show-no-plots] [--show-no-video] [--show-no-main] [--show-no-depth]\n"
             << "   [--render-output <file.png>] [--pose-output <pose-file>] [--benchmark-output <results-file>]\n"
             << "   [(--save | --load) <calibration-json>] [--append <calibration-append-json>] [--calibrate]\n"
             << "   [--disable-map] [--save-map <map-json>] [--load-map <map-json>]\n"
             << "   [--minimize-drops | --minimize-latency]\n"
             << "   [--dynamic-calibration]\n"
#ifdef ENABLE_TM2_PLAYBACK
             << "   [--tm2] [--show-no-map] [--show-no-feature] [--usb-sync]\n"
#endif
             << "   [--disable-relocalize] [--disable-odometry] [--incremental-ate]\n";
        return 1;
    }

    bool realtime = false, start_paused = false, benchmark = false, calibrate = false, zero_bias = false;
    bool progress = false;
    const char *save = nullptr, *load = nullptr, *append = nullptr;
    const char *save_map = nullptr, *load_map = nullptr;
    bool qvga = false, depth = true; int qres = 0;
    bool enable_gui = true, show_plots = false, show_video = true, show_depth = true, show_main = true;
    bool show_feature = true, show_map = true; // enabling displaying features or map when replaying over TM2
    bool enable_map = true;
    bool odometry = true;
    bool stats = false;
    rc_TrackerQueueStrategy queue_strategy = rc_QUEUE_MINIMIZE_DROPS;
    bool strategy_override = false;
    bool incremental_ate = false;
    bool relocalize = true;
    bool tm2_playback = false, usb_sync = false;
    char *rendername = nullptr, *benchmark_output = nullptr, *render_output = nullptr, *pose_output = nullptr;
    std::vector<const char*> filenames;
    char *pause_at = nullptr;
    float skip_secs = 0.f;
    rc_MessageLevel message_level = rc_MESSAGE_INFO;
    int threads = 0;
    int32_t run_flags = rc_RUN_SYNCHRONOUS | rc_RUN_FAST_PATH | rc_RUN_STATIC_CALIBRATION;
    for (int i=1; i<c; i++)
        if      (v[i][0] != '-') filenames.emplace_back(v[i]);
        else if (strcmp(v[i], "--no-gui") == 0) enable_gui = false;
        else if (strcmp(v[i], "--realtime") == 0) realtime = true;
        else if (strcmp(v[i], "--async") == 0) run_flags |= rc_RUN_ASYNCHRONOUS;
        else if (strcmp(v[i], "--no-realtime") == 0) realtime = false;
        else if (strcmp(v[i], "--no-fast-path")  == 0) run_flags &= ~(rc_RUN_FAST_PATH);
        else if (strcmp(v[i], "--dynamic-calibration") == 0) run_flags |= rc_RUN_DYNAMIC_CALIBRATION;
        else if (strcmp(v[i], "--show-no-plots") == 0) show_plots = false;
        else if (strcmp(v[i], "--show-no-depth") == 0) show_depth = false;
        else if (strcmp(v[i], "--show-no-video") == 0) show_video = false;
        else if (strcmp(v[i], "--show-no-main")  == 0) show_main  = false;
        else if (strcmp(v[i], "--threads") == 0 && i+1 < c) threads = std::atoi(v[++i]);
        else if (strcmp(v[i], "--pause")  == 0) start_paused  = true;
        else if (strcmp(v[i], "--pause-at")  == 0 && i+1 < c) pause_at = v[++i];
        else if (strcmp(v[i], "--skip") == 0 && i+1 < c) skip_secs = std::strtof(v[++i], nullptr);
        else if (strcmp(v[i], "--render") == 0 && i+1 < c) rendername = v[++i];
        else if (strcmp(v[i], "--qvga") == 0) qvga = true;
        else if (strcmp(v[i], "--qres") == 0) qres++;
        else if (strcmp(v[i], "--drop-depth") == 0) depth = false;
        else if (strcmp(v[i], "--disable-odometry") == 0) odometry = false;
        else if (strcmp(v[i], "--save") == 0 && i+1 < c) save = v[++i];
        else if (strcmp(v[i], "--disable-map") == 0) enable_map = false;
        else if (strcmp(v[i], "--save-map") == 0 && i+1 < c) save_map = v[++i];
        else if (strcmp(v[i], "--load-map") == 0 && i+1 < c) load_map = v[++i];
        else if (strcmp(v[i], "--load") == 0 && i+1 < c) load = v[++i];
        else if (strcmp(v[i], "--append") == 0 && i+1 < c) append = v[++i];
        else if (strcmp(v[i], "--benchmark") == 0) benchmark = true;
        else if (strcmp(v[i], "--benchmark-output") == 0 && i+1 < c) benchmark_output = v[++i];
        else if (strcmp(v[i], "--render-output") == 0 && i+1 < c) render_output = v[++i];
        else if (strcmp(v[i], "--pose-output") == 0 && i+1 < c) pose_output = v[++i];
        else if (strcmp(v[i], "--calibrate") == 0) calibrate = true;
        else if (strcmp(v[i], "--zero-bias") == 0) zero_bias = true;
        else if (strcmp(v[i], "--progress") == 0) progress = true;
        else if (strcmp(v[i], "--incremental-ate") == 0) incremental_ate = true;
        else if (strcmp(v[i], "--disable-relocalize") == 0) relocalize = false;
        else if (strcmp(v[i], "--trace") == 0) message_level = rc_MESSAGE_TRACE;
        else if (strcmp(v[i], "--debug") == 0) message_level = rc_MESSAGE_DEBUG;
        else if (strcmp(v[i], "--error") == 0) message_level = rc_MESSAGE_ERROR;
        else if (strcmp(v[i], "--info") == 0)  message_level = rc_MESSAGE_INFO;
        else if (strcmp(v[i], "--warn") == 0)  message_level = rc_MESSAGE_WARN;
        else if (strcmp(v[i], "--none") == 0)  message_level = rc_MESSAGE_NONE;
        else if (strcmp(v[i], "--minimize-drops") == 0) { queue_strategy = rc_QUEUE_MINIMIZE_DROPS; strategy_override = true; }
        else if (strcmp(v[i], "--minimize-latency") == 0) { queue_strategy = rc_QUEUE_MINIMIZE_LATENCY; strategy_override = true; }
        else if (strcmp(v[i], "--tm2") == 0)   tm2_playback = true;
        else if (strcmp(v[i], "--show-no-feature") == 0 ) show_feature = false;
        else if (strcmp(v[i], "--show-no-map") == 0) show_map = false;
        else if (strcmp(v[i], "--usb-sync") == 0) usb_sync = true;
        else if (strcmp(v[i], "--stats") == 0) stats = true;
        else goto usage;

    if (filenames.empty() || (filenames.size() > 1 && !benchmark))
        goto usage;

    auto replace = [](const char* hay, const char* needle, const char* text) {
        std::string str(hay);
        auto p = str.find(needle);
        if (p != std::string::npos)
            str.replace(p, std::strlen(needle), text);
        return str;
    };

    auto configure = [&](replay &rp, const char *capture_file) -> bool {
        if(!rp.init()) {
            cerr << "Error: failed to init streaming: " << capture_file << std::endl;
            return false;
        }
        rp.set_message_level(message_level);
        if(strategy_override) rp.set_queue_strategy(queue_strategy);
        if(qvga) rp.enable_qvga();
        if(qres) rp.enable_qres(qres);
        if(!depth) rp.disable_depth();
        if(odometry) rp.enable_odometry();
        if(realtime) rp.enable_realtime();
        if(enable_map) rp.start_mapping(relocalize, save_map != nullptr);
        if(usb_sync) rp.enable_usb_sync();
        rp.set_run_flags((rc_TrackerRunFlags)run_flags);
        if(!benchmark && enable_gui) {
            typedef replay_output::output_mode om;
            om mode = show_map ?    (show_feature ? om::POSE_FEATURE_MAP    : om::POSE_MAP) :
                                    (show_feature ? om::POSE_FEATURE        : om::POSE_ONLY);
            rp.set_replay_output_mode(mode);
        }
        if(pause_at) {
            rc_Timestamp pause_time = 0;
            try {
                pause_time = std::stoll(pause_at);
            } catch (...) {
                cerr << "invalid timestamp: " << pause_at << "\n";
                return false;
            }
            rp.set_pause(pause_time);
        }
        if(skip_secs != 0.f) {
            if (skip_secs < 0 || skip_secs == HUGE_VALF) {
                cerr << "invalid skip value\n";
                return false;
            }
            rp.delay_start(skip_secs * 1000000);
        }

        if(load) {
          if(!rp.load_calibration(replace(load, "%s", capture_file).c_str())) {
            cerr << "unable to load calibration: " << load << "\n";
            return false;
          }
          rp.calibration_file = load;
        } else {
          if(!rp.set_calibration_from_filename(capture_file)) {
            cerr << "calibration not found: " << capture_file << ".json nor calibration.json\n";
            return false;
          }
        }

        if(zero_bias) rp.zero_biases();

        if(append) {
          if(!rp.append_calibration(append)) {
            cerr << "unable to append calibration: " << append << "\n";
            return false;
          }
        }

        if(!rp.set_reference_from_filename(capture_file) && benchmark) {
            cerr << capture_file << ": unable to find a reference to measure against\n";
            return false;
        }

        if(load_map) {
            std::string map_filename = replace(load_map, "%s", capture_file);
            rp.load_map(map_filename.c_str());
            if(!rp.set_loaded_map_reference_from_file(map_filename.c_str()) && benchmark) {
                cerr << load_map << ": unable to find a reference to measure against a loaded map\n";
            }
        }
        return true;
    };

    auto print_results = [&calibrate,&replace,relocalize](replay &rp, struct benchmark_result &res, const char *capture_file, rc_TrackerConfidence tracking_confidence) {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Reference Straight-line length is " << 100*rp.get_reference_length() << " cm, total path length " << 100*rp.get_reference_path_length() << " cm\n";
        std::cout << "Computed  Straight-line length is " << res.length_cm.measured        << " cm, total path length " << res.path_length_cm.measured        << " cm\n";
        std::cout << "Dispatched " << rp.get_packets_dispatched() << " packets " << rp.get_bytes_dispatched()/1.e6 << " Mbytes\n";
        if (res.errors.calculate_ate()) {
            std::cout << "Trajectory Statistics :\n";
            std::cout << "\t ATE [m] : \n";
            std::cout << res.errors.ate << "\n";
        }
        if (res.errors.calculate_rpe_600ms()) {
            std::cout << "\t RPE translation (600ms) [m]:\n";
            std::cout << res.errors.rpe_T << "\n";
            std::cout << "\t RPE rotation (600ms) [deg]:\n";
            std::cout << res.errors.rpe_R*(f_t)(180/M_PI) << "\n";
        }
        if (res.errors.calculate_ate_60s()) {
            std::cout << "\t ATE (60s) [m] : \n";
            std::cout << res.errors.ate_60s << "\n";
        }
        if (res.errors.calculate_ate_600ms()) {
            std::cout << "\t ATE (600ms) [m] : \n";
            std::cout << res.errors.ate_600ms << "\n";
        }
        if (relocalize) {
            std::cout << "Relocalization Statistics :\n";
            if (res.errors.calculate_precision_recall()) {
                std::cout << "\t Precision-Recall [%] : \n";
                std::cout << res.errors.relocalization << "\n";
                std::cout << "\t translation RPE [m]:\n";
                std::cout << res.errors.reloc_rpe_T << "\n";
                std::cout << "\t rotation RPE [deg]:\n";
                std::cout << res.errors.reloc_rpe_R*(f_t)(180/M_PI) << "\n";
            } else {
                std::cout << res.errors.relocalization << "\n";
            }
            std::cout << "\t time between relocalizations [sec]:\n";
            std::cout << res.errors.reloc_time_sec << "\n";
        }

        if (std::any_of(std::begin(res.storage.items), std::end(res.storage.items), [](auto i) { return i > 0; }))
            std::cout << "Storage Statistics :\n" << res.storage << "\n";

        if(tracking_confidence >= rc_E_CONFIDENCE_MEDIUM && calibrate) {
            std::cout << "Updating " << rp.calibration_file << "\n";
            rp.save_calibration(replace(rp.calibration_file.c_str(), "%s", capture_file).c_str());
        }
        else
            std::cout << "Respected " << rp.calibration_file << "\n";
    };

    auto data_callback = [&enable_gui, &incremental_ate, &render_output, &run_flags]
        (world_state &ws, replay &rp, bool &first, struct benchmark_result &res, gt_generator &loop_gt, const replay_output *rp_output, const rc_Data *data, std::ostream *pose_st) {
        rc_PoseTime current_pose = rp_output->rc_getPose(rp_output->data_path);

        auto timestamp = sensor_clock::micros_to_tp(current_pose.time_us);
        tpose ref_tpose(timestamp), current_tpose(timestamp, to_transformation(current_pose.pose_m));
        bool has_reference = rp.get_reference_pose(timestamp, ref_tpose);
        if (has_reference) {
            if (enable_gui || render_output)
                ws.observe_position_gt(sensor_clock::tp_to_micros(ref_tpose.t),
                                       ref_tpose.G.T.x(), ref_tpose.G.T.y(), ref_tpose.G.T.z(),
                                       ref_tpose.G.Q.w(), ref_tpose.G.Q.x(), ref_tpose.G.Q.y(), ref_tpose.G.Q.z());
        }

        if(has_reference && rp_output->data_path == ((run_flags & rc_RUN_FAST_PATH) ? rc_DATA_PATH_FAST : rc_DATA_PATH_SLOW)) {
            if (first) {
                first = false;
                loop_gt.set_camera(rp.get_camera_extrinsics(0));
                if (rp.get_loaded_map_reference_poses())
                    loop_gt.add_reference_poses(*rp.get_loaded_map_reference_poses(), rc_SESSION_PREVIOUS_SESSION);
                loop_gt.add_reference_poses(rp.get_reference_poses(), rc_SESSION_CURRENT_SESSION);
                // transform reference trajectory to tracker world frame
                rp.set_relative_pose(timestamp, current_tpose);
                ref_tpose.G = current_tpose.G;
            }
            res.errors.add_pose(current_tpose,ref_tpose);
            if (incremental_ate) {
                res.errors.calculate_ate();
                if (enable_gui || render_output)
                    ws.observe_ate(rp_output->sensor_time_us, res.errors.ate.rmse);
            }
            if(res.errors.chunks_600ms.rpe_T_chunk_results.size())
                ws.observe_rpe(data->time_us, res.errors.chunks_600ms.rpe_T_chunk_results.back());
        }
        if(!first && rp_output->tracker && data->type == rc_SENSOR_TYPE_IMAGE) {
            rc_RelocEdge* reloc_edges = nullptr;
            rc_Timestamp reloc_source;
            int num_reloc_edges = rc_getRelocalizationEdges(rp_output->tracker, &reloc_source, &reloc_edges);
            if (reloc_source) {
                rc_MapNode* map_nodes = nullptr;
                int num_mapnodes = rc_getMapNodes(rp_output->tracker, &map_nodes);
                loop_gt.update_map(map_nodes, num_mapnodes);
                res.errors.add_edges(reloc_source,
                                     reloc_edges,
                                     num_reloc_edges,
                                     loop_gt.get_reference_edges(reloc_source),
                                     loop_gt.get_reference_poses());
            }
        }
        const int device_id = 0;
        if(pose_st && rp_output->data_path == ((run_flags & rc_RUN_FAST_PATH) ? rc_DATA_PATH_FAST : rc_DATA_PATH_SLOW))
            *pose_st << tpose_tum(current_pose.time_us/1e6, to_transformation(current_pose.pose_m), device_id, rp_output->confidence);
        if (enable_gui || render_output)
            ws.rc_data_callback(rp_output, data);
        if (enable_gui && rp_output->sensor_type == rc_SENSOR_TYPE_DEBUG && data->debug.pause)
            rp.pause();
        res.path_length_cm.measured = 100 * rp_output->path_length;
        res.length_cm.measured      = 100 * v3(rp_output->rc_getPose(rc_DATA_PATH_SLOW).pose_m.T.v).norm();
    };

    auto relocalization_callback = [](struct benchmark_result &res, const rc_Relocalization &reloc) {
        res.errors.add_relocalization(reloc);
    };

    if (benchmark) {
        enable_gui = false; if (realtime || start_paused) goto usage;
        threads = tm2_playback ? 1 : threads; // support only one thread on TM2
        std::ofstream benchmark_ofstream;
        std::ostream &stream = benchmark_output ? benchmark_ofstream.open(benchmark_output), benchmark_ofstream : std::cout;

        if (render_output)
            mkdir(render_output, 0777);

        benchmark_run(stream, filenames, threads,
        [&, confidence=rc_TrackerConfidence::rc_E_CONFIDENCE_NONE](const char *capture_file, struct benchmark_result &res) mutable -> bool {
            auto rp_ = std::make_unique<replay>(  // avoid blowing the stack when threaded or on Windows
#ifdef  ENABLE_TM2_PLAYBACK
                tm2_playback ? new tm2_host_stream(capture_file) :
#endif
                (host_stream *)(new file_stream(capture_file)), start_paused);
            replay &rp = *rp_;
            if (!configure(rp, capture_file)) return false;

            world_state * ws = new world_state();
            res.user_data = render_output ? ws : nullptr;

            gt_generator loop_gt;
            std::ofstream pose_fs; if (pose_output) pose_fs.open(replace(pose_output, "%s", capture_file).c_str());

            rp.set_data_callback([ws,&rp,first=true,&res, &loop_gt,&data_callback,&pose_fs,&confidence](const replay_output * output, const rc_Data * data) mutable {
                confidence = output->confidence;
                data_callback(*ws, rp, first, res, loop_gt, output, data, &pose_fs);
            });
            rp.set_relocalization_callback([&res,&relocalization_callback](const rc_Relocalization* reloc) {
                relocalization_callback(res, *reloc);
            });

            if (progress) std::cout << "Running  " << capture_file << std::endl;
            rp.start();
            if (progress) std::cout << "Finished " << capture_file << std::endl;
            res.length_cm.reference = 100*rp.get_reference_length();res.path_length_cm.reference = 100*rp.get_reference_path_length();
            res.storage = rp.get_storage_stat();

            if (progress) print_results(rp, res, capture_file, confidence);
            if (save_map) rp.save_map(replace(save_map, "%s", capture_file).c_str());
            if (save) rp.save_calibration(replace(save, "%s", capture_file).c_str());

            rp.end();
            return true;
        },
        [&](const char *capture_file, struct benchmark_result &res) -> void {
#ifdef HAVE_GLFW
            if(render_output && res.user_data) {
                world_state * ws = (world_state *)res.user_data;
                std::string render_filename = render_filename_from_filename(filenames[0], render_output, capture_file);

                if(!offscreen_render_to_file(render_filename.c_str(), ws))
                    cerr << "Failed to render " << render_filename << "\n";
                delete ws;
            }
#endif
        });
        return 0;
    }
    auto filename = filenames[0];
    auto rp_ = std::make_unique<replay>(
#ifdef  ENABLE_TM2_PLAYBACK
        tm2_playback ? new tm2_host_stream(filename) :
#endif
        (host_stream *)(new file_stream(filename)), start_paused);
    replay &rp = *rp_; // avoid blowing the stack when threaded or on Windows

    if (!configure(rp, filename))
        return 2;

    benchmark_result res;
    world_state ws;
    gt_generator loop_gt;
    std::ofstream pose_fs; if (pose_output) pose_fs.open(replace(pose_output, "%s", filename));
    rc_TrackerConfidence confidence = rc_E_CONFIDENCE_NONE;

    rp.set_data_callback([&ws,&rp,first=true,&res, &loop_gt,&data_callback,&pose_fs,&confidence](const replay_output * output, const rc_Data * data) mutable {
        confidence = output->confidence;
        data_callback(ws, rp, first, res, loop_gt, output, data, &pose_fs);
    });
    rp.set_relocalization_callback([&res,&relocalization_callback](const rc_Relocalization* reloc) {
        relocalization_callback(res, *reloc);
    });

#ifndef HAVE_GLFW
    rp.start();
#else
    if(enable_gui) { // The GUI must be on the main thread
        gui vis(&ws, show_main, show_video, show_depth, show_plots);
        rp.start_async();
        vis.start(&rp);
        rp.stop();
    } else {
        rp.start();
    }

    if(rendername && !offscreen_render_to_file(rendername, &ws)) {
        cerr << "Failed to render\n";
        return 1;
    }
#endif
    if (stats)
        std::cout << ws.get_feature_stats();

    res.storage = rp.get_storage_stat();

    if (save_map) rp.save_map(replace(save_map, "%s", filename).c_str());
    if (save) rp.save_calibration(replace(save, "%s", filename).c_str());

    if (stats)
        std::cout << rp.get_track_stat();

    print_results(rp,res,filename,confidence);
    rp.end();
    return 0;
}
