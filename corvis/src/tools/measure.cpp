#include "../filter/replay.h"
#include "../vis/world_state.h"
#include "../vis/offscreen_render.h"
#include "../vis/gui.h"

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        cerr << "Usage: " << argv[0] << " filename devicename\n";
        return -1;
    }
    replay rp;
    
    
    world_state ws;

    bool realtime = false;
    bool enable_offscreen_render = false;
    bool enable_gui = false;

    if(enable_gui)
        realtime = true;

    std::function<void (float)> progress = NULL;
    std::function<void (const filter *, enum packet_type)> packet = NULL;

    gui vis(&ws);

    // TODO: make this a command line option
    // For command line visualization
    if(enable_offscreen_render || enable_gui)
        packet = [&](const filter * f, enum packet_type packet_type) {
            ws.receive_packet(f, packet_type);
        };

    if(!rp.configure_all(argv[1], argv[2], realtime, progress, packet)) return -1;
    
    if(enable_gui) { // The GUI must be on the main thread
        std::thread replay_thread([&](void) { rp.start(); });
        vis.start();
        replay_thread.join();
    }
    else
        rp.start();

    if(enable_offscreen_render && !offscreen_render_to_file("render.png", &ws))
        cerr << "Failed to render\n";

    float length = rp.get_length();
    float path_length = rp.get_path_length();
    uint64_t packets_dispatched = rp.get_packets_dispatched();
    uint64_t bytes_dispatched = rp.get_bytes_dispatched();
    printf("Straight-line length is %.2f cm, total path length %.2f cm\n", length, path_length);
    printf("Dispatched %llu packets %.2f Mbytes\n", packets_dispatched, bytes_dispatched/1.e6);

    return 0;
}
