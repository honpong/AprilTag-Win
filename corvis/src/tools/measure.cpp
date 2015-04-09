#include "../filter/replay.h"

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        cerr << "Usage: replay filename device.\n";
        return -1;
    }
    replay rp;
    
    
    if(!rp.configure_all(argv[1], argv[2])) return -1;
    
    rp.runloop();
    
    return 0;
}
