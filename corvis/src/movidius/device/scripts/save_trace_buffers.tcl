puts "Trace buffers will be saved at exit"

namespace path [concat [namespace path] ::mdbg]
breset
displaystate verbose
cli::InternalErrorInfo


mdbg::dll::atexit {
    puts "saving trace buffers"
    target los
    #savefile ./output/tracebuff_lrt.bin lrt_traceBuffer
    savefile ./output/tracebuff_los.bin traceBuffer 
}
