//  Copyright (c) 2014 Caterpillar. All rights reserved.

//this code gets and manages the depth data, as well as computing distances in physical space between pixels in the image.

//variables + functions for loading and using spatial data
var spatial_data_loaded = false;
var spatial_data;

function dm_initialize(){
    //console.log('dm_initialize()');
    spatial_data_loaded = false;
    spatial_data = null;
}


//start conversion of pixel coorediantes into ray in space.
function dm_project_image_point(x, y, center_x, center_y, focal_length)
{
    return [(x - center_x)/focal_length, (y - center_y)/focal_length, 1, 1];
}

function dm_undistort_image_point(x, y, center_x, center_y, focal_length, k1, k2, k3)
{
    // Project distorted point
    var projected = [0, 0];
    var undistorted = [0, 0];
    projected[0] = (x - center_x)/focal_length;
    projected[1] = (y - center_y)/focal_length;
    // Estimate kr
    var r2 = projected[0]*projected[0] + projected[1]*projected[1];
    var r4 = r2 * r2;
    var r6 = r4 * r2;
    var kr = 1. + r2 * k1 + r4 * k2 + r6 * k3;

    // Adjust distorted projection by kr, and back project to image
    undistorted[0] = projected[0] * focal_length / kr + center_x;
    undistorted[1] = projected[1] * focal_length / kr + center_y;
    return undistorted;
}

function dm_calibrate_image_point(x, y, center_x, center_y, focal_length, k1, k2, k3)
{
    undistorted = dm_undistort_image_point(x, y, center_x, center_y, focal_length, k1, k2, k3);
    calibrated_point = dm_project_image_point(undistorted[0], undistorted[1], center_x, center_y, focal_length);
    return calibrated_point;
}

function dm_3d_location_from_pixel_location(x,y){
    var calibrated_ray;
    calibrated_ray = dm_calibrate_image_point(x, y, spatial_data.center[0], spatial_data.center[1],
            spatial_data.focal_length, spatial_data.k1, spatial_data.k2, spatial_data.k3);

    //iterate over all triangles until one is found.
    var t; //scaler which is multiplied by ray direction to get intersection of ray and triangle
    var ray_origin = [0,0,0];
    var ray_direction =  [calibrated_ray[0], calibrated_ray[1], 1];
    
    var ray_norm = Math.sqrt(ray_direction[0]*ray_direction[0] + ray_direction[1]*ray_direction[1] + ray_direction[2]*ray_direction[2])
    ray_direction[0] = ray_direction[0] / ray_norm;
    ray_direction[1] = ray_direction[1] / ray_norm;
    ray_direction[2] = ray_direction[2] / ray_norm;
    var v1 = [0,0,0], v2 = [0,0,0], v3 = [0,0,0];
    for (var i = 0; i < spatial_data['faces'].length; i++) {
        v1 = spatial_data['vertices'][spatial_data['faces'][i][0]];
        v2 = spatial_data['vertices'][spatial_data['faces'][i][1]];
        v3 = spatial_data['vertices'][spatial_data['faces'][i][2]];
        t = dm_triangle_intersect(v1, v2, v3, [0,0,0], ray_direction);
        if (t > EPSILON) {
            return [ray_direction[0]*t, ray_direction[1]*t, ray_direction[2]*t];
        }
    }
    return null;
}



function dm_cross(out, a, b) {
    out[0] = a[1]*b[2]-a[2]*b[1];
    out[1] = a[2]*b[0]-a[0]*b[2];
    out[2] = a[0]*b[1]-a[1]*b[0];
}

function dm_dot(a,b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

function dm_sub(return_v3, a, b) {
    return_v3[0] = a[0]-b[0];
    return_v3[1] = a[1]-b[1];
    return_v3[2] = a[2]-b[2];
}

var EPSILON = 0.000001;
// from wikipedia: http://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
function dm_triangle_intersect(   v1,v2,v3,  // Triangle vertices
                              O,  //Ray origin
                              D)  //Ray direction
{
    var e1 = [0,0,0], e2 = [0,0,0];  //Edge1, Edge2
    var P = [0,0,0], Q = [0,0,0], T = [0,0,0];
    var det, inv_det, u, v;
    var t;
    
    //Find vectors for two edges sharing V1
    dm_sub(e1, v2, v1);
    dm_sub(e2, v3, v1);
    //Begin calculating determinant - also used to calculate u parameter
    dm_cross(P, D, e2);
    //if determinant is near zero, ray lies in plane of triangle
    det = dm_dot(e1, P);
    //NOT CULLING
    if(det > -EPSILON && det < EPSILON) { return 0;}
    inv_det = 1 / det;
    
    //calculate distance from V1 to ray origin
    dm_sub(T, O, v1);
    
    //Calculate u parameter and test bound
    u = dm_dot(T, P) * inv_det;
    //The intersection lies outside of the triangle
    if(u < 0.0 || u > 1.0) {return 0;}
    
    //Prepare to test v parameter
    dm_cross(Q, T, e1);
    
    //Calculate V parameter and test bound
    v = dm_dot(D, Q) * inv_det;
    //The intersection lies outside of the triangle
    if(v < 0.0 || u + v  > 1.0) {return 0;}
    
    t = dm_dot(e2, Q) * inv_det;
    
    if(t > EPSILON) { //ray intersection
        return t;
    }
    
    // No hit, no win
    return 0;
}


function load_spatial_data(json_url) {   //image width needed becaues of image reversal
    //console.log('starting load_spatial_data('+json_url+')');
    $.getJSON(json_url, function(data) {
          //console.log('in spatial data calback');
          spatial_data = data;
          //console.log('number of vertixes loaded: ' + spatial_data['vertices'].length.toFixed() );
          spatial_data_loaded = true;
          //console.log('data callback finished');
      });
}

//just uses pythagorium theorem between two locations.
function distanceBetween(x1, y1, x2, y2) {
    var d3_1, d3_2, distance = null;
    try{
        d3_1 = dm_3d_location_from_pixel_location(x1,y1);
        d3_2 = dm_3d_location_from_pixel_location(x2,y2);
        distance = Math.sqrt( Math.pow((d3_1[0] - d3_2[0]), 2) + Math.pow((d3_1[1] - d3_2[1]), 2) + Math.pow((d3_1[2] - d3_2[2]), 2));
    }
    catch (err) {
        distance = null;
    }
    return distance;
}