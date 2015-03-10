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

        setupWebGLView();
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
            // do something with the data
        });

        RC3DK.onMatricesUpdate(function (matrices)
        {
            updateWebGLView(matrices);
        });
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

    function updateWebGLView(matrices)
    {
        var projectionMatrix = matrix4FromPlainObject(matrices.projection);
        if (!projectionMatrix) alert("no proj matrix");

        var cameraMatrix = matrix4FromPlainObject(matrices.camera);
        if (!cameraMatrix) alert("no camera matrix");

        camera.projectionMatrix = projectionMatrix;
        camera.matrixWorldInverse = cameraMatrix;

        renderer.render( scene, camera );
    }

    function matrix4FromPlainObject(plainObject)
    {
        var matrix = new THREE.Matrix4();
        matrix.set(plainObject.m00, plainObject.m01, plainObject.m02, plainObject.m03, plainObject.m10, plainObject.m11, plainObject.m12, plainObject.m13, plainObject.m20, plainObject.m21, plainObject.m22, plainObject.m23, plainObject.m30, plainObject.m31, plainObject.m32, plainObject.m33);
        return  matrix;
    }

    return module;

})(jQuery, window, RC3DK, THREE);