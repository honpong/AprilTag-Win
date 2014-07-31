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


function rc_initialize(){
    is_rc_initialized = true;

    //alert('starting initialization');

    
    /* create an svg drawing */
    draw = SVG('drawing').size(window.innerWidth, window.innerHeight);
    img_container = draw.nested();
    img_container.size(window.innerWidth, window.innerHeight - button_size); //initialize, this will change later depending on orientation
    draw_g = img_container.group();
    menu_svg = draw.nested();
    measured_svg = img_container.nested();
    draw_g.add(measured_svg);
    
    //alert('hammer initializaitons');

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
        if (lineNotStarted){
            lineNotStarted = false;
            click_image_x1 = i.x; //we map into image coordinates here, incase scale factor chagnes between clicks
            click_image_y1 = i.y;
            marker = measured_svg.circle(4).move(click_image_x1-2,click_image_y1-2).stroke({ color: line_color, opacity: 0.4, width : 2 }).fill({opacity:0});
        }
        else {
            lineNotStarted = true;
            // we want to instantiate a measurement here, and pass that measurement to be drawn
            rcMeasurements.new_measurement(click_image_x1, click_image_y1, i.x, i.y, measured_svg);
            click_image_x1 = null;
            click_image_y1 = null;
            marker.remove();
        }
    }
    
    function angle_handler(e) {
    
    }
    
    function text_entry_handler (e) {
    
    }
    
    function eraser_handler (e) {
    
    }

    function click_or_touch(e) {
        var i = pxl_to_img_xy(e.pageX, e.pageY);
        if ( i.x > image_width || i.y > image_height || i.x < 0 || i.y < 0) {return null;} //ignore taps off of image
        else if (rc_menu.current_button == rc_menu.button2) {line_handler(i);}
        else if (rc_menu.current_button == rc_menu.button3) {angle_handler(i);}
        else if (rc_menu.current_button == rc_menu.button4) {eraser_handler(i);}
        else if (rc_menu.current_button == rc_menu.button5) {text_entry_handler(i);}
    }
    
    //alert('fastclick');
    
    FastClick.attach(document.body);

    draw.click( function(e) { setTimeout(function(){ click_or_touch(e); },1);   e.stopPropagation(); e.preventDefault();} );

    
    function prepare_to_exit() {
        //TODO: put code here to save current measurements and any other shutdown to prepare to exit.
        return true;
    }


    //alert('switch image depthmap');
    
    var image_shown = true;
    switch_image_depthmap = function () { //we move the image svg off the dom, and move the depthmap on the dom.
        if (image_shown) {
            //remove image from dom tree put depthmap in its place
            draw_g.node.insertBefore(dm_svg.node,image.node);
            draw_g.node.removeChild(image.node);
            image_shown = false;
        }
        else{
            //remove depthmap from dom tree put image in its place
            draw_g.node.insertBefore(image.node,dm_svg.node);
            draw_g.node.removeChild(dm_svg.node);
            image_shown = true;
        }
    }

    //alert('build_rc menu');
    
    // construct menue
    build_rc_menu();

    //alert('touch start event document body to prevent scrolling');
    
    //prevent scrolling
    document.body.addEventListener('touchstart', function(e){ e.stopPropagation(); e.preventDefault(); });

    
    //alert('reseize');

    window.addEventListener('resize', function(event){
                            if (draw.node.contains(menu_svg.node)) {draw.node.removeChild(menu_svg.node);} //take off menu
                            doOnOrientationChange();
                            setTimeout(function(){
                                       if ( ! draw.node.contains(menu_svg.node)) {draw.node.appendChild(menu_svg.node);} //put menu back on
                                       },200)
                            start_pan_bounce(); start_zoom_return();
                            });

    //alert('np_callbacks');
    
    
    np_call_back_add = rcMeasurements.add_character;
    np_call_back_del = rcMeasurements.del_character;
    np_call_back_ent = rcMeasurements.finish_number_operation;
    np_call_back_unt = rc_menu.unit_menu;
    np_call_back_oth = rc_menu.color_menu;
    
    //alert('np_add_listeneres');
    np_add_listeners(); //setup image
    //alert('done np to portrait');
    //console.log('initialization_complete');
    
    dm_initialize();

}


function clear_all(){
    //try {
        //scale and roation handling
        initial_load = true;
    
        //clear all measurements, reset handlers
        if (draw_g.node.contains(measured_svg.node)) {draw_g.node.removeChild(measured_svg.node);}
        measured_svg = img_container.nested();
        draw_g.add(measured_svg);
        lineNotStarted = true;
        current_measurement = null;
        rcMeasurements.reset();

        // reset the depth map
        dm_initialize();
        
        //remove the image
        image.remove();
        image = null;
    
        //window.setTimeout( function() {alert('completed clear_all');}, 0)
        return 0;
    //}
    //catch(err){
    //    return(err.message);
   // }
}

function loadMPhoto(rc_img_url,rc_data_url){
    // only call initialization once.
    try {
        if (!is_rc_initialized) {
            rc_initialize();
        }
        
        //assume clear is called first if this is the second load

        image = img_container.image(rc_img_url).loaded(function(loader) {
                                              console.log('loading image');
                                              //this should rotate the image
                                              //alert('loading img');
                                              //image_width = loader.height;
                                              //image_height = loader.width;
                                              //image.rotate(90, image_width/2, image_height/2).move(-(image_height-image_width)/2,(image_height-image_width)/2);
                                              
                                                       
                                              image_width = loader.width;
                                              image_height = loader.height;

                                              draw_g.add(image);
                                              if ( ! draw.node.contains(menu_svg.node)) {draw.node.appendChild(menu_svg.node);}
                                              
                                              
                                              window.setTimeout( function() {
                                                                    //alert('loading spatial data');
                                                                    load_spatial_data(rc_data_url, image_width); //this function is defined in depth_data.js
                                                                    //size depthmap
                                                                    dm_size(image_width,image_height);
                                                                }, 0);
                                              
                                              
                                              
                                              // Initial dexecution if needed
                                              //alert('do on orientation change');
                                              doOnOrientationChange();
                                              
                                              
                                              });
    
        return 0;
    }
    catch(err) {
        console.log(err.message);
        return(err.message);
    }
    
    
}
