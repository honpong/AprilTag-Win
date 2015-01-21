//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

// this code creates the numeric keypad. callback functions can be provided for the buttons, and when np_add_listners is called they will be added to events on the keypad

var np_b_h = 50; //button hieght
var np_b_w = 90; //button width
var np_b_b = 2; //button border
var np_b_bgc = '#333'; //button background color
var np_b_fgc = '#666'; // button foreground color
var np_b_stc = '#111'; // button stroke color
var np_b_tc  = '#fff'; // text color
var np_node = document.createElement('np_node');
var np_svg = SVG(np_node).size(window.innerWidth, np_b_h * 4);
var np_gradient = np_svg.gradient('linear', function(stop) {
                                  stop.at(0, np_b_bgc);
                                  stop.at(1, np_b_fgc);
                                  }).from(0, 1).to(0, 0);
//create and place buttons
var np_backgrnd = np_svg.rect(window.innerWidth,np_b_h * 4).fill(np_b_bgc).opacity(0.75);
var np_1 = np_svg.group();
var np_1_r = np_svg.rect(np_b_w, np_b_h).attr({ fill: np_gradient, stroke: np_b_stc, 'stroke-width': np_b_b });
np_1.add(np_1_r);
var np_2 = np_svg.group().move(np_b_w*1,np_b_h*0); np_2.add(np_svg.use(np_1_r));
var np_3 = np_svg.group().move(np_b_w*2,np_b_h*0); np_3.add(np_svg.use(np_1_r));
var np_4 = np_svg.group().move(np_b_w*0,np_b_h*1); np_4.add(np_svg.use(np_1_r));
var np_5 = np_svg.group().move(np_b_w*1,np_b_h*1); np_5.add(np_svg.use(np_1_r));
var np_6 = np_svg.group().move(np_b_w*2,np_b_h*1); np_6.add(np_svg.use(np_1_r));
var np_7 = np_svg.group().move(np_b_w*0,np_b_h*2); np_7.add(np_svg.use(np_1_r));
var np_8 = np_svg.group().move(np_b_w*1,np_b_h*2); np_8.add(np_svg.use(np_1_r));
var np_9 = np_svg.group().move(np_b_w*2,np_b_h*2); np_9.add(np_svg.use(np_1_r));
var np_0 = np_svg.group().move(np_b_w*1,np_b_h*3); np_0.add(np_svg.use(np_1_r));
var np_prd = np_svg.group().move(np_b_w*0,np_b_h*3); np_prd.add(np_svg.use(np_1_r));
var np_del = np_svg.group().move(np_b_w*2,np_b_h*3); np_del.add(np_svg.use(np_1_r));
var np_ent = np_svg.group().size(np_b_w, np_b_h*2).move(np_b_w*3,np_b_h*2);
np_ent.add(np_svg.rect(np_b_w, np_b_h*2).attr({ fill: np_gradient, stroke: np_b_stc, 'stroke-width': np_b_b }));
var np_unt = np_svg.group().move(np_b_w*3,np_b_h*0); np_unt.add(np_svg.use(np_1_r));
var np_oth = np_svg.group().move(np_b_w*3,np_b_h*1); np_oth.add(np_svg.use(np_1_r));

//add text to buttons
var np_t1 = np_1.text("1").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t2 = np_2.text("2").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t3 = np_3.text("3").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t4 = np_4.text("4").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t5 = np_5.text("5").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t6 = np_6.text("6").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t7 = np_7.text("7").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t8 = np_8.text("8").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t9 = np_9.text("9").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t0 = np_0.text("0").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t_prd = np_prd.text(".").fill(np_b_tc).move(np_b_w/2, 0).font({ family:'Helvetica', size:20, anchor:'middle', leading:'1.5em'});
var np_t_del = np_del.text("Del").fill(np_b_tc).move(np_b_w/2, np_b_h/7).font({ family:'Helvetica', size:15, anchor:'middle', leading:'1.5em'});
var np_t_ent = np_ent.text("Ent").fill(np_b_tc).move(np_b_w/2, np_b_h/2).font({ family:'Helvetica', size:15, anchor:'middle', leading:'1.5em'});
var np_t_unt = np_unt.text("Units").fill(np_b_tc).move(np_b_w/2, np_b_h/7).font({ family:'Helvetica', size:15, anchor:'middle', leading:'1.5em'});
var np_t_oth = np_oth.text("'").fill(np_b_tc).move(np_b_w/2, np_b_h/7).font({ family:'Helvetica', size:15, anchor:'middle', leading:'1.5em'});

//add listeners to buttons


function np_to_landscape_bottom(){
    var np_scale = 1;
    var np_offset = 0.0;
    if (np_b_w*8 > (window.innerWidth)) {np_scale =(window.innerWidth)/np_b_w/8;}
    else {np_offset =  ((window.innerWidth ) - np_b_w*8)/2;}
    np_svg.size(window.innerWidth, np_scale*np_b_h * 2);
    np_svg.move(0, window.innerHeight - np_svg.height());
    np_backgrnd.size(window.innerWidth, np_scale*np_b_h * 2).rotate(0).scale(np_scale);
    np_1.move(np_offset + np_scale*np_b_w*0,np_scale*np_b_h*0).rotate(0).scale(np_scale);
    np_2.move(np_offset + np_scale*np_b_w*1,np_scale*np_b_h*0).rotate(0).scale(np_scale);
    np_3.move(np_offset + np_scale*np_b_w*2,np_scale*np_b_h*0).rotate(0).scale(np_scale);
    np_4.move(np_offset + np_scale*np_b_w*3,np_scale*np_b_h*0).rotate(0).scale(np_scale);
    np_5.move(np_offset + np_scale*np_b_w*4,np_scale*np_b_h*0).rotate(0).scale(np_scale);
    np_6.move(np_offset + np_scale*np_b_w*0,np_scale*np_b_h*1).rotate(0).scale(np_scale);
    np_7.move(np_offset + np_scale*np_b_w*1,np_scale*np_b_h*1).rotate(0).scale(np_scale);
    np_8.move(np_offset + np_scale*np_b_w*2,np_scale*np_b_h*1).rotate(0).scale(np_scale);
    np_9.move(np_offset + np_scale*np_b_w*3,np_scale*np_b_h*1).rotate(0).scale(np_scale);
    np_0.move(np_offset + np_scale*np_b_w*4,np_scale*np_b_h*1).rotate(0).scale(np_scale);
    np_prd.move(np_offset + np_scale*np_b_w*5,np_scale*np_b_h*0).rotate(0).scale(np_scale);
    np_del.move(np_offset + np_scale*np_b_w*5,np_scale*np_b_h*1).rotate(0).scale(np_scale);
    np_ent.move(np_offset + np_scale*np_b_w*7,np_scale*np_b_h*0).rotate(0).scale(np_scale);
    np_unt.move(np_offset + np_scale*np_b_w*6,np_scale*np_b_h*0).rotate(0).scale(np_scale);
    np_oth.move(np_offset + np_scale*np_b_w*6,np_scale*np_b_h*1).rotate(0).scale(np_scale);
    
}

function np_to_landscape_left(){
    var np_b_whd = (np_b_h - np_b_w)/2;
    var np_b_w2hd = (np_b_h*2 - np_b_w)/2;
    var np_scale = 1;
    var np_offset = 0;
    if (np_b_w * 8 > (window.innerHeight - portrait_offset)) {np_scale =(window.innerHeight - portrait_offset)/np_b_w/8;}
    else {np_offset = portrait_offset + ((window.innerHeight - portrait_offset) - np_b_w*8)/2;}
    np_svg.size( np_scale*np_b_h * 2, window.innerHeight);
    np_svg.move(0, 0);
    np_backgrnd.size(np_scale*np_b_h * 2, window.innerHeight).scale(np_scale);
    np_1.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*0 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_2.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*1 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_3.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*2 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_4.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*3 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_5.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*4 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_6.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*0 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_7.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*1 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_8.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*2 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_9.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*3 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_0.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*4 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_prd.move((np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*5 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_del.move((np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*5 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_ent.move((np_b_h*0 + np_b_w2hd)*np_scale, np_offset+ (np_b_w*7 - np_b_w2hd)*np_scale).rotate(90).scale(np_scale);
    np_unt.move((np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*6 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    np_oth.move((np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*6 - np_b_whd)*np_scale).rotate(90).scale(np_scale);
    
}

function np_to_landscape_right(){
    var np_b_whd = (np_b_h - np_b_w)/2;
    var np_b_w2hd = (np_b_h*2 - np_b_w)/2;
    var np_scale = 1;
    var np_offset = 0.0;
    if (np_b_w * 8 > (window.innerHeight - portrait_offset)) {np_scale = (window.innerHeight - portrait_offset)/np_b_w/8}
    else {np_offset = portrait_offset + ((window.innerHeight - portrait_offset) - np_b_w*8)/2;}
    np_svg.size( np_scale*np_b_h * 2, window.innerHeight);
    np_svg.move(window.innerWidth - np_svg.width(), 0);
    np_backgrnd.size(np_scale*np_b_h * 2, window.innerHeight).scale(np_scale);
    np_1.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*7 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_2.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*6 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_3.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*5 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_4.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*4 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_5.move( ( np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*3 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_6.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*7 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_7.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*6 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_8.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*5 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_9.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*4 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_0.move( ( np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*3 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_prd.move((np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*2 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_del.move((np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*2 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_ent.move((np_b_h*0 + np_b_w2hd)*np_scale,np_offset + (np_b_w*0 - np_b_w2hd)*np_scale).rotate(-90).scale(np_scale);
    np_unt.move((np_b_h*0 + np_b_whd)*np_scale, np_offset + (np_b_w*1 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    np_oth.move((np_b_h*1 + np_b_whd)*np_scale, np_offset + (np_b_w*1 - np_b_whd)*np_scale).rotate(-90).scale(np_scale);
    
}

var np_call_back_add = null;
var np_call_back_del = null;
var np_call_back_unt = null;
var np_call_back_ent = null;
function np_add_listeners() {
    np_1.click   (function (e) { if(np_call_back_add){np_call_back_add('1');console.log('np_1');} e.stopPropagation(); e.preventDefault();});
    np_2.click   (function (e) { if(np_call_back_add){np_call_back_add('2');} e.stopPropagation(); e.preventDefault();});
    np_3.click   (function (e) { if(np_call_back_add){np_call_back_add('3');} e.stopPropagation(); e.preventDefault();});
    np_4.click   (function (e) { if(np_call_back_add){np_call_back_add('4');} e.stopPropagation(); e.preventDefault();});
    np_5.click   (function (e) { if(np_call_back_add){np_call_back_add('5');} e.stopPropagation(); e.preventDefault();});
    np_6.click   (function (e) { if(np_call_back_add){np_call_back_add('6');} e.stopPropagation(); e.preventDefault();});
    np_7.click   (function (e) { if(np_call_back_add){np_call_back_add('7');} e.stopPropagation(); e.preventDefault();});
    np_8.click   (function (e) { if(np_call_back_add){np_call_back_add('8');} e.stopPropagation(); e.preventDefault();});
    np_9.click   (function (e) { if(np_call_back_add){np_call_back_add('9');} e.stopPropagation(); e.preventDefault();});
    np_0.click   (function (e) { if(np_call_back_add){np_call_back_add('0');} e.stopPropagation(); e.preventDefault();});
    np_prd.click (function (e) { if(np_call_back_add){np_call_back_add('.');} e.stopPropagation(); e.preventDefault();});
    np_del.click (function (e) { if(np_call_back_del){np_call_back_del();} e.stopPropagation(); e.preventDefault();});
    np_oth.click (function (e) { if(np_call_back_add){np_call_back_add("'");} e.stopPropagation(); e.preventDefault();});
    np_unt.click (function (e) { if(np_call_back_unt){np_call_back_unt();} e.stopPropagation(); e.preventDefault();});
    np_ent.click (function (e) { if(np_call_back_ent){np_call_back_ent();} e.stopPropagation(); e.preventDefault();});
}

