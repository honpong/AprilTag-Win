# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

from corvis import cor

class image_dump:
    def __init__(self):
        self.output_image = 0
        self.do_output = False

    def output_images(self, packet):
        if not self.do_output:
            return
        if packet.header.type == cor.packet_camera:
            path = replay_file + "_images/"
            if not os.path.exists(path):
                os.makedirs(path)
            outfn = path + "/" + str(self.output_image) + ".pgm"
            self.output_image += 1
            cor.packet_camera_write_image(packet,outfn)
            self.do_output = False

class progress_printer:
    last_time = 0
    last_percent = 0

    def __init__(self, dispatch):
        self.dispatch = dispatch

    def print_progress(self, packet):
        if self.dispatch.mb == None: return
        packet_time = packet.header.time / 1000000.
        progress_percent = 100 * self.dispatch.bytes_dispatched / self.dispatch.mb.total_bytes
        force_print = (progress_percent == 100 and self.last_percent != 100)

        if (packet_time - self.last_time > 1. and progress_percent -
          self.last_percent > 1) or force_print:
            self.last_time = packet_time
            self.last_percent = progress_percent
            print "%3.0f%% replayed" % progress_percent

class measurement_printer:
    last_time = 0
    def __init__(self, sfm):
        self.sfm = sfm

    def print_measurement(self, packet):
        if packet.header.type == cor.packet_filter_position:
            packet_time = packet.header.time / 1000000.
            if packet_time - self.last_time > 1.:
              self.last_time = packet_time
              print "Total path length (m): ", self.sfm.s.total_distance
              print "Straight line length (m): ", self.sfm.s.T.v

class time_printer:
    last_time = 0.
    def print_time(self, packet):
        time = packet.header.time / 1000000.
        if time - self.last_time > 1.:
            self.last_time = time
            print time

import time
class time_printer_pause:
    last_time = 0.
    def print_time(self, packet):
        ptype = "Ignored"
        if packet.header.type == cor.packet_camera:
            ptype = "Camera"
        if packet.header.type == cor.packet_gyroscope:
            ptype = "Gyro " + str(packet.w[0]) + ", " + str(packet.w[1]) + ", " + str(packet.w[2])
        if packet.header.type == cor.packet_accelerometer:
            ptype = "Accel " + str(packet.a[0]) + ", " + str(packet.a[1]) + ", " + str(packet.a[2])
        thistime = packet.header.time / 1000000.
        print str(thistime) + ": " + ptype
        time.sleep(.25)



"""
kml = kml_generator(args[1] + ".kml")
kmlp = cor.plugins_initialize_python(None, kml.close)
cor.plugins_register(kmlp)
cor.dispatch_addpython(capturedispatch, kml.packet_position_handler)
"""
class kml_generator:
    count = 0
    DEG__M = 1. / 111120.
    
    def __init__(self, name):
        self.startcos = cos(startlat * pi / 180.)
        self.rotation = array([[cos(startdir), -sin(startdir)],
                               [sin(startdir), cos(startdir)]])
        self.outfile = open(name, 'wt')
        self.outfile.write("""<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Document>
    <name>Corvis Solution</name>
    <Style id="yellowLineGreenPoly">
      <LineStyle>
        <color>ffffffff</color>
        <width>4</width>
      </LineStyle>
    </Style>
    <Placemark>
      <name>Corvis Path</name>
      <styleUrl>#yellowLineGreenPoly</styleUrl>
      <LineString>
        <tessellate>1</tessellate>
        <altitudeMode>clampToGround</altitudeMode>
        <coordinates>""")

    def packet_position_handler(self, p):
        if p.header.type == cor.packet_navsol:
            self.outfile.write(str(p.longitude) + "," + str(p.latitude) + "," + str(p.altitude) + "\n")
            print p.orientation
        self.count += 1
        if(self.count == skip):
            self.count = 0
        else:
            return
        if p.header.type == cor.packet_filter_position:
            pt = dot(self.rotation, array([p.position[0], p.position[1]]))        
            lon = startlon + pt[0] * (self.DEG__M / self.startcos)
            lat = startlat + pt[1] * self.DEG__M
            self.outfile.write(str(lon) + "," + str(lat) + "\n")
        else:
            pass

    def close(self):
        print "closing!"
        self.outfile.write("""</coordinates>
    </LineString>
  </Placemark>
</Document>
</kml>""")
        self.outfile.close()


"""
dumper = image_dump()
cor.dispatch_addpython(capturedispatch, dumper.frame)
"""
class image_dump:
    def __init__(self):
        self.output_index = 0
        self.output_image = 0
        print "init dumper"
        self.init = False
        
    def frame(self, packet):
        if not self.init:
            self.init = True
            self.starttime = packet.header.time
        if packet.header.type == cor.packet_camera:
            self.packet = packet
            self.dump()
        if packet.header.type == cor.packet_gyroscope:
            print str(packet.header.time - self.starttime) + ", " + str(packet.w[0]) + ", " + str(packet.w[1]) + ", " + str(packet.w[2])

    def dump(self):
        packet = self.packet
        if packet.header.type == cor.packet_camera:
            path = replay_file + ".images/" + str(self.output_index)
            if not os.path.exists(path):
                os.makedirs(path)
                #            outfn = path + "/" + "%05d" % self.output_image + ".pgm"
            outfn = path + "/" + "%05d" % (packet.header.time - self.starttime) + ".pgm"
            self.output_image += 1
            cor.packet_camera_write_image(packet,outfn)
