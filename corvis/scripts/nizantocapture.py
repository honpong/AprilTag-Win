from __future__ import print_function

def read_imu(file, type):
    import csv
    data = []
    with open(file, 'r') as f:
        for s in csv.DictReader(f):
            data.append({'time_us': int(s['ReportedTimestamp'])/1000, 'type': type, 'id': int(s['sensorID'])-1, 'value': [float(s[x]) for x in "XYZ"]})
    return data

def read_images(directory, type, id):
    import re, os
    data = []
    for png in os.listdir(directory):
        m = re.match("^IR.?_(?P<SeqID>\d+)_(?P<ReportedTimestamp>\d+).png$", png)
        if not m:
            print(png)
            continue
        data.append({ 'time_us': int(m.group('ReportedTimestamp'))/1000, 'type': type, 'id': id, 'filename': os.path.join(directory, png), 'exposure_us': 0 })
    return data

accel_type = 20
gyro_type = 21
image_raw_type = 29

# defined in rc_tracker.h
rc_IMAGE_GRAY8 = 0
rc_IMAGE_DEPTH16 = 1

def packets_write(data, output_filename):
    from PIL import Image
    from struct import pack
    with open(output_filename, "wb") as f:
      for d in data:
        if d['type'] == 'image':
            ptype, image_type = image_raw_type, rc_IMAGE_GRAY8
            im = Image.open(d['filename']).convert("L")
            w,h = im.size
            bytes = im.tobytes()
            assert w*h == len(bytes), "image {}x{} has wrong number of bytes {}".format(w,h,len(bytes))
            stride = w
            data = pack('QHHHH', d['exposure_us'], w, h, stride, image_type) + bytes
        elif d['type'] == 'gyro':
            ptype = gyro_type
            data = pack('fff', *d['value'])
        elif d['type'] == 'accel':
            ptype = accel_type
            data = pack('fff', *d['value'])
        else:
            print("Unexpected data type", type)
        pbytes = len(data) + 16
        header_str = pack('IHHQ', pbytes, ptype, int(d['id']), int(d['time_us']))
        f.write(header_str)
        f.write(data)

if __name__ == "__main__":
    import sys,os
    in_dir,out_file = sys.argv[1:]

    data = []
    data += read_imu(os.path.join(in_dir, "accel.csv"), 'accel')
    data += read_imu(os.path.join(in_dir, "gyro.csv"), 'gyro')
    data += read_images(os.path.join(in_dir, "IR"), 'image', 0)
    data += read_images(os.path.join(in_dir, "IR2"), 'image', 1)

    data.sort(key=lambda x: x['time_us'])

    packets_write(data, out_file)
