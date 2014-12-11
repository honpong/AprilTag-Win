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

rcMeasurements.click_action = function (m,e) {
    //console.log("click_action(m,e)");
    //if we just ended a drag, don't propagate click
    var now = new Date();
    var last_drag_time_diff = now - rcMeasurements.most_recent_drag;
    if (last_drag_time_diff < 100){
        e.stopPropagation(); e.preventDefault();
        return;
    }
    
    setTimeout(function(){ return false;},1);  //this just forces refresh for some browsers
    if ( rcMeasurements.is_annotation_being_deleted(m) ) {  //this is both a deletion and a check for deletion
        e.stopPropagation(); e.preventDefault(); // if deleted, dont do anything else
    }
    //otherwise allow anotations gesture to propegate to image to see if it has an effect on anotations.
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
//          TEXT ANNOTATIONS                                                                                                            //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



rcMeasurements.new_note = function (iX, iY, svg_target) {
    rcMeasurements.endNoteEdit(); //stop any active note eddits, as this note will become active
    var n = {};
    n.annotation_type = 'note';
    n.x = iX;
    n.y = iY;
    n.guid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {var r = Math.random()*16|0,v=c=='x'?r:r&0x3|0x8;return v.toString(16);});
    n.text = ' ' //we need to have at least one character or the svg text element creation will fail.
    
    rcMeasurements.draw_note(n, svg_target);
    rcMeasurements.active_note = n;
    rcMeasurements.start_cursor_animation(n);
}

rcMeasurements.saveable_note = function (n) {
    return {  x:n.x, y:n.y, guid:n.guid, text:n.text, annotation_type:n.annotation_type };
}

rcMeasurements.delete_note = function (n) {
    //remove visual elemnts
    if (n.svg_target.node.contains(n.text_display.node)) { n.text_display.remove();}
    if (n.svg_target.node.contains(n.text_shadow.node)) { n.text_shadow.remove();}
    if (n.svg_target.node.contains(n.text_cursor.node)) { n.text_cursor.remove();}
    //removed from measurements array
    delete rcMeasurements.notes[n.guid];
    delete n;
}


rcMeasurements.deleteCharacterFromNote = function (){
    if (rcMeasurements.active_note) {
        if(rcMeasurements.active_note.text != ' ') {
            rcMeasurements.active_note.text = rcMeasurements.active_note.text.slice(0, -1);
            if(rcMeasurements.active_note.text == '') {rcMeasurements.active_note.text = ' ';} //we need at least one character for svg to not fail
            rcMeasurements.redraw_note(rcMeasurements.active_note);
        }
    }
}

rcMeasurements.endNoteEdit = function (){
    if (rcMeasurements.active_note) {
        rcMeasurements.redraw_note(rcMeasurements.active_note);
        rcMeasurements.stop_cursor_animation(rcMeasurements.active_note);
        //if no content remove
        if ((rcMeasurements.active_note.text.length == 0) || (rcMeasurements.active_note.text == " ") || !rcMeasurements.active_note.text.trim()) {
            rcMeasurements.active_note.delete_self();
        }
        rcMeasurements.active_note = null;
        rcMeasurements.save_measurements();
    }
}


rcMeasurements.addCharacterToNote = function (char){
    if (rcMeasurements.active_note) {
        if(rcMeasurements.active_note.text == ' ') {rcMeasurements.active_note.text = '';}
        rcMeasurements.active_note.text = rcMeasurements.active_note.text + char;
        if(rcMeasurements.active_note.text.length == 0) {rcMeasurements.active_note.text = " ";} //if we failed to add a character we still need at least one character
        rcMeasurements.redraw_note(rcMeasurements.active_note);
    }
}

//this completely replaces the text annotation w/ the string and ends edit operation.
rcMeasurements.updateNote = function (str) {
    if (rcMeasurements.active_note) {
        if(str.length == 0 ) {str = ' ';} //if we failed to add a character we still need at least one character
        rcMeasurements.active_note.text = str;
        rcMeasurements.redraw_note(rcMeasurements.active_note);
    }
    rcMeasurements.endNoteEdit();
}


rcMeasurements.redraw_note = function (n) {

    if ((n.text.length == 0) || !n.text.trim()) {n.text = ' ';} //'' is invalid text for svg library
    n.saveable_copy = rcMeasurements.saveable_note(n);

    n.text_display.text(n.text).move(n.x, n.y);
    n.text_shadow.text(n.text).move(n.x, n.y);

    if ((n.text != ' ') && ( ! n.svg_target.node.contains(n.text_display.node))) {
        n.svg_target.node.appendChild(n.text_shadow.node);
        n.svg_target.node.appendChild(n.text_display.node); // shadow must be appened first so that the forground is shown on top.
        n.text_display.move(n.x, n.y);
        n.text_shadow.move(n.x, n.y); //we loose position information after detachement, so the must be placed again.
    }
    else if ((n.text == ' ') && (n.svg_target.node.contains(n.text_display.node))) {
        n.svg_target.node.removeChild(n.text_display.node);
        n.svg_target.node.removeChild(n.text_shadow.node);
    }

    n.place_cursor();
    
}

rcMeasurements.draw_note = function (n, svg_target) {
    //This allows a note to be saved. it is necessary to call this as part of draw, because
	//a note will not have this function when it is deserialized.
    n.saveable_copy = rcMeasurements.saveable_note(n);
    
    n.svg_target = svg_target;
    
    n.text_shadow = svg_target.text(n.text).move(n.x, n.y);
    n.text_shadow.font({
                       family: rcMeasurements.font_family,
                       size: 25
                       , anchor: 'middle'
                       , leading: 1
                       }).stroke({ color: shadow_color, opacity: 1, width: 2.5 });
    
    n.text_display = svg_target.text(n.text).move(n.x, n.y);
    n.text_display.font({
                family: rcMeasurements.font_family,
                size: 25
                , anchor: 'middle'
                , leading: 1
                }).fill({ color: line_color, opacity: 1});

    if (n.text == ' ') {
        n.svg_target.node.removeChild(n.text_display.node);
        n.svg_target.node.removeChild(n.text_shadow.node);
    }
    
    n.text_cursor = svg_target.line(0,0,0,24);
    n.place_cursor = function (){
        var c_offset = n.text_display.node.offsetWidth/2 + 4;
        n.text_cursor.move(n.x + c_offset, n.y + 4);
    };
    n.place_cursor();
    n.text_cursor.stroke({ color: shadow_color, opacity: 0, width: 2 });


    
    
    n.text_display.click (function (e) {
                            //this line does the deletion as well as checking for if it occured. - this checks the eraser button.
                            if ( rcMeasurements.is_annotation_being_deleted(n) ) {
                                  e.stopPropagation(); e.preventDefault();
                            }
                            else if (rc_menu.current_button == rc_menu.note_button) {
                                  rcMeasurements.endNoteEdit(); //end any active edit
                                  rcMeasurements.active_note = n; //set this note as the active note
                                  rcMeasurements.start_cursor_animation(n); //start the cursor on this note.
                                  e.stopPropagation(); e.preventDefault();
                            }
                            //do nothing and pass the event on if neither the text nor erase button is chosen
    });
    
    //draging text box
    Hammer(n.text_display.node).on("drag", function(e) {
                                     e.stopPropagation(); e.preventDefault();
                                     }).on("dragend", function(e) {
                                           e.stopPropagation(); e.preventDefault();
                                           });
    
    rcMeasurements.notes[n.guid] = n;
    
    
    //define delete funciton
    n.delete_self = function () { rcMeasurements.delete_note(n); };

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                      //
//          RANGE ANNOTATIONS                                                                                                            //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



rcMeasurements.new_range = function (iX, iY, svg_target) {
    var r = {};
    r.annotation_type = 'range';
    r.x = iX;
    r.y = iY;
    r.guid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {var r = Math.random()*16|0,v=c=='x'?r:r&0x3|0x8;return v.toString(16);});
    r.range = distanceTo(r.x, r.y); //distanceTo is defined in depth_data.js
    
    rcMeasurements.draw_range(r, svg_target);
}

rcMeasurements.saveable_range = function (n) {
    return {  x:n.x, y:n.y, guid:n.guid, annotation_type:n.annotation_type };
}

rcMeasurements.delete_range = function (n) {
    //remove visual elemnts
    if (n.svg_target.node.contains(n.text_display.node)) { n.text_display.remove();}
    if (n.svg_target.node.contains(n.text_shadow.node)) { n.text_shadow.remove();}
    if (n.svg_target.node.contains(n.text_cursor.node)) { n.text_cursor.remove();}
    //removed from measurements array
    delete rcMeasurements.notes[n.guid];
    delete n;
}


rcMeasurements.redraw_range = function (n) {
    
    if ((n.text.length == 0) || !n.text.trim()) {n.text = ' ';} //'' is invalid text for svg library
    n.saveable_copy = rcMeasurements.saveable_note(n);
    
    n.text_display.text(n.range.toFixed(1)).move(n.x, n.y);
    n.text_shadow.text(n.range.toFixed(1)).move(n.x, n.y);
    
    if ((n.text != ' ') && ( ! n.svg_target.node.contains(n.text_display.node))) {
        n.svg_target.node.appendChild(n.text_shadow.node);
        n.svg_target.node.appendChild(n.text_display.node); // shadow must be appened first so that the forground is shown on top.
        n.text_display.move(n.x, n.y);
        n.text_shadow.move(n.x, n.y); //we loose position information after detachement, so the must be placed again.
    }
    else if ((n.text == ' ') && (n.svg_target.node.contains(n.text_display.node))) {
        n.svg_target.node.removeChild(n.text_display.node);
        n.svg_target.node.removeChild(n.text_shadow.node);
    }
    
    n.place_cursor();
    
}

rcMeasurements.draw_range = function (n, svg_target) {
    //This allows a note to be saved. it is necessary to call this as part of draw, because
	//a note will not have this function when it is deserialized.
    n.saveable_copy = rcMeasurements.saveable_range(n);
    
    n.svg_target = svg_target;
    
    n.marker = svg_target.circle(4).move( n.x-2, n.y-2).stroke({ color: line_color, opacity: 1, width : 2 }).fill({opacity:0});
    n.text_shadow = svg_target.text(n.range.toFixed(1)).move(n.x, n.y);
    n.text_shadow.font({
                       family: rcMeasurements.font_family,
                       size: 25
                       , anchor: 'middle'
                       , leading: 1
                       }).stroke({ color: shadow_color, opacity: 1, width: 2.5 });
    
    n.text_display = svg_target.text(n.range.toFixed(1)).move(n.x, n.y);
    n.text_display.font({
                        family: rcMeasurements.font_family,
                        size: 25
                        , anchor: 'middle'
                        , leading: 1
                        }).fill({ color: line_color, opacity: 1});
    
    
    n.marker.click (function (e) {
                          //this line does the deletion as well as checking for if it occured. - this checks the eraser button.
                          if ( rcMeasurements.is_annotation_being_deleted(n) ) {
                            e.stopPropagation(); e.preventDefault();
                          }
                          //do nothing and pass the event on if erase button is not selected
                          });
    
    //draging
    rcMeasurements.ranges[n.guid] = n;
    
    
    //define delete funciton
    n.delete_self = function () { rcMeasurements.delete_range(n); };
    
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
            rcMeasurements.prior_measurement_states.push("{}");
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
    rc_menu.unit_button.highlight_active_unit(default_units_metric);
    rcMeasurements.reset_all_measurement_units_to_default();
}

rcMeasurements.is_undo_available = function () {
    if (rcMeasurements.prior_measurement_states.length > 1) { return true;}
    else {return false;}
}

//this performs the undo
rcMeasurements.revert_measurement_state = function () {
    if (rcMeasurements.prior_measurement_states.length > 1) {
        //delete all measurements
        for (var key in rcMeasurements.measurements) {
            rcMeasurements.measurements[key].delete_self();
        }
        for (var key in rcMeasurements.agles) {
            rcMeasurements.delete_angle(rcMeasurements.angles[key]);
        }
        for (var key in rcMeasurements.notes) {
            rcMeasurements.notes[key].delete_self();
        }
        //pull prior measurement state
        rcMeasurements.prior_measurement_states.pop(); //trow away the current state
        //console.log('prior_measurement_states length = ' + rcMeasurements.prior_measurement_states.length.toFixed());
        var prior_m_json = JSON.parse(rcMeasurements.prior_measurement_states.pop()); //go to the one before.
        //console.log('prior_measurement_states length = ' + rcMeasurements.prior_measurement_states.length.toFixed());

        //we over write the units here before saving so that the units are not changed by undoing.
        //in future any ui state that is persisted but not undoable can be over writen here
        prior_m_json['use_metric'] = default_units_metric;

        rcMeasurements.apply_json_data(prior_m_json);
        // save new state - this will add current state back to the stack
        rcMeasurements.save_measurements();
    }
}

// this checks for wether or not the rc_menu button designated as the eraser button is selected.
rcMeasurements.is_annotation_being_deleted = function (m) {
    //console.log('rcMeasurements.is_annotation_being_deleted(m)')
    if (rc_menu.current_button == rc_menu.eraser_button) {
        m.delete_self();
        setTimeout( function () {
                                    rcMeasurements.save_measurements();
                                    rc_menu.enable_disenable_undo(rcMeasurements.is_undo_available());
                                 }, 0)
        return true;
    }
    return false;
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
//          CURSOR DRAWING AND ANIMAITON                                                                                                //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



rcMeasurements.start_cursor_animation = function (m) {
    if (rcMeasurements.cursor_animation_id){ window.clearTimeout(rcMeasurements.cursor_animation_id) };
    rcMeasurements.cursor_animation_id = window.setTimeout(function(){rcMeasurements.show_cursor_frame(m)}, 10);
}

rcMeasurements.show_cursor_frame = function (m) {
    m.place_cursor();
    m.text_cursor.stroke({ color: shadow_color, opacity: .9, width: 2 });
    rcMeasurements.cursor_animation_id = window.setTimeout(function(){rcMeasurements.hide_cursor_frame(m)},700);
}

rcMeasurements.hide_cursor_frame = function (m) {
    m.text_cursor.stroke({ color: shadow_color, opacity: 0, width: 2 });
    rcMeasurements.cursor_animation_id = window.setTimeout(function(){rcMeasurements.show_cursor_frame(m)},700);
}

rcMeasurements.stop_cursor_animation = function (m) {
    if (rcMeasurements.cursor_animation_id){ window.clearTimeout(rcMeasurements.cursor_animation_id) };
    rcMeasurements.cursor_animation_id = null
    m.text_cursor.stroke({ color: shadow_color, opacity: 0, width: 2 });
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


rcMeasurements.add_character = function (key) {
    var str      = rcMeasurements.measurement_being_edited.text.text();
    var unit_str = str.substring(str.length - 2);
    str = str.substring(0, str.length - 2);

    if (str == '?') { rcMeasurements.setText(rcMeasurements.measurement_being_edited, key + unit_str); }
    else {rcMeasurements.setText(rcMeasurements.measurement_being_edited,  str + key + unit_str);}
    rcMeasurements.measurement_being_edited.place_cursor();
    rcMeasurements.redraw_lines(rcMeasurements.measurement_being_edited);

}
rcMeasurements.del_character = function (key) {
    var str      = rcMeasurements.measurement_being_edited.text.text();
    var unit_str = str.substring(str.length - 2);
    str = str.substring(0, str.length - 2);
    
    if (str.length <= 1) { rcMeasurements.setText(rcMeasurements.measurement_being_edited, '?' + unit_str); }
    else{
        rcMeasurements.setText( rcMeasurements.measurement_being_edited,  str.substring(0, str.length - 1) + unit_str );
    }
    rcMeasurements.measurement_being_edited.place_cursor();
    rcMeasurements.redraw_lines(rcMeasurements.measurement_being_edited);
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
        rcMeasurements.redraw_lines(rcMeasurements.measurement_being_edited);
        rcMeasurements.measurement_being_edited.saveable_copy = rcMeasurements.saveable_liniar_measurement(rcMeasurements.measurement_being_edited);
        rcMeasurements.end_measurement_edit();
    }
}

