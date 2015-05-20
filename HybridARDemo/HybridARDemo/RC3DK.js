//
//  Created by Ben Hirashima on 2/25/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//
// depends on JQuery 2.0+

;(function (window, $) {
    "use strict";

    var RC3DK = function ()
    {
        if (RC3DK.prototype._singletonInstance) {
            return RC3DK.prototype_singletonInstance;
        }
        RC3DK.prototype._singletonInstance = this;

        var statusUpdateCallback, dataUpdateCallback, matricesUpdateCallback;

        this.baseUrl = "http://dummy.realitycap.com/";

        this.startSensors = function () {
            $.ajax({ type: "GET", url: this.baseUrl + "startSensors"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.startSensorFusion = function () {
            $.ajax({ type: "GET", url: this.baseUrl + "startSensorFusion"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.stopSensors = function () {
            $.ajax({ type: "GET", url: this.baseUrl + "stopSensors"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.stopSensorFusion = function () {
            $.ajax({ type: "GET", url: this.baseUrl + "stopSensorFusion"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.startStaticCalibration = function () {
            $.ajax({ type: "GET", url: this.baseUrl + "startStaticCalibration"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.showVideoView = function () {
            $.ajax({ type: "GET", url: this.baseUrl + "showVideoView"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.hideVideoView = function () {
            $.ajax({ type: "GET", url: baseUrl + "hideVideoView"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.setLicenseKey = function (licenseKey) {
            var jsonData = { "licenseKey": licenseKey };
            $.ajax({ type: "POST", url: this.baseUrl + "setLicenseKey", contentType: "application/json", processData: false, dataType: "json", data: JSON.stringify(jsonData) })
                .done(function (data, textStatus, jqXHR) { // on a post, data is an object
                    if (!data.result) alert(textStatus + ": " + JSON.stringify(data));
                })
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.hasCalibrationData = function (callback) {
            var isCalibrated = false;
            $.ajax({ type: "GET", dataType: "json", url: this.baseUrl + "hasCalibrationData"})
                .done(function (data, textStatus, jqXHR) {
                    callback(data.result);
                })
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        /*
         the status object has the same stucture and content as RCSensorFusionStatus. the error object may not be present.
         ex: {runState:4,confidence:1,error:{class:"RCSensorFusionError",code:1},progress:1}
         */
        this.sensorFusionDidChangeStatus = function (status)
        {
            if (statusUpdateCallback) statusUpdateCallback(status);
        };

        /*
         the data object has the same stucture and content as RCSensorFusionData, minus the featurePoints. ex:
         {
         transformation:{
         rotation:{qx:-0.012587299570441246,qw:0.99985092878341675,qz:0.00024901828146539629,qy:-0.011816162616014481},
         translation:{v2:0.0001231919159181416,v0:0.002296853344887495,std1:0.001661463757045567,v3:0,std2:0.0026393774896860123,v1:-0.0022083036601543427,std3:0,std0:0.0014444828266277909}
         },
         cameraParameters:{radialFouthDegree:-0.2295377254486084,radialSecondDegree:0.11349671334028244,opticalCenterY:247.20547485351562,opticalCenterX:325.925048828125,focalLength:587.55810546875},
         cameraTransformation:{
         rotation:{qx:0.70717746019363403,qw:0.00054524908773601055,qz:0.017255853861570358,qy:-0.70682525634765625},
         translation:{v2:-0.0017100467812269926,v0:-0.00071634328924119473,std1:0.057653535157442093,v3:0,std2:0.072655044496059418,v1:0.067767113447189331,std3:0,std0:0.053758375346660614}
         },
         timestamp:248998938250,
         totalPathLength:{scalar:0,standardDeviation:0}
         }
         */
        this.sensorFusionDidUpdateData = function (data, medianFeatureDepth)
        {
            if (dataUpdateCallback) dataUpdateCallback(data, medianFeatureDepth);
        };

        /*
        the data object contains two matrices: projection and camera. each matrix has the properties m00...m33
         */
        this.sensorFusionDidUpdateMatrices = function (matrices)
        {
            if (matricesUpdateCallback) matricesUpdateCallback(matrices);
        };

        this.onStatusUpdate = function (callback)
        {
            statusUpdateCallback = callback;
        };

        this.onDataUpdate = function (callback)
        {
            dataUpdateCallback = callback;
        };

        this.onMatricesUpdate = function(callback)
        {
            matricesUpdateCallback = callback;
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

    if (!window.RC3DK) window.RC3DK = new RC3DK();

})(window, jQuery);