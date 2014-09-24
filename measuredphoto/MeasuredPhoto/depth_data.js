//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

//this code gets and manages the depth data, including populating the depth map, as well as computing distances in physical space between pixels in the image.


var dm_node = document.createElement('dm_node');
var dm_svg;

//variables + functions for loading and using spatial data
var spatial_data_loaded = false;
var spatial_data;

var dm_canvas;
var dm_context;
var dm_drawn = false;
var dm_loading_message;


var dm_center_x, dm_center_y;

function dm_initialize(){
    spatial_data_loaded = false;
    dm_drawn = false;
    spatial_data = null;
    avg_depth_sqr = 0;
    min_depth_sqr = 100000000;
    max_depth_sqr = 0;
    if(dm_svg && draw.node.contains(dm_svg.node)) {draw_g.node.removeChild(dm_svg.node);}
    dm_svg = SVG(dm_node);
    dm_canvas= document.createElement('canvas');
    dm_canvas.id     = "dm_canvas";
    dm_canvas.width  = 100;
    dm_canvas.height = 100;
    dm_canvas.style.position = "absolute";
    dm_context = dm_canvas.getContext('2d');
    dm_loading_message = dm_svg.text("Loading Depth Map ...")

}

function dm_size(x,y){
    dm_svg.size(x,y);
    dm_canvas.width  = x;
    dm_canvas.height = y;
    dm_context.fillStyle   = 'rgba(0,0,0,0.4)';
    dm_context.fillRect  (0,   0, dm_canvas.width, dm_canvas.height);
}


//function to estimate lens distortion
function dm_estimate_kr(point, k1, k2, k3)
{
    r2 = point[0]*point[0] + point[1]*point[1];
    r4 = r2 * r2;
    r6 = r4 * r2;
    kr = 1. + r2 * k1 + r4 * k2 + r6 * k3;
    return kr;
}

//apply lens distortion correction
function dm_calibrate_im_point(normalized_point, k1, k2, k3)
{
    var kr;
    var calibrated_point = normalized_point;
    //forward calculation - guess calibrated from initial
    kr = dm_estimate_kr(normalized_point, k1, k2, k3);
    calibrated_point[0] /= kr;
    calibrated_point[1] /= kr;
    //backward calculation - use calibrated guess to get new parameters and recompute
    kr = dm_estimate_kr(calibrated_point, k1, k2, k3);
    calibrated_point = normalized_point;
    calibrated_point[0] /= kr;
    calibrated_point[1] /= kr;
    return calibrated_point;
}

//start conversion of pixel coorediantes into ray in space.
function dm_project_point(x, y, center_x, center_y, focal_length)
{
    return [(x - center_x)/focal_length, (y - center_y)/focal_length, 1, 1];
}

function dm_3d_location_from_pixel_location(x,y){
    var ray = dm_project_point(x, y, dm_center_x, dm_center_y, spatial_data.focal_length);
    var calibrated_ray = dm_calibrate_im_point(ray, spatial_data.k1, spatial_data.k2, spatial_data.k3);
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


var dm_clr;
function dm_add_point(percent_depth,x,y,size){
    // dm_svg.rect(size,size).move(x,y).fill('#fff').opacity(percent_depth);
    dm_clr = (255*percent_depth).toFixed(0);
    dm_context.fillStyle = 'rgba('+dm_clr+','+dm_clr+','+dm_clr+',.3)';
    dm_context.fillRect(x - size/2,   y-size/2, size, size);
}

function finalize_dm(){
    dm_loading_message.remove();
    delete dm_loading_message;
    img_clone =image.clone();
    dm_svg.add(img_clone);
    img_clone.filter(function(add) {
                      add.colorMatrix('saturate', 0)
                      }); //desatureate
    dm_svg.image(dm_canvas.toDataURL("image/png")); //image/png is a mime type.
    dm_context = null;
}


var avg_depth_sqr = 0;
var min_depth_sqr = 100000000;
var max_depth_sqr = 0;

function fill_depth_map(){
    //iterate over spatial data and asign colors to each location based on total depth. will take two itterations. one to find maximal depth in image, another to create pixels.
    var total_depth_sqr =0;
    var current_depth_sqr;
    var v1,v2,v3;
    
    // calculate the average depth so we understand how to color
    dm_loading_message.text('loading depth data \n calculating average depth...');
    
    window.setTimeout(function () {
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
        var coords;
        
        var draw_start = new Date();
        dm_loading_message.text('loading depth data \n drawing tiangles...');
      window.setTimeout ( function () {
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

                dm_add_traingle(dm_t);
            }
            

            var draw_end = new Date();
            console.log('compute time for triangels = ' + Math.abs(draw_end-draw_start).toString());
            
            finalize_dm();
        }, 5);
     }, 5);
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

    
}

function grey_dm_clr_from_depth( current_depth_sqr) {
    var clr_int_str =  (255*(avg_depth_sqr-min_depth_sqr)/1.2/((avg_depth_sqr-min_depth_sqr)/1.2 + (current_depth_sqr-min_depth_sqr))).toFixed(0);
    return 'rgba('+clr_int_str+','+clr_int_str+','+clr_int_str+',0.3)';
}

function dm_clr_from_depth( current_depth_sqr) {
    var x
    try {
        x = (current_depth_sqr - min_depth_sqr)/(max_depth_sqr - min_depth_sqr);
    }
    catch(err) {
        x = 0.5;
    }
    var blue = Math.min( Math.max( 255*4*(x-0.45), 0), 255);
    var red  = Math.min( Math.max( 255*4*(0.45-x), 0), 255);
    var green= Math.min( Math.max( 255*4*(.45 - Math.abs(x-0.5)), 0), 255);
    return 'rgba('+red.toFixed()+','+green.toFixed()+','+blue.toFixed()+',0.3)';
}


//face is specified as an array of 3 vertices, each one x,y,z - z is not used
function point_in_triangle(x,y,face, tol){ // tol for tolarnce, how far outside in pixels the point can be from the traingle.
    //test that the x,y coordinate is in the larger rectangel that bounds the triangle looking at max x,y, min x,y
    if ((x-tol>face[0][0]) && (x-tol>face[1][0]) && (x-tol>face[2][0]) ){return false;}
    if ((x+tol<face[0][0]) && (x+tol<face[1][0]) && (x+tol<face[2][0]) ){return false;}
    if ((y-tol>face[0][1]) && (y-tol>face[1][1]) && (y-tol>face[2][1]) ){return false;}
    if ((y+tol<face[0][1]) && (y+tol<face[1][1]) && (y+tol<face[2][1]) ){return false;}
    
    //if so, test taht the x,y coordinate is actually inside the triangle by calculating slope boundaries.
    //we add and subtract 1 to x so that if a traingle boundtry cuts though a pixel we count it as being in that traingle.
    //the traingles will therefor 'overlap' a bit randomely, but this should eliminate the chance of not finind a traingle for a pixel.
    var a, b, c;
    a = (face[1][1]-face[0][1])/(face[1][0]-face[0][0]);
    b = face[0][1];
    if (( b + a*(face[2][0]-face[0][0]) > face[2][1] ) && (b + a*(x-face[0][0])+tol < y )){return false;}
    if (( b + a*(face[2][0]-face[0][0]) < face[2][1] ) && (b + a*(x-face[0][0])-tol > y )){return false;}
    a = (face[2][1]-face[0][1])/(face[2][0]-face[0][0]);
    b = face[0][1];
    if (( b + a*(face[1][0]-face[0][0]) > face[1][1] ) && (b + a*(x-face[0][0])+tol < y )){return false;}
    if (( b + a*(face[1][0]-face[0][0]) < face[1][1] ) && (b + a*(x-face[0][0])-tol > y )){return false;}
    a = (face[2][1]-face[1][1])/(face[2][0]-face[1][0]);
    b = face[1][1];
    if (( b + a*(face[0][0]-face[1][0]) > face[0][1] ) && (b + a*(x-face[1][0])+tol < y )){return false;}
    if (( b + a*(face[0][0]-face[1][0]) < face[0][1] ) && (b + a*(x-face[1][0])-tol > y )){return false;}
    
    return true;
}

// face is a 3 element array, each element is an array with: [ img_x, img_y, 3d_x, 3d_y, 3d_z]
function d3_coord_for_img_x_y(x,y,face){
    // we use the formula for the intersection of two lines to determine the point at which a ray (from face[0][img_x], face[0][img_y]) through x,y
    // intersects the line segment from (from face[1][img_x], face[1][img_y]) two (from face[2][img_x], face[2][img_y]).
    var inter_x = ((face[0][0]*y-face[0][1]*x)*(face[1][0]-face[2][0]) - (face[0][0]-x)*(face[1][0]*face[2][1]-face[1][1]*face[2][0])) /
                  ((face[0][0] - x) * ( face[1][1] - face[2][1] )      - (face[0][1]-y)*(face[1][0]-face[2][0]) );
    
    var inter_y = ((face[0][0]*y-face[0][1]*x)*(face[1][1]-face[2][1]) - (face[0][1]-y)*(face[1][0]*face[2][1]-face[1][1]*face[2][0])) /
                  ((face[0][0] - x) * ( face[1][1] - face[2][1] )      - (face[0][1]-y)*(face[1][0]-face[2][0]) );
    
    // we can now calculate a linear combinatino of face[1] and face[2]'s 3D coordinates at the intesect based on the relative distance from each
    var d1 = Math.sqrt(Math.pow(face[1][0]-inter_x,2)+Math.pow(face[1][1]-inter_y,2));
    var d2 = Math.sqrt(Math.pow(face[2][0]-inter_x,2)+Math.pow(face[2][1]-inter_y,2));
    var inter_coords = [ face[1][2] + d1/(d1+d2)*(face[2][2]-face[1][2]),
                         face[1][3] + d1/(d1+d2)*(face[2][3]-face[1][3]),
                         face[1][4] + d1/(d1+d2)*(face[2][4]-face[1][4])];
    
    
    // we can then caluclate a linear comination of 3D coordinates from the intersect point and face[1] based on the reltaive distance to x,y.
    d1 = Math.sqrt(Math.pow(face[0][0] - x,2)+Math.pow(face[0][1]-y,2));
    d2 = Math.sqrt(Math.pow(inter_x - x,2)   +Math.pow(inter_y-y,2));
    return [face[0][2] + d1/(d1+d2)*(inter_coords[0]-face[0][2]),
            face[0][3] + d1/(d1+d2)*(inter_coords[1]-face[0][3]),
            face[0][4] + d1/(d1+d2)*(inter_coords[2]-face[0][4])];
    
}


function load_spatial_data(json_url) {   //image width needed becaues of image reversal
    console.log('starting load_spatial_data('+json_url+')');
    $.getJSON(json_url, function(data) {
          console.log('in spatial data calback');
          spatial_data = data;
          console.log('number of vertixes loaded: ' + spatial_data['vertices'].length.toFixed() );
          spatial_data_loaded = true;
          dm_center_x = image_width / 2; //relies on global image width/height having been set in main
          dm_center_y = image_height / 2;
          console.log('data callback finished');
      });
}

function spatial_coord_for_img_coord(x,y){
    for (var i = 0; i < spatial_data['faces'].length; i++) {
        
        
        if (point_in_triangle(x,y,[[image_width-spatial_data['vertices'][spatial_data['faces'][i][0]][4],spatial_data['vertices'][spatial_data['faces'][i][0]][3],0],
                                   [image_width-spatial_data['vertices'][spatial_data['faces'][i][1]][4],spatial_data['vertices'][spatial_data['faces'][i][1]][3],0],
                                   [image_width-spatial_data['vertices'][spatial_data['faces'][i][2]][4],spatial_data['vertices'][spatial_data['faces'][i][2]][3],0]],1)){
            var v1,v2,v3;
            var dm_t = [[0,0,0,0,0,0],[0,0,0,0,0],[0,0,0,0,0]];
            v1 = spatial_data['vertices'][spatial_data['faces'][i][0]];
            v2 = spatial_data['vertices'][spatial_data['faces'][i][1]];
            v3 = spatial_data['vertices'][spatial_data['faces'][i][2]];
            dm_t[0][0] = image_width-v1[4];
            dm_t[1][0] = image_width-v2[4];
            dm_t[2][0] = image_width-v3[4];
            dm_t[0][1] = v1[3];
            dm_t[1][1] = v2[3];
            dm_t[2][1] = v3[3];
            dm_t[0][2] = v1[0];
            dm_t[1][2] = v2[0];
            dm_t[2][2] = v3[0];
            dm_t[0][3] = v1[1];
            dm_t[1][3] = v2[1];
            dm_t[2][3] = v3[1];
            dm_t[0][4] = v1[2];
            dm_t[1][4] = v2[2];
            dm_t[2][4] = v3[2];
            return d3_coord_for_img_x_y(x,y,dm_t);
        }
        
    }
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

