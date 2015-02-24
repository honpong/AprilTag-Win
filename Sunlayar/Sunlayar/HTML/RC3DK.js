console.log("Loading RC3DK.js");

(function (window) {
    "use strict";

    var RC3DK = function () {
        var baseUrl = "http://internal.realitycap.com/";

        if (RC3DK.prototype._singletonInstance) {
            return RC3DK.prototype_singletonInstance;
        }
        RC3DK.prototype._singletonInstance = this;

        this.startSensors = function () {
            $.ajax({ type: "GET", url: baseUrl + "startSensors"});
        };

        this.startSensorFusion = function () {
            $.ajax({ type: "GET", url: baseUrl + "startSensorFusion"});
        };

        this.stopSensors = function () {
            $.ajax({ type: "GET", url: baseUrl + "stopSensors"});
        };

        this.stopSensorFusion = function () {
            $.ajax({ type: "GET", url: baseUrl + "stopSensorFusion"});
        };

        this.startStaticCalibration = function () {
            $.ajax({ type: "GET", url: baseUrl + "startStaticCalibration"});
        };

        this.setLicenseKey = function (licenseKey) {
            var jsonData = { "licenseKey": licenseKey };
            $.ajax({ type: "POST", url: baseUrl + "setLicenseKey", contentType: "application/json", processData: false, dataType: "json", data: JSON.stringify(jsonData) })
                .done(function (data, textStatus, jqXHR) { // on a post, data is an object
                    if (!data.result) alert(textStatus + ": " + JSON.stringify(data));
                })
                .fail(function (jqXHR, textStatus, errorThrown) {
                    alert(textStatus + ": " + JSON.stringify(jqXHR));
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
                    alert(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.logNative = function (message) {
            var jsonData = { "message": message };
            $.ajax({ type: "POST", url: baseUrl + "log", contentType: "application/json", processData: false, dataType: "json", data: JSON.stringify(jsonData) })
                .fail(function (jqXHR, textStatus, errorThrown) {
                    alert(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };
    }

    if (!window.RC3DK) window.RC3DK = new RC3DK();

})(window);