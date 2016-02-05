//  Copyright (c) 2014 Sunlayar. All rights reserved.



rcMeasurements = {
    inches_to_meter : 39.3701,
    most_recent_drag : 0,
    font_family : 'HelveticaNeue-Light, Helvetica, Arial',
    roof_measurement : null
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                      //
//          Roof Object                                                                                                //
//                                                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// instantiate a measurement and add it to the measurment list
// takes image locations
rcMeasurements.new_measurement = function (iX1, iY1, iX2, iY2, iX3, iY3, iX4, iY4, measured_svg){
    var m = {};
    m.distance = distanceBetween(iX3, iY3, iX4, iY4); //distanceBetween is defined in depth_data.js
    m.x1 = iX1;
    m.y1 = iY1;
    m.x2 = iX2;
    m.y2 = iY2;
    m.x3 = iX3;
    m.y3 = iY3;
    m.x4 = iX4;
    m.y4 = iY4;
    m.units_metric = default_units_metric;
    m.overwriten = false;
    m.saveable_copy = rcMeasurements.saveable_liniar_measurement(m)
    
    rcMeasurements.draw_measurement(m, measured_svg);
    rcMeasurements.roof_measurement = m;
}

rcMeasurements.roof_object_valid = function () {
    return rcMeasurements.roof_measurement.isValid;
}


rcMeasurements.slope_angle = function (m) {
    var v1 = m.coords3D[1], v2 = m.coords3D[2], v3 = m.coords3D[3];
    
    var v12=[0,0,0], v13=[0,0,0], norm_vec = [0,0,0];
    dm_sub(v12, v2, v1);      //calculate cross product of edge vectors to get normal vector.
    dm_sub(v13, v3, v1);      // dm_sub and dm_cross are defined in depth_data.js
    dm_cross(norm_vec, v12, v13);
    norm_vec_length = Math.sqrt( norm_vec[0]*norm_vec[0] + norm_vec[1]*norm_vec[1] + norm_vec[2]*norm_vec[2] );
    norm_vec[0] = norm_vec[0]/norm_vec_length;
    norm_vec[1] = norm_vec[1]/norm_vec_length;
    norm_vec[2] = norm_vec[2]/norm_vec_length;
    
    return Math.acos(dm_dot(norm_vec, spatial_data.gravity)) * 180 / Math.PI; //take the inverse cosine of the z normal to determine slope
}

rcMeasurements.saveable_liniar_measurement = function (m) {
    m.coords3D = [null,null,null,null]; //we need this for the AR engine
    m.isValid = true;
    var gutter_adj_ratio = 1;
    if (m.overwriten){
        gutter_adj_ratio = m.distance / distanceBetween(m.x3, m.y3, m.x4, m.y4);
    }
    try{m.coords3D[0] = dm_3d_location_from_pixel_location(m.x1,m.y1)} catch(err) {m.isValid = false; m.coords3D[0] = null;}
    try{m.coords3D[1] = dm_3d_location_from_pixel_location(m.x2,m.y2)} catch(err) {m.isValid = false; m.coords3D[1] = null;}
    try{m.coords3D[2] = dm_3d_location_from_pixel_location(m.x3,m.y3)} catch(err) {m.isValid = false; m.coords3D[2] = null;}
    try{m.coords3D[3] = dm_3d_location_from_pixel_location(m.x4,m.y4)} catch(err) {m.isValid = false; m.coords3D[3] = null;}
    if (!m.coords3D[0] || !m.coords3D[1] || !m.coords3D[2] || !m.coords3D[3]) {m.isValid = false} //check for null results. 
    if (m.isValid){
        m.slope_angle = rcMeasurements.slope_angle(m);
        
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 3; j++) {
                m.coords3D[i][j] = m.coords3D[i][j] * gutter_adj_ratio;
            }
        }
    }
    else {m.slope_angle = null;}
    
    
    //we only want a subset of the measurements content, so we create a temp object we write the content we want to keep into
    return { gutter_length:m.distance, gutter_length_overwriten:m.overwriten, x1:m.x1, y1:m.y1, x2:m.x2, y2:m.y2, x3:m.x3, y3:m.y3, x4:m.x4, y4:m.y4, coords3D: m.coords3D};
}


rcMeasurements.draw_measurement = function (m, measured_svg){
    //gutter distance is from point 3 to point 4
    
    //we need to write gutter distance onto screen
    var d_string = rcMeasurements.format_dist(m);
    var s_string; //slope
    if (m.slope_angle) {s_string = m.slope_angle.toFixed() + "\u00B0";} else {s_string = "no slope";}
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

    m.slope_shadow = measured_svg.text(s_string);
    m.slope_shadow.font({
                       family: rcMeasurements.font_family,
                       size: 25
                       , anchor: 'middle'
                       , leading: 1
                       }).stroke({ color: shadow_color, opacity: 1, width: 2.5 });
    
    m.slope_text = measured_svg.text(s_string);
    m.slope_text.font({
                family: rcMeasurements.font_family,
                size: 25
                , anchor: 'middle'
                , leading: 1
                }).fill({ color: line_color, opacity: 1});

    
    
    m.font_offset_x = 0;
    m.font_offset_y = 10;
    m.mid_x = m.x3 + (m.x4 - m.x3)/2;   //midpoint of gutter line
    m.mid_y = m.y3 + (m.y4 - m.y3)/2;

    
    m.circle1 = measured_svg.circle(10).move(m.x1-5,m.y1-3).stroke({ color: shadow_color, width : 1.5 }).fill({ color: valid_color , opacity:1});
    m.circle2 = measured_svg.circle(10).move(m.x2-5,m.y2-3).stroke({ color: shadow_color, width : 1.5 }).fill({ color: valid_color , opacity:1 });
    m.circle3 = measured_svg.circle(10).move(m.x3-5,m.y3-3).stroke({ color: shadow_color, width : 1.5 }).fill({ color: valid_color , opacity:1});
    m.circle4 = measured_svg.circle(10).move(m.x4-5,m.y4-3).stroke({ color: shadow_color, width : 1.5 }).fill({ color: valid_color , opacity:1 });
    m.fp_x_off = 85;
    m.fp_y_off = 105;
    m.fingerprint1 = measured_svg.image("fp_left.png");
    m.fingerprint2 = measured_svg.image("fp_right.png");
    m.fingerprint3 = measured_svg.image("fp_r_btm.png");
    m.fingerprint4 = measured_svg.image("fp_l_btm.png");
    if (window.innerWidth > 700) { //make the fingerprint buttons smaller on bigger screens
        m.fp_x_off = 42.5;
        m.fp_y_off = 52.5;
        m.fingerprint1.scale(0.5);
        m.fingerprint2.scale(0.5);
        m.fingerprint3.scale(0.5);
        m.fingerprint4.scale(0.5);
    }
    m.fingerprint1.move(m.x1 - m.fp_x_off,m.y1-m.fp_y_off);
    m.fingerprint2.move(m.x2,m.y2-m.fp_y_off);
    m.fingerprint3.move(m.x3,m.y3);
    m.fingerprint4.move(m.x4 - m.fp_x_off,m.y4);


    
    //move text to correct location
    m.text_shadow.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.slope_shadow.move(image_width - 40, image_height - 30);
    m.slope_text.move(image_width - 40, image_height - 30);

    poly_str = m.x1.toFixed() +','+m.y1.toFixed()+' '+m.x2.toFixed()+','+ m.y2.toFixed()+' '+ m.x3.toFixed()+','+ m.y3.toFixed()+' '+m.x4.toFixed()+','+m.y4.toFixed()
    m.polygon = measured_svg.polygon(poly_str).stroke({ color: line_color, width: 2 }).fill({ color: '#008899', opacity: 0.3 });
    
    
    //text editing
    m.text.click(function (e) {
                     setTimeout(function(){ return false;},1);
                     rcMeasurements.start_distance_change_dialouge(m);
                     e.stopPropagation(); e.preventDefault();
                 })
    //cursor for text editing 
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

    
    
    //draging measurement end points.
    Hammer(m.fingerprint1.node).on("drag",  function(e) {
                                       e.stopPropagation(); e.preventDefault();
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, i.x+m.fp_x_off/2, i.y+m.fp_y_off/2, m.x2, m.y2, m.x3, m.y3, m.x4, m.y4);
                                       }).on("dragend", function(e) { rcMeasurements.dragEndHandler(m,e); });
    Hammer(m.fingerprint2.node).on("drag", function(e) {
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, m.x1, m.y1, i.x-m.fp_x_off/2, i.y+m.fp_y_off/2, m.x3, m.y3, m.x4, m.y4);
                                       e.stopPropagation(); e.preventDefault();}).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e);});
    Hammer(m.fingerprint3.node).on("drag", function(e) {
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, m.x1, m.y1, m.x2, m.y2, i.x-m.fp_x_off/2, i.y-m.fp_y_off/2, m.x4, m.y4);
                                       e.stopPropagation(); e.preventDefault();}).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e);});
    Hammer(m.fingerprint4.node).on("drag", function(e) {
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, m.x1, m.y1, m.x2, m.y2, m.x3, m.y3, i.x+m.fp_x_off/2, i.y-m.fp_y_off/2);
                                       e.stopPropagation(); e.preventDefault();}).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e);});

    

    //highlight invalid corners
    if (!m.coords3D[0]) { m.circle1.fill({ color: invalid_color , opacity:1}); }
    if (!m.coords3D[1]) { m.circle2.fill({ color: invalid_color , opacity:1}); }
    if (!m.coords3D[2]) { m.circle3.fill({ color: invalid_color , opacity:1}); }
    if (!m.coords3D[3]) { m.circle4.fill({ color: invalid_color , opacity:1}); }

    
}

rcMeasurements.dragEndHandler = function (m, e) {
    //console.log('rcMeasurements.dragEndHandler()');
    e.stopPropagation(); e.preventDefault();
    rcMeasurements.most_recent_drag = new Date();
    rcMeasurements.save_measurements();
}


rcMeasurements.redraw_measurement = function (m) {
    
    m.text_shadow.text(rcMeasurements.format_dist(m));
    m.text.text(rcMeasurements.format_dist(m));
    var s_string; //slope
    if (m.slope_angle) {s_string = m.slope_angle.toFixed() + "\u00B0";} else {s_string = "no slope";}
    m.slope_shadow.text(s_string);
    m.slope_text.text(s_string);
    
    
    m.font_offset_x = 0;
    m.font_offset_y = 10;
    m.mid_x = m.x3 + (m.x4 - m.x3)/2;   //midpoint of gutter line
    m.mid_y = m.y3 + (m.y4 - m.y3)/2;
    
    
    //move text to correct location
    m.text_shadow.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    
    m.text_shadow.x(m.mid_x + m.font_offset_x).dy(m.mid_y + - m.text.node.attributes.y.value + m.font_offset_y);
    m.text.x(m.mid_x + m.font_offset_x).dy(m.mid_y + - m.text.node.attributes.y.value + m.font_offset_y); //hacky thing because move has a bug
    
    m.circle1.move(m.x1-5,m.y1-5);
    m.circle2.move(m.x2-5,m.y2-5);
    m.circle3.move(m.x3-5,m.y3-5);
    m.circle4.move(m.x4-5,m.y4-5);
    m.fingerprint1.move(m.x1-m.fp_x_off,m.y1-m.fp_y_off);
    m.fingerprint2.move(m.x2,m.y2-m.fp_y_off);
    m.fingerprint3.move(m.x3,m.y3);
    m.fingerprint4.move(m.x4-m.fp_x_off,m.y4);

    
    m.polygon.plot([[m.x1,m.y1], [m.x2,m.y2], [m.x3,m.y3], [m.x4,m.y4]])
    

    //highlight valid/invalid corners
    if (!m.coords3D[0]) { m.circle1.fill({ color: invalid_color , opacity:1}); } else {m.circle1.fill({ color: valid_color , opacity:1});}
    if (!m.coords3D[1]) { m.circle2.fill({ color: invalid_color , opacity:1}); } else {m.circle2.fill({ color: valid_color , opacity:1});}
    if (!m.coords3D[2]) { m.circle3.fill({ color: invalid_color , opacity:1}); } else {m.circle3.fill({ color: valid_color , opacity:1});}
    if (!m.coords3D[3]) { m.circle4.fill({ color: invalid_color , opacity:1}); } else {m.circle4.fill({ color: valid_color , opacity:1});}

    
}





//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                      //
//          EDITING AND DYNAMIC PRESENTATION OF LINEAR MEAUREMENTS                                                                      //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



rcMeasurements.move_measurement = function (m, nx1, ny1, nx2, ny2, nx3, ny3, nx4, ny4) {
    m.x1 = nx1;
    m.y1 = ny1;
    m.x2 = nx2;
    m.y2 = ny2;
    m.x3 = nx3;
    m.y3 = ny3;
    m.x4 = nx4;
    m.y4 = ny4;
    if (!m.overwriten){ m.distance = distanceBetween(m.x3, m.y3, m.x4, m.y4); } //distanceBetween is defineed in depth_data.js
    m.saveable_copy = rcMeasurements.saveable_liniar_measurement(m)
    
    
    rcMeasurements.redraw_measurement(m);
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                             //
//          SAVING, UNDOING, DATA MANAGEMENT                                                                  //
//                                                                                                           //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////



rcMeasurements.to_json = function () {
    //console.log('rcMeasurements.to_json');
    var annotations_to_save = {};
    // save the default units.
    annotations_to_save['global_use_metric'] = default_units_metric;
    annotations_to_save['roof_object'] = rcMeasurements.roof_measurement.saveable_copy;
    return JSON.stringify(annotations_to_save);
}

rcMeasurements.save_measurements = function (optional_not_undoable_flag) {
    jsonStr = rcMeasurements.to_json();
    console.log(jsonStr);
    try{
    $.ajax({ type: "PUT", url: rc_server_location + "true_measure/api/v1/m_photo/" + m_photo_guid + "/annotations/", contentType: "application/json", processData: false, dataType: "json", data: jsonStr })
    .done(function(data, textStatus, jqXHR) {
          //alert(textStatus + ": " + JSON.stringify(data));
          })
    .fail(function(jqXHR, textStatus, errorThrown) {
          //alert(textStatus + ": " + JSON.stringify(jqXHR));
          })
    ;
    } catch (err) {}
}


//////////////////////////////////////////////////////////////////////
//
//  number pad functions
//
///////////////////////////////////////////////////////////////////


rcMeasurements.setText = function (m, str) {
    m.text_shadow.text(str);
    m.text.text(str);
}


rcMeasurements.start_distance_change_dialouge = function (m) {
    rcMeasurements.measurement_being_edited = m;
    draw.node.appendChild(np_svg.node); //show number pad
    //move_image_for_number_pad(m.text.x(), m.text.y());
    //rcMeasurements.start_cursor_animation(m);
    
}

rcMeasurements.end_measurement_edit = function (){
    if (rcMeasurements.measurement_being_edited) {
        if(draw.node.contains(np_svg.node)) {draw.node.removeChild(np_svg.node);} //hide number pad
        //return_image_after_number_pad();
        //rcMeasurements.stop_cursor_animation(rcMeasurements.measurement_being_edited);
        setTimeout( function () {rcMeasurements.save_measurements();}, 0)
        
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
    //rcMeasurements.redraw_measurement(rcMeasurements.measurement_being_edited);
    
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
    //rcMeasurements.redraw_measurement(rcMeasurements.measurement_being_edited);
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
    if (str.indexOf('?') > -1) {   //if theirs a question mark we have no distance
        return null;
    }
    var split_str;
    if (units_metric) {  //if units are metric we parse this way
        str = str.substring(0, str.length - 2); //strip of the final 'm'
        if ( rcMeasurements.isNumber( str ) ) {
            return parseFloat(str);
        }
        else{
            return 'err';
        }
    }
    else {   //parse imperial units
        str = str.substring(0, str.length - 2); //strip of the final ' " '
        split_str = str.split("'");
        if( split_str.length > 1){ // string had feet and inches
            if (rcMeasurements.isNumber( split_str[0]) && rcMeasurements.isNumber( split_str[1])){  //both " and ' are valid numbers
                return ( parseFloat(split_str[0])*12 + parseFloat(split_str[1]) ) / rcMeasurements.inches_to_meter;
            }
            else{ //fail becasue non numeric
                return 'err';
            }
        }
        else { //only " no '
            if (rcMeasurements.isNumber( split_str[0]) ){  // "  are valid numbers
                return parseFloat(str) / rcMeasurements.inches_to_meter;
            }
            else{ //fail becasue non numeric
                return 'err';
            }
        }
        
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
        rcMeasurements.measurement_being_edited.overwriten = true;
        rcMeasurements.redraw_measurement(rcMeasurements.measurement_being_edited);
        rcMeasurements.measurement_being_edited.saveable_copy = rcMeasurements.saveable_liniar_measurement(rcMeasurements.measurement_being_edited);
        rcMeasurements.end_measurement_edit();
    }
}

