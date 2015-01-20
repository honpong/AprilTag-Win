//  Copyright (c) 2014 Sunlayar. All rights reserved.

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////                        CODE TO DO PAN AND ZOOM ANIMATIONS , scalling,                 ////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// draw is the primary SVG node. it contains the img_container
// img_container contains the images and the measurements
// draw_g is an svg group that is inside img_container.
// draw_g has the measurements and the image and depth map as members.
// draw_g is scaled durring zoom operations.
// zoom_factor is the ratio between the number of screen pixels along a line over image pixels.
// draw_g is panned and centered by x and y offsets which are in screen pixels rather than image pixels.
// x_offset and y_offset determine where the centerof the scaled image sits in screen coordiates.


var last_bounce_animation_time = 10;
var pan_bounce_frame_start = null;

function calculate_min_zoom(){
    var effective_width = img_container.width();
    var effective_height = img_container.height();
    
    var z_factor;
    if ((image_width)/(image_height) >= effective_width/effective_height) { //image is too wide for screen, scale by width
        z_factor = effective_width / image_width;
    }
    else {
        //image is too tall for screen, scale by hieght
        z_factor = effective_height / image_height;
    }
    return z_factor; //we never want to let the user go smaller than this.
}

function scaleImageToMatchScreen() {
    //console.log('scale image to match screen');
    if (draw_g.node.contains(measured_svg.node)) {draw_g.node.removeChild(measured_svg.node);}
    
    img_container.size(window.innerWidth, window.innerHeight);
    img_container.move(0,0);
    
    //scale image to match screen
    if (initial_load) {
        x_offset = img_container.width()/2;
        y_offset = img_container.height()/2;
        zoom_factor = calculate_min_zoom();
        min_zoom = zoom_factor; //we never want to let the user go smaller than this.
    }
    
    calculate_zoom_boundaries(); //set the max/min zoom offsets
    draw_g.scale(zoom_factor);
    drawing_pan_offset();
    
    draw_g.node.appendChild(measured_svg.node);
    initial_load = false;
    
}


function shift_on_zoom(zoom_amount, center_x, center_y) {
    //the scale change will change the size of the image by a certain number of pixels given the current scale.
    //we need to change the offset such that the 'center' point stays in place.
    //this means multiplying the total amount of image size change in pixles by percentage of the images size it is away from the center point
    var i = pxl_to_img_xy(center_x, center_y);
    return {'x':(i.x - image_width/2)   * zoom_factor  * (zoom_amount - 1), 'y':(i.y - image_height/2)  * zoom_factor  * (zoom_amount - 1)};
}


function zoom(zoom_amount, center_x, center_y) {
    var start_time = new Date();
    
    if (zoom_factor < .9 * min_zoom && zoom_amount < 1){ zoom_amount = 1; } //limits amount off zoom out allowed
    
    var s = shift_on_zoom(zoom_amount, center_x, center_y);
    zoom_factor = zoom_factor * zoom_amount;
    x_offset = x_offset - s.x;
    y_offset = y_offset - s.y;
    
    var draw_start = new Date();
    
    scaleImageToMatchScreen();
    
    var draw_end = new Date();
    var m_time = (draw_start.getSeconds() - start_time.getSeconds()).toString() + "sec, " + (draw_start.getMilliseconds() - start_time.getMilliseconds()).toString() + " milisec"
    var d_time = (draw_end.getSeconds() - draw_start.getSeconds()).toString() + "sec, " + (draw_end.getMilliseconds() - draw_start.getMilliseconds()).toString() + " milisec"
}


function start_pan_bounce(){
    if (bounce_animation_id) {window.cancelAnimationFrame(bounce_animation_id)};
    pan_bounce_frame_start = new Date();
    bounce_animation_id = window.requestAnimationFrame(animate_pan_bounce);}


function animate_pan_bounce(unused_time) {
    if (pan_bounce_frame_start == null) { pan_bounce_frame_start = new Date();}
    
    if (( x_offset > max_x_offset) || ( x_offset < min_x_offset) ||
        ( y_offset > max_y_offset) || ( y_offset < min_y_offset))
    {
        if ((x_offset ) > max_x_offset ) { x_offset =  Math.max( x_offset - Math.max(last_bounce_animation_time, (x_offset - max_x_offset)/5), max_x_offset );}
        if ((x_offset ) < min_x_offset ) { x_offset =  Math.min( x_offset + Math.max(last_bounce_animation_time, (min_x_offset - x_offset)/5), min_x_offset );}
        if ((y_offset ) > max_y_offset ) { y_offset =  Math.max( y_offset - Math.max(last_bounce_animation_time, (y_offset - max_y_offset)/5), max_y_offset );}
        if ((y_offset ) < min_y_offset ) { y_offset =  Math.min( y_offset + Math.max(last_bounce_animation_time, (min_y_offset - y_offset)/5), min_y_offset );}
        drawing_pan_offset();
        bounce_animation_id = window.requestAnimationFrame(animate_pan_bounce);
    }
    else { if (bounce_animation_id) { window.cancelAnimationFrame(bounce_animation_id); } }
    
    var draw_end = new Date();
    if (draw_end.getMilliseconds() - pan_bounce_frame_start.getMilliseconds() > 0 ) {last_bounce_animation_time = draw_end.getMilliseconds() - pan_bounce_frame_start.getMilliseconds();}
    pan_bounce_frame_start = new Date();
    
}

var zoom_animation_id = null;
var last_zoom_animation_time = 10;
var zoom_frame_start = null;

function start_zoom_return(){
    if (zoom_animation_id) { return null; }
    else {
        zoom_frame_start = new Date();
        zoom_animation_id = window.requestAnimationFrame(animate_zoom_return);
    }
}


function animate_zoom_return(unused_time) {
    if (zoom_frame_start == null) { zoom_frame_start = new Date();}
    if (zoom_factor < min_zoom)
    {
        zoom( 1 + last_bounce_animation_time/1000, img_container.width() / 2, img_container.height() / 2);
        zoom_animation_id = window.requestAnimationFrame(animate_zoom_return);
    }
    else { if (zoom_animation_id) {
        window.cancelAnimationFrame(zoom_animation_id);
        zoom_animation_id = null;
        start_pan_bounce();
    } }
    
    var draw_end = new Date();
    if (draw_end.getMilliseconds() - zoom_frame_start.getMilliseconds() > 0 ) {last_bounce_animation_time = draw_end.getMilliseconds() - zoom_frame_start.getMilliseconds();}
    zoom_frame_start = new Date();
    
}


function calculate_zoom_boundaries() {
    //calculate zoom offsets
    max_x_offset = Math.max( image_width*zoom_factor/2, img_container.width()/2   );
    max_y_offset = Math.max( image_height*zoom_factor/2,  img_container.height()/2 );
    
    if (img_container.width() <  image_width * zoom_factor) {
        min_x_offset = img_container.width() - image_width * zoom_factor/2;}
    else { min_x_offset = Math.max( image_width * zoom_factor/2, img_container.width()/2   );}
    
    if (img_container.height() < image_height * zoom_factor) {
        min_y_offset = img_container.height() - image_height * zoom_factor / 2;}
    else { min_y_offset = Math.max( 0 - image_height * zoom_factor / 2,  img_container.height()/2);}
    
}


//this is the method which actually moves the content lateraly and vertically on the screen. it does so by centering the drawing group inside the img_container.
function drawing_pan_offset() {
    // slow down the drag if ouside boundry
    var x, y;
    if ((x_offset ) > max_x_offset ) {
        x = max_x_offset + ( x_offset - max_x_offset)/2; }
    else if ((x_offset ) < min_x_offset) {
        x = min_x_offset + ( x_offset - min_x_offset)/2;}
    else {
        x = x_offset;
    }
    if ((y_offset ) > max_y_offset) {
        y = max_y_offset + ( y_offset - max_y_offset) /2 ;}
    else if ((y_offset ) < min_y_offset) {
        y = min_y_offset + ( y_offset - min_y_offset)/2;}
    else {
        y = y_offset;
    }
    draw_g.center(x,y);
    
}

// method to keep track of how screen is changing with regrads zoom and pan
// returns an image coordinate given a pixel coordinate
function pxl_to_img_xy(pX, pY){
    var iX, iY;
        iX = (pX - x_offset - img_container.x())/zoom_factor + image_width/2; //we add half image width because x_offset is relative to image center
        iY = (pY - y_offset)/zoom_factor + image_height/2;
    return {'x':iX, 'y':iY};
}

// method to keep track of how screen is changing with regrads zoom, pan, and rotation.
// returns a pixel coordinate given an image coordinate
function img_to_pxl_xy(iX, iY){
    //console.log(JSON.stringify({'ix':iX, 'iy':iY}))
    var pX, pY;
        pX = (iX - image_width/2)*zoom_factor + x_offset + img_container.x();
        pY = ( iY - image_height/2 ) * zoom_factor + y_offset;
    return {'x':pX, 'y':pY};
}

