#!/usr/bin/env python
# Convert accel.txt, gyro.txt to ViCalib format: accel.csv, gyro.csv, time.csv

# from http://stackoverflow.com/a/28382515 for windows symlink support
import os
if os.name == "nt":
    def symlink_ms(source, link_name):
        import ctypes
        csl = ctypes.windll.kernel32.CreateSymbolicLinkW
        csl.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32)
        csl.restype = ctypes.c_ubyte
        flags = 1 if os.path.isdir(source) else 0
        try:
            if csl(link_name, source.replace('/', '\\'), flags) == 0:
                raise ctypes.WinError()
        except:
            pass
    os.symlink = symlink_ms


def merge(accel,gyro):
    tm, am, gm = [], [], []
    ai, gi = 0, 0
    while ai < len(accel) and gi < len(gyro):
        a, g = accel[ai], gyro[gi]
        if accel[ai][0] < gyro[gi][0]:
            if gi > 0:
                g, a, G = gyro[gi-1], accel[ai], gyro[gi]
                tm.append((a[0]/1000,0))
                am.append((a[1],
                           a[2],
                           a[3]))
                gm.append((g[1] + (a[0]-g[0])/(G[0]-g[0]) * (G[1] - g[1]),
                           g[2] + (a[0]-g[0])/(G[0]-g[0]) * (G[2] - g[2]),
                           g[3] + (a[0]-g[0])/(G[0]-g[0]) * (G[3] - g[3])))
            ai = ai + 1
        elif accel[ai][0] > gyro[gi][0]:
            if ai > 0:
                a, g, A = accel[ai-1], gyro[gi], accel[ai]
                tm.append((g[0]/1000,0))
                am.append((a[1] + (g[0]-a[0])/(A[0]-a[0]) * (A[1] - a[1]),
                           a[2] + (g[0]-a[0])/(A[0]-a[0]) * (A[2] - a[2]),
                           a[3] + (g[0]-a[0])/(A[0]-a[0]) * (A[3] - a[3])))
                gm.append((g[1],
                           g[2],
                           g[3]))
            gi = gi + 1
        else:
            g, a = gyro[gi], accel[ai]
            tm.append((g[0]/1000,0))
            am.append((a[1],
                       a[2],
                       a[3]))
            gm.append((g[1],
                       g[2],
                       g[3]))
            gi = gi + 1
            ai = ai + 1
    return tm, am, gm

def ensure_dir(dirname):
    import errno
    try:
        os.mkdir(dirname, 0o777)
    except OSError as e:
        if e.errno == errno.EEXIST and os.path.isdir(dirname):
            pass

def convert_images(dirname, sensor_id, data, last_imu_timestamp):
    for t, n in data:
        if t/1000 >= last_imu_timestamp:
            break
        f = "{:s}_{:d}_{:015.7f}.pgm".format(dirname, sensor_id, t/1000)
        try:
            os.unlink(f)
        except:
            pass
        os.symlink(os.path.join("..", n), os.path.join(dirname, f))

def read_images(filename):
    d = []
    with open(filename, 'r') as f:
        for line in f:
            n,t = line.split()
            d.append((float(t), n))
    return d

def read_imu(filename):
    d = []
    with open(filename, 'r') as f:
        for line in f:
            (t,x,y,z) = line.split(",")
            d.append(list(map(float,(t,x,y,z))))
    return d

def write_csv(filename, data):
    with open(filename, 'w') as f:
        for d in data:
            f.write(", ".join(map(str,d))+"\n")

if __name__ == '__main__':
    import sys
    import os.path
    assert len(sys.argv) == 2, "Usage: " + sys.argv[0] + ": <directory-with-accel.txt-and-gyro.txt>"
    os.chdir(sys.argv[1])
    tm, am, gm = merge(read_imu("accel.txt"),
                       read_imu("gyro.txt"))
    ensure_dir("vifisheye")
    if os.path.isfile("fisheye_0_timestamps.txt"):
        fe0 = read_images("fisheye_0_timestamps.txt")
        convert_images("vifisheye", 0, fe0, tm[-1][0])
        fe1 = read_images("fisheye_1_timestamps.txt")
        convert_images("vifisheye", 1, fe1, tm[-1][0])
    else:
        fe = read_images("fisheye_timestamps.txt")
        convert_images("vifisheye", fe, tm[-1][0])
    write_csv("vifisheye/timestamp.txt", tm)
    write_csv("vifisheye/accel.txt", am)
    write_csv("vifisheye/gyro.txt", gm)
