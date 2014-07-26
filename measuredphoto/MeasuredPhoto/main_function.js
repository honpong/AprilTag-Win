
function main(rc_img_url,rc_data_url){
    
    /* create an svg drawing */
    draw = SVG('drawing').size(window.innerWidth, window.innerHeight);
    draw_g = draw.group();
    menu_svg = draw.nested();
    
    np_to_portrait(); //this is initializing style of the number pad
    
    image = draw.image(rc_img_url).loaded(function(loader) {
                                          //this should rotate the image
                                          image_width = loader.height;
                                          image_height = loader.width;
                                          image.rotate(90, image_width/2, image_height/2).move(-(image_height-image_width)/2,(image_height-image_width)/2);
                                          
                                          //image_width = loader.width;
                                          //image_height = loader.height;
                                          load_spatial_data(rc_data_url, image_width); //this function is defined in depth_data.js
                                          draw_g.add(image);
                                          
                                          // Initial dexecution if needed
                                          doOnOrientationChange();
                                          if ( ! draw.node.contains(menu_svg.node)) {draw.node.appendChild(menu_svg.node);}
                                          
                                          //size depthmap
                                          dm_size(image_width,image_height);
                                          });
    
    measured_svg = draw.nested();
    draw_g.add(measured_svg);
    
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
    
    
        
    function click_or_touch(e) {
        //if ( e.pageX > image_width || e.pageY > image_height) {return null;} //ignore taps out of range
        if (current_button == button2) {
            if (lineNotStarted){
                lineNotStarted = false;
                var i = pxl_to_img_xy(e.pageX, e.pageY);
                click_image_x1 = i.x; //we map into image coordinates here, incase scale factor chagnes between clicks
                click_image_y1 = i.y;
                marker = measured_svg.circle(4).move(click_image_x1-2,click_image_y1-2).stroke({ color: line_color, opacity: 0.4, width : 2 }).fill({opacity:0});
            }
            else {
                lineNotStarted = true;
                var i = pxl_to_img_xy(e.pageX, e.pageY);
                // we want to instantiate a measurement here, and pass that measurement to be drawn
                rcMeasurements.new_measurement(click_image_x1, click_image_y1, i.x, i.y, measured_svg);
                click_image_x1 = null;
                click_image_y1 = null;
                marker.remove();
                deselect_button(button2);
            }
        }
    }
    
    
    FastClick.attach(document.body);
    
    draw.click( function(e) { setTimeout(function(){ click_or_touch(e); },1);   e.stopPropagation(); e.preventDefault();} );
    menu_background = menu_svg.rect(menu_svg.width(), menu_svg.height()).fill('#333');
    button1 = menu_svg.group();
    button2 = menu_svg.group();
    button3 = menu_svg.group();
    button4 = menu_svg.group();
    button5 = menu_svg.group();
    button6 = menu_svg.group();
    //var button7 = menu_svg.group();
    //var button8 = menu_svg.group();
    button1.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    button2.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    button3.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    button4.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    button5.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    button6.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    //button7.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    //button8.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    
    // we are no longer using a mouse icon button...
    //// add mouse arrow icon to button1
    //button1.add(menu_svg.polyline('23.42,31.7 19,25.72 15.1,29.1 9.9,8 26.8,19.35 22.9,22.99 27.19,28.45').stroke({ color: button_outline_color, opacity: 1, width: 2.5 }).fill('none'));
    
    // add 3d button to button 1
    button1.add(menu_svg.line(14,24,14,5).stroke({ color: button_outline_color, opacity: 1, width: 2.1 }));
    button1.add(menu_svg.line(14,24,7,32).stroke({ color: button_outline_color, opacity: 1, width: 2.5 }));
    button1.add(menu_svg.line(14,24,33,30).stroke({ color: button_outline_color, opacity: 1, width: 2.2 }));
    
    //add line icon to button2
    button2.add(menu_svg.circle(2).move(8,27).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill('none'));
    button2.add(menu_svg.circle(2).move(27,8).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill('none'));
    button2.add(menu_svg.line(9,28,28,9).stroke({ color: button_outline_color, opacity: 1, width: 2 }));
    
    //add angle icon to button2
    button3.add(menu_svg.path('M23,33 a 15,15 0 0,0 -12,-16').stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill('none'));
    button3.add(menu_svg.line(7,28,24,11).stroke({ color: button_outline_color, opacity: 1, width: 2 }));
    button3.add(menu_svg.line(7,28,30,28).stroke({ color: button_outline_color, opacity: 1, width: 2 }));
    
    
    // add magnifying glass to button4
    button4.add(menu_svg.circle(17).move(13,7).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill('none'));
    button4.add(menu_svg.line(15.5,21,7,30).stroke({ color: button_outline_color, opacity: 1, width: 3 }));
    button4.add(menu_svg.line(21.5,10,21.5,20).stroke({ color: button_outline_color, opacity: 1, width: 2 }));
    button4.add(menu_svg.line(16.5,15,26.5,15).stroke({ color: button_outline_color, opacity: 1, width: 2 }));
    
    // add magnifying glass - to button5
    button5.add(menu_svg.circle(17).move(13,7).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill('none'));
    button5.add(menu_svg.line(15.5,21,7,30).stroke({ color: button_outline_color, opacity: 1, width: 3 }));
    button5.add(menu_svg.line(16.5,15,26.5,15).stroke({ color: button_outline_color, opacity: 1, width: 2 }));
    
    // add trash can to button6
    button6.add(menu_svg.polyline('10,12 10,31 28,31 28,12').stroke({ color: button_outline_color, opacity: 1, width: 2.7 }).fill('none'));
    button6.add(menu_svg.polyline('15,7 15,5 23,5 23,7').stroke({ color: button_outline_color, opacity: 1, width: 2 }).fill('none'));
    button6.add(menu_svg.line( 7, 9, 31, 9).stroke({ color: button_outline_color, opacity: 1, width: 2.7 }));
    button6.add(menu_svg.line(15,13,15,25).stroke({ color: button_outline_color, opacity: 1, width: 2 }));
    button6.add(menu_svg.line(19,13,19,25).stroke({ color: button_outline_color, opacity: 1, width: 2 }));
    button6.add(menu_svg.line(23,13,23,25).stroke({ color: button_outline_color, opacity: 1, width: 2 }));
    rearrange_menu();
    
    var current_button = null;
    
    function select_button(button) {
        //this relies on the fact that the first thing we added to the button groups was the background rectangles.
        if (current_button) {current_button.get(0).fill(button_fill_color);}
        if (button) {
            current_button = button;
            current_button.get(0).fill(button_highlight_color);
        }
    }
    
    function deselect_button(button) {
        //this relies on the fact that the first thing we added to the button groups was the background rectangles.
        button.get(0).fill(button_fill_color);
        if (current_button == button) { current_button = null; }
    }
    
    var image_shown = true;
    function switch_image_depthmap() { //we move the image svg off the dom, and move the depthmap on the dom.
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
    
    //add events to buttons
    button1.click (function (e) { switch_image_depthmap(); e.stopPropagation(); e.preventDefault();});
    button2.click (function (e) { select_button(button2); e.stopPropagation(); e.preventDefault();});
    button3.click (function (e) { select_button(button3); e.stopPropagation(); e.preventDefault();
                   var next_orientation;
                   if (last_orientation == 1) {next_orientation = 3;}
                   else if (last_orientation == 3) {next_orientation = 2;}
                   else if (last_orientation == 2) {next_orientation = 4;}
                   else if (last_orientation == 4) {next_orientation = 1;}
                   forceOrientationChange(next_orientation);})
    ;
    button4.click (function (e) { setTimeout(function(){ zoom(1.2, window.innerWidth / 2, window.innerHeight / 2) ;},1); e.stopPropagation(); e.preventDefault(); start_pan_bounce(); start_zoom_return();});
    button5.click (function (e) { setTimeout(function(){ zoom(0.8, window.innerWidth / 2, window.innerHeight / 2) ;},1); e.stopPropagation(); e.preventDefault(); start_pan_bounce(); setTimeout(function() {start_zoom_return();},10); });

    //add a button to delete a measurement using delete_selected();
    
    button6.click (function (e) { setTimeout(function(){
                                             prepare_to_exit();
                                             document.location = 'native://finish';
                                             },1); e.stopPropagation(); e.preventDefault();});

    function prepare_to_exit() {
        //TODO: put code here to save current measurements and any other shutdown to prepare to exit.
        return true;
    }
    
    
    //button7.click (function () { select_button(button7); })
    //button8.click (function () { select_button(button8); })
    
    
    function delete_selected() {
        current_measurement.is_deleted = true;
        rcMeasurements.redraw_all_measurements();
    }
    
    
    function unit_menu(){
        alert ('no unit options at this time');
    }
    
    
    function color_menu() {
        alert ('no color options at this time');
    }
    
    np_call_back_add = rcMeasurements.add_character;
    np_call_back_del = rcMeasurements.del_character;
    np_call_back_unt = unit_menu;
    np_call_back_ent = rcMeasurements.finish_number_operation;
    np_call_back_oth = color_menu;
    
    np_add_listeners(); //setup image
    np_to_portrait(); //scale and orient keyboard prior to first use
    
    function doOnOrientationChange()
    {
        //change body and document size here too.
        
        document.body.style.width = window.innerWidth.toFixed(0) + 'px';
        document.body.style.height = window.innerHeight.toFixed(0) + 'px';
        viewport = document.querySelector("meta[name=viewport]");
        viewport.setAttribute('content', "width=device-width, height=device-height, initial-scale=1, user-scalable=0");
        draw.node.style.height = window.innerHeight;
        draw.node.style.width = window.innerWidth;
        draw.size(window.innerWidth, window.innerHeight);
        
        scaleImageToMatchScreen();
        rearrange_menu ();
        if (orientation_drawn_landsacep){np_to_landscape_bottom();}
        else {np_to_portrait();}
        
    }
    
    
    function rearrange_menu () {
        var h, v;
        
        if (draw.node.contains(menu_svg.node)) {draw.node.removeChild(menu_svg.node);}
        if(window.innerHeight > window.innerWidth){
            //portrait, we want buttons on the bottom and pad the viewport by buttom height
            menu_svg.size( window.innerWidth, button_size);
            h=1; v=0;
        }
        else {
            //landsape, we want buttons on side and pad the viewport by buttom width
            menu_svg.size(button_size, window.innerHeight);
            h=0; v=1;
        }
        
        v_offset = v * (window.innerHeight - button_size * 6) / 2;
        h_offset = h * (window.innerWidth - button_size * 6) / 2;
        menu_background.size(menu_svg.width(), menu_svg.height());
        button1.move(h_offset + 0 * button_size * h, v_offset + 0 * button_size * v);
        button2.move(h_offset + 1 * button_size * h, v_offset + 1 * button_size * v);
        button3.move(h_offset + 2 * button_size * h, v_offset + 2 * button_size * v);
        button4.move(h_offset + 3 * button_size * h, v_offset + 3 * button_size * v);
        button5.move(h_offset + 4 * button_size * h, v_offset + 4 * button_size * v);
        button6.move(h_offset + 5 * button_size * h, v_offset + 5 * button_size * v);
        //button7.move(h_offset + 6 * button_size * h, v_offset + 6 * button_size * v);
        //button8.move(h_offset + 7 * button_size * h, v_offset + 7 * button_size * v);
        
    }
    
    //prevent scrolling
    document.body.addEventListener('touchstart', function(e){ e.stopPropagation(); e.preventDefault(); });
    
    window.addEventListener('resize', function(event){
                            if (draw.node.contains(menu_svg.node)) {draw.node.removeChild(menu_svg.node);}
                            doOnOrientationChange();
                            setTimeout(function(){
                                       if ( ! draw.node.contains(menu_svg.node)) {draw.node.appendChild(menu_svg.node);}
                                       },100)
                            start_pan_bounce(); start_zoom_return();
                            });
}
