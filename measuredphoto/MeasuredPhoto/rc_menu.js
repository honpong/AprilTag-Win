//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

rc_menu = {
    current_button : null, button1 : null, button2 : null, button3 : null, button4 : null, button5 : null, button6 : null
}
rc_menu.unit_menu = function () { alert('no unit options yet'); }
rc_menu.color_menu = function () { alert('no color options yet'); }

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
        
        var v_offset = v * (window.innerHeight - button_size * 5) / 10;
        var h_offset = h * (window.innerWidth - button_size * 5) / 10;
        rc_menu.menu_background.size(menu_svg.width(), menu_svg.height());
        rc_menu.button1.move(h_offset + 0 * button_size * h, v_offset + 0 * button_size * v + 1);
        rc_menu.button2.move(h_offset*3 + 1 * button_size * h, v_offset + 1 * button_size * v + 1);
        rc_menu.button3.move(h_offset*5 + 2 * button_size * h, v_offset + 2 * button_size * v + 1);
        rc_menu.button4.move(h_offset*7 + 3 * button_size * h, v_offset + 3 * button_size * v + 1);
        rc_menu.button5.move(h_offset*9 + 4 * button_size * h, v_offset + 4 * button_size * v + 1);
        //rc_menu.button6.move(h_offset + 5 * button_size * h, v_offset + 5 * button_size * v + 1);
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
    //rc_menu.button6 = menu_svg.group();
    //var rc_menu.button7 = menu_svg.group();
    //var rc_menu.button8 = menu_svg.group();

    rc_menu.button1.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 1 }).fill(button_fill_color));
    rc_menu.button2.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 1 }).fill(button_fill_color));
    rc_menu.button3.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 1 }).fill(button_fill_color));
    rc_menu.button4.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 1 }).fill(button_fill_color));
    rc_menu.button5.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 1 }).fill(button_fill_color));
    //rc_menu.button6.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 1 }).fill(button_fill_color));
    //rc_menu.button7.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));
    //rc_menu.button8.add(menu_svg.rect(button_size -2, button_size -2).stroke({ color: button_outline_color, opacity: 1, width: 3 }).fill(button_fill_color));

    // we are no longer using a mouse icon button...
    //// add mouse arrow icon to button1
    //rc_menu.button1.add(menu_svg.polyline('23.42,31.7 19,25.72 15.1,29.1 9.9,8 26.8,19.35 22.9,22.99 27.19,28.45').stroke({ color: button_outline_color, opacity: 1, width: 2.5 }).fill('none'));

    function draw_3d_axis(button) {
        button.add(menu_svg.line(14,24,14,5).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(14,24,7,32).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(14,24,33,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    }
    
    // add 3d button to button 1
    draw_3d_axis( rc_menu.button1);

    //add line icon to button2
    
    function draw_line_icon (button) {
        button.add(menu_svg.circle(2).move(8,27).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.circle(2).move(27,8).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line(9,28,28,9).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    }
    draw_line_icon( rc_menu.button2 );

    //add angle icon to button2
    function draw_angle_icon(button){
        button.add(menu_svg.path('M23,33 a 15,15 0 0,0 -12,-16').stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line(7,28,24,11).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(7,28,30,28).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    }
    
    draw_angle_icon(rc_menu.button3);
    


    // add magnifying glass to button4
    function draw_mag_glass_icon (button) {
        button.add(menu_svg.circle(17).move(13,7).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line(15.5,21,7,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(21.5,10,21.5,20).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(16.5,15,26.5,15).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    }
    
    // add magnifying glass - to button5
    function draw_mag_glass_minus (button) {
        button.add(menu_svg.circle(17).move(13,7).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
        button.add(menu_svg.line(15.5,21,7,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(16.5,15,26.5,15).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    }

    
    function draw_eraser_icon (button) {
        button.add(menu_svg.line(5,10,19,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(5,10,5,15).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(5,15,20,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(20,30,20,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(20,30,34,30).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(34,30,34,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(5,10,20,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(20,25,34,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(34,25,19,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    }
    draw_eraser_icon (rc_menu.button4);

    
    function draw_text_icon (button) {
        button.add(menu_svg.line(11,10,29,10).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(20,10,20,29).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(11,9,11,12).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(29,9,29,12).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
        button.add(menu_svg.line(18,29,22,29).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    }
    draw_text_icon (rc_menu.button5);
    
    // add trash can to button6
    //rc_menu.button6.add(menu_svg.polyline('10,12 10,31 28,31 28,12').stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
    //rc_menu.button6.add(menu_svg.polyline('15,7 15,5 23,5 23,7').stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }).fill('none'));
    //rc_menu.button6.add(menu_svg.line( 7, 9, 31, 9).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    //rc_menu.button6.add(menu_svg.line(15,13,15,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    //rc_menu.button6.add(menu_svg.line(19,13,19,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    //rc_menu.button6.add(menu_svg.line(23,13,23,25).stroke({ color: button_icon_color, opacity: 1, width: button_icon_stoke_width }));
    rc_menu.rearrange_menu();

    rc_menu.select_button = function (button) {
        //this relies on the fact that the first thing we added to the button groups was the background rectangles.
        if (rc_menu.current_button) {
            rc_menu.deselect_button(rc_menu.current_button);
        }
        if (button) {
            rc_menu.current_button = button;
            rc_menu.current_button.each( function(i, children) {this.stroke({color: button_highlight_color});}); ;
            rc_menu.current_button.get(0).stroke({color: button_outline_color});
        }
    }
    
    //start with line button selected
    rc_menu.select_button(rc_menu.button2);

    rc_menu.deselect_button = function (button) {
        //this relies on the fact that the first thing we added to the button groups was the background rectangles.
        button.each( function(i, children) {this.stroke({color: button_icon_color});});
        button.get(0).stroke({color: button_outline_color});
        if (rc_menu.current_button == button) { rc_menu.current_button = null; }
    }

    //button7.click (function () { select_button(button7); })
    //button8.click (function () { select_button(button8); })


    rc_menu.delete_selected = function () {
        current_measurement.is_deleted = true;
        rcMeasurements.redraw_all_measurements();
    }

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
    
    //add events to buttons
    rc_menu.button1.click (function (e) { switch_image_depthmap(); e.stopPropagation(); e.preventDefault();});
    rc_menu.button2.click (function (e) { rc_menu.select_button(rc_menu.button2); e.stopPropagation(); e.preventDefault(); });
    rc_menu.button3.click (function (e) { rc_menu.select_button(rc_menu.button3); e.stopPropagation(); e.preventDefault(); });
    rc_menu.button4.click (function (e) { rc_menu.select_button(rc_menu.button4); e.stopPropagation(); e.preventDefault(); });
    rc_menu.button5.click (function (e) { rc_menu.select_button(rc_menu.button5); e.stopPropagation(); e.preventDefault(); });

    
    
    //add a button to delete a measurement using delete_selected();

    //rc_menu.button6.click (function (e) { setTimeout(function(){
     //                                        prepare_to_exit();
      //                                       document.location = 'native://finish';
       //                                      },1); e.stopPropagation(); e.preventDefault();});

}