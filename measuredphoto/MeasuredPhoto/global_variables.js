
////////////////////////////////////////////////////////////////////////////////
//   DEFINE GLOBAL VARIABLES                                                  //
////////////////////////////////////////////////////////////////////////////////

var is_touch_device = 'ontouchstart' in document.documentElement; //boolean value which determins if we need to show custom softkeyboard

var image_width, image_height;  //the actual size of the image
var initial_load = true;  //determines if we set scale factor, or preserve prior zoom levle.
var zoom_factor = 1; //how much we've zoomed in
var min_zoom; //the smallest we let the user make the image
var x_offset = 0; max_x_offset = 0; min_x_offset = 0; //deffines pan location, stores pan bounce boudaries
var y_offset = 0; max_y_offset = 0; min_y_offset = 0; //deffines pan location, stores pan bounce boudaries
var portrait_offset, landscape_offset; // additional offsets to acoomidate the buttons
var prior_window_inner_width = window.innerWidth; //the last window size, so we can figure out how to change zoom / zoom offsets when resize occurs
var prior_window_inner_height = window.innerHeight; //

var hammer; //the gesture recognition object

var is_rc_initialized = false; //we use this to determine whether or not we need to call a bunch of setup functions.
var draw;  //the main SVG DOM object we'll be adding to
var draw_g; //the SVG group which contains the image and the measurements - it is rotated and scaled inside draw to allow for dynamic editing behavior.
var measured_svg; //a nested svg object inside draw that contains the measurements we draw to the screen.
var image;  // the image that goes in the svg node
var orientation_drawn_landsacep = false; //lets us know which way we were drawn when we're re-drawing. this may be helpful for optimizaitons, or could be factored out.
var bounce_animation_id = null; // allows tracking and canceling of the pan bounce animation

var last_orientation = 1; //used to track forced orientation changes

//variables for tracking and instantiating measurements
var current_measurement = null;
var lineNotStarted = true; //tracking user behavior to know what actions to take TODO: replace w/ null check on click_image_x1
var click_image_x1, click_image_y1; //tracing of first point
var marker; //svg element which should apear at imate cooridnates of click_image_x1, click_image_y1


// color constants
var shadow_color = '#000';
var highlight_color = '#ff0';
var line_color = '#f00';

// menu buttons
var button_size = 40;
var button_outline_color = '#333';
var button_fill_color = '#999';
var button_highlight_color = '#bb6';
var menu_svg;


