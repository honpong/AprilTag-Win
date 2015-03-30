//
//  Created by Ben Hirashima on 2/25/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//
// depends on JQuery 2.0+, RC3DK.js, RC3DKPlus.js, three.js

;
var CaptureController = (function ($, window, RC3DK, THREE)
{
    "use strict";

    var module = {};

    var WorkflowStates =
    {
        READY:          1,
        INITIALIZING:   2,
        MOVING:         3,
        FRAMING:        4,
        PROCESSING:     5,
        FINISHED:       6,
        ERROR:          7
    };

    var currentRunState = RC3DK.SensorFusionRunState.Inactive;
    var workflowState = WorkflowStates.READY;
    var photoGuid;

    $(document).ready(function()
    {
        enterReadyState();

        $("#shutterButton").on( "click", function() {
            switch (workflowState)
            {
                case WorkflowStates.READY:
                    enterInitializationState();
                    break;

                case WorkflowStates.FRAMING:
                    enterProcessingState();
                    break;

                case WorkflowStates.ERROR:
                    enterReadyState();
                    break;
            }
        });

        RC3DK.onStatusUpdate(function (status)
        {
            if (status.runState !== currentRunState) handleNewSensorFusionRunState(status.runState);

            if (status.runState === RC3DK.SensorFusionRunState.SteadyInitialization && workflowState === WorkflowStates.INITIALIZING)
            {
                updateInitializationProgress(status.progress);
            }

            if (status.error)
            {
                if (status.error.class == RC3DK.RCSensorFusionErrorClass)
                {
                    handleSensorFusionError(status.error);
                }
                else if (status.error.class == RC3DK.RCLicenseErrorClass)
                {
                    handleLicenseError(status.error);
                }
            }
        });

        RC3DK.onDataUpdate(function (data, medianFeatureDepth)
        {
            if (workflowState === WorkflowStates.MOVING)
            {
                var currentPosition = new THREE.Vector3(data.transformation.translation.v0, data.transformation.translation.v1, data.transformation.translation.v2);
                var progress = currentPosition.length() / .1; // TODO: calculate based on median feature depth

                if (progress < 1)
                    updateMoveProgress(progress);
                else
                    enterFramingState();
            }
        });

        RC3DK.onStereoProgressUpdated(function (progress)
        {
             if (workflowState === WorkflowStates.PROCESSING) updateProcessingProgress(progress);
        });

        RC3DK.onStereoProcessingFinished(function ()
        {
            if (workflowState === WorkflowStates.PROCESSING) enterFinishedState();
        });
    });

    function handleNewSensorFusionRunState(runState)
    {
        currentRunState = runState;

        switch (runState)
        {
            case RC3DK.SensorFusionRunState.Running:
                enterMovingState();
                break;
        }
    }

    function enterReadyState()
    {
        showMessage("Point the camera at the roof, then press the button.");

        RC3DK.stopSensorFusion();
        RC3DK.showVideoView();
        RC3DK.startSensors();

        workflowState = WorkflowStates.READY;
    }

    function enterInitializationState()
    {
        RC3DK.startSensorFusion();
        workflowState = WorkflowStates.INITIALIZING;
    }

    function enterMovingState()
    {
        RC3DK.startStereoCapture(function (success){
            showMessage("Moving state");
            if (success)
                workflowState = WorkflowStates.MOVING;
            else
                enterErrorState("Failed to start stereo capture.");
        });
    }

    function enterFramingState()
    {
        showMessage("Press the button to finish.");
        workflowState = WorkflowStates.FRAMING;
    }

    function enterProcessingState()
    {
        showMessage("Please wait...");

        RC3DK.stopSensorFusion();
        RC3DK.stopSensors();
        RC3DK.finishStereoCapture(function (guid){
            photoGuid = guid;
        });

        workflowState = WorkflowStates.PROCESSING;
    }

    function enterFinishedState()
    {
        showMessage("Finished");
        workflowState = WorkflowStates.FINISHED;
    }

    function enterErrorState(message)
    {
        if (!message) message = "Whoops, something went wrong.";
        showMessage(message);
        RC3DK.stopSensorFusion();
        workflowState = WorkflowStates.ERROR;
    }

    function showMessage(message)
    {
        $("#message").html(message);
    }

    function handleSensorFusionError(error)
    {
        if (error.code > 1)
        {
            enterErrorState(error.class + ": " + error.code);
        }
    }

    function handleLicenseError(error)
    {
        enterErrorState(error.class + ": " + error.code);
    }

    function updateInitializationProgress(progress)
    {
        var percentage = progress * 100;
        showMessage("Hold still " + Math.ceil(percentage) + "%");
    }

    function updateMoveProgress(progress)
    {
        var percentage = progress * 100;
        showMessage("Move sideways left or right... " + Math.ceil(percentage) + "%")
    }

    function updateProcessingProgress(progress)
    {
        var percentage = progress * 100;
        showMessage("Please wait... " + Math.ceil(percentage) + "%")
    }

    return module;

})(jQuery, window, RC3DK, THREE);