//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.



rcMeasurements = {
    measurements : {},
    angles : {},
    notes : {},
    ranges : {},
    measurement_being_edited : null,
    active_note : null,
    inches_to_meter : 39.3701,
    cursor_animation_id : null,
    most_recent_drag : 0,
    font_family : 'HelveticaNeue-Light, Helvetica, Arial',
    prior_measurement_states : []
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                      //
//          LINEAR MEASUREMENTS                                                                                                         //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// instantiate a measurement and add it to the measurment list
// takes image locations
rcMeasurements.new_measurement = function (iX1, iY1, iX2, iY2, measured_svg){
    var m = {};
    m.annotation_type = 'measurement';
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
    return { distance:m.distance, overwriten:m.overwriten, x1:m.x1, y1:m.y1, x2:m.x2, y2:m.y2, guid:m.guid, units_metric:m.units_metric, annotation_type:m.annotation_type};
}

rcMeasurements.draw_measurement = function (m, measured_svg){

    
    //we need to write distance onto screen
    var d_string = rcMeasurements.format_dist(m);
    m.text_shadow = measured_svg.text(d_string);
    m.text_shadow.font({
                       family: rcMeasurements.font_family,
                        size: 25
                       , anchor: 'middle'
                       , leading: 1
                       }).stroke({ color: shadow_color, opacity: 1, width: 2.5 });
    
    m.text = measured_svg.text(d_string);
    m.text.font({
                family: rcMeasurements.font_family,
                 size: 25
                , anchor: 'middle'
                , leading: 1
                }).fill({ color: line_color, opacity: 1});
    // calculate how big of a gap we need for text, certain other layout paramaters
    var hlf_text_w = m.text_shadow.node.offsetWidth/2;
    var hlf_text_h = m.text_shadow.node.offsetHeight/2;
    
    m.pixel_distatnce = Math.sqrt( Math.pow((m.x1 - m.x2), 2) + Math.pow((m.y1 - m.y2), 2)); //how long the line is
    m.xdiffrt = (m.x1-m.x2) / m.pixel_distatnce; //ratio of x length to line length
    m.ydiffrt = (m.y1-m.y2) / m.pixel_distatnce; //ratio of y lenght to line length
    m.mid_x = m.x1 + (m.x2 - m.x1)/2;   //midpoint of line
    m.mid_y = m.y1 + (m.y2 - m.y1)/2;
    
    //console.log('hlf_text_w hlf_text_h  m.xdiffrt m.ydiffrt ' + hlf_text_w.toFixed() + ' ' + hlf_text_h.toFixed() + ' ' +  m.xdiffrt.toFixed(2) + ' ' +  m.ydiffrt.toFixed(2) );
    
    if (hlf_text_w/hlf_text_h < Math.abs(m.xdiffrt/m.ydiffrt)) { //line is more horizontal, text width dominates gap
        m.half_font_gap = Math.sqrt(hlf_text_w*hlf_text_w + hlf_text_w*hlf_text_w*m.ydiffrt*m.ydiffrt/m.xdiffrt/m.xdiffrt)+5;
    }
    else { //line is more vertical, we want to use a smaller gap dependent on angle
        m.half_font_gap = Math.sqrt(hlf_text_h*hlf_text_h + hlf_text_h*hlf_text_h*m.xdiffrt*m.xdiffrt/m.ydiffrt/m.ydiffrt );
    }
    
    m.font_offset = false;
    m.font_offset_x = 0;
    m.font_offset_y = - hlf_text_h;
    
    if (m.pixel_distatnce < m.half_font_gap * 2 + 10) {
        m.font_offset = true;
        m.font_offset_x = -m.half_font_gap * m.ydiffrt;
        m.font_offset_y = m.half_font_gap * m.xdiffrt;
    }
    
    //This allows a measurement to be saved. it is necessary to call this as part of draw, because
	//a measurement will not have this function when it is deserialized.
    m.saveable_copy = rcMeasurements.saveable_liniar_measurement(m)
    

    //move text to correct location
    m.text_shadow.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    //create hiden text box for entry on devices with keyboards
    m.text_input_box = measured_svg.foreignObject(m.half_font_gap * 2, m.half_font_gap * 2).move(m.mid_x - m.half_font_gap, m.mid_y - m.half_font_gap );

    
    var foo = m.text.lines.members[0];
    //console.log( foo.width().toFixed() );
    
    //text box cursor approach //m.text_cursor = measured_svg.plain("").move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text_cursor = measured_svg.line(0,0,0,24);
    m.place_cursor = function() {
            var u_offset;
            if (m.units_metric) {u_offset = 23}
            else {u_offset = 12}
            var c_offset = m.text.node.offsetWidth/2 - u_offset;
            m.text_cursor.move(m.mid_x + m.font_offset_x + c_offset, m.mid_y + m.font_offset_y);
    }
    m.place_cursor();
    m.text_cursor.stroke({ color: shadow_color, opacity: 0, width: 2 });

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
    m.shadow_line1.click (function (e) { rcMeasurements.click_action(m,e);});
    m.shadow_line2.click (function (e) { rcMeasurements.click_action(m, e);});
    m.line1.click (function (e) { rcMeasurements.click_action(m, e);});
    m.line2.click (function (e) { rcMeasurements.click_action(m, e);});
    m.circle1.click (function (e) { rcMeasurements.click_action(m, e);});
    m.circle2.click (function (e) { rcMeasurements.click_action(m, e);});
    m.selector_circle1.click(function (e) { rcMeasurements.click_action(m, e);});
    m.selector_circle2.click(function (e) { rcMeasurements.click_action(m, e);});

    //non - editable distance click action
    m.text.click(function (e) { rcMeasurements.click_action(m, e);});
    
// this has been commented out becasue we're not allowing distance editing for MVP release
//    //text editing
//    m.text.click(function (e) {
//                     setTimeout(function(){ return false;},1);
//                     rcMeasurements.start_distance_change_dialouge(m);
//                     e.stopPropagation(); e.preventDefault();
//                 })
    
    //draging measurement end points.
    Hammer(m.circle1.node).on("drag", function(e) {
                                  i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                  rcMeasurements.move_measurement(m, i.x, i.y, m.x2, m.y2);
                                  e.stopPropagation(); e.preventDefault();
                              }).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e); });
    Hammer(m.circle2.node).on("drag",  function(e) { i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, m.x1, m.y1, i.x, i.y); e.stopPropagation(); e.preventDefault();}).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e); });
    Hammer(m.selector_circle1.node).on("drag",  function(e) {
                                       e.stopPropagation(); e.preventDefault();
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, i.x, i.y, m.x2, m.y2);
                                       }).on("dragend", function(e) { rcMeasurements.dragEndHandler(m,e); });
    Hammer(m.selector_circle2.node).on("drag", function(e) {i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, m.x1, m.y1, i.x, i.y); e.stopPropagation(); e.preventDefault(); }).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e);});
    
    rcMeasurements.measurements[m.guid] = m;

    //define delete funciton
    m.delete_self = function () {rcMeasurements.delete_measurement(m);};
    
}

rcMeasurements.dragEndHandler = function (m, e) {
    //console.log('rcMeasurements.dragEndHandler()');
    e.stopPropagation(); e.preventDefault();
    rcMeasurements.most_recent_drag = new Date();
    rcMeasurements.deselect_measurement(m);
    rcMeasurements.save_measurements();
}


rcMeasurements.redraw_measurement = function (m) {
    
    m.text_shadow.text(rcMeasurements.format_dist(m));
    m.text.text(rcMeasurements.format_dist(m));
    
    // calculate how big of a gap we need for text, certain other layout paramaters
    var hlf_text_w = m.text_shadow.node.offsetWidth/2;
    var hlf_text_h = m.text_shadow.node.offsetHeight/2;
    
    m.pixel_distatnce = Math.sqrt( Math.pow((m.x1 - m.x2), 2) + Math.pow((m.y1 - m.y2), 2)); //how long the line is
    m.xdiffrt = (m.x1-m.x2) / m.pixel_distatnce; //ratio of x length to line length
    m.ydiffrt = (m.y1-m.y2) / m.pixel_distatnce; //ratio of y lenght to line length
    m.mid_x = m.x1 + (m.x2 - m.x1)/2;   //midpoint of line
    m.mid_y = m.y1 + (m.y2 - m.y1)/2;
    
    //console.log('hlf_text_w hlf_text_h  m.xdiffrt m.ydiffrt ' + hlf_text_w.toFixed() + ' ' + hlf_text_h.toFixed() + ' ' +  m.xdiffrt.toFixed(2) + ' ' +  m.ydiffrt.toFixed(2) );
    
    if (hlf_text_w/hlf_text_h < Math.abs(m.xdiffrt/m.ydiffrt)) { //line is more horizontal, text width dominates gap
        m.half_font_gap = Math.sqrt(hlf_text_w*hlf_text_w + hlf_text_w*hlf_text_w*m.ydiffrt*m.ydiffrt/m.xdiffrt/m.xdiffrt)+5;
    }
    else { //line is more vertical, we want to use a smaller gap dependent on angle
        m.half_font_gap = Math.sqrt(hlf_text_h*hlf_text_h + hlf_text_h*hlf_text_h*m.xdiffrt*m.xdiffrt/m.ydiffrt/m.ydiffrt );
    }
    
    m.font_offset = false;
    m.font_offset_x = 0;
    m.font_offset_y = - hlf_text_h;
    
    if (m.pixel_distatnce < m.half_font_gap * 2 + 10) {
        m.font_offset = true;
        m.font_offset_x = -m.half_font_gap * m.ydiffrt;
        m.font_offset_y = m.half_font_gap * m.xdiffrt;
    }
    
    m.text_shadow.x(m.mid_x + m.font_offset_x).dy(m.mid_y + - m.text.node.attributes.y.value + m.font_offset_y);
    m.text.x(m.mid_x + m.font_offset_x).dy(m.mid_y + - m.text.node.attributes.y.value + m.font_offset_y); //hacky thing because move has a bug
    
    m.text_input_box.x(m.mid_x + m.font_offset_x).y(m.mid_y + m.font_offset_y*2); //do the same movement with the input box
    m.place_cursor();
    
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

rcMeasurements.redraw_lines = function(m) {
    // calculate how big of a gap we need for text, certain other layout paramaters
    var hlf_text_w = m.text_shadow.node.offsetWidth/2;
    var hlf_text_h = m.text_shadow.node.offsetHeight/2;
    
    if (hlf_text_w/hlf_text_h < Math.abs(m.xdiffrt/m.ydiffrt)) { //line is more horizontal, text width dominates gap
        m.half_font_gap = Math.sqrt(hlf_text_w*hlf_text_w + hlf_text_w*hlf_text_w*m.ydiffrt*m.ydiffrt/m.xdiffrt/m.xdiffrt)+5;
    }
    else { //line is more vertical, we want to use a smaller gap dependent on angle
        m.half_font_gap = Math.sqrt(hlf_text_h*hlf_text_h + hlf_text_h*hlf_text_h*m.xdiffrt*m.xdiffrt/m.ydiffrt/m.ydiffrt );
    }
    
    if (m.pixel_distatnce < m.half_font_gap * 2 + 10) {
        m.font_offset = true;
        m.font_offset_x = -m.half_font_gap * m.ydiffrt;
        m.font_offset_y = m.half_font_gap * m.xdiffrt;
    }
    
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                      //
//          SAVING, UNDOING, DATA MANAGEMENT                                                                                            //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



rcMeasurements.redraw_all_measurements = function (){
    for (var key in rcMeasurements.measurements) {
        rcMeasurements.redraw_measurement(rcMeasurements.measurements[key]);
    }
}



rcMeasurements.to_json = function () {
    //console.log('rcMeasurements.to_json');
    var annotations_to_save = {measurements : {}, notes : {}, angles : {} };
    // save the default units.
    annotations_to_save['use_metric'] = default_units_metric;
    for (var key in rcMeasurements.measurements) {
        annotations_to_save.measurements[key] = rcMeasurements.measurements[key].saveable_copy;
    }
    for (var key in rcMeasurements.angles) {
        annotations_to_save.angles[key] = rcMeasurements.angles[key].saveable_copy;
    }
    for (var key in rcMeasurements.notes) {
        annotations_to_save.notes[key] = rcMeasurements.notes[key].saveable_copy;
    }
    return JSON.stringify(annotations_to_save);
}

rcMeasurements.save_measurements = function (optional_not_undoable_flag) {
    jsonStr = rcMeasurements.to_json();
    if (!optional_not_undoable_flag) {rcMeasurements.prior_measurement_states.push(jsonStr);}
    //console.log('prior_measurement_states length = ' + rcMeasurements.prior_measurement_states.length.toFixed());
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
}

rcMeasurements.load_json  = function (m_url, callback_function) {
    //get measurements
    setTimeout(function(){
        $.ajaxSetup({ cache: false });
        $.getJSON(m_url, function(data) {
            rcMeasurements.apply_json_data(data);
            //initialize prior measurments
            rcMeasurements.prior_measurement_states.push(rcMeasurements.to_json());
            //console.log('prior_measurement_states length = ' + rcMeasurements.prior_measurement_states.length.toFixed());
            callback_function();
        }).error(function () {
            //window.setTimeout(function(){alert('failed to load annotations')},0);
            rcMeasurements.measurements = {};
            callback_function();
        });
    }, 0);
}

rcMeasurements.apply_json_data = function (data) {
    if ('use_metric' in data) { default_units_metric = data['use_metric']; } //use saved units preference over units specified externaly, ie: iOS app default units
    if ('measurements' in data) { rcMeasurements.measurements = data.measurements; } else {rcMeasurements.measurements = {};}
    if ('angles' in data) { rcMeasurements.angles = data.angles; } else {rcMeasurements.angles = {};}
    if ('notes' in data) { rcMeasurements.notes = data.notes; } else {rcMeasurements.notes = {};}
    //for each measurement, draw measurement
    for (var key in rcMeasurements.measurements) {
        try {rcMeasurements.draw_measurement(rcMeasurements.measurements[key], measured_svg);}
        catch(err) {console.log(JSON.stringify(err)); delete rcMeasurements.measurements[key];}
    }
    for (var key in rcMeasurements.agles) {
        try {rcMeasurements.draw_agles(rcMeasurements.agles[key], measured_svg);}
        catch(err) {console.log(JSON.stringify(err)); delete rcMeasurements.agles[key];}
    }
    for (var key in rcMeasurements.notes) {
        try {rcMeasurements.draw_note(rcMeasurements.notes[key], measured_svg);}
        catch(err) {console.log(JSON.stringify(err)); delete rcMeasurements.notes[key];}
    }
    rcMeasurements.reset_all_measurement_units_to_default();
}

rcMeasurements.is_undo_available = function () {
    if (rcMeasurements.prior_measurement_states.length > 1) { return true;}
    else {return false;}
}



// this clears all measurements, used when we are switching between measured photos but not reloading the app
rcMeasurements.reset = function () {
    rcMeasurements.measurements = {};
    rcMeasurements.angles = {};
    rcMeasurements.notes = {};
    rcMeasurements.prior_measurement_states = [];
    rcMeasurements.measurement_being_edited = null;
    rcMeasurements.active_note = null;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                      //
//          EDITING AND DYNAMIC PRESENTATION OF LINEAR MEAUREMENTS                                                                      //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rcMeasurements.reset_all_measurement_units_to_default = function(){
    for (var key in rcMeasurements.measurements) {
        rcMeasurements.measurements[key].units_metric = default_units_metric; //overwrite the measurements unit information w/ the current global default.
    }
    rcMeasurements.redraw_all_measurements();
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

rcMeasurements.setText = function (m, str) {
    m.text_shadow.text(str);
    m.text.text(str);
}


rcMeasurements.end_measurement_edit = function (){
    if (rcMeasurements.measurement_being_edited) {
        //if (is_touch_device) {
        rcMeasurements.deselect_measurement(rcMeasurements.measurement_being_edited); //un-highlight measurement we're editing
        if(draw.node.contains(np_svg.node)) {draw.node.removeChild(np_svg.node);} //hide number pad
        return_image_after_number_pad();
        rcMeasurements.stop_cursor_animation(rcMeasurements.measurement_being_edited);
        setTimeout( function () {rcMeasurements.save_measurements();}, 0)

        //}
        //else {
        //    measured_svg.node.removeChild( measurement_being_edited.text_input_box.node); //hide input box...
        //    measured_svg.node.appendChild( measurement_being_edited.text.node ); //show formated distance
        //}
    }
    rcMeasurements.measurement_being_edited = null; //so we know wether or not we have a sesion open.
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
    rcMeasurements.measurement_being_edited.place_cursor();
}


// called on distance before drawn to screen
rcMeasurements.format_dist = function (m){
    
    if (m.distance) {
        if (m.units_metric) {
            if ( m.distance >= 0.995) {
                return m.distance.toFixed(2)+' m';
            }
            else {
                return (100 * m.distance).toFixed(0)+' cm';
            }
        }
        else {
            if ((m.distance * rcMeasurements.inches_to_meter) >= 23.95) {
                var inches = m.distance * rcMeasurements.inches_to_meter;
                var feet = Math.floor( (inches + 0.5) / 12);
                var remaining_inches = Math.max(0,(inches - feet * 12));
                return feet.toFixed(0) + " ' " + remaining_inches.toFixed(0) + ' "';
            }
            else {
                return (m.distance * rcMeasurements.inches_to_meter).toFixed(1) + ' "';
            }
        }
    }
    else {
        if (m.units_metric) { return '? m'; }
        else { return '? "'; }
    }
    
}


rcMeasurements.isNumber = function  (n) {
    return !isNaN(parseFloat(n)) && isFinite(n);
}

