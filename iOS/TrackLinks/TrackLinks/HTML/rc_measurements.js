//  Copyright (c) 2014 Caterpillar. All rights reserved.



rcMeasurements = {
    inches_to_meter : 39.3701,
    most_recent_drag : 0,
    font_family : 'HelveticaNeue-Light, Helvetica, Arial',
};


// instantiate a measurement and add it to the measurment list
// takes image locations
rcMeasurements.new_measurement = function (iX1, iY1, iX2, iY2, measured_svg){
    var m = {};
    m.distance = distanceBetween(iX1, iY1, iX2, iY2); //distanceBetween is defined in depth_data.js
    m.x1 = iX1;
    m.y1 = iY1;
    m.x2 = iX2;
    m.y2 = iY2;
    m.units_metric = default_units_metric;

    rcMeasurements.draw_measurement(m, measured_svg);
    
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
    
    //move text to correct location
    m.text_shadow.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    m.text.move(m.mid_x + m.font_offset_x, m.mid_y + m.font_offset_y);
    
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
    

    //draging measurement end points.
    Hammer(m.selector_circle1.node).on("drag",  function(e) {
                                       e.stopPropagation(); e.preventDefault();
                                       i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY);
                                       rcMeasurements.move_measurement(m, i.x, i.y, m.x2, m.y2);
                                       }).on("dragend", function(e) { rcMeasurements.dragEndHandler(m,e); });
    Hammer(m.selector_circle2.node).on("drag", function(e) {i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); rcMeasurements.move_measurement(m, m.x1, m.y1, i.x, i.y); e.stopPropagation(); e.preventDefault(); }).on("dragend", function(e) {rcMeasurements.dragEndHandler(m,e);});
    
}

rcMeasurements.dragEndHandler = function (m, e) {
    //console.log('rcMeasurements.dragEndHandler()');
    e.stopPropagation(); e.preventDefault();
    rcMeasurements.most_recent_drag = new Date();
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
    
    m.selector_circle1.move(m.x1-15,m.y1-15);
    m.selector_circle2.move(m.x2-15,m.y2-15);
    
    
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
        m.line1.plot(m.x1 - 1 * m.xdiffrt, m.y1 - 1 * m.ydiffrt, m.x2 + 1 * m.xdiffrt, m.y2 + 1 * m.ydiffrt)
        m.line2.plot(m.x1 - 1 * m.xdiffrt, m.y1 - 1 * m.ydiffrt, m.x2 + 1 * m.xdiffrt, m.y2 + 1 * m.ydiffrt)
    }
    else {
        m.line1.plot(m.x1 - 1 * m.xdiffrt, m.y1 - 1 * m.ydiffrt,  m.mid_x + (m.half_font_gap + 1) * m.xdiffrt, m.mid_y + (m.half_font_gap + 1) * m.ydiffrt)
        m.line2.plot(m.mid_x - (m.half_font_gap + 1) * m.xdiffrt, m.mid_y - (m.half_font_gap + 1) * m.ydiffrt, m.x2 + 1 * m.xdiffrt, m.y2 + 1 * m.ydiffrt)
    }
    
}




// this clears all measurements, used when we are switching between measured photos but not reloading the app
rcMeasurements.reset = function () {
}


rcMeasurements.move_measurement = function (m, nx1, ny1, nx2, ny2) {
    m.x1 = nx1;
    m.y1 = ny1;
    m.x2 = nx2;
    m.y2 = ny2;
    if (!m.overwriten){ m.distance = distanceBetween(m.x1, m.y1, m.x2, m.y2); } //distanceBetween is defineed in depth_data.js
    
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

