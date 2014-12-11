//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.


// This code, at its heart, calls the svg.js function 'rotate' on draw_g and the menu buttons.
// The majority of the remaining code simply handles annimations of rotation, including determining which direction to perform the rotation in


var center_x, center_y, x_off_c, y_off_c, cur_sin, cur_cos, rotation_start_img_center;

var target_rotation = 0;
var current_rotation = 0;
var target_orientation = 1;

function prep_inc_zoom_offset_stepping(){
    //console.log('prep_inc_zoom_offset_stepping');
    rotation_start_img_center = pxl_to_img_xy(window.innerWidth/2, window.innerHeight/2);
    center_x = image_width/2*zoom_factor;
    center_y = image_height/2*zoom_factor;
    x_off_c = (rotation_start_img_center.x - image_width /2)*zoom_factor;
    y_off_c = (rotation_start_img_center.y - image_height/2)*zoom_factor;
    
    
}

function step_zoom_offset() {
    //console.log('step_zoom_offset');
    i = rotation_start_img_center;
    cur_sin = Math.sin(current_rotation/180*Math.PI);
    cur_cos = Math.cos(current_rotation/180*Math.PI);
    
    x_offset = window.innerWidth/2 - center_x - cur_cos * x_off_c + cur_sin * y_off_c;
    
    y_offset = window.innerHeight/2 - center_y - cur_sin * x_off_c - cur_cos * y_off_c;
    
    setTimeout(function(){ drawing_pan_offset();},0);
}


function forceOrientationChange(orientation, use_annimation) {
    window.setTimeout( function () {
    
    console.log ('forcing orientation change');
    if ( orientation === 1) { // portrait or portrait upsidedown (iOS takes care of this one, so its the same as 1)
        target_rotation = 0;
    }
    else if ( orientation === 2 ) {
        target_rotation = 180;
    }
    else if (orientation === 3) { // landscape left
        target_rotation = 90;
    }
    else if (orientation === 4) { // landscape right
        target_rotation = 270;
    }
    
    target_orientation = orientation;
    
    if (use_annimation === 1) {    start_orientation_change_rotation();}
    else {rotate_w_no_animation();}
    
    last_orientation = orientation;
    
    console.log('forceOrientationChange finished');
                      },0);
}

var rotation_animation_id, roation_frame_start, total_rotation_diff; //rotation animaiton variables
var last_rotation_animation_time = 20;

function start_orientation_change_rotation(){
    console.log ('start orientatino change');

    if (rotation_animation_id) {window.cancelAnimationFrame(rotation_animation_id)};
    if (current_rotation == 0 && target_rotation >= 181) {current_rotation = 360;} //adjust for wrap arround at 360/0 for subtraction
    if (current_rotation == 270 && target_rotation == 0) {target_rotation = 360;}
    if (current_rotation == 360 && target_rotation == 90) {current_rotation = 0;}
    if (current_rotation == 360 && target_rotation == 0) {current_rotation = 0;}
    total_rotation_diff = target_rotation - current_rotation;
    console.log()
    console.log ('current rotation: ' + current_rotation.toFixed() + ', target rotation: ' +target_rotation.toFixed() + ', total rotation diff = ' + total_rotation_diff.toFixed());
    prep_inc_zoom_offset_stepping();
    roation_frame_start = new Date();
    rotation_animation_id = window.requestAnimationFrame(animate_screen_rotation);}

function rotate_w_no_animation() {
    draw_g.rotate(target_rotation, img_container.width()/2, img_container.height()/2);
    rc_menu.rotate_buttons(target_rotation);
    last_orientation = target_orientation;
    current_rotation = target_rotation;
    post_rotation_scaling();
}


var rotation_delta, rotation_number_of_frames;
var next_rotation;
function animate_screen_rotation(unused_time) {
    //console.log ('animate screen');

    if (roation_frame_start == null) { roation_frame_start = new Date();}

    //console.log ('current rotation: ' + current_rotation.toFixed() + ', target rotation: ' +target_rotation.toFixed());
    
    if (current_rotation != target_rotation) //if current frame rotation is less than target frame rotation, keep working.
    {
        //console.log('current_rotation != target_rotation');
        //calculate next rotation step - use last_frame rotation time.
        if (current_rotation == 0 && target_rotation >= 180) { //adjust for wrap arround at 360/0 for subtraction
            console.log('wrapping current rotation to 360');
            current_rotation = 360;
        }
        rotation_number_of_frames = 200 / last_rotation_animation_time; //rotation happens in 300ms. number of frames contingent on speed
        if (rotation_number_of_frames < 3) {rotation_number_of_frames = 3;} //use at least 3 frames to rotate
        rotation_delta = total_rotation_diff / rotation_number_of_frames;
        if (((rotation_delta < 0) && (current_rotation + rotation_delta < target_rotation)) ||
            ((rotation_delta > 0) && (current_rotation + rotation_delta > target_rotation)) ) {
            next_rotation = target_rotation;} //rotation termination condition
        else { next_rotation = current_rotation + rotation_delta;}
        //console.log('rotation delta: '+rotation_delta.toFixed()+', next rotation: '+next_rotation.toFixed());
        
        
        //calculate next offest step
        //step_zoom_offset();
        
        draw_g.rotate(next_rotation, img_container.width()/2, img_container.height()/2);
        rc_menu.rotate_buttons(next_rotation);
        //draw rotation and offset
        current_rotation = next_rotation; //set for next iteration. hard to look at div style to get it due to browser differences
        
        rotation_animation_id = window.requestAnimationFrame(animate_screen_rotation); //call self recusively
    }
    else {
        post_rotation_scaling();
    }
    var draw_end = new Date(); // calculate how long the animation frame took, so we can compute the deltas for the next frame
    if (draw_end.getMilliseconds() - roation_frame_start.getMilliseconds() > 0 ) {last_rotation_animation_time = draw_end.getMilliseconds() - roation_frame_start.getMilliseconds();}
    roation_frame_start = new Date(); //reset counter for traking how long it is taking to draw a frame
    
    
}

function post_rotation_scaling() {
    console.log('post_rotation_scaling');
    //minimum zoom may be different in the new orientation
    old_min_zoom = min_zoom;
    min_zoom = calculate_min_zoom();
    console.log('old min zoom = ' + old_min_zoom.toFixed(2) + ' min zoom' + min_zoom.toFixed(2));
    if (min_zoom > zoom_factor || zoom_factor <= old_min_zoom + 0.000001) {  //if picture is gettin more real estate, or the user hasn't zoomed, adjust zoom.
        zoom(min_zoom/zoom_factor, img_container.width()/2, img_container.height()/2);
        console.log('adjusted to larger space');
    }
    calculate_zoom_boundaries(target_orientation);
    start_pan_bounce();
    if (rotation_animation_id) { window.cancelAnimationFrame(rotation_animation_id);} //calcel if done
    np_rotate(target_orientation);
    //because we've already reset the zoom offset, we can set the offset for np_rotate again, if the number pad is visible...
    if(draw.node.contains(np_svg.node)){
        move_image_for_number_pad(rcMeasurements.measurement_being_edited.text.x(), rcMeasurements.measurement_being_edited.text.y());
    }
    
    //last but not least, move any message text
    rcMessage.rotate();

}