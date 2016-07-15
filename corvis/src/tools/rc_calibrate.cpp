#include "rc_tracker.h"
#include "rc_sensor_listener.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <atomic>
#include <signal.h>

static bool read_file(const std::string name, std::string &contents)
{
    std::ifstream t(name);
    std::istreambuf_iterator<char> b(t), e;
    contents.assign(b,e);
    return t.good() && b == e;
}

static bool write_file(const std::string name, const char *contents)
{
    std::ofstream t(name);
    t << contents;
    t.close();
    return t.good();
}

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <condition_variable>

int main(int c, char **v) {
    if (0) { usage:
        std::cerr << "Usage: " << v[0] << " [--id=<n>] [--xml <calibration.xml>] [(--save | --load) <calibration-json>]\n";
        return 1;
    }

    const char *capture_file = nullptr;
    const char *calibration_load = nullptr,
        *calibration_save = "/sdcard/config/calibration.json",
        *calibration_xml = "/sdcard/config/calibration.xml";
    bool drop_depth = false;
    int device_id = 0;

    for (int i=1; i<c; i++)
             if (strcmp(v[i], "--save") == 0 && i+1 < c) calibration_save = v[++i];
        else if (strcmp(v[i], "--load") == 0 && i+1 < c) calibration_load = v[++i];
        else if (strcmp(v[i], "--xml") == 0  && i+1 < c) calibration_xml  = v[++i];
        else if (strcmp(v[i], "--id") == 0  && i+1 < c) device_id  = atoi(v[++i]);
        else if (strcmp(v[i], "--capture") == 0  && i+1 < c) capture_file = v[++i];
        else goto usage;

    std::unique_ptr<rc_Tracker, decltype(rc_destroy)*> rc_{rc_create(), rc_destroy}; rc_Tracker *rc = rc_.get();

    struct camera_info {
        rc_Extrinsics extrinsics_wrt_accel_m;
        rc_CameraIntrinsics intrinsics;
    } fisheye = {}, color = {}, depth = {};

    if (calibration_xml) {
        std::string cal;
        if (!read_file(calibration_xml, cal)) { std::cerr << calibration_xml << ": error loading file\n"; return 1; }
        if (!rc_setCalibration(rc, cal.c_str())) { std::cerr << calibration_xml << ": error parsing file\n"; return 1; }

        rc_describeCamera(rc, 0, rc_FORMAT_GRAY8, &fisheye.extrinsics_wrt_accel_m, &fisheye.intrinsics);
        rc_describeCamera(rc, 1, rc_FORMAT_GRAY8,   &color.extrinsics_wrt_accel_m,   &color.intrinsics);
        rc_describeCamera(rc, 0, rc_FORMAT_DEPTH16, &depth.extrinsics_wrt_accel_m,   &depth.intrinsics);
    }

    if (calibration_load) {
        std::string cal;
        if (!read_file(calibration_load, cal)) {
            std::cerr << calibration_load << ": error loading file\n";
            return 1;
        }
        if (!rc_setCalibration(rc, cal.c_str())) {
            std::cerr << calibration_load << ": error parsing file\n";
            return 1;
        }
    } else {
        if (!rc_setCalibration(rc, R"(
{
  "device": "e6t",
  "calibrationVersion": 7,
  "abias0": 0,
  "abias1": 0,
  "abias2": 0,
  "wbias0": 0,
  "wbias1": 0,
  "wbias2": 0,
  "abiasvar0": 0.001,
  "abiasvar1": 0.001,
  "abiasvar2": 0.001,
  "wbiasvar0": 0.00001,
  "wbiasvar1": 0.00001,
  "wbiasvar2": 0.00001,
  "Tc0": 0.00685,
  "Tc1": 0.00229,
  "Tc2": 0.0,
  "Wc0": 0,
  "Wc1": 0,
  "Wc2": 0,
  "TcVar0": 1e-6,
  "TcVar1": 1e-6,
  "TcVar2": 1e-6,
  "WcVar0": 1e-6,
  "WcVar1": 1e-6,
  "WcVar2": 1e-6,
  "wMeasVar": 0.00001,
  "aMeasVar": 0.0004,
  "imageWidth": 640,
  "imageHeight": 480,
  "Fx": 260,
  "Fy": 260,
  "Cx": 320,
  "Cy": 240,
  "Kw": 0.90,
  "distortionModel": 1,
  "accelerometerTransform": [
     0.0, 1.0, 0.0,
    -1.0, 0.0, 0.0,
     0.0, 0.0, 1.0
  ],
  "gyroscopeTransform": [
     0.0, 1.0, 0.0,
    -1.0, 0.0, 0.0,
     0.0, 0.0, 1.0
  ]
}
)")) {
            std::cerr << "error loading default calibration\n";
            return 1;
        }
    }

    if (color.intrinsics.type)
        rc_configureCamera(rc, 0, rc_FORMAT_GRAY8,   &color.extrinsics_wrt_accel_m,   &color.intrinsics);
    else if (fisheye.intrinsics.type)
        rc_configureCamera(rc, 0, rc_FORMAT_GRAY8, &fisheye.extrinsics_wrt_accel_m, &fisheye.intrinsics);
    if (depth.intrinsics.type)
        rc_configureCamera(rc, 0, rc_FORMAT_DEPTH16, &depth.extrinsics_wrt_accel_m,   &depth.intrinsics);

    struct status {
        std::mutex m;
        std::condition_variable cv_done;
        std::atomic<bool> done {false};
        rc_TrackerConfidence confidence {rc_E_CONFIDENCE_NONE};
        rc_TrackerError error {rc_E_ERROR_NONE};
        rc_TrackerState state {(rc_TrackerState)-1};
        int percentage {-1};
    } status;

    rc_setStatusCallback(rc, [](void *status, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence, float progress) {
        auto s = static_cast<struct status *>(status);
        auto percentage = static_cast<int>(std::round(100*progress));
        if (s->error != error)
            switch(error) {
                break; case rc_E_ERROR_NONE:   printf("Errors cleared.\n");    fflush(stdout);
                break; case rc_E_ERROR_VISION: printf("Camera covered?\n");    fflush(stdout);
                break; case rc_E_ERROR_SPEED:  printf("Slow down.\n");         fflush(stdout);
                break; case rc_E_ERROR_OTHER:  printf("Undetermined error\n"); fflush(stdout);
            }
        s->error = error;

        if (s->confidence != confidence)
            switch(confidence) {
                break; case rc_E_CONFIDENCE_NONE:   printf("No confidence.\n");    fflush(stdout);
                break; case rc_E_CONFIDENCE_LOW:    printf("Low confidence\n");    fflush(stdout);
                break; case rc_E_CONFIDENCE_MEDIUM: printf("Medium confidence\n"); fflush(stdout);
                break; case rc_E_CONFIDENCE_HIGH:   printf("High confidence\n");   fflush(stdout);
            }
        s->confidence = confidence;

        if (s->state != state || s->percentage != percentage)
            switch(state) {
                break; case rc_E_STATIC_CALIBRATION:     printf("\r\eKPlace device on a flat surface. Progress: %3d%%", percentage);       fflush(stdout);
                break; case rc_E_PORTRAIT_CALIBRATION:   printf("\r\eKHold steady in portrait orientation. Progress: %3d%%", percentage);  fflush(stdout);
                break; case rc_E_LANDSCAPE_CALIBRATION:  printf("\r\eKHold steady in landscape orientation. Progress: %3d%%", percentage); fflush(stdout);
                break; case rc_E_INACTIVE:               printf("\nCalibration successful.\n"); s->done = true; s->cv_done.notify_all();   fflush(stdout);
                break; case rc_E_INERTIAL_ONLY:          printf("Orientation-only Mode...\n");                                             fflush(stdout);
                break; case rc_E_STEADY_INITIALIZATION:  printf("Hold the device Steady.\n");                                              fflush(stdout);
                break; case rc_E_DYNAMIC_INITIALIZATION: printf("Dynamic Initialization...\n");                                            fflush(stdout);
                break; case rc_E_RUNNING:                printf("Running...\n");                                                           fflush(stdout);
            }
        s->state = state;
        s->percentage = percentage;
    }, &status);

    if (capture_file) {
        printf("Recording to %s\n", capture_file); fflush(stdout);
        rc_setOutputLog(rc, capture_file, rc_E_ASYNCHRONOUS);
        static struct status *s = &status;
        signal(SIGINT, [](int) { s->done = true; s->cv_done.notify_all(); printf("Finishing...\n"); fflush(stdout); });
    } else
        rc_startCalibration(rc, rc_E_ASYNCHRONOUS);

    {
        rc::sensor_listener sl(rc, 0, device_id);
        if (!sl) {
            fprintf(stderr, "unable to connect to the motionservice\n");
            return 1;
        }
        std::unique_lock<std::mutex> lock(status.m);
        status.cv_done.wait(lock, [&status]() -> bool { return status.done; });
    }

    rc_stopTracker(rc);

    std::string calibration_save_;
    if (capture_file)
        calibration_save = (calibration_save_ = capture_file + std::string(".json")).c_str();

    if (calibration_save) {
        const char *buffer = nullptr;
        if (!rc_getCalibration(rc, &buffer)) { std::cerr << calibration_save << ": error getting calibration\n"; return 1; }
        if (!write_file(calibration_save, buffer)) { std::cerr << calibration_save << ": error writing file\n"; return 1; }
        printf("Wrote %s\n", calibration_save);
        if (calibration_save == std::string("/sdcard/config/calibration.json")) {
            bool worked = true;
            for (auto &copy : { "/sdcard/calibration_from_RCUtility.json", "/sdcard/DS4RCapture/calibration.json" })
                if (write_file(copy, buffer))
                    printf("Wrote a copy to %s\n", copy);
                else {
                    fprintf(stderr, "Failed to write a copy to %s\n", copy);
                    worked = false;
                }
            if (!worked)
                return 1;
        }
    }
    return 0;
}
