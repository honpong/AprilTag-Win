#Sample Movidius project

This project contains a complete 6dof system that takes data replayed
over USB and outputs poses over USB. It supports TM2 hardware (both
ES0 and ES1) on Ubuntu 16.04.

##Building

There are two components to build, the firmware and the host client
app.

###Firmware:

```
# downloads and patches mdk if necessary and sets up environment variables
source mvenv

# start the jtag server
cd device
make start_server

# build the firmware and upload it over jtag
cd device
make debug
```

###Host:

```
mkdir -p host/build
cd host/build
cmake ../
make
```

Running:

Once the firmware has started via JTAG, run:

```
slam_client -p <filename.rc> -6 <output_pose_filename.tum>
```
