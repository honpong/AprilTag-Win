//  Copyright (c) 2014 Sunlayar. All rights reserved.

doing_orientatino_change = false;
function doOnOrientationChange()
{
    console.log('do on orientation chagne');
    if (!doing_orientatino_change) {
        doing_orientatino_change = true;  //theres a lot of nested calls, prevent some listeners double triggering this
        //change body and document size here too.
        
        document.body.style.width = window.innerWidth.toFixed(0) + 'px';
        document.body.style.height = window.innerHeight.toFixed(0) + 'px';
        viewport = document.querySelector("meta[name=viewport]");
        viewport.setAttribute('content', "width=device-width, height=device-height, initial-scale=1, user-scalable=0");
        draw.node.style.height = window.innerHeight;
        draw.node.style.width = window.innerWidth;
        draw.size(window.innerWidth, window.innerHeight);
        scaleImageToMatchScreen();
        doing_orientatino_change = false;
        np_to_landscape_bottom(); //function in svg_number_page.js
    }
}

var switch_image_depthmap; //declaring this in the global scope for later initialization
var dm_mask_fade_animaiton_id; //used for iterative callback for animating the mask fade.


function rc_initialize(){
    //console.log = logNative;
    console.log("starting rc_initialize()");
    
    is_rc_initialized = true;
    
    console.log("creating svg");
    
    /* create an svg drawing */
    draw = SVG('drawing').size(window.innerWidth, window.innerHeight);
    img_container = draw.nested();
    img_container.size(window.innerWidth, window.innerHeight);
    draw_g = img_container.group();
    measured_svg = img_container.nested();
    draw_g.add(measured_svg);

    console.log("adding hammer");
    
    //console.log('hammer initializaitons');

    hammer = Hammer(document.body);
    
    Hammer(draw.node).on("release", function(e) { start_pan_bounce(); start_zoom_return(); });
    
    
    var last_gesture_touches = null;
    var last_touch_time = null;
    
    function record_current_touchs(g) {
        last_gesture_touches = [];
        last_touch_time = g.timeStamp;
        for (var i=0; i < g.touches.length; i++) {
            last_gesture_touches.push({ identifier: g.touches[i].identifier, pageX: g.touches[i].pageX, pageY: g.touches[i].pageY });
        }
    }

    
    Hammer(draw.node).on("pinch drag", function(e) {
                         e.stopPropagation(); e.preventDefault(); e.gesture.preventDefault(); e.gesture.stopPropagation();
                         
                         //initialize last gesture variables, or reset it if it is older than the current gesture
                         if (last_gesture_touches) {
                         if (last_touch_time < e.gesture.startEvent.timeStamp){ record_current_touchs(e.gesture.startEvent);;
                         }}
                         else {record_current_touchs(e.gesture.startEvent);}
                         
                         var scale_change = 1;
                         var x_pan = 0;
                         var y_pan = 0;
                         // we have four conditions, one or two touches last gesture by one or two this gesture. any more than 2 touches will have ill defined behavior
                         // in each condition, a different translation is defined, and in one condition a scale change is defined.
                         if (e.gesture.touches.length == 1) {
                         if (last_gesture_touches.length == 1){ // pan by the difference in position
                         x_pan = e.gesture.touches[0].pageX - last_gesture_touches[0].pageX;
                         y_pan = e.gesture.touches[0].pageY - last_gesture_touches[0].pageY;
                         }
                         else if (last_gesture_touches.length == 2) { // pan by the minimum difference in position between the two
                         var distance_sq_0 = Math.pow( e.gesture.touches[0].pageX - last_gesture_touches[0].pageX, 2) +
                         Math.pow( e.gesture.touches[0].pageY - last_gesture_touches[0].pageY, 2);
                         var distance_sq_1 = Math.pow( e.gesture.touches[0].pageX - last_gesture_touches[1].pageX, 2) +
                         Math.pow( e.gesture.touches[0].pageY - last_gesture_touches[1].pageY, 2);
                         if( distance_sq_0 < distance_sq_1) {
                         x_pan = e.gesture.touches[0].pageX - last_gesture_touches[0].pageX;
                         y_pan = e.gesture.touches[0].pageY - last_gesture_touches[0].pageY; }
                         else {
                         x_pan = e.gesture.touches[0].pageX - last_gesture_touches[1].pageX;
                         y_pan = e.gesture.touches[0].pageY - last_gesture_touches[1].pageY; }
                         }
                         }
                         else if (e.gesture.touches.length == 2) {
                         if (last_gesture_touches.length == 1){ // pan by the minimum difference in position between the two
                         var distance_sq_0 = Math.pow( e.gesture.touches[0].pageX - last_gesture_touches[0].pageX, 2) +
                         Math.pow( e.gesture.touches[0].pageY - last_gesture_touches[0].pageY, 2);
                         var distance_sq_1 = Math.pow( e.gesture.touches[1].pageX - last_gesture_touches[0].pageX, 2) +
                         Math.pow( e.gesture.touches[1].pageY - last_gesture_touches[0].pageY, 2);
                         if( distance_sq_0 < distance_sq_1) {
                         x_pan = e.gesture.touches[0].pageX - last_gesture_touches[0].pageX;
                         y_pan = e.gesture.touches[0].pageY - last_gesture_touches[0].pageY; }
                         else {
                         x_pan = e.gesture.touches[1].pageX - last_gesture_touches[1].pageX;
                         y_pan = e.gesture.touches[1].pageY - last_gesture_touches[1].pageY; }
                         }
                         else if (last_gesture_touches.length == 2) { // pan by the difference in center positions between the two, scale by scale differnce bewteen the two
                         x_pan = (e.gesture.touches[0].pageX + e.gesture.touches[1].pageX - last_gesture_touches[0].pageX - last_gesture_touches[1].pageX)/2;
                         y_pan = (e.gesture.touches[0].pageY + e.gesture.touches[1].pageY - last_gesture_touches[0].pageY - last_gesture_touches[1].pageY)/2;
                         var distance_0 = Math.sqrt( Math.pow( last_gesture_touches[0].pageX - last_gesture_touches[1].pageX, 2) +
                                                    Math.pow( last_gesture_touches[0].pageY - last_gesture_touches[1].pageY, 2));
                         var distance_1 = Math.sqrt( Math.pow( e.gesture.touches[0].pageX - e.gesture.touches[1].pageX, 2) +
                                                    Math.pow( e.gesture.touches[0].pageY - e.gesture.touches[1].pageY, 2));
                         
                         scale_change = distance_1 / distance_0;
                         }
                         }
                         
                         //when we started using a transmormation with an origin property via the svg library, our x and y offsets started acting strangely when the image was rotated
                         x_offset = x_offset + x_pan;
                         y_offset = y_offset + y_pan;
                         
                         if (scale_change != 1 ) {
                         window.requestAnimationFrame(function(unused_time){
                                                      zoom( scale_change, e.gesture.center.pageX, e.gesture.center.pageY);  }); }
                         else { setTimeout(function(){ drawing_pan_offset();},0); }
                         
                         record_current_touchs(e.gesture); // save current gesture as last_gesture for next use.
                         
                         });
    
    
    console.log('done with hammer, starting line handler');
    
    FastClick.attach(document.body);
    
    switch_image_depthmap = function () { //we move the image svg off the dom, and move the depthmap on the dom.
        //console.log('switch_image_depthmap()');
        //remove image from dom tree put mask in its place
        if (draw.node.contains(image.node)) {
            show_dm_mask();
        }
        //remove image mask put depthmap in its place
        else if(draw.node.contains(dm_mask_svg.node)) {
            draw_g.node.insertBefore(dm_svg.node,dm_mask_svg.node);
            draw_g.node.removeChild(dm_mask_svg.node);
            //start depthmap calculation if not yet done.
            if (!dm_drawn) {fill_depth_map();}
        }
        //remove depthmap from dom tree put image in its place
        else if(draw.node.contains(dm_svg.node)){
            draw_g.node.insertBefore(image.node,dm_svg.node);
            draw_g.node.removeChild(dm_svg.node);
            
        }
    }
    
    show_dm_mask = function () {
        //console.log('show_dm_mask()');
        //make the dm_mask_svg node the current image svg node
        if (draw.node.contains(image.node)) {
            draw_g.node.insertBefore(dm_mask_svg.node,image.node);
            draw_g.node.removeChild(image.node);
        }
        else if(draw.node.contains(dm_svg.node)){
            draw_g.node.insertBefore(dm_mask_svg.node,dm_svg.node);
            draw_g.node.removeChild(dm_svg.node);
            
        }
        if (!dm_mask_drawn) {fill_dm_mask();}
        
        //if an animation is happening we need to stop it
        if (dm_mask_fade_animaiton_id) {
            window.cancelAnimationFrame(dm_mask_fade_animaiton_id);
            dm_mask_fade_animaiton_id = null;
            if(dm_masking_image) {dm_masking_image.opacity(1);}
        }

        
    }
    
    start_dm_mask_fade_out = function () {
        //console.log('start_dm_mask_fade_out()');
        //start the fade out animation
        if (dm_mask_fade_animaiton_id) {window.cancelAnimationFrame(dm_mask_fade_animaiton_id);}
        dm_mask_fade_animaiton_id = window.requestAnimationFrame(fade_dm_mask_step_animation);
    }
    
    fade_dm_mask_step_animation = function () {
        var current_mask_opacity = dm_masking_image.opacity();
        //console.log('dm_masking_image_opacity ' + current_mask_opacity.toFixed(2));
        //make the mask progresively more transparent, unill full transparency is reached
        if (current_mask_opacity > 0) { //if we havn't hit zero transparency, then make it more transparent, and call again
            var next_opicity = Math.max(0, current_mask_opacity - 0.01);
            dm_masking_image.opacity(next_opicity);
            dm_mask_fade_animaiton_id = window.requestAnimationFrame(fade_dm_mask_step_animation);
        }
        else {
            fade_dm_mask_stop_animation();
        }
    }
    
    fade_dm_mask_stop_animation = function () {
        ////console.log('fade_dm_mask_stop_animation()');
        //terminate the iteration of the animation step
        if (dm_mask_fade_animaiton_id) {
            window.cancelAnimationFrame(dm_mask_fade_animaiton_id);
            dm_mask_fade_animaiton_id = null;
        }
        //remove dm_svg.node as main image, restore image.node
        if (draw.node.contains(dm_mask_svg.node)) {
            //console.log('removing depth mask')
            draw_g.node.insertBefore(image.node, dm_mask_svg.node);
            draw_g.node.removeChild(dm_mask_svg.node);
        }
        //restore the opacity of the mask.
        if(dm_masking_image) {dm_masking_image.opacity(1);}
    }
    
    
    //prevent scrolling
    document.body.addEventListener('touchstart', function(e){ e.stopPropagation(); e.preventDefault(); });

    np_call_back_add = rcMeasurements.add_character;
    np_call_back_del = rcMeasurements.del_character;
    np_call_back_ent = rcMeasurements.finish_number_operation;
    np_add_listeners(); //attach events to nuber pad

    
    dm_initialize();
    
    doOnOrientationChange();

}

//define next button - next button function
var nextContaner, nextButton;
function create_next_button() {
    nextContaner = draw.nested('message');
    nextContaner.size(window.innerWidth, window.innerHeight)
    nextButtonShadow = nextContaner.text('Next >').font({
                                                        family: rcMeasurements.font_family,
                                                        size: 15
                                                        , anchor: 'middle'
                                                        , leading: 1
                                                        }).stroke({ color: '#000', opacity: 1 , width: 3});
    nextButton = nextContaner.text('Next >').font({
                                                       family: rcMeasurements.font_family,
                                                       size: 15
                                                       , anchor: 'middle'
                                                       , leading: 1
                                                       }).fill({ color: '#FFFFFF', opacity: 1 });
    nextButton.move(window.innerWidth-30,window.innerHeight/2);
    nextButtonShadow.move(window.innerWidth-30,window.innerHeight/2);
    nextButton.click(next_button_pressed); //call the next button pressed function when the next button is clicked.
}

function next_button_pressed() {
    console.log('next_button_pressed()');
    //location.reload(); // useful for debuging if things are very broken
    if (rcMeasurements.roof_measurement.isValid) {
        //request a URL to signal to main app to start AR session
        $.ajax({ type: "GET", url: rc_server_location + "next/"});
    }
    else {
        alert("roof object is missing data, please try again");
    }
}


function clear_all(){
    console.log('starting clear_all()');
    window.setTimeout(function (){
    try {
        //scale and roation handling
        initial_load = true;
        console.log('tried setting initial_load to true, it is now = ' + initial_load.toString());
    
        //clear all measurements, reset handlers
        if (draw_g.node.contains(measured_svg.node)) {draw_g.node.removeChild(measured_svg.node);}
        measured_svg = img_container.nested();
        draw_g.add(measured_svg);
        rcMeasurements.reset();
    
        // reset the depth map
        dm_initialize();
            
        //remove the image
        image.remove();
        image = null;
    
        //remove the svg number pad
        if(draw.node.contains(np_svg.node)) {draw.node.removeChild(np_svg.node);}
                      
        //window.setTimeout( function() {alert('completed clear_all');}, 0)
        return 0;
                      
    }
    catch(err){
        return(err.message);
   }
                      },0);
}

function setDefaultUnits(use_metric) {
    window.setTimeout (function () {default_units_metric = use_metric;} , 0);
}

var rc_img_url_glbl;
function loadMPhoto(rc_img_url,rc_data_url, rc_annotation_url, guid, use_metric){
    //console.log("startin loadMPhoto()");
    window.setTimeout( function () {
        rc_img_url_glbl = rc_img_url;
        m_photo_guid = guid;
        if (typeof use_metric === "undefined" || use_metric === null) { default_units_metric = false; } //metric is our default if not set
                      else {default_units_metric = use_metric; }//set default if provided.
        // only call initialization once.
        try {
            if (!is_rc_initialized) {
                rc_initialize();
            }
            load_spatial_data(rc_data_url); //this function is defined in depth_data.js
                      
            
            //assume clear is called first if this is the second load
            console.log('loading image from '+ rc_img_url);
            initial_load = true; //were setting this incase zooming was called and set it to false after a clear was called
            image = img_container.image(rc_img_url).loaded(function(loader) {
                                                  console.log('starting image load callback');
                                                           
                                                  image_width = loader.width;
                                                  image_height = loader.height;
                                                           
                                                  draw_g.add(image);
                                                           
                                                  doOnOrientationChange();
                                                           
                                                  rcMeasurements.new_measurement(image_width/4,image_height/4, 3*image_width/4, image_height/4, 3*image_width/4,3*image_height/4, image_width/4, 3*image_height/4, measured_svg);

                                                  });
            create_next_button(); //we do this last so its at the bottom of the dom and hence apears on top of all other content
            return 0;
        }
        catch(err) {
            //console.log(err.message);
            return(err.message);
        }
        //console.log('finished loadMPhoto()');
    },0 );
}

function logNative(message)
{
    // log to browser console
    console.debug(message);
    
    // log to native land
    var jsonData = { "message": message };
    $.ajax({ type: "POST", url: "http://internal.realitycap.com/log/", contentType: "application/json", processData: false, dataType: "json", data: JSON.stringify(jsonData) })
        .fail(function(jqXHR, textStatus, errorThrown) {
              alert(textStatus + ": " + JSON.stringify(jqXHR));
        })
    ;
}
