//
//  Created by Ben Hirashima on 2/25/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//
// depends on JQuery 2.0+, RC3DK.js, RC3DKPlus.js

;
var CaptureController = (function ($, window)
{
    "use strict";

    var module = {};

    var currentRunState = RC3DK.SensorFusionRunState.Inactive;

    $(document).ready(function()
    {
        console.log("documentReady()");

        module.resetWorkflow();

        $("#shutterButton").on( "click", function() {
            switch (currentRunState)
            {
                case RC3DK.SensorFusionRunState.Inactive:
                    module.initializeSensorFusion();
                    break;
            }
        });

        RC3DK.onStatusUpdate(function (status)
        {
            if (status.runState !== currentRunState) currentRunState = status.runState;

            switch (status.runState)
            {
                case RC3DK.SensorFusionRunState.SteadyInitialization:
                    module.updateInitializationProgress(status.progress);
                    break;
            }

            if (status.error)
            {
                if (status.error.class === RC3DK.RCLicenseErrorClass)
                {
                    module.handleSensorFusionError(status.error);
                }
                else if (status.error.class === RC3DK.RCLicenseErrorClass)
                {
                    module.handleLicenseError(status.error);
                }
            }
        });

        RC3DK.onDataUpdate(function (data, medianFeatureDepth)
        {
//                var currentPosition = new Vector3(data.transformation.translation.x, data.transformation.translation.y, data.transformation.translation.z);

        });
    });

    module.showMessage = function(message)
    {
        $("#message").html(message);
    }

    module.handleSensorFusionError = function(error)
    {
        if (error.code > 1)
        {
            RC3DK.stopSensorFusion();
            RC3DK.stopSensors();
            alert(status.error.class + ":" + status.error.code);
        }
    }

    module.handleLicenseError = function(error)
    {
        alert(status.error.class + ":" + status.error.code);
    }

    module.resetWorkflow = function()
    {
        module.showMessage("Point the camera at the roof, then press the button.");

        RC3DK.stopSensorFusion();
        RC3DK.showVideoView();
        RC3DK.startSensors();
    }

    module.initializeSensorFusion = function()
    {
        module.showMessage("Hold still");
        RC3DK.startSensorFusion();
    }

    module.updateInitializationProgress = function(progress)
    {
        var percentage = progress * 100;
        module.showMessage("Hold still " + Math.ceil(percentage) + "%");
    }

    module.updateMoveProgress = function(progress)
    {
        module.showMessage("Move sideways left or right until the progress bar is full. " + Math.ceil(percentage) + "%")
    }

    return module;

})(jQuery, window);