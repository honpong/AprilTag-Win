console.log("Loading RC3DKPlus.js");

(function (window) {
    "use strict";

    var RC3DKPlus = function () {
        var baseUrl = "http://dummy.realitycap.com/";
        var stereoDidUpdateProgressCallback, stereoDidFinishCallback;

        if (RC3DKPlus.prototype._singletonInstance) {
            return RC3DKPlus.prototype_singletonInstance;
        }
        RC3DKPlus.prototype._singletonInstance = this;

        this.startStereoCapture = function (callback) {
            $.ajax({ type: "GET", url: baseUrl + "startStereoCapture"})
                .done(function (data, textStatus, jqXHR) { // on a get, data is a string
                    var resultObj = JSON.parse(data);
                    callback(resultObj.result);
                })
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.finishStereoCapture = function (callback) {
            $.ajax({ type: "GET", url: baseUrl + "finishStereoCapture"})
                .done(function (data, textStatus, jqXHR) { // on a get, data is a string
                    var resultObj = JSON.parse(data);
                    callback(resultObj.result);
                })
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.cancelStereoCapture = function () {
            $.ajax({ type: "GET", url: baseUrl + "cancelStereoCapture"})
                .fail(function (jqXHR, textStatus, errorThrown) {
                    logNative(textStatus + ": " + JSON.stringify(jqXHR));
                })
            ;
        };

        this.stereoDidUpdateProgress = function (progress)
        {
            if (stereoDidUpdateProgressCallback) stereoDidUpdateProgressCallback(progress);
        };

        this.stereoDidFinish = function ()
        {
            if (stereoDidFinishCallback) stereoDidFinishCallback();
        };

        this.onStereoProgressUpdated = function (callback)
        {
            stereoDidUpdateProgressCallback = callback;
        };

        this.onStereoProcessingFinished = function (callback)
        {
            stereoDidFinishCallback = callback;
        };

        this.logNative = function (message) {
            console.log(message);
            var jsonData = { "message": message };
            $.ajax({ type: "POST", url: baseUrl + "log", contentType: "application/json", processData: false, dataType: "json", data: JSON.stringify(jsonData) });
        };
    }

    if (!window.RC3DKPlus) window.RC3DKPlus = new RC3DKPlus();

})(window);