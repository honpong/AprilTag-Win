//  Copyright (c) 2014 Caterpillar. All rights reserved.

////////////////////////////////////////////////////////////////////////////////
//   DEFINE GLOBAL VARIABLES                                                  //
////////////////////////////////////////////////////////////////////////////////

var is_touch_device = 'ontouchstart' in document.documentElement; //boolean value which determins if we need to show custom softkeyboard

var image_width, image_height;  //the actual size of the image
var image; //the svg image element, tracked to allow resets
var initial_load = true;  //determines if we set scale factor, or preserve prior zoom levle.
var zoom_factor = 1; //how much we've zoomed in
var min_zoom; //the smallest we let the user make the image
var x_offset = 0; max_x_offset = 0; min_x_offset = 0; //deffines pan location, stores pan bounce boudaries. in pixel units on screen, not image units
var y_offset = 0; max_y_offset = 0; min_y_offset = 0; //deffines pan location, stores pan bounce boudaries. in pixel units on screen, not image units

var hammer; //the gesture recognition object

var is_rc_initialized = false; //we use this to determine whether or not we need to call a bunch of setup functions.
var draw;  //the main SVG DOM object we'll be adding to
var img_container;//the nested SVG element which contains the image and the measurements
var draw_g; //the SVG group which contains the image and the measurements - it is rotated and scaled inside draw_container to allow for dynamic editing behavior.
var measured_svg; //a nested svg object inside draw_g that contains the measurements we draw to the screen.
var image;  // the image that goes in the svg node
var bounce_animation_id = null; // allows tracking and canceling of the pan bounce animation

// color constants
var shadow_color = '#222';
var highlight_color = '#992';
var line_color = '#ca0';


//unit display
var default_units_metric = false; // false for imperial
var unit_display_fractional = true;
var unit_default_set_by_app = false;

