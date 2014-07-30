//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

//this code gets and manages the depth data, including populating the depth map, as well as computing distances in physical space between pixels in the image.


var dm_node = document.createElement('dm_node');
var dm_svg;

//variables + functions for loading and using spatial data
var spatial_data_loaded = false;
var spatial_data;



var dm_canvas;
var dm_context;

function dm_initialize(){
    spatial_data_loaded = false;
    spatial_data = null;
    dm_svg = SVG(dm_node);
    dm_canvas= document.createElement('canvas');
    dm_canvas.id     = "dm_canvas";
    dm_canvas.width  = 100;
    dm_canvas.height = 100;
    dm_canvas.style.position = "absolute";
    dm_context = dm_canvas.getContext('2d');
}

function dm_size(x,y){
    dm_svg.size(x,y);
    dm_canvas.width  = x;
    dm_canvas.height = y;
    dm_context.fillStyle   = '#000066';
    dm_context.fillRect  (0,   0, dm_canvas.width, dm_canvas.height);
}

var dm_clr;
function dm_add_point(percent_depth,x,y,size){
    // dm_svg.rect(size,size).move(x,y).fill('#fff').opacity(percent_depth);
    dm_clr = (255*percent_depth).toFixed(0);
    dm_context.fillStyle = 'rgb('+dm_clr+','+dm_clr+','+dm_clr+')';
    dm_context.fillRect(x - size/2,   y-size/2, size, size);
}

function finalize_dm(){
    dm_svg.image(dm_canvas.toDataURL("image/png")); //image/png is a mime type.
    dm_context = null;
}

function fill_depth_map(img_width){
    //iterate over spatial data and asign colors to each location based on total depth. will take two itterations. one to find maximal depth in image, another to create pixels.
    var total_depth_sqr =0;
    var current_depth_sqr;
    
    for (var i = 0; i < spatial_data['vertices'].length; i++) {
        current_depth_sqr = spatial_data['vertices'][i][0]*spatial_data['vertices'][i][0] + spatial_data['vertices'][i][1]*spatial_data['vertices'][i][1] + spatial_data['vertices'][i][2]*spatial_data['vertices'][i][2];
        total_depth_sqr = total_depth_sqr + current_depth_sqr;
    }
    
    var avg_depth_sqr = total_depth_sqr/spatial_data['vertices'].length;
    var coords;
    
    
    var v1,v2,v3;
    var dm_t = [[0,0,0,0,0,0],[0,0,0,0,0],[0,0,0,0,0]];
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

        dm_add_traingle(dm_t, avg_depth_sqr);
    }

    for (var i = 0; i < spatial_data['vertices'].length; i++) { //now that we know the avg depth, draw to the canvas...
        coords = [spatial_data['vertices'][i][3],spatial_data['vertices'][i][4]];
        current_depth_sqr = spatial_data['vertices'][i][0]*spatial_data['vertices'][i][0] + spatial_data['vertices'][i][1]*spatial_data['vertices'][i][1] + spatial_data['vertices'][i][2]*spatial_data['vertices'][i][2];
        // we use 1/(x+1) because its bounded between 1 and 0 over 0->infity. it also concentrates contrast in features closer than the average
        dm_add_point(avg_depth_sqr/2/(avg_depth_sqr/2 + current_depth_sqr), coords[0], coords[1], 6); //if we want to rotate, you can do something like x = img_width - coords[1], y = coords[0]
    }
    
    finalize_dm();
    
}


function dm_add_traingle(vs, avg_depth_sqr){
    
    var min_x, max_x, min_y, max_y;
    min_x = Math.min(vs[0][0],Math.min(vs[1][0],vs[2][0]));
    max_x = Math.max(vs[0][0],Math.max(vs[1][0],vs[2][0]));
    min_y = Math.min(vs[0][1],Math.min(vs[1][1],vs[2][1]));
    max_y = Math.max(vs[0][1],Math.max(vs[1][1],vs[2][1]));
    
    var coords, dist_sqr;
    for (var i = min_x; i < max_x; i+=2) {
        for (var j = min_y; j < max_y; j+=2) {
            if (point_in_triangle(i,j,vs,2)) {
                coords = d3_coord_for_img_x_y(i,j,vs);
                dist_sqr = coords[0]*coords[0]+coords[1]*coords[1]+coords[2]*coords[2];
                dm_add_point( avg_depth_sqr/2/(avg_depth_sqr/2 + dist_sqr), i, j, 3);
            }
        }
    }
    
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


function load_spatial_data(json_url, img_width) {   //image width needed becaues of image reversal
    $.getJSON(json_url, function(data) {
          spatial_data = data;
          spatial_data_loaded = true;
          fill_depth_map(img_width);
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
        d3_1 = spatial_coord_for_img_coord(x1,y1);
        d3_2 = spatial_coord_for_img_coord(x2,y2);
        distance = Math.sqrt( Math.pow((d3_1[0] - d3_2[0]), 2) + Math.pow((d3_1[1] - d3_2[1]), 2) + Math.pow((d3_1[2] - d3_2[2]), 2));
    }
    catch (err) {
        distance = null;
    }
    return distance;
}
