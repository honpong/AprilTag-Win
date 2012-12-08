# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

cor.cvar.cor_time_pb_real = False

from numpy import *

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

replay_file = args[1]
config_file = "config/kml_cfg.py"
if len(args) >= 3:
    config_file = args[2]

execfile(config_file)

solution = cor.mapbuffer()
solution.file_writable = False
solution.mem_writable = False
solution.filename = replay_file
solution.indexsize = 1000000
cor.plugins_register(cor.mapbuffer_open(solution))

capturedispatch = cor.dispatch_t()
capturedispatch.mb = solution
capturedispatch.threaded = True
cor.plugins_register(cor.dispatch_init(capturedispatch))

kml = kml_generator(args[1] + ".kml")
kmlp = cor.plugins_initialize_python(None, kml.close)
cor.plugins_register(kmlp)
cor.dispatch_addpython(capturedispatch, kml.packet_position_handler)

from script_tools import time_printer
tp = time_printer()
cor.dispatch_addpython(capturedispatch, tp.print_time)
