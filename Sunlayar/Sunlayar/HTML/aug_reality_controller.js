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

    var scene, camera, renderer, roof;
    var initialCamera;

    var roofJson;

    $(document).ready(function()
    {
    
        loadRoofJsonFile(function (data){
            roofJson = data;
            enterReadyState();
            setupWebGLView();
        });

        $("#shutterButton").on( "click", function() {
            switch (workflowState)
            {
                case WorkflowStates.READY:
                    enterInitializationState();
                    break;
		
		case WorkflowStates.ALIGN:
		    enterVisualizationState();
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
            var projectionMatrix = matrix4FromPlainObject(matrices.projection);
            var cameraMatrix = matrix4FromPlainObject(matrices.camera);
            updateWebGLView(projectionMatrix, cameraMatrix);
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

        var canvas = document.getElementById("webGLCanvas");
        renderer = new THREE.WebGLRenderer({ canvas: canvas });
        renderer.setSize( window.innerWidth, window.innerHeight );
	
        var roofGeometry = new THREE.Geometry();
        roofGeometry.vertices.push(
            new THREE.Vector3(roofJson.roof_object.coords3D[0][0], roofJson.roof_object.coords3D[0][1], roofJson.roof_object.coords3D[0][2]),
                new THREE.Vector3(roofJson.roof_object.coords3D[1][0], roofJson.roof_object.coords3D[1][1], roofJson.roof_object.coords3D[1][2]),
                new THREE.Vector3(roofJson.roof_object.coords3D[2][0], roofJson.roof_object.coords3D[2][1], roofJson.roof_object.coords3D[2][2]),
                new THREE.Vector3(roofJson.roof_object.coords3D[3][0], roofJson.roof_object.coords3D[3][1], roofJson.roof_object.coords3D[3][2])
        );
        roofGeometry.faces.push(
            new THREE.Face3(0, 1, 2),
            new THREE.Face3(0, 2, 3)
        );
        roof = new THREE.Mesh(roofGeometry, new THREE.MeshLambertMaterial( { color: 0x00FFFF, side: THREE.DoubleSide, opacity: 0.5, transparent: true } ));

        scene.add( roof );

            roof.matrixAutoUpdate = false;
            var light;

            // top
            light = new THREE.DirectionalLight( 0xCDEDDF );
            light.position.set( 0, 1, 0 );
            scene.add( light );

        light = new THREE.AmbientLight( 0x404040 );
            scene.add( light );
    }

    function updateWebGLView(projectionMatrix, cameraMatrix)
    {
        if (!projectionMatrix) alert("no proj matrix");
        if (!cameraMatrix) alert("no camera matrix");

        camera.projectionMatrix = projectionMatrix;
        camera.matrixAutoUpdate = false;
        camera.matrixWorld = cameraMatrix;
        if(workflowState == WorkflowStates.ALIGN) initialCamera = cameraMatrix.clone();
        roof.matrix = initialCamera.clone();

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
