#include "../filter/replay.h"
#include "../vis/world_state.h"
#include "../vis/offscreen_render.h"
#include "../vis/gui.h"

int main(int c, char **v)
{
    if (0) { usage:
        cerr << "Usage: " << v[0] << " [--pause] [--realtime] [--no-gui] [--no-plots] [--no-video] [--no-main] [--qvga] [--render <file.png>] <filename>\n";
        return 1;
    }

    world_state ws;

    bool realtime = false, start_paused = false;
    bool qvga = false;
    bool enable_gui = true, show_plots = false, show_video = true, show_depth = true, show_main = true;
    char *filename = nullptr, *rendername = nullptr;
    for (int i=1; i<c; i++)
        if      (v[i][0] != '-' && !filename) filename = v[i];
        else if (strcmp(v[i], "--no-gui") == 0) enable_gui = false;
        else if (strcmp(v[i], "--realtime") == 0) realtime = true;
        else if (strcmp(v[i], "--no-realtime") == 0) realtime = false;
        else if (strcmp(v[i], "--no-plots") == 0) show_plots = false;
        else if (strcmp(v[i], "--no-depth") == 0) show_depth = false;
        else if (strcmp(v[i], "--no-video") == 0) show_video = false;
        else if (strcmp(v[i], "--no-main")  == 0) show_main  = false;
        else if (strcmp(v[i], "--pause")  == 0) start_paused  = true;
        else if (strcmp(v[i], "--render") == 0 && i+1 < c) rendername = v[++i];
        else if (strcmp(v[i], "--qvga") == 0) qvga = true;
        else goto usage;

    if (!filename)
        goto usage;

    std::function<void (float)> progress;
    std::function<void (const filter *, camera_data &&)> camera_callback;

    replay rp(start_paused);
    gui vis(&ws, show_main, show_video, show_depth, show_plots);

    camera_callback = [&](const filter * f, camera_data &&d) {
        ws.receive_camera(f, std::move(d));
    };

    if(!rp.configure_all(filename, realtime, progress, camera_callback)) {
        cerr << filename << ": configure_all failed! (Check your calibration.json?)\n";
        return 2;
    }

    if(qvga) rp.enable_qvga();

    if (!rp.set_reference_from_filename(filename) && !enable_gui) {
        cerr << filename << ": unable to find a reference to measure against\n";
    }

    if(enable_gui) { // The GUI must be on the main thread
        std::thread replay_thread([&](void) { rp.start(); });
        vis.start(&rp);
        rp.stop();
        replay_thread.join();
        return 0;
    }
    else
        rp.start();

    if(rendername && !offscreen_render_to_file(rendername, &ws)) {
        cerr << "Failed to render\n";
        return 1;
    }

    std::cout << filename << std::endl;
    std::cout << ws.get_feature_stats();
    printf("Reference Straight-line length is %.2f cm, total path length %.2f cm\n", 100*rp.get_reference_length(), 100*rp.get_reference_path_length());
    printf("Computed  Straight-line length is %.2f cm, total path length %.2f cm\n", 100*rp.get_length(), 100*rp.get_path_length());
    printf("Dispatched %llu packets %.2f Mbytes\n", rp.get_packets_dispatched(), rp.get_bytes_dispatched()/1.e6);

    return 0;
}
