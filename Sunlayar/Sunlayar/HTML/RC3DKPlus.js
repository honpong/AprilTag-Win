//
//  Created by Ben Hirashima on 2/25/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//
// depends on JQuery 2.0+, RC3DK.js

;(function (window, $, RC3DK) {
    "use strict";

    RC3DK.startStereoCapture = function (callback) {
        $.ajax({ type: "GET", dataType: "json", url: RC3DK.baseUrl + "startStereoCapture"})
            .done(function (data, textStatus, jqXHR) {
                callback(data.result);
            })
            .fail(function (jqXHR, textStatus, errorThrown) {
                RC3DK.logNative(textStatus + ": " + JSON.stringify(jqXHR));
            })
        ;
    };

    RC3DK.finishStereoCapture = function (callback) {
        $.ajax({ type: "GET", dataType: "json", url: RC3DK.baseUrl + "finishStereoCapture"})
            .done(function (data, textStatus, jqXHR) {
                callback(data.result);
            })
            .fail(function (jqXHR, textStatus, errorThrown) {
                RC3DK.logNative(textStatus + ": " + JSON.stringify(jqXHR));
            })
        ;
    };

    RC3DK.cancelStereoCapture = function () {
        $.ajax({ type: "GET", url: RC3DK.baseUrl + "cancelStereoCapture"})
            .fail(function (jqXHR, textStatus, errorThrown) {
                RC3DK.logNative(textStatus + ": " + JSON.stringify(jqXHR));
            })
        ;
    };

    RC3DK.stereoDidUpdateProgress = function (progress)
    {
        if (RC3DK.stereoDidUpdateProgressCallback) RC3DK.stereoDidUpdateProgressCallback(progress);
    };

    RC3DK.stereoDidFinish = function ()
    {
        if (RC3DK.stereoDidFinishCallback) RC3DK.stereoDidFinishCallback();
    };

    RC3DK.onStereoProgressUpdated = function (callback)
    {
        RC3DK.stereoDidUpdateProgressCallback = callback;
    };

    RC3DK.onStereoProcessingFinished = function (callback)
    {
        RC3DK.stereoDidFinishCallback = callback;
    };

})(window, jQuery, RC3DK);