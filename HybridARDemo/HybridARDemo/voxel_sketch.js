//
//  Created by Ben Hirashima on 2/25/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//
// depends on JQuery 2.0+, RC3DK.js, RC3DKPlus.js, three.js

;
var MainController = (function ($, window, RC3DK, THREE)
{
    "use strict";

    var module = {};

    var WorkflowStates =
    {
        STARTUP:        0,
        READY:          1,
        INITIALIZING:   2,
        AUGMENTED:      3,
        ERROR:          4
    };

    var currentRunState = RC3DK.SensorFusionRunState.Inactive;
    var workflowState = WorkflowStates.STARTUP;

    var scene, camera, renderer;
    var plane, cube;
    var mouse, raycaster;

    var rollOverGeo, rollOverMesh, rollOverMaterial;
    var cubeGeo, cubeMaterial;

    var objects = [];

    $(document).ready(function()
    {
        enterReadyState();

        $("#shutterButton").on( "click", function() {
            switch (workflowState)
            {
                case WorkflowStates.READY:
                    enterInitializationState();
                    break;

                case WorkflowStates.AUGMENTED:
                    enterReadyState();
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

        RC3DK.onMatricesUpdate(function (matrices)
        {
            var projectionMatrix = matrix4FromPlainObject(matrices.projection);
            var cameraMatrix = matrix4FromPlainObject(matrices.camera);
            updateWebGLView(projectionMatrix, cameraMatrix);
        });
    });

    function handleNewSensorFusionRunState(runState)
    {
        currentRunState = runState;

        switch (runState)
        {
            case RC3DK.SensorFusionRunState.Running:
                enterAugmentedState();
                break;
        }
    }

    function enterReadyState()
    {
        showMessage("Press the button to start.");

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

    function enterAugmentedState()
    {
        setupWebGLView();
        showMessage("Augmented reality!");
        workflowState = WorkflowStates.AUGMENTED;
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

                      
        // roll-over helpers

        rollOverGeo = new THREE.BoxGeometry( 0.05, 0.05, 0.05);
        rollOverMaterial = new THREE.MeshBasicMaterial( { color: 0xff0000, opacity: 0.5, transparent: true } );
        rollOverMesh = new THREE.Mesh( rollOverGeo, rollOverMaterial );
        rollOverMesh.position.set(0, 0.5, -0.5);
        scene.add( rollOverMesh );

        // cubes

        cubeGeo = new THREE.BoxGeometry( 0.05, 0.05, 0.05);
        cubeMaterial = new THREE.MeshLambertMaterial( { color: 0xfeb74c, shading: THREE.FlatShading, map: THREE.ImageUtils.loadTexture( "square-outline-textured.png" ) } );

        // grid

        var size = .5, step = 0.05;

        var geometry = new THREE.Geometry();

        for ( var i = - size; i <= size; i += step ) {

            geometry.vertices.push( new THREE.Vector3( - size, i, 0) );
            geometry.vertices.push( new THREE.Vector3(   size, i, 0) );

            geometry.vertices.push( new THREE.Vector3( i, - size, 0) );
            geometry.vertices.push( new THREE.Vector3( i, size, 0) );

        }

        var material = new THREE.LineBasicMaterial( { color: 0x000000, opacity: 0.2, transparent: true } );

        var line = new THREE.Line( geometry, material, THREE.LinePieces );
        line.position.set(0,0.5,-0.5);
        scene.add( line );

        //

        raycaster = new THREE.Raycaster();
        mouse = new THREE.Vector2();

        var geometry = new THREE.PlaneBufferGeometry( 1, 1 );
        geometry.applyMatrix( new THREE.Matrix4().makeRotationX( - Math.PI / 2 ) );

        plane = new THREE.Mesh( geometry );
        plane.visible = false;
        plane.position.set(0,0.5,-0.25);
        scene.add( plane );

        objects.push( plane );
              
                      
        var light;

        // top
        light = new THREE.DirectionalLight( 0xCDEDDF );
        light.position.set( 0, 1, 1 );
        scene.add( light );

        light = new THREE.AmbientLight( 0x404040 );
        scene.add( light );
    }

    function updateWebGLView(projectionMatrix, cameraMatrix)
    {
        if (!projectionMatrix) alert("no projection matrix");
        if (!cameraMatrix) alert("no camera matrix");

        camera.projectionMatrix = projectionMatrix;
        camera.matrixAutoUpdate = false;
        camera.matrixWorld = cameraMatrix;

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