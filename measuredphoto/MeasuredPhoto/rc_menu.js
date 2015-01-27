//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

rc_menu = {
    current_button : null, eraser_button : null, line_button : null, angle_button: null, text_button : null, unit_button : null, depthmap_button : null, help_button : null, button1 : null, button2 : null, button3 : null, button4 : null, button5 : null, button6 : null, help_shown : null
}
rc_menu.color_menu = function () { alert('no color options yet'); }

rc_menu.show_depth_map = true;

function build_rc_menu() {
    //alert ('in build_rc_menu');
    rc_menu.rearrange_menu = function () {
        var h, v;
        
        if (draw.node.contains(menu_svg.node)) {draw.node.removeChild(menu_svg.node);}
        if(window.innerHeight > window.innerWidth){
            //portrait, we want buttons on the bottom and pad the viewport by buttom height
            menu_svg.size( window.innerWidth, button_size);
            menu_svg.move(0, window.innerHeight - button_size);
            h=1; v=0;
        }
        else {
            //landsape, we want buttons on side and pad the viewport by buttom width
            menu_svg.size(button_size, window.innerHeight);
            h=0; v=1;
        }

        if (rc_menu.show_depth_map) {
            var v_offset = v * (window.innerHeight - button_size * 6) / 12;
            var h_offset = h * (window.innerWidth - button_size * 6) / 12;
        }
        else{
            var v_offset = v * (window.innerHeight - button_size * 5) / 10;
            var h_offset = h * (window.innerWidth - button_size * 5) / 10;
        }
        rc_menu.menu_background.size(menu_svg.width(), menu_svg.height());
        rc_menu.button1.move(h_offset + 0 * button_size * h, v_offset + 0 * button_size * v + 1);
        rc_menu.button2.move(h_offset*3 + 1 * button_size * h, v_offset + 1 * button_size * v + 1);
        rc_menu.button3.move(h_offset*5 + 2 * button_size * h, v_offset + 2 * button_size * v + 1);
        rc_menu.button4.move(h_offset*7 + 3 * button_size * h, v_offset + 3 * button_size * v + 1);
        rc_menu.button5.move(h_offset*9 + 4 * button_size * h, v_offset + 4 * button_size * v + 1);
        if (rc_menu.show_depth_map) {
            rc_menu.button6.move(h_offset*11 + 5 * button_size * h, v_offset + 5 * button_size * v + 1);
        }
        //button7.move(h_offset + 6 * button_size * h, v_offset + 6 * button_size * v);
        //button8.move(h_offset + 7 * button_size * h, v_offset + 7 * button_size * v);
        if ( ! draw.node.contains(menu_svg.node)) {draw.node.appendChild(menu_svg.node);}
        
    }

    rc_menu.menu_background = menu_svg.rect(menu_svg.width(), menu_svg.height()).fill(menu_background_color);
    rc_menu.button1 = menu_svg.group();
    rc_menu.button2 = menu_svg.group();
    rc_menu.button3 = menu_svg.group();
    rc_menu.button4 = menu_svg.group();
    rc_menu.button5 = menu_svg.group();
    if (rc_menu.show_depth_map) {
        rc_menu.button6 = menu_svg.group();
    }
    //var rc_menu.button7 = menu_svg.group();
    //var rc_menu.button8 = menu_svg.group();

    rc_menu.button1.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: button_fill_opacity, width: 1 }).fill({color: button_fill_color, opacity: button_fill_opacity}));
    rc_menu.button2.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: button_fill_opacity, width: 1 }).fill({color: button_fill_color, opacity: button_fill_opacity}));
    rc_menu.button3.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: button_fill_opacity, width: 1 }).fill({color: button_fill_color, opacity: button_fill_opacity}));
    rc_menu.button4.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: button_fill_opacity, width: 1 }).fill({color: button_fill_color, opacity: button_fill_opacity}));
    rc_menu.button5.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: button_fill_opacity, width: 1 }).fill({color: button_fill_color, opacity: button_fill_opacity}));
    if (rc_menu.show_depth_map) {
        rc_menu.button6.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: button_fill_opacity, width: 1 }).fill({color: button_fill_color, opacity: button_fill_opacity}));
    }
    //rc_menu.button7.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    //rc_menu.button8.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));

    // we are no longer using a mouse icon button...
    //// add mouse arrow icon to button1
    //rc_menu.button1.add(menu_svg.polyline('23.42,31.7 19,25.72 15.1,29.1 9.9,8 26.8,19.35 22.9,22.99 27.19,28.45').stroke({ color: button_outline_color, opacity: 1, width: 2.5 }).fill('none'));

    function draw_3d_axis(button) {
        rc_menu.depthmap_button = button;
        button.add(menu_svg.line(14,24,14,5).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(14,24,7,32).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(14,24,33,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        rc_menu.depthmap_button.click (function (e) { switch_image_depthmap(); e.stopPropagation(); e.preventDefault();});
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
    }
    if (rc_menu.show_depth_map) {
        // add 3d button to button 6
        draw_3d_axis( rc_menu.button6);
    }
    
    
    //add line icon to button1
    function draw_line_icon (button) {
        rc_menu.line_button = button;
        button.add(menu_svg.circle(2).move(8,27).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.circle(2).move(27,8).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line(9,28,28,9).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        rc_menu.line_button.click (function (e) { rc_menu.select_button(button); e.stopPropagation(); e.preventDefault(); });
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
    }
    draw_line_icon( rc_menu.button1 );

    //add angle icon to button2
    function draw_angle_icon(button){
        rc_menu.angle_button = button;
        button.add(menu_svg.path('M23,33 a 15,15 0 0,0 -12,-16').stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line(7,28,24,11).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(7,28,30,28).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        rc_menu.angle_button.click (function (e) { rc_menu.select_button(rc_menu.button3); e.stopPropagation(); e.preventDefault(); });
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
    }
    //draw_angle_icon(rc_menu.button3);
    

    function draw_undo_icon (button) {
        rc_menu.undo_button = button;
        button.add(menu_svg.path('M20,35 a 3,3 0 0,0 0,-20').stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line(20,35,10,35).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(20,15,10,15).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(10,15,15,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(10,15,15,20).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        rc_menu.undo_button.click (function (e) { undo_last_change(); e.stopPropagation(); e.preventDefault();});
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
    }
    draw_undo_icon (rc_menu.button4);

    // add magnifying glass to button4
    function draw_mag_glass_icon (button) {
        button.add(menu_svg.circle(17).move(13,7).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line(15.5,21,7,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(21.5,10,21.5,20).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(16.5,15,26.5,15).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
    }
    
    // add magnifying glass - to button5
    function draw_mag_glass_minus (button) {
        button.add(menu_svg.circle(17).move(13,7).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line(15.5,21,7,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(16.5,15,26.5,15).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
    }

    
    function draw_unit_toggle (button) {
        rc_menu.unit_button = button;
        
        rc_menu.metric_unit_text = menu_svg.text('m').font({family: rcMeasurements.font_family, size: 15, anchor: 'middle', leading: 1
                                                           }).move(10,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width/2.5 });
        rc_menu.imperial_unit_text = menu_svg.text('ft').font({family: rcMeasurements.font_family, size: 15, anchor: 'middle', leading: 1
                                                              }).move(30,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width/2.5 });
        button.add(menu_svg.text('|').font({family: rcMeasurements.font_family, size: 15, anchor: 'middle', leading: 1
                                           }).move(20,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width/2.5 }));
        button.add(rc_menu.metric_unit_text);
        button.add(rc_menu.imperial_unit_text);
        
        rc_menu.unit_button.click (function (e) {
                                       toggle_all_units(); e.stopPropagation(); e.preventDefault();
                                       });
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
        
        rc_menu.unit_button.highlight_active_unit = function (metric_is_selected) {
            if (metric_is_selected) {
                rc_menu.metric_unit_text.stroke({color: button_highlight_color});
                rc_menu.imperial_unit_text.stroke({color: button_icon_color});
            }
            else {
                rc_menu.imperial_unit_text.stroke({color: button_highlight_color});
                rc_menu.metric_unit_text.stroke({color: button_icon_color});
            }
        }
    }
    draw_unit_toggle(rc_menu.button2);

    rc_menu.help_shown = false;
    
    function togle_help() {
        if (rc_menu.help_shown) {
            rc_menu.help_shown = false;
            rcMessage.clearHTML();
        }
        else {
            rc_menu.help_shown = true;
            rcMessage.postHTML('<ul style="background-color:white; border-radius: 15px; margin:6px; font-family: Helvetica;" ><br/>'+
                               '<li style="margin: 6px;">To create a measurement, select the line tool, then tap two points.</li>'+
                           '<li style="margin: 6px;">Select the eraser and touch a measurement to delet it.</li>'+
                           '<li style="margin: 6px;">The "m | ft" button to toggles units.</li>'+
                           '<li style="margin: 6px;">Hit undo to undo your last change.</li>'+
                           '<li style="margin: 6px;">You can pinch to zoom, and pan when zoomed in.</li>'+
                           '<li style="margin: 6px;">The trash can erases the photo.</li>'+
                           '<li style="margin: 6px;">Tap the title box to rename the photo.</li>'+
                           '<li style="margin: 6px;">Hit "?" again to hide this.</li><br/></ul>', 30000);
        }
    }
    
    function draw_help_button (button) {
        rc_menu.help_button = button;
        
        rc_menu.question_mark_text = menu_svg.text('?').font({family: rcMeasurements.font_family, size: 25, anchor: 'middle', leading: 1
                                                              }).move(20,5).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width/10 });
        button.add(rc_menu.question_mark_text);
        
        rc_menu.help_button.click (function (e) {
                                   togle_help(); e.stopPropagation(); e.preventDefault();
                                   });
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
        
    }
    draw_help_button(rc_menu.button5);

    
    function draw_eraser_icon (button) {
        rc_menu.eraser_button = button;
        button.add(menu_svg.line(5,10,19,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(5,10,5,15).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(5,15,20,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(20,30,20,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(20,30,34,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(34,30,34,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(5,10,20,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(20,25,34,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(34,25,19,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        rc_menu.eraser_button.click (function (e) { rc_menu.select_button(button); e.stopPropagation(); e.preventDefault(); });
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
    }
    draw_eraser_icon (rc_menu.button3);

    
    function draw_text_icon (button) {
        rc_menu.note_button = button;
        button.add(menu_svg.line(11,10,29,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(20,10,20,29).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(11,9,11,12).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(29,9,29,12).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(18,29,22,29).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        rc_menu.text_button.click (function (e) { rc_menu.select_button(button); e.stopPropagation(); e.preventDefault(); });
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
    }
    //draw_text_icon (rc_menu.button5);
    
    function draw_trash_icon (button) {
        button.add(menu_svg.polyline('10,12 10,31 28,31 28,12').stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.polyline('15,7 15,5 23,5 23,7').stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line( 7, 9, 31, 9).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(15,13,15,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(19,13,19,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(23,13,23,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.rotate_button = function (target_rotation) {
            button.rotate(target_rotation, button.x() + button_size/2, button.y() + button_size/2);
        }
    }

    rc_menu.rearrange_menu();

    rc_menu.select_button = function (button) {
        //this relies on the fact that the first thing we added to the button groups was the background rectangles.
        if (rc_menu.current_button) {
            rc_menu.deselect_button(rc_menu.current_button);
        }
        if (button) {
            clear_tool_data();
            rc_menu.current_button = button;
            rc_menu.current_button.each( function(i, children) {this.stroke({color: button_highlight_color});}); ;
            rc_menu.current_button.get(0).stroke({color: button_outline_color});
        }
    }
    
    //start with line button selected
    rc_menu.select_button(rc_menu.button1);

    rc_menu.deselect_button = function (button) {
        //this relies on the fact that the first thing we added to the button groups was the background rectangles.
        button.each( function(i, children) {this.stroke({color: button_icon_color});});
        button.get(0).stroke({color: button_outline_color});
        if (rc_menu.current_button == button) { rc_menu.current_button = null; }
    }

    //button7.click (function () { select_button(button7); })
    //button8.click (function () { select_button(button8); })

    //functions to zoom left over from mangifying buttons - may be added in desktop version.
    function zoom_in_action() {
        setTimeout(function(){ zoom(1.2, window.innerWidth / 2, window.innerHeight / 2) ;},1);
        start_pan_bounce();
        start_zoom_return();
    }
    function zoom_out_action() {
        setTimeout(function(){ zoom(0.8, window.innerWidth / 2, window.innerHeight / 2) ;},1);
        start_pan_bounce();
        setTimeout(function() {start_zoom_return();},10);
    }
    //function for debuging in desktop - when added to a click action it simulates the display rotation for debugging purposes.
    function simulate_rotation () {
        var next_orientation;
        if (last_orientation == 1) {next_orientation = 3;}
        else if (last_orientation == 3) {next_orientation = 2;}
        else if (last_orientation == 2) {next_orientation = 4;}
        else if (last_orientation == 4) {next_orientation = 1;}
        forceOrientationChange(next_orientation);
    }
    
    rc_menu.enable_disenable_undo = function (undo_possible) {
        //console.log('rc_menu.enable_disenable_undo(' + undo_possible.toString() + ')');
        var undo_button_color;
        if (undo_possible) { undo_button_color = button_icon_color; }
        else { undo_button_color = button_icon_inactive_color; }
        
        rc_menu.undo_button.each(function(i, children) {
                                 this.stroke({ color: undo_button_color })
                                 });
        
    }
    
    rc_menu.rotate_buttons = function (target_rotation) {
        rc_menu.button1.rotate_button(target_rotation);
        rc_menu.button2.rotate_button(target_rotation);
        rc_menu.button3.rotate_button(target_rotation);
        rc_menu.button4.rotate_button(target_rotation);
        rc_menu.button5.rotate_button(target_rotation);
    }
    
}

rc_menu.reset =  function () {
    rc_menu.select_button(rc_menu.button1);
}