//
//  Created by Ben Hirashima on 11/23/15.
//  Copyright (c) 2015 Intel. All rights reserved.
//
// depends on JQuery 2.0+, three.js

;
var MainController = (function ($, window, RealSense, THREE)
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

    var currentRunState = Tracker.SensorFusionRunState.Inactive;
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
        FastClick.attach(document.body);

        $("#shutterButton").on( "click", function() {
            switch (workflowState)
            {
                case WorkflowStates.READY:
                    enterAugmentedState();
                    break;

                case WorkflowStates.AUGMENTED:
                    enterReadyState();
                    break;

                case WorkflowStates.ERROR:
                    enterReadyState();
                    break;
            }
        });

        Tracker.onStatusUpdate(function (status)
        {
            showMessage(status);

//            if (status.runState !== currentRunState) handleNewSensorFusionRunState(status.runState);
//
//            if (status.runState === Tracker.SensorFusionRunState.SteadyInitialization && workflowState === WorkflowStates.INITIALIZING)
//            {
//                updateInitializationProgress(status.progress);
//            }
//
//            if (status.error)
//            {
//                if (status.error.class == Tracker.RCSensorFusionErrorClass)
//                {
//                    handleSensorFusionError(status.error);
//                }
//                else if (status.error.class == Tracker.RCLicenseErrorClass)
//                {
//                    handleLicenseError(status.error);
//                }
//            }
        });

        Tracker.onPoseUpdate(function (projMatrix, camMatrix)
        {
            showMessage("pose update");
            var projectionMatrix = matrix4FromPlainObject(projMatrix);
            var cameraMatrix = matrix4FromPlainObject(camMatrix);
//            updateWebGLView(projectionMatrix, cameraMatrix);
        });
    });

    function handleNewSensorFusionRunState(runState)
    {
        currentRunState = runState;

        switch (runState)
        {
            case Tracker.SensorFusionRunState.Running:
                enterAugmentedState();
                break;
        }
    }

    function enterReadyState()
    {
        showMessage("Press the button to start.");

//        Tracker.stopSensorFusion();
//        Tracker.showVideoView();
//        Tracker.startSensors();

        workflowState = WorkflowStates.READY;
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
//        Tracker.stopSensorFusion();
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

    function setupWebGLView()
    {
        //prevent scrolling
        document.body.addEventListener('touchstart', function(e){ e.stopPropagation(); e.preventDefault(); });

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
        //because we're using meter grids, and many objects are sub meter, we need to increase the resolution of the raycaster
        raycaster.near = 0.01; //assume we're at least a centameter off the object
        raycaster.far = 500000;
        raycaster.precision = 0.0000000000001;
        raycaster.linePrecision = 0.0000000000001;
                      
        mouse = new THREE.Vector2();

        var geometry = new THREE.PlaneBufferGeometry( 1, 1 );
        var material = new THREE.MeshBasicMaterial( {color: 0xffffff, opacity: 0.05, side: THREE.DoubleSide} );

        plane = new THREE.Mesh( geometry);
        plane.visible = false;
        plane.position.set(0,0.5,-0.5);
        scene.add( plane );

        objects.push( plane );
              
                      
        var light;

        // top
        light = new THREE.DirectionalLight( 0xCDEDDF );
        light.position.set( 0, 1, 1 );
        scene.add( light );

        light = new THREE.AmbientLight( 0x404040 );
        scene.add( light );
                      
                      
        document.addEventListener('click', onDocumentClick, false );
    }

    function onDocumentClick( event ) {

        event.preventDefault();

        mouse.set( ( event.clientX / window.innerWidth ) * 2 - 1, - ( event.clientY / window.innerHeight ) * 2 + 1 );

        raycaster.setFromCamera( mouse, camera );
        raycaster.ray
        //alert("camera matrixWorld [[ " + camera.matrixWorld.elements[0].toFixed(2) + ", " + camera.matrixWorld.elements[1].toFixed(2) + ", " + camera.matrixWorld.elements[2].toFixed(2) + ", " + camera.matrixWorld.elements[3].toFixed(2) + " ],[" + camera.matrixWorld.elements[4].toFixed(2) + ", " + camera.matrixWorld.elements[5].toFixed(2) + ", " + camera.matrixWorld.elements[6].toFixed(2) + ", " + camera.matrixWorld.elements[7].toFixed(2) + "],[" + camera.matrixWorld.elements[8].toFixed(2) + ", " + camera.matrixWorld.elements[9].toFixed(2) + ", " + camera.matrixWorld.elements[10].toFixed(2) + ", " + camera.matrixWorld.elements[11].toFixed(2) + "],[" + camera.matrixWorld.elements[12].toFixed(2) + ", " + camera.matrixWorld.elements[13].toFixed(2) + ", " + camera.matrixWorld.elements[14].toFixed(2) + ", " + camera.matrixWorld.elements[15].toFixed(2) + "] ], ray origin ( " + raycaster.ray.origin.x.toFixed(2) + ", " + raycaster.ray.origin.y.toFixed(2) + ", " + raycaster.ray.origin.z.toFixed(2) + " ), direction (" + raycaster.ray.direction.x.toFixed(2) + ", " + raycaster.ray.direction.y.toFixed(2) + ", " + raycaster.ray.direction.z.toFixed(2) + " )");
        //alert("ray origin ( " + raycaster.ray.origin.x.toFixed(2) + ", " + raycaster.ray.origin.y.toFixed(2) + ", " + raycaster.ray.origin.z.toFixed(2) + " ), direction (" + raycaster.ray.direction.x.toFixed(2) + ", " + raycaster.ray.direction.y.toFixed(2) + ", " + raycaster.ray.direction.z.toFixed(2) + " )");

                      
                      
        var intersects = raycaster.intersectObjects( scene.children);

        if ( intersects.length > 0 ) {
                      
            var intersect = intersects[ 0 ];

            //alert("object found ( " + intersect.point.x.toString() + ", " + intersect.point.y.toString() + ", " + + intersect.point.z.toString() + " )");
            
            //we need to put in a switch here
            if (intersect.object.id == rollOverMesh.id) {
                  var voxel = new THREE.Mesh( cubeGeo, cubeMaterial );
                  voxel.position.copy( rollOverMesh.position );
                  voxel.position.divideScalar( .05 ).floor().multiplyScalar( .05 ).addScalar( .025 );
                  scene.add( voxel );
                  
                  objects.push( voxel );

            }
            
            rollOverMesh.position.copy( intersect.point );
            //we want the selection box to apear adjacent to newly created voxels, we have to introduce a slight bias to its position so
                      // numerical error on the intersect doesn't push it into the newly created voxel.
            rollOverMesh.position.x = rollOverMesh.position.x + (raycaster.ray.origin.x - raycaster.ray.direction.x)/1000; //bais closer to camera.
            rollOverMesh.position.y = rollOverMesh.position.y + (raycaster.ray.origin.y - raycaster.ray.direction.y)/1000; //bais closer to camera.
            rollOverMesh.position.z = rollOverMesh.position.z + 0.001; //bais selection box to apear on top of objects.
            rollOverMesh.position.divideScalar( .05 ).floor().multiplyScalar( .05 ).addScalar( .025 );
                      
            
                      
                      
        }

        renderer.render( scene, camera );

    }
    
                      
    function updateWebGLView(projectionMatrix, cameraMatrix)
    {
        if (!projectionMatrix) alert("no projection matrix");
        if (!cameraMatrix) alert("no camera matrix");

                      
        camera.projectionMatrix = projectionMatrix;
        camera.matrixAutoUpdate = false;
        camera.matrixWorld = cameraMatrix;
        //some three.js functionality relies on position being set independently of matrixWorld defining position.
        camera.position.x = cameraMatrix.elements[12];
        camera.position.y = cameraMatrix.elements[13];
        camera.position.z = cameraMatrix.elements[14];
                      
        renderer.render( scene, camera );
    }

    function matrix4FromPlainObject(plainObject)
    {
        var matrix = new THREE.Matrix4();
        matrix.set(plainObject.m00, plainObject.m01, plainObject.m02, plainObject.m03, plainObject.m10, plainObject.m11, plainObject.m12, plainObject.m13, plainObject.m20, plainObject.m21, plainObject.m22, plainObject.m23, plainObject.m30, plainObject.m31, plainObject.m32, plainObject.m33);
        return  matrix;
    }

    return module;

})(jQuery, window, Tracker, THREE);