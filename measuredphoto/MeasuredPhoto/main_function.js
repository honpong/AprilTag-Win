//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

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
        rc_menu.rearrange_menu ();
        if (orientation_drawn_landsacep){np_to_landscape_bottom();}
        else {np_to_portrait();}
        doing_orientatino_change = false;
    }
}

var switch_image_depthmap; //declaring this in the global scope for later initialization
var dm_mask_fade_animaiton_id; //used for iterative callback for animating the mask fade.

function clear_tool_data(){ //this should be called whenever theres a switch in tools
    //line tool data
    //console.log('starting clear_tool_data');
    lineNotStarted = true;
    click_image_x1 = null;
    click_image_y1 = null;
    if(marker) { marker.remove(); }
    //text annotation data
    rcMeasurements.endNoteEdit();
}


function rc_initialize(){
    //console.log = logNative;
    console.log("starting rc_initialize()");
    
    is_rc_initialized = true;
    
        //precent backspace button from bring page back.
    $(document).bind("keydown", function(e){
        if (e.keyCode == 8) {e.preventDefault();}
    });
    //attach listern for key presses
    $(document).keyup(function(e){
        e.preventDefault();
        e.stopPropagation();
        if(e.keyCode == 46 || e.keyCode == 8) {rcMeasurements.deleteCharacterFromNote();}
        else if(e.keyCode == 13) {rcMeasurements.endNoteEdit();}
    })
    $(document).keypress(function(e){
                         //console.log(String.fromCharCode(e.keyCode));
                         rcMeasurements.addCharacterToNote ( String.fromCharCode(e.keyCode) );
                      })
    
    
    /* create an svg drawing */
    draw = SVG('drawing').size(window.innerWidth, window.innerHeight);
    img_container = draw.nested();
    img_container.size(window.innerWidth, window.innerHeight - button_size); //initialize, this will change later depending on orientation
    draw_g = img_container.group();
    menu_svg = draw.nested();
    measured_svg = img_container.nested();
    draw_g.add(measured_svg);
    
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
                         // this conditional statement causes panning gestures and mouse movements to behave correctly when we have a forced orientation change.
                         if (last_orientation == 1) {
                         x_offset = x_offset + x_pan;
                         y_offset = y_offset + y_pan;
                         }
                         else if (last_orientation == 2) {
                         x_offset = x_offset - x_pan;
                         y_offset = y_offset - y_pan;
                         }
                         else if (last_orientation == 3) {
                         x_offset = x_offset + y_pan;
                         y_offset = y_offset + -x_pan;
                         }
                         else if (last_orientation == 4) {
                         x_offset = x_offset + -y_pan;
                         y_offset = y_offset + x_pan;
                         }
                         if (scale_change != 1 ) {
                         window.requestAnimationFrame(function(unused_time){
                                                      zoom( scale_change, e.gesture.center.pageX, e.gesture.center.pageY);  }); }
                         else { setTimeout(function(){ drawing_pan_offset();},0); }
                         
                         record_current_touchs(e.gesture); // save current gesture as last_gesture for next use.
                         
                         });
    
    
    function line_handler(i) { //takes an image coordinate pair as an input
        //test if we have depth information at this point. if not show the depth mask then animate its fade and give error message
        if (! distanceTo(i.x,i.y)) {
            show_dm_mask();
            window.setTimeout( start_dm_mask_fade_out, 2000);
            rcMessage.post("Sorry, no measurement data there.",4000);
        }
        
        if (lineNotStarted){
            if (! distanceTo(i.x,i.y)) {return;} //don't allow a measurement to be started in a dead area
            lineNotStarted = false;
            click_image_x1 = i.x; //we map into image coordinates here, incase scale factor chagnes between clicks
            click_image_y1 = i.y;
            marker = measured_svg.circle(4).move(click_image_x1-2,click_image_y1-2).stroke({ color: line_color, opacity: 0.4, width : 2 }).fill({opacity:0});
        }
        else {
            // we want to instantiate a measurement here, and pass that measurement to be drawn
            rcMeasurements.new_measurement(click_image_x1, click_image_y1, i.x, i.y, measured_svg);
            clear_tool_data();
            setTimeout( function () {rcMeasurements.save_measurements();
                        rc_menu.enable_disenable_undo(rcMeasurements.is_undo_available());
                       }, 0)
        }
    }
    
    function angle_handler(i) {
        rcMeasurements.new_range(i.x, i.y, measured_svg);
    }
    
    function text_entry_handler (i) {
        //create a new text box at this location
        rcMeasurements.new_note(i.x, i.y, measured_svg);
    }
    
    function eraser_handler (e) {
        // do nothing - annotations have their own click listners for erase.
    }

    function click_or_touch(e) {
        var i = pxl_to_img_xy(e.pageX, e.pageY);
        if ( i.x > image_width || i.y > image_height || i.x < 0 || i.y < 0) {return null;} //ignore taps off of image
        else if (rc_menu.current_button == rc_menu.line_button) {line_handler(i);}
        else if (rc_menu.current_button == rc_menu.angle_button) {angle_handler(i);}
        else if (rc_menu.current_button == rc_menu.eraser_button) {eraser_handler(i);}
        else if (rc_menu.current_button == rc_menu.text_button) {text_entry_handler(i);}
        rc_menu.enable_disenable_undo(rcMeasurements.is_undo_available());
    }
    
    FastClick.attach(document.body);

    draw.click( function(e) { setTimeout(function(){ click_or_touch(e); },1);   e.stopPropagation(); e.preventDefault();} );
    
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
            window.setTimeout(function (){dm_mask_fade_animaiton_id = window.requestAnimationFrame(fade_dm_mask_step_animation);},10);
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
    
    undo_last_change = function() {
        rcMeasurements.revert_measurement_state();
        rc_menu.enable_disenable_undo(rcMeasurements.is_undo_available());
    }
    
    toggle_all_units = function() {
        if   (default_units_metric) {default_units_metric = false;}
        else                        {default_units_metric = true;}
        rcMeasurements.reset_all_measurement_units_to_default();
        rc_menu.unit_button.highlight_active_unit(default_units_metric);
        rcMeasurements.save_measurements(true); //pass in true for 'optional_not_undoable_flag' meaning that the undo button shouldn't undo this action.
    }
    
    // construct menue
    build_rc_menu();
    
    //prevent scrolling
    document.body.addEventListener('touchstart', function(e){ e.stopPropagation(); e.preventDefault(); });

    window.addEventListener('resize', function(event){
                            if (draw.node.contains(menu_svg.node)) {draw.node.removeChild(menu_svg.node);} //take off menu
                            doOnOrientationChange();
                            setTimeout(function(){
                                       if ( ! draw.node.contains(menu_svg.node)) {draw.node.appendChild(menu_svg.node);} //put menu back on
                                       },200)
                            start_pan_bounce(); start_zoom_return();
                            });

    np_call_back_add = rcMeasurements.add_character;
    np_call_back_del = rcMeasurements.del_character;
    np_call_back_ent = rcMeasurements.finish_number_operation;
    np_call_back_unt = rcMeasurements.switch_units;
    np_call_back_oth = rc_menu.color_menu;
    
    np_add_listeners(); //setup image
    
    dm_initialize();
    
    doOnOrientationChange();

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
        current_measurement = null;
        rcMeasurements.reset();

        // reset tool data / button data
        clear_tool_data();
        rc_menu.reset();
    
        // reset the depth map
        dm_initialize();
    
        // removed numberpad if attached
        if(draw.node.contains(np_svg.node)) {draw.node.removeChild(np_svg.node);}
        
        //remove the image
        image.remove();
        image = null;
    
        //window.setTimeout( function() {alert('completed clear_all');}, 0)
        return 0;
                      
        rc_menu.enable_disenable_undo(rcMeasurements.is_undo_available());
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
            
            //assume clear is called first if this is the second load
            console.log('loading image from '+ rc_img_url);
            initial_load = true; //were setting this incase zooming was called and set it to false after a clear was called
            image = img_container.image(rc_img_url).loaded(function(loader) {
                                                  console.log('starting image load callback');
                                                  //this should rotate the image
                                                  //alert('loading img');
                                                  //image_width = loader.height;
                                                  //image_height = loader.width;
                                                  //image.rotate(90, image_width/2, image_height/2).move(-(image_height-image_width)/2,(image_height-image_width)/2);
                                                  
                                                           
                                                  image_width = loader.width;
                                                  image_height = loader.height;
                                                   //console.log('loaded image dimensions: ' + image_width.toFixed()+ ' x ' + image_height.toFixed() );
                                                           
                                                  draw_g.add(image);
                                                  if ( ! draw.node.contains(menu_svg.node)) {draw.node.appendChild(menu_svg.node);}
                                                  
                                                  // load measurements
                                                  rcMeasurements.load_json(rc_annotation_url, function() {
                                                                                    //alert('loading spatial data');
                                                                                    //console.log('starting annotation data load calback');
                                                                                    load_spatial_data(rc_data_url); //this function is defined in depth_data.js
                                                                                    //console.log('finished rcMeasurement.load_json callback');
                                                                                    rc_menu.enable_disenable_undo(rcMeasurements.is_undo_available());
                                                                                }
                                                                           );

                                                           
                                                  // Initial dexecution if needed
                                                  //alert('do on orientation change');
                                                  doOnOrientationChange();
                                                  rc_menu.unit_button.highlight_active_unit(default_units_metric);
                                                  
                                                  });
        
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
