#include "../filter/replay.h"
#include "../vis/world_state.h"
#include "../vis/offscreen_render.h"

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
    std::function<void (float)> progress = NULL;

    // TODO: make this a command line option
    // For command line visualization
    std::function<void (const filter *, enum packet_type)> packet = NULL;
    if(enable_offscreen_render)
        packet = [&](const filter * f, enum packet_type packet_type) {
            if(packet_type == packet_camera) {
                // Only update position and features on packet camera,
                // matches what we do in other visualizations
                for(auto feat : f->s.features) {
                    if(feat->is_valid()) {
                        float stdev = feat->v.stdev_meters(sqrt(feat->variance()));
                        bool good = stdev / feat->v.depth() < .02;
                        ws.observe_feature(f->last_time, feat->id,
                                feat->world[0], feat->world[1], feat->world[2], good);
                    }
                }

                v4 T = f->s.T.v;
                quaternion q = to_quaternion(f->s.W.v);
                ws.observe_position(f->last_time, T[0], T[1], T[2], q.w(), q.x(), q.y(), q.z());
            }
        };

    if(!rp.configure_all(argv[1], argv[2], realtime, progress, packet)) return -1;
    
    rp.start();

    if(enable_offscreen_render && !offscreen_render_to_file("render.png", ws))
        cerr << "Failed to render\n";

    float length = rp.get_length();
    float path_length = rp.get_path_length();
    uint64_t packets_dispatched = rp.get_packets_dispatched();
    uint64_t bytes_dispatched = rp.get_bytes_dispatched();
    fprintf(stderr, "Straight-line length is %.2f cm, total path length %.2f cm\n", length, path_length);
    fprintf(stderr, "Dispatched %llu packets %.2f Mbytes\n", packets_dispatched, bytes_dispatched/1.e6);

    return 0;
}
