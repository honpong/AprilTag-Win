//
//  Created by Ben Hirashima on 2/25/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//
// depends on JQuery 2.0+, RC3DK.js, three.js

;
var ARController = (function ($, window, RC3DK, THREE)
{
    "use strict";

    var module = {};

    var WorkflowStates =
    {
        STARTUP:        0,
        READY:          1,
        INITIALIZING:   2,
        ALIGN:          3,
        VISUALIZATION:  4,
        FINISHED:       5,
        ERROR:          6
    };

    var currentRunState = RC3DK.SensorFusionRunState.Inactive;
    var workflowState = WorkflowStates.STARTUP;

    var scene, camera, renderer;

    var roofJson;

    $(document).ready(function()
    {
//        loadRoofJsonFile(function (data){
//            roofJson = data;
//            enterReadyState();
//        });

        enterReadyState();

        $("#shutterButton").on( "click", function() {
            switch (workflowState)
            {
                case WorkflowStates.READY:
                    enterInitializationState();
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
            updateWebGLView(data);
        });

        setupWebGLView();
    });

    function loadRoofJsonFile(callback)
    {
        // get path of json file
        $.ajax({ type: "GET", dataType: "json", url: RC3DK.baseUrl + "getRoofJsonFilePath"})
            .done(function (data, textStatus, jqXHR) {
                // load json file
                $.ajax({ type: "GET", dataType: "json", url: data.filePath })
                    .done(function (data, textStatus, jqXHR) {
                        callback(data);
                    })
                    .fail(function (jqXHR, textStatus, errorThrown) {
                        alert(textStatus + ": " + errorThrown + ": " + JSON.stringify(jqXHR));
                    })
                ;
            })
            .fail(function (jqXHR, textStatus, errorThrown) {
                alert(textStatus + ": " + JSON.stringify(jqXHR));
            })
        ;
    }

    function handleNewSensorFusionRunState(runState)
    {
        currentRunState = runState;

        switch (runState)
        {
            case RC3DK.SensorFusionRunState.Running:
                enterAlignState();
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

    function enterAlignState()
    {
        showMessage("Align the outline with the roof.");
        workflowState = WorkflowStates.ALIGN;
    }

    function enterVisualizationState()
    {
        showMessage("Roof visualization active.");
        workflowState = WorkflowStates.VISUALIZATION;
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

    function setupWebGLView()
    {
        scene = new THREE.Scene();
        camera = new THREE.PerspectiveCamera( 75, window.innerWidth / window.innerHeight, 0.1, 1000 );
        renderer = new THREE.WebGLRenderer();
        renderer.setSize( window.innerWidth, window.innerHeight );
        window.document.body.appendChild( renderer.domElement );

        var cube = new THREE.Mesh(
            new THREE.CubeGeometry( 1, 1, 1 ),
            new THREE.MeshLambertMaterial( { color: 0xFFFFFF } )
        );
        scene.add( cube );
        cube.position.z = -5;

        var light;

        // top
        light = new THREE.DirectionalLight( 0x0DEDDF );
        light.position.set( 0, 1, 0 );
        scene.add( light );

        // bottom
        light = new THREE.DirectionalLight( 0x0DEDDF );
        light.position.set( 0, -1, 0 );
        scene.add( light );

        // back
        light = new THREE.DirectionalLight( 0xB685F3 );
        light.position.set( 1, 0, 0 );
        scene.add( light );

        // front
        light = new THREE.DirectionalLight( 0xB685F3 );
        light.position.set( -1, 0, 0 );
        scene.add( light );

        // right
        light = new THREE.DirectionalLight( 0x89A7F5 );
        light.position.set( 0, 0, 1 );
        scene.add( light );

        // left
        light = new THREE.DirectionalLight( 0x89A7F5 );
        light.position.set( 0, 0, -1 );
        scene.add( light );
    }

    var exFactor = 15; // translation exaggeration factor
    function updateWebGLView(data)
    {
        if (data.transformation.translation)
        {
            camera.position.set(-data.cameraTransformation.translation.v1 * exFactor, data.cameraTransformation.translation.v2 * exFactor, -data.cameraTransformation.translation.v0 * exFactor);
        }

        // the rotation is not working correctly yet. it needs to be converted into the WebGL coordinate system.
//        if (data.transformation.rotation)
//        {
//            camera.quaternion.set(data.transformation.rotation.qy, data.transformation.rotation.qz, data.transformation.rotation.qx, data.transformation.rotation.qw);
//        }

        renderer.render( scene, camera );
    }

    return module;

})(jQuery, window, RC3DK, THREE);