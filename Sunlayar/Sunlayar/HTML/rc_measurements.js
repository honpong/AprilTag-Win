//  Copyright (c) 2014 Sunlayar. All rights reserved.



rcMeasurements = {
    inches_to_meter : 39.3701,
    most_recent_drag : 0,
    font_family : 'HelveticaNeue-Light, Helvetica, Arial',
    roof_measurement : null
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                      //
//          LINEAR MEASUREMENTS                                                                                                         //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

    rcMeasurements.draw_measurement(m, measured_svg);
    rcMeasurements.roof_measurement = m;
}

rcMeasurements.saveable_liniar_measurement = function (m) {
    m.coords3D = [[0,0,0],[0,0,0],[0,0,0],[0,0,0]]; //we need this for the AR engine
    try{m.coords3D[0] = dm_3d_location_from_pixel_location(m.x1,m.y1)} catch(err) {}
    try{m.coords3D[1] = dm_3d_location_from_pixel_location(m.x2,m.y2)} catch(err) {}
    try{m.coords3D[2] = dm_3d_location_from_pixel_location(m.x3,m.y3)} catch(err) {}
    try{m.coords3D[3] = dm_3d_location_from_pixel_location(m.x4,m.y4)} catch(err) {}

    //we only want a subset of the measurements content, so we create a temp object we write the content we want to keep into
    return { gutter_length:m.distance, gutter_length_overwriten:m.overwriten, x1:m.x1, y1:m.y1, x2:m.x2, y2:m.y2, x3:m.x3, y3:m.y3, x4:m.x4, y4:m.y4, coords3D: m.coords3D};
}


rcMeasurements.draw_measurement = function (m, measured_svg){
    //gutter distance is from point 3 to point 4
    
    //we need to write gutter distance onto screen
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
    
    
    m.font_offset_x = 0;
    m.font_offset_y = 10;
    m.mid_x = m.x3 + (m.x4 - m.x3)/2;   //midpoint of gutter line
    m.mid_y = m.y3 + (m.y4 - m.y3)/2;

    
    //move text to correct location
    m.text_shadow.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);

    poly_str = m.x1.toFixed() +','+m.y1.toFixed()+' '+m.x2.toFixed()+','+ m.y2.toFixed()+' '+ m.x3.toFixed()+','+ m.y3.toFixed()+' '+m.x4.toFixed()+','+m.y4.toFixed()
    m.polygon = measured_svg.polygon(poly_str).stroke({ color: line_color, width: 2 }).fill({ color: '#008899', opacity: 0.3 });
    
    m.selector_circle1 = measured_svg.circle(30).move(m.x1-15,m.y1-15).fill({opacity:0});
    m.selector_circle2 = measured_svg.circle(30).move(m.x2-15,m.y2-15).fill({opacity:0});
    m.selector_circle3 = measured_svg.circle(30).move(m.x3-15,m.y3-15).fill({opacity:0});
    m.selector_circle4 = measured_svg.circle(30).move(m.x4-15,m.y4-15).fill({opacity:0});
    
    
    
    //draging measurement end points.
    Hammer(m.selector_circle1.node).on("drag",  function(e) {
                                       e.stopPropagation(); e.preventDefault();
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, i.x, i.y, m.x2, m.y2, m.x3, m.y3, m.x4, m.y4);
                                       }).on("dragend", function(e) { rcMeasurements.dragEndHandler(m,e); });
    Hammer(m.selector_circle2.node).on("drag", function(e) {
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, m.x1, m.y1, i.x, i.y, m.x3, m.y3, m.x4, m.y4);
                                       e.stopPropagation(); e.preventDefault();}).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e);});
    Hammer(m.selector_circle3.node).on("drag", function(e) {
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, m.x1, m.y1, m.x2, m.y2, i.x, i.y, m.x4, m.y4);
                                       e.stopPropagation(); e.preventDefault();}).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e);});
    Hammer(m.selector_circle4.node).on("drag", function(e) {
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, m.x1, m.y1, m.x2, m.y2, m.x3, m.y3, i.x, i.y);
                                       e.stopPropagation(); e.preventDefault();}).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e);});

    
    m.saveable_copy = rcMeasurements.saveable_liniar_measurement(m)

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
    
    
    m.font_offset_x = 0;
    m.font_offset_y = 10;
    m.mid_x = m.x3 + (m.x4 - m.x3)/2;   //midpoint of gutter line
    m.mid_y = m.y3 + (m.y4 - m.y3)/2;
    
    
    //move text to correct location
    m.text_shadow.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    
    m.text_shadow.x(m.mid_x + m.font_offset_x).dy(m.mid_y + - m.text.node.attributes.y.value + m.font_offset_y);
    m.text.x(m.mid_x + m.font_offset_x).dy(m.mid_y + - m.text.node.attributes.y.value + m.font_offset_y); //hacky thing because move has a bug
    
    m.selector_circle1.move(m.x1-15,m.y1-15);
    m.selector_circle2.move(m.x2-15,m.y2-15);
    m.selector_circle3.move(m.x3-15,m.y3-15);
    m.selector_circle4.move(m.x4-15,m.y4-15);
    
    m.polygon.plot([[m.x1,m.y1], [m.x2,m.y2], [m.x3,m.y3], [m.x4,m.y4]])
    
    m.saveable_copy = rcMeasurements.saveable_liniar_measurement(m)

    
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
    if (!m.overwriten){ m.distance = distanceBetween(m.x3, m.y3, m.x4, m.x4); } //distanceBetween is defineed in depth_data.js
    
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                      //
//          SAVING, UNDOING, DATA MANAGEMENT                                                                                            //
//                                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



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
    $.ajax({ type: "PUT", url: rc_server_location + "true_measure/api/v1/m_photo/" + m_photo_guid + "/annotations/", contentType: "application/json", processData: false, dataType: "json", data: jsonStr })
    .done(function(data, textStatus, jqXHR) {
          //alert(textStatus + ": " + JSON.stringify(data));
          })
    .fail(function(jqXHR, textStatus, errorThrown) {
          //alert(textStatus + ": " + JSON.stringify(jqXHR));
          })
    ;
}

