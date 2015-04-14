#include "../filter/replay.h"

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        cerr << "Usage: " << argv[0] << " filename devicename\n";
        return -1;
    }
    replay rp;
    
    
    if(!rp.configure_all(argv[1], argv[2])) return -1;
    
    rp.start();

    float length = rp.get_length();
    float path_length = rp.get_path_length();
    uint64_t packets_dispatched = rp.get_packets_dispatched();
    uint64_t bytes_dispatched = rp.get_bytes_dispatched();
    fprintf(stderr, "Straight-line length is %.2f cm, total path length %.2f cm\n", length, path_length);
    fprintf(stderr, "Dispatched %llu packets %.2f Mbytes\n", packets_dispatched, bytes_dispatched/1.e6);

    return 0;
}
