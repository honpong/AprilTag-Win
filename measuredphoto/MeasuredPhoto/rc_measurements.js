//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

rcMeasurements = {
    measurements : {}, measurement_being_edited : null, inches_to_meter : 39.3701, cursor_animation_id : null
}

// instantiate a measurement and add it to the measurment list
// takes image locations
rcMeasurements.new_measurement = function (iX1, iY1, iX2, iY2, measured_svg){
    var m = {};
    var start_time = new Date();
    m.distance = distanceBetween(iX1, iY1, iX2, iY2); //distanceBetween is defined in depth_data.js
    m.overwriten = false;
    m.x1 = iX1;
    m.y1 = iY1;
    m.x2 = iX2;
    m.y2 = iY2;
    m.units_metric = default_units_metric;
    //use for identifying in measurements dicitonary
    m.guid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {var r = Math.random()*16|0,v=c=='x'?r:r&0x3|0x8;return v.toString(16);});

    rcMeasurements.draw_measurement(m, measured_svg);
    
}

rcMeasurements.saveable_liniar_measurement = function (m) {
    //we only want a subset of the measurements content, so we create a temp object we write the content we want to keep into
    return { distance:m.distance, overwriten:m.overwriten, x1:m.x1, y1:m.y1, x2:m.x2, y2:m.y2, guid:m.guid, units_metric:m.units_metric}
}

rcMeasurements.draw_measurement = function (m, measured_svg){
    
    m.pixel_distatnce = Math.sqrt( Math.pow((m.x1 - m.x2), 2) + Math.pow((m.y1 - m.y2), 2));
    m.xdiffrt = (m.x1-m.x2) / m.pixel_distatnce;
    m.ydiffrt = (m.y1-m.y2) / m.pixel_distatnce;
    m.mid_x = m.x1 + (m.x2 - m.x1)/2;
    m.mid_y = m.y1 + (m.y2 - m.y1)/2;
    
    m.half_font_gap = 25;
    m.font_offset = false;
    m.font_offset_x = 0;
    m.font_offset_y = -5;
    
    if (m.pixel_distatnce < m.half_font_gap * 2 + 10) {
        m.font_offset = true;
        m.font_offset_x = -m.half_font_gap * m.ydiffrt;
        m.font_offset_y = m.half_font_gap * m.xdiffrt;
    }
    
    //This allows a measurement to be saved. it is necessary to call this as part of draw, because
	//a measurement will not have this function when it is deserialized.
    m.saveable_copy = rcMeasurements.saveable_liniar_measurement(m)
    
    //we need to write distance onto screen
    var d_string = rcMeasurements.format_dist(m);
    m.text_shadow = measured_svg.text(d_string).move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text_shadow.font({
                family: 'San Serif'
                , size: 25
                , anchor: 'middle'
                , leading: 1
                }).stroke({ color: shadow_color, opacity: 1, width: 5 });

    m.text = measured_svg.text(d_string).move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text.font({
                family: 'San Serif'
                , size: 25
                , anchor: 'middle'
                , leading: 1
                }).fill({ color: line_color, opacity: 1});
    m.text_input_box = measured_svg.foreignObject(m.half_font_gap * 2, m.half_font_gap * 2).move(m.mid_x - m.half_font_gap, m.mid_y - m.half_font_gap );
    console.log('text width = ' + m.text.width().toFixed());
    
    var foo = m.text.lines.members[0];
    console.log( foo.width().toFixed() );
    
    //text box cursor approach //m.text_cursor = measured_svg.plain("").move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text_cursor = measured_svg.line(0,0,0,24);
    rcMeasurements.place_cursor(m);
    m.text_cursor.stroke({ color: shadow_color, opacity: 0, width: 2 });
    //m.text_cursor.font({
    //            family: 'Courier'
    //            , size: 18
    //            , anchor: 'middle'
    //            , leading: 1
    //            }).fill({'color':highlight_color, 'opacity':0});

    var input = document.createElement("input");
    input.name = "distance-" + m.guid
    input.value = m.distance;
    m.text_input_box.appendChild(input);
    measured_svg.node.removeChild(m.text_input_box.node); //hide the input box after adding it
    
    
    if (m.font_offset){
        m.shadow_line1 = measured_svg.line(m.x1 - 3 * m.xdiffrt, m.y1 - 3 * m.ydiffrt, m.x2 + 3 * m.xdiffrt, m.y2 + 3 * m.ydiffrt);
        m.shadow_line2 = measured_svg.line(m.x1 - 3 * m.xdiffrt, m.y1 - 3 * m.ydiffrt, m.x2 + 3 * m.xdiffrt, m.y2 + 3 * m.ydiffrt);
    }
    else {
        m.shadow_line1 = measured_svg.line(m.x1 - 3 * m.xdiffrt, m.y1 - 3 * m.ydiffrt, m.mid_x + m.half_font_gap * m.xdiffrt, m.mid_y + m.half_font_gap * m.ydiffrt);
        m.shadow_line2 = measured_svg.line(m.mid_x - m.half_font_gap * m.xdiffrt, m.mid_y - m.half_font_gap * m.ydiffrt, m.x2 + 3 * m.xdiffrt, m.y2 + 3 * m.ydiffrt);
    }
    m.shadow_line1.stroke({ color: shadow_color, opacity: .7, width: 4 });
    m.shadow_line2.stroke({ color: shadow_color, opacity: .7, width: 4 });
    
    m.circle1 = measured_svg.circle(6).move(m.x1-3,m.y1-3).stroke({ color: shadow_color, width : 1.5 }).fill({ color: line_color , opacity:1});
    m.circle2 = measured_svg.circle(6).move(m.x2-3,m.y2-3).stroke({ color: shadow_color, width : 1.5 }).fill({ color: line_color , opacity:1 });
    
    if (m.font_offset){
        m.line1 = measured_svg.line(m.x1 - 1 * m.xdiffrt, m.y1 - 1 * m.ydiffrt, m.x2 + 1 * m.xdiffrt, m.y2 + 1 * m.ydiffrt)
        m.line2 = measured_svg.line(m.x1 - 1 * m.xdiffrt, m.y1 - 1 * m.ydiffrt, m.x2 + 1 * m.xdiffrt, m.y2 + 1 * m.ydiffrt)
    }
    else {
        m.line1 = measured_svg.line(m.x1 - 1 * m.xdiffrt, m.y1 - 1 * m.ydiffrt,  m.mid_x + (m.half_font_gap + 1) * m.xdiffrt, m.mid_y + (m.half_font_gap + 1) * m.ydiffrt)
        m.line2 = measured_svg.line(m.mid_x - (m.half_font_gap + 1) * m.xdiffrt, m.mid_y - (m.half_font_gap + 1) * m.ydiffrt, m.x2 + 1 * m.xdiffrt, m.y2 + 1 * m.ydiffrt)
    }
    
    m.line1.stroke({ color: line_color, width: 2 }).fill({ color: line_color });
    m.line2.stroke({ color: line_color, width: 2 }).fill({ color: line_color });
    
    m.selector_circle1 = measured_svg.circle(30).move(m.x1-15,m.y1-15).fill({opacity:0});
    m.selector_circle2 = measured_svg.circle(30).move(m.x2-15,m.y2-15).fill({opacity:0});
    
    //acctions for what happens on clicks
    m.shadow_line1.click (function (e) { rcMeasurements.click_action(m); e.stopPropagation(); e.preventDefault(); })
    m.shadow_line2.click (function (e) { rcMeasurements.click_action(m); e.stopPropagation(); e.preventDefault();})
    m.line1.click (function (e) { rcMeasurements.click_action(m); e.stopPropagation(); e.preventDefault();})
    m.line2.click (function (e) { rcMeasurements.click_action(m); e.stopPropagation(); e.preventDefault();})
    m.circle1.click (function (e) { rcMeasurements.click_action(m); e.stopPropagation(); e.preventDefault();})
    m.circle2.click (function (e) { rcMeasurements.click_action(m); e.stopPropagation(); e.preventDefault();})
    m.selector_circle1.click(function (e) { rcMeasurements.click_action(m); e.stopPropagation(); e.preventDefault();})
    m.selector_circle2.click(function (e) { rcMeasurements.click_action(m); e.stopPropagation(); e.preventDefault();})
    //text editing
    m.text.click(function (e) {
                     setTimeout(function(){ return false;},1);
                     rcMeasurements.start_distance_change_dialouge(m);
                     e.stopPropagation(); e.preventDefault();
                 })
    
    //draging measurement end points.
    Hammer(m.circle1.node).on("drag", function(e) {
                                  i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                  rcMeasurements.move_measurement(m, i.x, i.y, m.x2, m.y2);
                                  e.stopPropagation(); e.preventDefault();
                              }).on("dragend", function(e) {rcMeasurements.deselect_measurement(m); rcMeasurements.save_measurements(); e.stopPropagation(); e.preventDefault();});
    Hammer(m.circle2.node).on("drag",  function(e) { i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, m.x1, m.y1, i.x, i.y); e.stopPropagation(); e.preventDefault();}).on("dragend", function(e) {rcMeasurements.deselect_measurement(m); rcMeasurements.save_measurements();e.stopPropagation(); e.preventDefault();});
    Hammer(m.selector_circle1.node).on("drag",  function(e) {i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, i.x, i.y, m.x2, m.y2); e.stopPropagation(); e.preventDefault(); }).on("dragend", function(e) {rcMeasurements.deselect_measurement(m); rcMeasurements.save_measurements();e.stopPropagation(); e.preventDefault();});
    Hammer(m.selector_circle2.node).on("drag", function(e) {i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, m.x1, m.y1, i.x, i.y); e.stopPropagation(); e.preventDefault(); }).on("dragend", function(e) {rcMeasurements.deselect_measurement(m); rcMeasurements.save_measurements();e.stopPropagation(); e.preventDefault();});
    
    rcMeasurements.measurements[m.guid] = m;

}

rcMeasurements.click_action = function (m) {
    setTimeout(function(){ return false;},1);  //this just forces refresh for some browsers
    if ( ! rcMeasurements.is_measurement_being_deleted(m) ) {  //this is both a deletion and a check for deletion
        //rcMeasurements.select_measurement(m); //we aren't doing anything here any more
    }
}

rcMeasurements.redraw_measurement = function (m) {
    
    m.pixel_distatnce = Math.sqrt( Math.pow((m.x1 - m.x2), 2) + Math.pow((m.y1 - m.y2), 2));
    m.xdiffrt = (m.x1-m.x2) / m.pixel_distatnce;
    m.ydiffrt = (m.y1-m.y2) / m.pixel_distatnce;
    m.mid_x = m.x1 + (m.x2 - m.x1)/2;
    m.mid_y = m.y1 + (m.y2 - m.y1)/2;
    
    m.half_font_gap = 25;
    m.font_offset = false;
    m.font_offset_x = 0;
    m.font_offset_y = -5;
    
    if (m.pixel_distatnce < m.half_font_gap * 2 + 10) {
        m.font_offset = true;
        m.font_offset_x = -m.half_font_gap * m.ydiffrt;
        m.font_offset_y = m.half_font_gap * m.xdiffrt;
    }
    m.text_shadow.text(rcMeasurements.format_dist(m)).x(m.mid_x + m.font_offset_x).dy(m.mid_y + - m.text.node.attributes.y.value + m.font_offset_y*2);
    m.text.text(rcMeasurements.format_dist(m)).x(m.mid_x + m.font_offset_x).dy(m.mid_y + - m.text.node.attributes.y.value + m.font_offset_y*2); //hacky thing because move has a bug
    m.text_input_box.x(m.mid_x + m.font_offset_x).y(m.mid_y + m.font_offset_y*2); //do the same movement with the input box
    rcMeasurements.place_cursor(m);

    m.saveable_copy = rcMeasurements.saveable_liniar_measurement(m); // need to change how this measurement is saved
    
    m.circle1.move(m.x1-3,m.y1-3);
    m.circle2.move(m.x2-3,m.y2-3);
    m.selector_circle1.move(m.x1-15,m.y1-15);
    m.selector_circle2.move(m.x2-15,m.y2-15);
    
    if (m.font_offset){
        m.shadow_line1.plot(m.x1 - 3 * m.xdiffrt, m.y1 - 3 * m.ydiffrt, m.x2 + 3 * m.xdiffrt, m.y2 + 3 * m.ydiffrt);
        m.shadow_line2.plot(m.x1 - 3 * m.xdiffrt, m.y1 - 3 * m.ydiffrt, m.x2 + 3 * m.xdiffrt, m.y2 + 3 * m.ydiffrt);
    }
    else {
        m.shadow_line1.plot(m.x1 - 3 * m.xdiffrt, m.y1 - 3 * m.ydiffrt, m.mid_x + m.half_font_gap * m.xdiffrt, m.mid_y + m.half_font_gap * m.ydiffrt);
        m.shadow_line2.plot(m.mid_x - m.half_font_gap * m.xdiffrt, m.mid_y - m.half_font_gap * m.ydiffrt, m.x2 + 3 * m.xdiffrt, m.y2 + 3 * m.ydiffrt);
    }
    
    if (m.font_offset){
        m.line1.plot(m.x1 - 1 * m.xdiffrt, m.y1 - 1 * m.ydiffrt, m.x2 + 1 * m.xdiffrt, m.y2 + 1 * m.ydiffrt)
        m.line2.plot(m.x1 - 1 * m.xdiffrt, m.y1 - 1 * m.ydiffrt, m.x2 + 1 * m.xdiffrt, m.y2 + 1 * m.ydiffrt)
    }
    else {
        m.line1.plot(m.x1 - 1 * m.xdiffrt, m.y1 - 1 * m.ydiffrt,  m.mid_x + (m.half_font_gap + 1) * m.xdiffrt, m.mid_y + (m.half_font_gap + 1) * m.ydiffrt)
        m.line2.plot(m.mid_x - (m.half_font_gap + 1) * m.xdiffrt, m.mid_y - (m.half_font_gap + 1) * m.ydiffrt, m.x2 + 1 * m.xdiffrt, m.y2 + 1 * m.ydiffrt)
    }
}

rcMeasurements.redraw_all_measurements = function (){
    for (var key in rcMeasurements.measurements) {
        rcMeasurements.redraw_measurement(measurements[key]);
    }
}



rcMeasurements.to_json = function () {
    var measurements_to_save = {};
    // if the app has set (or reset) the default units then save the default units.
    if (unit_default_set_by_app) { measurements_to_save['use_metric'] = default_units_metric; }
    for (var key in rcMeasurements.measurements) {
        measurements_to_save[key] = rcMeasurements.measurements[key].saveable_copy;
    }
    return JSON.stringify(measurements_to_save);
}

rcMeasurements.save_measurements = function () {
    jsonStr = rcMeasurements.to_json();
    $.ajax({ type: "PUT", url: rc_server_location + "true_measure/api/v1/m_photo/" + m_photo_guid + "/annotations/", contentType: "application/json", processData: false, dataType: "json", data: jsonStr })
    .done(function(data, textStatus, jqXHR) {
          //alert(textStatus + ": " + JSON.stringify(data));
          })
    .fail(function(jqXHR, textStatus, errorThrown) {
          //alert(textStatus + ": " + JSON.stringify(jqXHR));
          })
    ;
}


rcMeasurements.delete_measurement  = function (m) {
    window.setTimeout( function() {
          if (confirm('delete measurement?')) {
              //remove visual elemnts
              m.text.remove();
              m.text_shadow.remove();
              m.circle1.remove();
              m.circle2.remove();
              m.selector_circle1.remove();
              m.selector_circle2.remove();
              m.shadow_line1.remove();
              m.shadow_line2.remove();
              m.line1.remove();
              m.line2.remove();
              //removed from measurements array
              delete rcMeasurements.measurements[m.guid];
              //alert(rcMeasurements.to_json());
          }
    }, 0)
}

rcMeasurements.load_json  = function (m_url, callback_function) {
    //get measurements
    setTimeout(function(){
        $.ajaxSetup({ cache: false });
        $.getJSON(m_url, function(data) {
            //use then strip out metadata
            if ('use_metric' in data) {
                //we only overwrite the default units with the stored defualt units if the app hasn't told us what units to use
                if (! unit_default_set_by_app) {default_units_metric = data['use_metric'];}
                delete myArray['use_metric'];
            }
            rcMeasurements.measurements = data;
            //for each measurement, draw measurement
            for (var key in rcMeasurements.measurements) {
             rcMeasurements.draw_measurement(rcMeasurements.measurements[key], measured_svg);
            }
            callback_function();
        }).error(function () {
            //window.setTimeout(function(){alert('failed to load annotations')},0);
            rcMeasurements.measurements = {};
            callback_function();
        });
    }, 0);
}

// functions for coloring and decoloring selected lines
rcMeasurements.paint_selected = function (m) {
    m.circle1.stroke({ color: highlight_color, opacity: 1, width: 1.5 });
    m.circle2.stroke({ color: highlight_color, opacity: 1, width: 1.5 });
    m.shadow_line1.stroke({ color: highlight_color, opacity: 1, width: 4 });
    m.shadow_line2.stroke({ color: highlight_color, opacity: 1, width: 4 });
}

rcMeasurements.paint_deselected = function (m) {
    m.circle1.stroke({ color: shadow_color, opacity: 1, width: 1.5 });
    m.circle2.stroke({ color: shadow_color, opacity: 1, width: 1.5 });
    m.shadow_line1.stroke({ color: shadow_color, opacity: 1, width: 4 });
    m.shadow_line2.stroke({ color: shadow_color, opacity: 1, width: 4 });
}

// functions for selecting and moving measurements

rcMeasurements.select_measurement = function (m) {
    if (current_measurement == m) { return null;} //do nothing
    else if (current_measurement ) { //switch measurements
        rcMeasurements.paint_deselected(current_measurement);
    }
    rcMeasurements.end_measurement_edit();            //if we're switching current measurements we need to terminate any open measurement dialogues
    current_measurement = m;
    rcMeasurements.paint_selected(current_measurement);
}

rcMeasurements.deselect_measurement = function (m) {
    if (current_measurement == m) { current_measurement = null;}
    rcMeasurements.paint_deselected(m);
}


rcMeasurements.move_measurement = function (m, nx1, ny1, nx2, ny2) {
    rcMeasurements.select_measurement(m);
    m.x1 = nx1;
    m.y1 = ny1;
    m.x2 = nx2;
    m.y2 = ny2;
    if (!m.overwriten){ m.distance = distanceBetween(m.x1, m.y1, m.x2, m.y2); } //distanceBetween is defineed in depth_data.js
    
    rcMeasurements.redraw_measurement(m);
}

// functions for modifying measurements
//rcMeasurements.cursor_string_for_distance_string = function (d_str) {
//    var c_str = ".";
//    for (i=0; i < (d_str.length*2); i++) {
//        c_str = c_str + " ";
//    }
//    c_str = c_str + "|";
//    return c_str;
//}

rcMeasurements.setText = function (m, str) {
    m.text_shadow.text(str);
    m.text.text(str);
}

rcMeasurements.place_cursor = function (m){
    var u_offset;
    if (m.units_metric) {u_offset = 23}
    else {u_offset = 12}
    var c_offset = m.text.node.offsetWidth/2 - u_offset;
    m.text_cursor.move(m.mid_x + m.font_offset_x + c_offset, m.mid_y + m.font_offset_y); //and for curser

}


rcMeasurements.start_cursor_animation = function (m) {
    if (rcMeasurements.cursor_animation_id){ window.clearTimeout(rcMeasurements.cursor_animation_id) };
    rcMeasurements.cursor_animation_id = window.setTimeout(function(){rcMeasurements.show_cursor_frame(m)}, 10);
}

rcMeasurements.show_cursor_frame = function (m) {
    rcMeasurements.place_cursor(m);
    m.text_cursor.stroke({ color: shadow_color, opacity: .9, width: 2 });
    rcMeasurements.cursor_animation_id = window.setTimeout(function(){rcMeasurements.hide_cursor_frame(m)},700);
}

rcMeasurements.hide_cursor_frame = function (m) {
    m.text_cursor.stroke({ color: shadow_color, opacity: 0, width: 2 });
    rcMeasurements.cursor_animation_id = window.setTimeout(function(){rcMeasurements.show_cursor_frame(m)},700);
}

rcMeasurements.stop_cursor_animation = function (m) {
    m.text_cursor.stroke({ color: shadow_color, opacity: 0, width: 2 });
    if (rcMeasurements.cursor_animation_id){ window.clearTimeout(rcMeasurements.cursor_animation_id) };
    rcMeasurements.cursor_animation_id = null
}

rcMeasurements.start_distance_change_dialouge = function (m) {
        rcMeasurements.select_measurement(m); //highlight measurement we're editing
        rcMeasurements.measurement_being_edited = m;
        //if (is_touch_device) { //use svg text element durring edit
        draw.node.appendChild(np_svg.node); //show number pad
        np_rotate(last_orientation); //this is initializing style of the number pad
        move_image_for_number_pad(m.text.x(), m.text.y());
        rcMeasurements.start_cursor_animation(m);
    

        // this is commented out because it is only for use on desktop systems.

        //}
        //else { // use
        //    measured_svg.node.removeChild( m.text.node ); //detatch measurement from svg
        //    measured_svg.node.appendChild( m.text_input_box.node); //show input box
        //    var n = m.text_input_box.getChild(0)
        //    n.focus();
        //    n.select();
        //}
        
}

rcMeasurements.end_measurement_edit = function (){
    if (rcMeasurements.measurement_being_edited) {
        //if (is_touch_device) {
        rcMeasurements.deselect_measurement(rcMeasurements.measurement_being_edited); //un-highlight measurement we're editing
        if(draw.node.contains(np_svg.node)) {draw.node.removeChild(np_svg.node);} //hide number pad
        return_image_after_number_pad();
        rcMeasurements.stop_cursor_animation(rcMeasurements.measurement_being_edited);

        //}
        //else {
        //    measured_svg.node.removeChild( measurement_being_edited.text_input_box.node); //hide input box...
        //    measured_svg.node.appendChild( measurement_being_edited.text.node ); //show formated distance
        //}
    }
    rcMeasurements.measurement_being_edited = null; //so we know wether or not we have a sesion open.
    setTimeout( function () {rcMeasurements.save_measurements();}, 0)
}

rcMeasurements.switch_units = function () {
    var parsed_distance = rcMeasurements.parse_dist(rcMeasurements.measurement_being_edited.text.text(), rcMeasurements.measurement_being_edited.units_metric);
    if (parsed_distance == 'err') {
            setTimeout(function () { alert("Invalid number entered, please correct before proceeding"); }, 0);
    }
    else  {
        rcMeasurements.measurement_being_edited.distance = parsed_distance;
        if(rcMeasurements.measurement_being_edited.units_metric == true){ rcMeasurements.measurement_being_edited.units_metric = false; }
        else{ rcMeasurements.measurement_being_edited.units_metric = true; }
        rcMeasurements.setText(rcMeasurements.measurement_being_edited, rcMeasurements.format_dist(rcMeasurements.measurement_being_edited));
    }
    rcMeasurements.place_cursor(rcMeasurements.measurement_being_edited);
}


rcMeasurements.add_character = function (key) {
    var str      = rcMeasurements.measurement_being_edited.text.text();
    var unit_str = str.substring(str.length - 2);
    str = str.substring(0, str.length - 2);

    if (str == '?') { rcMeasurements.setText(rcMeasurements.measurement_being_edited, key + unit_str); }
    else {rcMeasurements.setText(rcMeasurements.measurement_being_edited,  str + key + unit_str);}
    rcMeasurements.place_cursor(rcMeasurements.measurement_being_edited);

}
rcMeasurements.del_character = function (key) {
    var str      = rcMeasurements.measurement_being_edited.text.text();
    var unit_str = str.substring(str.length - 2);
    str = str.substring(0, str.length - 2);
    
    if (str.length <= 1) { rcMeasurements.setText(rcMeasurements.measurement_being_edited, '?' + unit_str); }
    else{
        rcMeasurements.setText( rcMeasurements.measurement_being_edited,  str.substring(0, str.length - 1) + unit_str );
    }
    rcMeasurements.place_cursor(rcMeasurements.measurement_being_edited);
}

// called on distance before drawn to screen
rcMeasurements.format_dist = function (m){
    
    if (m.distance) {
        if (m.units_metric) { return m.distance.toFixed(2)+' m'; }
        else { return (m.distance * rcMeasurements.inches_to_meter).toFixed(2) + ' "'; }
    }
    else {
        if (m.units_metric) { return '? m'; }
        else { return '? "'; }
    }
    
}

// sets distance for a measurement based on the value of a string
rcMeasurements.parse_dist = function (str, units_metric){
    str = str.substring(0, str.length - 2);
    if (str == '?') {
        return null;
    }
    else if ( rcMeasurements.isNumber( str ) ) {
        if (units_metric) { return parseFloat(str); }
        else { return parseFloat(str) / rcMeasurements.inches_to_meter; }
    }
    else{
        return 'err';
    }
}


rcMeasurements.isNumber = function  (n) {
    return !isNaN(parseFloat(n)) && isFinite(n);
}
rcMeasurements.finish_number_operation = function (){
    //we need to check the validity of the input. if not a valid number, then raise a warning to the user, and either cancel or return to eiditing, if valid, update measuremnt
    var parsed_distance = rcMeasurements.parse_dist(rcMeasurements.measurement_being_edited.text.text(), rcMeasurements.measurement_being_edited.units_metric);
    if (parsed_distance == 'err') {
        setTimeout(function () { alert("invalid number, please correct before proceeding"); }, 0);
    }
    else  {
        rcMeasurements.measurement_being_edited.distance = parsed_distance;
        rcMeasurements.setText( rcMeasurements.measurement_being_edited, rcMeasurements.format_dist(rcMeasurements.measurement_being_edited));
        rcMeasurements.measurement_being_edited.saveable_copy = rcMeasurements.saveable_liniar_measurement(rcMeasurements.measurement_being_edited);
        rcMeasurements.end_measurement_edit();
    }
}

// this checks for wether or not the rc_menu button designated as the eraser button is selected.
rcMeasurements.is_measurement_being_deleted = function (m) {
    if (rc_menu.current_button == rc_menu.eraser_button) {
        rcMeasurements.delete_measurement(m);
        setTimeout( function () {rcMeasurements.save_measurements();}, 0)
        return true;
    }
    return false;
}

// this clears all measurements, used when we are switching between measured photos but not reloading the app
rcMeasurements.reset = function () {
    rcMeasurements.measurements = {};
    rcMeasurements.measurement_being_edited = null;
}
