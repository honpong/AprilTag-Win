//
//  Created by Ben Hirashima on 11/23/15.
//  Copyright (c) 2015 Intel. All rights reserved.
//
// depends on JQuery 2.0+

;(function (window, $) {
    "use strict";

    var Tracker = function ()
    {
        if (Tracker.prototype._singletonInstance) {
            return Tracker.prototype_singletonInstance;
        }
        Tracker.prototype._singletonInstance = this;

        var statusUpdateCallback, sceneQualityCallback, poseUpdateCallback;

        this.statusUpdate = function (trackingStatus, fps)
        {
            if (statusUpdateCallback) statusUpdateCallback(trackingStatus, fps);
        };

        this.sceneQualityUpdate = function (quality)
        {
            if (sceneQualityCallback) sceneQualityCallback(quality);
        };

        /*
         each matrix has the properties m00...m33
         */
        this.poseUpdate = function (projMatrix, camMatrix)
        {
            if (poseUpdateCallback) poseUpdateCallback(projMatrix, camMatrix);
        };

        this.onStatusUpdate = function (callback)
        {
            statusUpdateCallback = callback;
        };

        this.onSceneQualityUpdate = function (callback)
        {
            sceneQualityCallback = callback;
        };

        this.onPoseUpdate = function(callback)
        {
            poseUpdateCallback = callback;
        };

        this.logNative = function (message) {
            console.log(message);
            var jsonData = { "message": message };
            $.ajax({ type: "POST", url: this.baseUrl + "log", contentType: "application/json", processData: false, dataType: "json", data: JSON.stringify(jsonData) });
        };

        this.SensorFusionRunState = {
            Inactive : 0,
            StaticCalibration : 1,
            SteadyInitialization : 2,
            DynamicInitialization : 3,
            Running : 4,
            PortraitCalibration : 5,
            LandscapeCalibration : 6
        };

        this.SensorFusionErrorCode = {
            RCSensorFusionErrorCodeNone : 0, // No error has occurred.
            RCSensorFusionErrorCodeVision : 1, // No visual features were detected in the most recent image. This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. However, if this is received repeatedly, it may indicate that the camera is covered or it is too dark. RCSensorFusion will continue.
            RCSensorFusionErrorCodeTooFast : 2, // The device moved more rapidly than expected for typical handheld motion. This may indicate that RCSensorFusion has failed and is providing invalid data. RCSensorFusion will continue.
            RCSensorFusionErrorCodeOther : 3, // A fatal internal error has occured. Please contact RealityCap and provide [RCSensorFusionStatus statusCode] from the status property of the last received RCSensorFusionData object. RCSensorFusion will be reset.
            RCSensorFusionErrorCodeLicense : 4 // A license error indicates that the license has not been properly validated, or needs to be validated again.
        }

        this.RCSensorFusionErrorClass = "RCSensorFusionError";
        this.RCLicenseErrorClass = "RCLicenseError";
    }

    if (!window.Tracker) window.Tracker = new Tracker();

})(window, jQuery);