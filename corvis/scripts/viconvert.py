#!/usr/bin/env python
# Convert accel.txt, gyro.txt to ViCalib format: accel.csv, gyro.csv, time.csv

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
            a, g = gyro[gi], accel[ai]
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

def convert_images(dirname, data, last_imu_timestamp):
    import os, errno
    try:
        os.mkdir(dirname, 0777)
    except OSError as e:
        if e.errno == errno.EEXIST and os.path.isdir(dirname):
            pass
    for t, n in data:
        if t/1000 >= last_imu_timestamp:
            break
        f = "{:s}_{:015.7f}.pgm".format(dirname, t/1000)
        try:
            os.unlink(f)
        except:
            pass
        os.symlink(os.path.join("..", n), os.path.join(dirname, f))

def read_images(filename):
    d = []
    with open(filename, 'rb') as f:
        for line in f.xreadlines():
            n,t = line.split()
            d.append((float(t), n))
    return d

def read_imu(filename):
    d = []
    with open(filename, 'rb') as f:
        for line in f.xreadlines():
            (t,x,y,z) = line.split(",")
            d.append(map(float,(t,x,y,z)))
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
    fe = read_images("fisheye_timestamps.txt")
    convert_images("vifisheye", fe, tm[-1][0])
    write_csv("vifisheye/timestamp.txt", tm)
    write_csv("vifisheye/accel.txt", am)
    write_csv("vifisheye/gyro.txt", gm)
