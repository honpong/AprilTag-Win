//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

rcMeasurements = {
    measurements : {}, measurement_being_edited : null
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
    //use for identifying in measurements dicitonary
    m.guid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {var r = Math.random()*16|0,v=c=='x'?r:r&0x3|0x8;return v.toString(16);});

    rcMeasurements.draw_measurement(m, measured_svg);
    
    //each anotation type must implement this.
    m.saveable_copy = function() {
        //we only want a subset of the measurements content, so we create a temp object we write the content we want to keep into
        return { distance:m.distance, overwriten:m.overwriten, x1:m.x1, y1:m.y1, x2:m.x2, y2:m.y2, guid:m.guid}
    }

}

rcMeasurements.draw_measurement = function (m, measured_svg){
    
    var x1 = m.x1, y1 = m.y1;
    var x2 = m.x2, y2 = m.y2;
    
    
    var pixel_distatnce = Math.sqrt( Math.pow((x1 - x2), 2) + Math.pow((y1 - y2), 2));
    var xdiffrt = (x1-x2) / pixel_distatnce;
    var ydiffrt = (y1-y2) / pixel_distatnce;
    var mid_x = x1 + (x2 - x1)/2;
    var mid_y = y1 + (y2 - y1)/2;
    
    var half_font_gap = 15;
    var font_offset = false;
    var font_offset_x = 0;
    var font_offset_y = 0;
    
    if (pixel_distatnce < half_font_gap * 2 + 10) {
        font_offset = true;
        font_offset_x = -half_font_gap * ydiffrt;
        font_offset_y = half_font_gap * xdiffrt;
    }
    
    
    //we need to write distance onto screen
    var d_string = format_dist(m);
    m.text = measured_svg.text(d_string).move(mid_x + font_offset_x, mid_y + font_offset_y);
    m.text.font({
                family: 'Source Sans Pro'
                , size: 18
                , anchor: 'middle'
                , leading: 1
                }).fill(line_color).stroke({ color: shadow_color, opacity: .5, width: 1 });
    m.text_input_box = measured_svg.foreignObject(half_font_gap * 2, half_font_gap * 2).move(mid_x - half_font_gap, mid_y - half_font_gap );
    var input = document.createElement("input");
    input.name = "distance-" + m.guid
    input.value = m.distance;
    m.text_input_box.appendChild(input);
    measured_svg.node.removeChild(m.text_input_box.node); //hide the input box after adding it
    
    
    if (font_offset){
        m.shadow_line1 = measured_svg.line(x1 - 3 * xdiffrt, y1 - 3 * ydiffrt, x2 + 3 * xdiffrt, y2 + 3 * ydiffrt);
        m.shadow_line2 = measured_svg.line(x1 - 3 * xdiffrt, y1 - 3 * ydiffrt, x2 + 3 * xdiffrt, y2 + 3 * ydiffrt);
    }
    else {
        m.shadow_line1 = measured_svg.line(x1 - 3 * xdiffrt, y1 - 3 * ydiffrt, mid_x + half_font_gap * xdiffrt, mid_y + half_font_gap * ydiffrt);
        m.shadow_line2 = measured_svg.line(mid_x - half_font_gap * xdiffrt, mid_y - half_font_gap * ydiffrt, x2 + 3 * xdiffrt, y2 + 3 * ydiffrt);
    }
    m.shadow_line1.stroke({ color: shadow_color, opacity: 1, width: 4 });
    m.shadow_line2.stroke({ color: shadow_color, opacity: 1, width: 4 });
    
    m.circle1 = measured_svg.circle(6).move(x1-3,y1-3).stroke({ color: shadow_color, width : 1.5 }).fill({ color: line_color , opacity:1});
    m.circle2 = measured_svg.circle(6).move(x2-3,y2-3).stroke({ color: shadow_color, width : 1.5 }).fill({ color: line_color , opacity:1 });
    
    if (font_offset){
        m.line1 = measured_svg.line(x1 - 1 * xdiffrt, y1 - 1 * ydiffrt, x2 + 1 * xdiffrt, y2 + 1 * ydiffrt)
        m.line2 = measured_svg.line(x1 - 1 * xdiffrt, y1 - 1 * ydiffrt, x2 + 1 * xdiffrt, y2 + 1 * ydiffrt)
    }
    else {
        m.line1 = measured_svg.line(x1 - 1 * xdiffrt, y1 - 1 * ydiffrt,  mid_x + (half_font_gap + 1) * xdiffrt, mid_y + (half_font_gap + 1) * ydiffrt)
        m.line2 = measured_svg.line(mid_x - (half_font_gap + 1) * xdiffrt, mid_y - (half_font_gap + 1) * ydiffrt, x2 + 1 * xdiffrt, y2 + 1 * ydiffrt)
    }
    
    m.line1.stroke({ color: line_color, width: 1 }).fill({ color: line_color });
    m.line2.stroke({ color: line_color, width: 1 }).fill({ color: line_color });
    
    m.selector_circle1 = measured_svg.circle(30).move(x1-15,y1-15).fill({opacity:0});
    m.selector_circle2 = measured_svg.circle(30).move(x2-15,y2-15).fill({opacity:0});
    
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
    Hammer(m.circle1.node).on("drag", function(e) { i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, i.x, i.y, m.x2, m.y2); e.stopPropagation(); e.preventDefault(); });
    Hammer(m.circle2.node).on("drag",  function(e) { i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, m.x1, m.y1, i.x, i.y); e.stopPropagation(); e.preventDefault();});
    Hammer(m.selector_circle1.node).on("drag",  function(e) {i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, i.x, i.y, m.x2, m.y2); e.stopPropagation(); e.preventDefault(); });
    Hammer(m.selector_circle2.node).on("drag", function(e) {i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, m.x1, m.y1, i.x, i.y); e.stopPropagation(); e.preventDefault(); });
    
    rcMeasurements.measurements[m.guid] = m;

}

rcMeasurements.click_action = function (m) {
    setTimeout(function(){ return false;},1);  //this just forces refresh for some browsers
    if ( ! rcMeasurements.is_measurement_being_deleted(m) ) {  //this is both a deletion
        rcMeasurements.select_measurement(m);
    }
}

rcMeasurements.redraw_measurement = function (m) {
    var x1 = m.x1, y1 = m.y1;
    var x2 = m.x2, y2 = m.y2;
    
    var pixel_distatnce = Math.sqrt( Math.pow((x1 - x2), 2) + Math.pow((y1 - y2), 2));
    var xdiffrt = (x1-x2) / pixel_distatnce;
    var ydiffrt = (y1-y2) / pixel_distatnce;
    var mid_x = x1 + (x2 - x1)/2;
    var mid_y = y1 + (y2 - y1)/2;
    
    var half_font_gap = 12;
    var font_offset = false;
    var font_offset_x = 0;
    var font_offset_y = 0;
    
    if (pixel_distatnce < half_font_gap * 2 + 10) {
        font_offset = true;
        font_offset_x = -half_font_gap * ydiffrt;
        font_offset_y = half_font_gap * xdiffrt;
    }
    m.text.text(format_dist(m)).x(mid_x + font_offset_x).dy(mid_y + - m.text.node.attributes.y.value - font_offset_y*2); //hacky thing because move has a bug
    m.text_input_box.x(mid_x + font_offset_x).y(mid_y + font_offset_y*2); //do the same movement with the input box
    m.circle1.move(x1-3,y1-3)
    m.circle2.move(x2-3,y2-3)
    m.selector_circle1.move(x1-15,y1-15)
    m.selector_circle2.move(x2-15,y2-15)
    
    if (font_offset){
        m.shadow_line1.plot(x1 - 3 * xdiffrt, y1 - 3 * ydiffrt, x2 + 3 * xdiffrt, y2 + 3 * ydiffrt);
        m.shadow_line2.plot(x1 - 3 * xdiffrt, y1 - 3 * ydiffrt, x2 + 3 * xdiffrt, y2 + 3 * ydiffrt);
    }
    else {
        m.shadow_line1.plot(x1 - 3 * xdiffrt, y1 - 3 * ydiffrt, mid_x + half_font_gap * xdiffrt, mid_y + half_font_gap * ydiffrt);
        m.shadow_line2.plot(mid_x - half_font_gap * xdiffrt, mid_y - half_font_gap * ydiffrt, x2 + 3 * xdiffrt, y2 + 3 * ydiffrt);
    }
    
    if (font_offset){
        m.line1.plot(x1 - 1 * xdiffrt, y1 - 1 * ydiffrt, x2 + 1 * xdiffrt, y2 + 1 * ydiffrt)
        m.line2.plot(x1 - 1 * xdiffrt, y1 - 1 * ydiffrt, x2 + 1 * xdiffrt, y2 + 1 * ydiffrt)
    }
    else {
        m.line1.plot(x1 - 1 * xdiffrt, y1 - 1 * ydiffrt,  mid_x + (half_font_gap + 1) * xdiffrt, mid_y + (half_font_gap + 1) * ydiffrt)
        m.line2.plot(mid_x - (half_font_gap + 1) * xdiffrt, mid_y - (half_font_gap + 1) * ydiffrt, x2 + 1 * xdiffrt, y2 + 1 * ydiffrt)
    }
}

rcMeasurements.redraw_all_measurements = function (){
    for (var key in rcMeasurements.measurements) {
        rcMeasurements.redraw_measurement(measurements[key]);
    }
}



rcMeasurements.to_json = function () {
    var measurements_to_save = {};
    for (var key in rcMeasurements.measurements) {
        measurements_to_save[key] = rcMeasurements.measurements[key].saveable_copy();
    }
    return JSON.stringify(measurements_to_save);
}

rcMeasurements.save_measurements = function () {
    jsonStr = rcMeasurements.to_json();
    $.ajax({ type: "PUT", url: rc_server_location + "true_measure/m_photo/" + m_photo_guid + "/annotations/", contentType: "application/json", processData: false, dataType: "json", data: jsonStr })
    .done(function(data, textStatus, jqXHR) {
          alert(textStatus + ": " + JSON.stringify(data));
          })
    .fail(function(jqXHR, textStatus, errorThrown) {
          alert(textStatus + ": " + JSON.stringify(jqXHR));
          })
    ;
}


rcMeasurements.delete_measurement  = function (m) {
    window.setTimeout( function() {
          if (confirm('delete measurement?')) {
              //remove visual elemnts
              m.text.remove();
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

rcMeasurements.load_json  = function () {

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

rcMeasurements.move_measurement = function (m, nx1, ny1, nx2, ny2) {
    m.x1 = nx1;
    m.y1 = ny1;
    m.x2 = nx2;
    m.y2 = ny2;
    if (!m.overwriten){ m.distance = distanceBetween(m.x1, m.y1, m.x2, m.y2); } //distanceBetween is defineed in depth_data.js
    
    rcMeasurements.redraw_measurement(m);
}

// functions for modifying measurements

rcMeasurements.start_distance_change_dialouge = function (m) {
        rcMeasurements.select_measurement(m); //highlight measurement we're editing
        rcMeasurements.measurement_being_edited = m;
        //if (is_touch_device) { //use svg text element durring edit
        draw.node.appendChild(np_svg.node); //show number pad
        np_to_portrait(); //this is initializing style of the number pad

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
        draw.node.removeChild(np_svg.node); //hide number pad
        //}
        //else {
        //    measured_svg.node.removeChild( measurement_being_edited.text_input_box.node); //hide input box...
        //    measured_svg.node.appendChild( measurement_being_edited.text.node ); //show formated distance
        //}
    }
    rcMeasurements.measurement_being_edited = null; //so we know wether or not we have a sesion open.
    setTimeout( function () {rcMeasurements.save_measurements();}, 0)
}

rcMeasurements.reset = function () {
    rcMeasurements.measurements = {};
    rcMeasurements.measurement_being_edited = null;
}


rcMeasurements.add_character = function (key) {
    if (rcMeasurements.measurement_being_edited.text.text() == '?') { rcMeasurements.measurement_being_edited.text.text(key); }
    else {rcMeasurements.measurement_being_edited.text.text( rcMeasurements.measurement_being_edited.text.text() + key);}
}
rcMeasurements.del_character = function (key) {
    if (rcMeasurements.measurement_being_edited.text.text().length <= 1) { rcMeasurements.measurement_being_edited.text.text('?'); }
    else{
        rcMeasurements.measurement_being_edited.text.text( rcMeasurements.measurement_being_edited.text.text().substring(0, rcMeasurements.measurement_being_edited.text.text().length - 1) );
    }
}

rcMeasurements.isNumber = function  (n) {
    return !isNaN(parseFloat(n)) && isFinite(n);
}
rcMeasurements.finish_number_operation = function (){
    //we need to check the validity of the input. if not a valid number, then raise a warning to the user, and either cancel or return to eiditing, if valid, update measuremnt
    if (rcMeasurements.measurement_being_edited.text.text() == '?') {
        rcMeasurements.measurement_being_edited.distance = null;
        rcMeasurements.measurement_being_edited.text.text(format_dist(rcMeasurements.measurement_being_edited));
        rcMeasurements.end_measurement_edit();
    }
    else if ( rcMeasurements.isNumber( rcMeasurements.measurement_being_edited.text.text() ) ) {
        rcMeasurements.measurement_being_edited.distance = parseFloat(rcMeasurements.measurement_being_edited.text.text());
        rcMeasurements.measurement_being_edited.text.text(format_dist(rcMeasurements.measurement_being_edited));
        rcMeasurements.end_measurement_edit();
    }
    else {
        alert("invalid number, please correct before proceeding");
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

