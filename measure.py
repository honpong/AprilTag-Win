# capture, cor, fc are defined in benchmark.py

class monitor():
    done = False
    def __init__(self, capture):
        self.capture = capture

    def finished(self, packet):
        if self.done: return
        if self.capture.dispatch.mb.total_bytes == self.capture.dispatch.bytes_dispatched:
          self.done = True

mc = monitor(capture)
cor.dispatch_addpython(fc.solution.dispatch, mc.finished)

import time
while not mc.done:
    time.sleep(0.1)

print "Total path length (m):", fc.sfm.s.total_distance
print "Straight line length(m):", fc.sfm.s.T.v

