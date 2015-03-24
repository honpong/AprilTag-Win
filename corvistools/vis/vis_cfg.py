# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

sys.path.extend(["vis/", "vis/.libs"])
from vis import Vis

myvis = Vis()
mvp = cor.plugins_initialize_python(None, myvis.stop)
mvp.priority = 0
cor.plugins_register(mvp)

import ImagePanel
ip = myvis.frame.image_widget
imageover = ImagePanel.ImageOverlay(ip)
featover = ImagePanel.FeatureOverlay(ip)

ip.renderables.append(imageover)
ip.renderables.append(featover)
