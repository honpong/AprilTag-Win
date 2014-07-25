//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

rcMeasurements = {
    measurements : []
}

// instantiate a measurement and add it to the measurment list
// takes image locations
rcMeasurements.new_measurement = function (iX1, iY1, iX2, iY2){
    var m = {};
    var start_time = new Date();
    m.distance = distanceBetween(iX1, iY1, iX2, iY2); //distanceBetween is defined in depth_data.js
    m.overwriten = false;
    m.is_deleted = false;
    m.x1 = iX1;
    m.y1 = iY1;
    m.x2 = iX2;
    m.y2 = iY2;
    rcMeasurements.draw_measurement(m);

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
    input.name = "distance" + rcMeasurements.measurements.length;
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
    
    
    m.shadow_line1.click (function (e) { setTimeout(function(){ return false;},1); select_measurement(m); e.stopPropagation(); e.preventDefault(); })
    m.shadow_line2.click (function (e) { setTimeout(function(){ return false;},1); select_measurement(m); e.stopPropagation(); e.preventDefault();})
    m.line1.click (function (e) { setTimeout(function(){ return false;},1); select_measurement(m); e.stopPropagation(); e.preventDefault();})
    m.line2.click (function (e) { setTimeout(function(){ return false;},1); select_measurement(m); e.stopPropagation(); e.preventDefault();})
    m.circle1.click (function (e) { setTimeout(function(){ return false;},1); select_measurement(m); e.stopPropagation(); e.preventDefault();})
    m.circle2.click (function (e) { setTimeout(function(){ return false;},1); select_measurement(m); e.stopPropagation(); e.preventDefault();})
    m.text.click(function (e) { setTimeout(function(){ return false;},1); start_distance_change_dialouge(m); e.stopPropagation(); e.preventDefault();})
    m.selector_circle1.click(function (e) { setTimeout(function(){ return false;},1); select_measurement(m); e.stopPropagation(); e.preventDefault();})
    m.selector_circle2.click(function (e) { setTimeout(function(){ return false;},1); select_measurement(m); e.stopPropagation(); e.preventDefault();})
    
    
    Hammer(m.circle1.node).on("drag", function(e) { i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); move_measurement(m, i.x, i.y, m.x2, m.y2); e.stopPropagation(); e.preventDefault(); });
    Hammer(m.circle2.node).on("drag",  function(e) { i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); move_measurement(m, m.x1, m.y1, i.x, i.y); e.stopPropagation(); e.preventDefault();});
    Hammer(m.selector_circle1.node).on("drag",  function(e) {i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); move_measurement(m, i.x, i.y, m.x2, m.y2); e.stopPropagation(); e.preventDefault(); });
    Hammer(m.selector_circle2.node).on("drag", function(e) {i = pxl_to_img_xy(e.gesture.center.pageX, e.gesture.center.pageY); move_measurement(m, m.x1, m.y1, i.x, i.y); e.stopPropagation(); e.preventDefault(); });
    
    rcMeasurements.measurements.push(m);

}

rcMeasurements.redraw_measurement = function (m) {
    if (m.is_deleted) {
        m.text.remove();
        m.circle1.remove();
        m.circle2.remove();
        m.selector_circle1.remove();
        m.selector_circle2.remove();
        m.shadow_line1.remove();
        m.shadow_line2.remove();
        m.line1.remove();
        m.line2.remove();
    }
    
    else {
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

}

rcMeasurements.redraw_all_measurements = function (){
    for (var i = 0; i < rcMeasurements.measurements.length; i++) {
        rcMeasurements.redraw_measurement(measurements[i]);
    }
}


rcMeasurements.to_json = function () {

}

rcMeasurements.measurement_to_json  = function () {

}

rcMeasurements.delete_measurement  = function () {

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
