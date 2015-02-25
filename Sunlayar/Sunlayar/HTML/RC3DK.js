console.log("Loading RC3DK.js");

(function (window) {
    "use strict";

    var RC3DK = function () {
        var baseUrl = "http://dummy.realitycap.com/";
        var statusUpdateCallback, dataUpdateCallback;

        if (RC3DK.prototype._singletonInstance) {
            return RC3DK.prototype_singletonInstance;
        }
        RC3DK.prototype._singletonInstance = this;

        this.startSensors = function () {
            $.ajax({ type: "GET", url: baseUrl + "startSensors"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.startSensorFusion = function () {
            $.ajax({ type: "GET", url: baseUrl + "startSensorFusion"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.stopSensors = function () {
            $.ajax({ type: "GET", url: baseUrl + "stopSensors"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.stopSensorFusion = function () {
            $.ajax({ type: "GET", url: baseUrl + "stopSensorFusion"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.startStaticCalibration = function () {
            $.ajax({ type: "GET", url: baseUrl + "startStaticCalibration"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.showVideoView = function () {
            $.ajax({ type: "GET", url: baseUrl + "showVideoView"})
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
            $.ajax({ type: "POST", url: baseUrl + "setLicenseKey", contentType: "application/json", processData: false, dataType: "json", data: JSON.stringify(jsonData) })
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
            $.ajax({ type: "GET", url: baseUrl + "hasCalibrationData"})
                .done(function (data, textStatus, jqXHR) { // on a get, data is a string
                    var resultObj = JSON.parse(data);
                    callback(resultObj.result);
                })
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.sensorFusionDidChangeStatus = function (status)
        {
            if (statusUpdateCallback) statusUpdateCallback(status);
        };

        this.sensorFusionDidUpdateData = function (data)
        {
            if (dataUpdateCallback) dataUpdateCallback(data);
        };

        this.onStatusUpdate = function (callback)
        {
            statusUpdateCallback = callback;
        };

        this.onDataUpdate = function (callback)
        {
            dataUpdateCallback = callback;
        };

        this.logNative = function (message) {
            console.log(message);
            var jsonData = { "message": message };
            $.ajax({ type: "POST", url: baseUrl + "log", contentType: "application/json", processData: false, dataType: "json", data: JSON.stringify(jsonData) });
        };
    }

    if (!window.RC3DK) window.RC3DK = new RC3DK();

})(window);