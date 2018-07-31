#!/usr/bin/env python
from __future__ import print_function
import sys
from packet import Packet, PacketType
from struct import unpack
from subprocess import Popen, PIPE
import argparse

parser = argparse.ArgumentParser(description='Generate a video from a capture.')
parser.add_argument('capture_filename', type=str,                   help='Path to the input rc file')
parser.add_argument('video_filename',   type=str,                   help='Path to the output video file')
parser.add_argument('--max-width',      type=int, default=-1,       help='Constrain video to max-width pixels')
parser.add_argument('--max-height',     type=int, default=-1,       help='Constrain video to max-height pixels')
parser.add_argument('--sensor-id',      type=int, default=0,        help='The id of the video sensor to use')
parser.add_argument('--start-at',       type=int, default=-1,       help='Start at START_AT microseconds')
parser.add_argument('--end-at',         type=int, default=-1,       help='End at END_AT microseconds')
parser.add_argument('--ffmpeg-path',    type=str, default='ffmpeg', help='Path to ffmpeg binary (default "ffmpeg")')
args = parser.parse_args()

rc_IMAGE_GRAY8 = 0

# returns the width, height, and framerate of the capture from the first 30 frames
def peek_capture(filename, sensor_id):
    (width, height, fps) = (None, None, None)
    frames = []
    
    f = open(filename, 'rb')
    p = Packet.from_file(f)
    while p is not None:
        if (p.header.type == PacketType.stereo_raw or p.header.type == PacketType.image_raw) and p.header.sensor_id == sensor_id:
            (exposure, width, height, stride) = unpack('QHHH', p.data[:14]) # stereo_raw and image_raw start the same
            frames.append(p.timestamp())
            if len(frames) == 30:
                break
        p = Packet.from_file(f)

    f.close()
    fps = 1e6/(frames[-1] - frames[-2])
    return (width, height, fps)

(file_width, file_height, fps) = peek_capture(args.capture_filename, args.sensor_id)
dimensions = "%dx%d" % (file_width, file_height)
rate = "%.2f" % (fps)
print("Found video with dimensions", dimensions, "@", rate)

f = open(args.capture_filename, 'rb')

cmd = [args.ffmpeg_path,
        '-f', 'rawvideo',
        '-pixel_format', 'gray',
        '-s', dimensions,
        '-framerate', rate,
        '-i', 'pipe:0',
        '-an', # no audio
        '-y' # overwrite
        ]

vf_line = 'format=yuv420p' # quicktime only supports yuv420p
if args.max_width != -1 or args.max_height != -1:
    # pad scaled down odd dimensions with 1 pixel so yuv420 works
    vf_line += ", scale=w=%d:h=%d:force_original_aspect_ratio=decrease, pad=w=ceil(iw/2)*2:h=ceil(ih/2)*2" % (args.max_width, args.max_height)

cmd.extend(['-vf', vf_line, args.video_filename])
print("Running", cmd)

ffmpeg_pipe = Popen(cmd, stdin=PIPE, bufsize=848*800*2)

p = Packet.from_file(f)
i = 0
while p is not None:
    if (args.start_at == -1 or p.timestamp() > args.start_at) and \
       (p.header.type == PacketType.stereo_raw or p.header.type == PacketType.image_raw) and \
       p.header.sensor_id == args.sensor_id:
        (exposure, width, height, stride0) = unpack('QHHH', p.data[:14])
        assert(stride0 == width and file_width == width and file_height == height)
        if p.header.type == PacketType.stereo_raw:
            (stride1, camera_format) = unpack('HH', p.data[14:18])
            assert(stride0 == stride1 and camera_format == rc_IMAGE_GRAY8)
            ffmpeg_pipe.stdin.write(p.data[18:18+width*height])
        else:
            ffmpeg_pipe.stdin.write(p.data[16:16+width*height])
        i += 1
    if (args.end_at != -1 and p.timestamp() > args.end_at):
        break
    p = Packet.from_file(f)

ffmpeg_pipe.stdin.close()
ffmpeg_pipe.wait()
print("Wrote", i, "frames")
