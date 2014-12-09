//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

//this code gets and manages the depth data, including populating the depth map, as well as computing distances in physical space between pixels in the image.


var dm_node = document.createElement('dm_node');
var dm_mask_node = document.createElement('dm_mask_node');
var dm_svg;
var dm_mask_svg;

//variables + functions for loading and using spatial data
var spatial_data_loaded = false;
var spatial_data;

var dm_canvas;
var dm_context;
var dm_canvas_imgData;
var dm_mask_canvas;
var dm_mask_context;
var dm_mask_canvas_backgound_color = 'rgba(9,0,0,1)';
var dm_masking_image;
var dm_drawn = false;
var dm_mask_drawn = false;


function dm_initialize(){
    //console.log('dm_initialize()');
    spatial_data_loaded = false;
    dm_drawn = false;
    dm_mask_drawn = false;
    spatial_data = null;
    avg_depth_sqr = 0;
    min_depth_sqr = 100000000;
    max_depth_sqr = 0;
    if(dm_svg && draw_g.node.contains(dm_svg.node)) {draw_g.node.removeChild(dm_svg.node);}
    if(dm_mask_svg && draw_g.node.contains(dm_mask_svg.node)) {draw_g.node.removeChild(dm_mask_svg.node);}
    dm_svg = SVG(dm_node);
    dm_mask_svg = SVG(dm_mask_node);
    dm_canvas = null;
    dm_context = null;
    dm_canvas_imgData = null;
    dm_mask_canvas= null;
    dm_mask_context = null;
}

function dm_size(x,y){
    dm_svg.size(x,y);
    rcMessage.post("Loading Depth Map ...", 2000);

    dm_canvas= document.createElement('canvas');
    dm_canvas.id     = "dm_canvas";
    dm_canvas.width  = x;
    dm_canvas.height = y;
    dm_context = dm_canvas.getContext('2d');
    dm_canvas_imgData = dm_context.createImageData(x,y);
    //we're just updataing the alpha here, put in a local variable for faster reference.
    //var canvas_optimizer = dm_canvas_imgData.data;
    //for(var i = 3; i < dm_canvas_imgData.data.length; i+= 4) {
     //   canvas_optimizer[i] = 200;
    //}

    dm_context.strokeStyle = 'rgba(0,0,0,.3)'; //for line stroking, so that we don't have to update stroke style in drawing loop.
}

function dm_mask_size(x,y){
    dm_mask_svg.size(x,y);

    dm_mask_canvas= document.createElement('canvas');
    dm_mask_canvas.id     = "dm_canvas";
    dm_mask_canvas.width  = x;
    dm_mask_canvas.height = y;
    
    dm_mask_context = dm_mask_canvas.getContext('2d');
    dm_mask_context.fillStyle   = dm_mask_canvas_backgound_color;
    dm_mask_context.fillRect  (0,   0, dm_mask_canvas.width, dm_mask_canvas.height);
    dm_mask_context.globalCompositeOperation = 'destination-out'; //so subsequent drawing remvoes content
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

function dm_3d_location_from_pixel_in_face(x,y,v1,v2,v3){
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
    t = dm_triangle_intersect(v1, v2, v3, [0,0,0], ray_direction);
    if (t > EPSILON) {
        return [ray_direction[0]*t, ray_direction[1]*t, ray_direction[2]*t];
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

function finalize_dm(){
    //console.log('finalize_dm()');
    //rcMessage.clear();
    
    dm_context.putImageData(dm_canvas_imgData,0,0);
    
    img_clone =image.clone();
    mask_clone = dm_masking_image.clone();
    //img_clone.filter(function(add) {
    //                 add.colorMatrix('saturate', 0); //desatureate
    //                 //add.componentTransfer({ rgb: { type: 'linear', slope: 1.5, intercept: 0.2 }}); //lighten
    //                  });
    dm_svg.add(img_clone);
    dm_svg.image(dm_canvas.toDataURL("image/png")); //image/png is a mime type.
    dm_svg.add(mask_clone);
    dm_context = null;
    dm_canvas = null;
    
    dm_drawn = true;
    //console.log('finished finalize_dm()');

}

function finalize_dm_mask(){
    //console.log('finalize_dm_mask()');
    
    //do the same for the masking canvas
    img_clone2 =image.clone();
    dm_mask_svg.add(img_clone2);
    dm_masking_image = dm_mask_svg.image(dm_mask_canvas.toDataURL("image/png")); //image/png is a mime type.
    dm_mask_context = null;
    dm_mask_canvas = null;
    
    dm_mask_drawn = true;
    //console.log('finished finalize_dm_mask()');
}


var avg_depth_sqr = 0;
var min_depth_sqr = 100000000;
var max_depth_sqr = 0;


function fill_dm_mask(){
    
    if (!spatial_data_loaded) {return false;}
    
    dm_mask_size(image_width, image_height);
    
    for (var i = 0; i < spatial_data['faces'].length; i++) { //now that we know the avg depth, draw to the canvas...
        v1 = spatial_data['vertices'][spatial_data['faces'][i][0]];
        v2 = spatial_data['vertices'][spatial_data['faces'][i][1]];
        v3 = spatial_data['vertices'][spatial_data['faces'][i][2]];
        
        dm_mask_context.moveTo(v1[3],v1[4]);
        dm_mask_context.lineTo(v2[3],v2[4]);
        dm_mask_context.lineTo(v3[3],v3[4]);
        dm_mask_context.lineTo(v1[3],v1[4]);

    }
    dm_mask_context.fill();

    finalize_dm_mask();
    
}

function fill_depth_map(){
    if (!spatial_data_loaded) {return false;}
    //size depthmap
    time_in_iner_triangle_loop = 0
    dm_size(image_width,image_height);

    //iterate over spatial data and asign colors to each location based on total depth. will take two itterations. one to find maximal depth in image, another to create pixels.
    var total_depth_sqr =0;
    var current_depth_sqr;
    var v1,v2,v3;
    var cnvs_img_data = dm_canvas_imgData.data;
    
    // calculate the average depth so we understand how to color
    rcMessage.post('loading depth data \n calculating average depth...',2000);
    
    var avg_calc_start = new Date().getTime();
//    window.setTimeout(function () {
        for (var i = 0; i < spatial_data['faces'].length; i++) {
            v1 = spatial_data['vertices'][spatial_data['faces'][i][0]];
            v2 = spatial_data['vertices'][spatial_data['faces'][i][1]];
            v3 = spatial_data['vertices'][spatial_data['faces'][i][2]];
            current_depth_sqr = (v1[0]*v1[0] +
                                 v1[1]*v1[1] +
                                 v1[2]*v1[2] +
                                 v2[0]*v2[0] +
                                 v2[1]*v2[1] +
                                 v2[2]*v2[2] +
                                 v3[0]*v3[0] +
                                 v3[1]*v3[1] +
                                 v3[2]*v3[2]
                                 )/3;
            if (current_depth_sqr < min_depth_sqr) {min_depth_sqr = current_depth_sqr;}
            if (current_depth_sqr > max_depth_sqr) {max_depth_sqr = current_depth_sqr;}
            total_depth_sqr = total_depth_sqr + current_depth_sqr;
        }
  
    
        avg_depth_sqr = total_depth_sqr/spatial_data['vertices'].length;
        var avg_calc_time = new Date().getTime() - avg_calc_start;

    
        var coords;
        
        var draw_start = new Date().getTime();
        rcMessage.post('loading depth data \n drawing triangles...',500);
      //window.setTimeout ( function () {
            var dm_t = [[0,0,0,0,0],[0,0,0,0,0],[0,0,0,0,0]];
            for (var i = 0; i < spatial_data['faces'].length; i++) { //now that we know the avg depth, draw to the canvas...
                v1 = spatial_data['vertices'][spatial_data['faces'][i][0]];
                v2 = spatial_data['vertices'][spatial_data['faces'][i][1]];
                v3 = spatial_data['vertices'][spatial_data['faces'][i][2]];
                // if we want these to be rotated, set x to img_width - v[4] and y to v[3]
                dm_t[0][0] = v1[3]; // image x coordinate
                dm_t[1][0] = v2[3];
                dm_t[2][0] = v3[3];
                dm_t[0][1] = v1[4]; // image y coordinates
                dm_t[1][1] = v2[4];
                dm_t[2][1] = v3[4];
                dm_t[0][2] = v1[0]; //spatial x
                dm_t[1][2] = v2[0];
                dm_t[2][2] = v3[0];
                dm_t[0][3] = v1[1]; //spatial y
                dm_t[1][3] = v2[1];
                dm_t[2][3] = v3[1];
                dm_t[0][4] = v1[2];  //spatial z
                dm_t[1][4] = v2[2];
                dm_t[2][4] = v3[2];

                dm_imgData_traingleLoop(dm_t, cnvs_img_data); //draw the color to each triangle w/ gradient
            }


            var draw_end = new Date().getTime();
    
            finalize_dm();
        rcMessage.post('triangel time = ' + Math.abs(draw_end-draw_start).toString() + '\n avg_calc_time = ' +  Math.abs(avg_calc_time).toString(), 5000);
        console.log('triangel time = ' + (draw_end-draw_start).toString() + '\n avg_calc_time = ' +  avg_calc_time.toString() + ' \n inner loop = ' + time_in_iner_triangle_loop.toString() );
        //}, 0);
   //  }, 0);
}


function dm_add_traingle(vs){
    //a triangle describes a plain, where x and y are image coordinates, and z is depth.
    //we will find the equation of the plain to calculated dx,dy in order to generate a gradiant.
    var v1 = [vs[0][0],vs[0][1],0], v2 = [vs[1][0],vs[1][1],0], v3 = [vs[2][0],vs[2][1],0];
    v1[2] = vs[0][2]*vs[0][2]+vs[0][3]*vs[0][3]+vs[0][4]*vs[0][4]
    v2[2] = vs[1][2]*vs[1][2]+vs[1][3]*vs[1][3]+vs[1][4]*vs[1][4]
    v3[2] = vs[2][2]*vs[2][2]+vs[2][3]*vs[2][3]+vs[2][4]*vs[2][4]
    
    var v12=[0,0,0], v13=[0,0,0], norm_vec = [0,0,0];
    dm_sub(v12, v2, v1);      //calculate cross product of edge vectors to get normal vector.
    dm_sub(v13, v3, v1);
    dm_cross(norm_vec, v12, v13);
    //var d = dm_dot(norm_vec, v3); //calculate offset constant using normal
    //console.log(JSON.stringify(norm_vec));
    //console.log('ds = ' + dm_dot(norm_vec, v1).toFixed(3) + ', ' + dm_dot(norm_vec, v2).toFixed(3) + ', ' + dm_dot(norm_vec, v3).toFixed(3));
    //we now have solved for the equation of the plane ax + by +cz = d where x and y represent pixel location and z represent depth
    //dz/dx and dz/dy tell us the direction the gradient needs to have,
    var max_z, grad_start, grad_end = [0,0,0], grad_diff = [0,0,0];
    max_z = v1;
    grad_start = v1; //start gradiant at the closest vertex to camera
    if (max_z[2] <= v2[2]) {max_z = v2}
    if (max_z[2] <= v3[2]) {max_z = v3}
    if (grad_start[2] >= v2[2]) {grad_start = v2}
    if (grad_start[2] >= v3[2]) {grad_start = v3}

    var delta_x = (max_z[2] - grad_start[2]) * -1 * norm_vec[2] * norm_vec[0] / (norm_vec[0] * norm_vec[0] + norm_vec[1] * norm_vec[1]);
    var delta_y = norm_vec[1] / norm_vec[0] * delta_x;
    grad_end[0] = grad_start[0] + delta_x;
    grad_end[1] = grad_start[1] + delta_y;
    grad_end[2] = max_z[2];
    
    //console.log('d from grad_end = ' + dm_dot(norm_vec, grad_end).toFixed(3) + '  d from grad_start = ' + dm_dot(norm_vec, grad_start).toFixed(3))
    
    var grad = dm_context.createLinearGradient(grad_start[0], grad_start[1], grad_end[0], grad_end[1]); //create gradients from vectors
    grad.addColorStop(0, dm_clr_from_depth( grad_start[2]));
    grad.addColorStop(1, dm_clr_from_depth( grad_end[2]));

    dm_context.beginPath();
    dm_context.moveTo(vs[0][0],vs[0][1]);
    dm_context.lineTo(vs[1][0],vs[1][1]);
    dm_context.lineTo(vs[2][0],vs[2][1]);
    dm_context.fillStyle= grad;
    dm_context.fill();
    dm_context.stroke();
    
}

function dm_triangle_sign (p1, p2, p3)
{
    return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
}

function dm_PointInTriangle (pt, v1, v2, v3)
{
    var b1, b2, b3;
    
    b1 = dm_triangle_sign(pt, v1, v2) < 0.0001;
    b2 = dm_triangle_sign(pt, v2, v3) < 0.0001;
    b3 = dm_triangle_sign(pt, v3, v1) < 0.0001;
    
    return ((b1 == b2) && (b2 == b3));
}

var time_in_iner_triangle_loop  = 0;
function dm_imgData_add_traingle(vs, cnvs_img_data){
    //a triangle describes a plain, where x and y are image coordinates, and z is depth.
    //we will find the equation of the plain to calculated dx,dy in order to generate a gradiant.
    var v0 = [Math.round(vs[0][0]),Math.round(vs[0][1]),0], v1 = [Math.round(vs[1][0]),Math.round(vs[1][1]),0], v2 = [Math.round(vs[2][0]),Math.round(vs[2][1]),0];
    v0[2] = vs[0][2]*vs[0][2]+vs[0][3]*vs[0][3]+vs[0][4]*vs[0][4]
    v1[2] = vs[1][2]*vs[1][2]+vs[1][3]*vs[1][3]+vs[1][4]*vs[1][4]
    v2[2] = vs[2][2]*vs[2][2]+vs[2][3]*vs[2][3]+vs[2][4]*vs[2][4]
    
    //determine equaiton of plan s.t. we can subsequently calculate depth
    var v12=[0,0,0], v13=[0,0,0], norm_vec = [0,0,0];
    dm_sub(v12, v1, v0);      //calculate cross product of edge vectors to get normal vector.
    dm_sub(v13, v2, v0);
    dm_cross(norm_vec, v12, v13);
    var d = dm_dot(norm_vec, v2); //calculate offset constant using normal
    var doc = d/norm_vec[2];
    var aoc = norm_vec[0]/norm_vec[2];
    var boc = norm_vec[1]/norm_vec[2];
    
    // figure out bounds of iteration
    var min_x = v0[0], max_x = v0[0], min_y = v0[1], max_y = v0[1];
    if (v0[0]>v1[0]||v0[0]>v2[0]) { min_x = Math.min(v1[0], v2[0]); }
    if (v0[0]<v1[0]||v0[0]<v2[0]) { max_x = Math.max(v1[0], v2[0]); }
    if (v0[1]>v1[1]||v0[1]>v2[1]) { min_y = Math.min(v1[1], v2[1]); }
    if (v0[1]<v1[1]||v0[1]<v2[1]) { max_y = Math.max(v1[1], v2[1]); }
    
    var cv, s, t, id_indx, clrx;
    //inspired by http://stackoverflow.com/questions/2049582/how-to-determine-a-point-in-a-triangle
    var area = 1/2 * (-v1[1] * v2[0] + v0[1] * (-v1[0] + v2[0]) + v0[0] * (v1[1] - v2[1]) + v1[0] * v2[1]);
    var sign = area < 0 ? -1 : 1;
    var s_b = (v0[1] * v2[0] - v0[0] * v2[1])* sign;
    var s_x = (v2[1] - v0[1]) * sign;
    var s_y = (v0[0] - v2[0]) * sign;
    var t_b = (v0[0] * v1[1] - v0[1] * v1[0]) * sign;
    var t_x = (v0[1] - v1[1]) * sign;
    var t_y = (v1[0] - v0[0]) * sign;
    var areaxsignx2 = 2.2 * area * sign;
    var color_denominator = (max_depth_sqr - min_depth_sqr);

    //find maxx vertex, minx vertex, midx vertex. (if tied doesn't matter)
    //find formula for maxx_x->minx, maxx->midx, midx-minx lines.
    //find if midx vertext to left or right of maxx->minx line
    
    //for maxx to minx
    
    //for y value at that x on maxx->minx to
        //if above midx midx->maxx line at that x
        //else midx->minx line at that x
           //calc depth and draw depth.
    
    //iterate over triangle and fill imageData w/ color
    var innerLoopStart = new Date().getTime();
    for(var i = min_x; i <= max_x; i+=1){
        for (var j = min_y; j <= max_y; j+=1) {
            s = s_b + s_x * i + s_y * j;
            t = t_b + t_x * i + t_y * j;
            if (s > -0.1 && t > -0.1 && (s + t) < areaxsignx2) {
                //moved dm_clr_values_from_depth code into loop to try to further optimize.
                // depth, z = (d - ax - by) / c
                clrx = ((doc - aoc * i - boc * j) - min_depth_sqr)/color_denominator*1023;
                cnvs_img_data.set( [460 - clrx,
                                    460 - Math.abs(clrx-500),
                                    clrx - 460,
                                    64], ((j-1)*image_width+i)*4);
            }
        }
    }
    time_in_iner_triangle_loop += (innerLoopStart - new Date().getTime());
    
}

function dm_imgData_traingleLoop(vs, cnvs_img_data){
    //a triangle describes a plain, where x and y are image coordinates, and z is depth.
    //we will find the equation of the plain to calculated dx,dy in order to generate a gradiant.
    var v0 = [Math.round(vs[0][0]),Math.round(vs[0][1]),0], v1 = [Math.round(vs[1][0]),Math.round(vs[1][1]),0], v2 = [Math.round(vs[2][0]),Math.round(vs[2][1]),0];
    v0[2] = vs[0][2]*vs[0][2]+vs[0][3]*vs[0][3]+vs[0][4]*vs[0][4]
    v1[2] = vs[1][2]*vs[1][2]+vs[1][3]*vs[1][3]+vs[1][4]*vs[1][4]
    v2[2] = vs[2][2]*vs[2][2]+vs[2][3]*vs[2][3]+vs[2][4]*vs[2][4]
    
    //determine equaiton of plan s.t. we can subsequently calculate depth
    var v12=[0,0,0], v13=[0,0,0], norm_vec = [0,0,0];
    dm_sub(v12, v1, v0);      //calculate cross product of edge vectors to get normal vector.
    dm_sub(v13, v2, v0);
    dm_cross(norm_vec, v12, v13);
    var d = dm_dot(norm_vec, v2); //calculate offset constant using normal
    var doc = d/norm_vec[2];
    var aoc = norm_vec[0]/norm_vec[2];
    var boc = norm_vec[1]/norm_vec[2];
    
    //find maxx vertex, minx vertex, midx vertex. (if tied doesn't matter)
    var min_x = v0, max_x = v0, mid_x = v0;
    if (v0[0]>=v1[0]&&v0[0]>=v2[0])
    {
        if (v1[0]>v2[0]) { mid_x = v1; min_x = v2}
        else {mid_x = v2; min_x = v1}
    }
    else if (v1[0]>=v0[0]&&v1[0]>=v2[0])
    {
        max_x = v1;
        if (v0[0]>v2[0]) { mid_x = v0; min_x = v2}
        else {mid_x = v2; min_x = v0}
    }
    else //v2max
    {
        max_x = v2;
        if (v0[0]>v1[0]) { mid_x = v0; min_x = v1}
        else {mid_x = v1; min_x = v0}
    }
    
    if (mid_x[0] == max_x[0]) {
        max_x[0] = max_x[0]+1;
    }
    
    var clrx;
    var color_denominator = (max_depth_sqr - min_depth_sqr);
    
    var no_triangle_drawn = true;
    
    //find formula for maxx_x->minx, maxx->midx, midx-minx lines.
    var btc, bmc, mtc;
    if (max_x[0] - min_x[0] != 0){  //bottom->top c
        btc = (max_x[1] - min_x[1]) / (max_x[0] - min_x[0]);
    }
    else {
        btc = (max_x[1] - min_x[1]) / (1/(max_x[1] - min_x[1]));
    }
    if (mid_x[0] - min_x[0] != 0){
        bmc = (mid_x[1] - min_x[1])/(mid_x[0] - min_x[0]); //botom->mid c
    }
    else {
        bmc = (mid_x[1] - min_x[1]) / (1/(mid_x[1] - min_x[1]));
    }
    if (max_x[0] - mid_x[0] != 0) {
        mtc = (max_x[1] - mid_x[1])/ (max_x[0] - mid_x[0]); //mid->top c
    }
    else {
        mtc = (max_x[1] - mid_x[1]) / (1/(max_x[1] - mid_x[1]));
    }
    
    //find if midx vertext to left or right of maxx->minx line
    var mid_less_bt = (mid_x[1] < (min_x[1] + btc*(mid_x[0]-min_x[0])));
    //for maxx to minx
    var max_j, min_j;
    var innerLoopStart = new Date().getTime();
    for( var i = min_x[0]; i < max_x[0]; i++){
        //for y value at that x on maxx->minx to
        //if above midx midx->maxx line at that x
        //else midx->minx line at that x
        if (mid_less_bt) {
            max_j = Math.round( min_x[1] + btc*(i-min_x[0]));
            if (i > mid_x[0]){ min_j =  Math.floor(mid_x[1] + mtc*(i-mid_x[0])); }
            else {  min_j = Math.floor(min_x[1] + bmc*(i-min_x[0])); }
        }
        else {
            min_j = Math.floor(min_x[1] + btc*(i-min_x[0]));
            if (i > mid_x[0]) { max_j = Math.round(mid_x[1] + mtc*(i-mid_x[0])); }
            else{ max_j = Math.round(min_x[1] + bmc*(i-min_x[0])); }
        }
        if (i > 0 && i <= image_width){
            //calc depth and draw depth.
            for (var j = min_j; j <= max_j; j++){
                //moved dm_clr_values_from_depth code into loop to try to further optimize.
                // depth, z = (d - ax - by) / c
                clrx = ((doc - aoc * i - boc * j) - min_depth_sqr)/color_denominator*1023;
                if (  j > 0 && j <= image_height){
                    cnvs_img_data.set( [460 - clrx,
                                        460 - Math.abs(clrx-500),
                                        clrx - 460,
                                        64], ((j-1)*image_width+i)*4);
                    no_triangle_drawn = false;
                }
                
            }
        }
    }
    
    if (no_triangle_drawn) {
        console.log('no tirangle drawn');
    }
    
    time_in_iner_triangle_loop += (innerLoopStart - new Date().getTime());
    
}


function grey_dm_clr_from_depth( current_depth_sqr) {
    var clr_int_str =  (255*(avg_depth_sqr-min_depth_sqr)/1.2/((avg_depth_sqr-min_depth_sqr)/1.2 + (current_depth_sqr-min_depth_sqr))).toFixed(0);
    return 'rgba('+clr_int_str+','+clr_int_str+','+clr_int_str+',0.5)';
}

function dm_clr_from_depth( current_depth_sqr) {
    var cv = dm_clr_values_from_depth(current_depth_sqr);
    return 'rgba('+cv[0].toFixed()+','+cv[1].toFixed()+','+cv[2].toFixed()+',0.25)';
}

function dm_clr_values_from_depth( current_depth_sqr) {
    var dm_clrv_x;
    try {
        dm_clrv_x = (current_depth_sqr - min_depth_sqr)/(max_depth_sqr - min_depth_sqr);
    }
    catch(err) {
        dm_clrv_x = 0.5;
    }
    return [ Math.min( Math.max( 255*4*(0.45-dm_clrv_x), 0), 255),
             Math.min( Math.max( 255*4*(0.45 - Math.abs(dm_clrv_x-0.5)), 0), 255),
             Math.min( Math.max( 255*4*(dm_clrv_x-0.45), 0), 255),
            64];
}



function load_spatial_data(json_url) {   //image width needed becaues of image reversal
    //console.log('starting load_spatial_data('+json_url+')');
    $.getJSON(json_url, function(data) {
          //console.log('in spatial data calback');
          spatial_data = data;
          //console.log('number of vertixes loaded: ' + spatial_data['vertices'].length.toFixed() );
          spatial_data_loaded = true;
          //console.log('data callback finished');
          if (!dm_mask_drawn) {fill_dm_mask();} //draw dm_mask on load so that it can be used in annimations around missing data
      });
}


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

function distanceTo(x1, y1) {
    //cammera is at 0,0,0
    var d3_1, distance = null;
    try{
        d3_1 = dm_3d_location_from_pixel_location(x1,y1);
        distance = Math.sqrt( Math.pow((d3_1[0]), 2) + Math.pow((d3_1[1]), 2) + Math.pow((d3_1[2]), 2));
    }
    catch (err) {
        distance = null;
    }
    return distance;
}



