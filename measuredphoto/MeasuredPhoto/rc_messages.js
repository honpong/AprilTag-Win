//Copywrite (c) 2014 by RealityCap, Inc. Written by Jordan Miller for the exclusive use of RealityCap, Inc.

//RC messages places text content at the front of the top level SVG node for a fixed period of time

rcMessage = { contaner : null, text : null, box : null, message_color : '#fff', box_padding : 10 , html : null, html_contaner : null};


rcMessage.initalize = function () {
    rcMessage.contaner = draw.nested('message');
    rcMessage.html_contaner = draw.nested('message');
    rcMessage.contaner.size(window.innerWidth, window.innerHeight)
    rcMessage.html_contaner.size(window.innerWidth, window.innerHeight)
    rcMessage.box = rcMessage.contaner.rect(10, 10);
    rcMessage.html = rcMessage.html_contaner.foreignObject(100,100).attr({id: 'fobj'});
    rcMessage.html.size(window.innerWidth/1.05, window.innerHeight/1.15);
    rcMessage.html.center(window.innerWidth/2, window.innerHeight/2);
    rcMessage.text = rcMessage.contaner.text(' ').font({
                                                      family: rcMeasurements.font_family,
                                                      size: 18
                                                      , anchor: 'middle'
                                                      , leading: 1
                                                      }).fill({ color: rcMessage.message_color, opacity: 1 });
    if (draw.node.contains(rcMessage.contaner.node)) {draw.node.removeChild(rcMessage.contaner.node);}
    if (draw.node.contains(rcMessage.html_contaner.node)) {draw.node.removeChild(rcMessage.html_contaner.node);}

}

rcMessage.rotate = function() {
    //rotate and center correctly
    var current_rotation = draw_g.trans.rotation;
    
    if ((current_rotation >= -45 && current_rotation <= 45) || (current_rotation >= 320 && current_rotation <= 400)) { //portrate
        if(rcMessage.text){rcMessage.text.center(window.innerWidth/2 + rcMessage.text.node.offsetWidth/2 + 5, draw_g.cy() + window.innerHeight/2 - rcMessage.text.node.offsetHeight  - button_size);}
        rcMessage.html.size(window.innerWidth/1.05, (window.innerHeight-button_size)/1.05);
    }
    
    else if (current_rotation >= 140 && current_rotation <= 220) { //portrait upside down
        if(rcMessage.text){rcMessage.text.center(window.innerWidth/2 + rcMessage.text.node.offsetWidth/2 + 5, draw_g.cy() + window.innerHeight/2 - rcMessage.text.node.offsetHeight );}
        rcMessage.html.size(window.innerWidth/1.15, (window.innerHeight-button_size)/1.15);

    }
    else if (current_rotation >= 50 && current_rotation <= 135) { //lanscape
        if(rcMessage.text){rcMessage.text.center(window.innerWidth/2 + rcMessage.text.node.offsetWidth/2 - button_size/2, draw_g.cy() + window.innerWidth/2 - rcMessage.text.node.offsetHeight/2);}
        rcMessage.html.size((window.innerHeight-button_size)/1.05, window.innerWidth/1.05);

    }
    else { //lanscape
        if(rcMessage.text){rcMessage.text.center(window.innerWidth/2 + rcMessage.text.node.offsetWidth/2 + button_size/2, draw_g.cy() + window.innerWidth/2 - rcMessage.text.node.offsetHeight/2);}
        rcMessage.html.size((window.innerHeight-button_size)/1.05, window.innerWidth/1.05);

    }


    // draw dark border box with rounded edges
    if(rcMessage.text){rcMessage.box.size(rcMessage.text.node.offsetWidth+rcMessage.box_padding*2, rcMessage.text.node.offsetHeight+rcMessage.box_padding).fill('#000').radius(rcMessage.box_padding).opacity(0.5).center(rcMessage.text.cx()-rcMessage.box_padding/1.5, rcMessage.text.cy());}

    
    if(rcMessage.text){rcMessage.text.rotate(current_rotation, window.innerWidth/2, window.innerHeight/2);}
    if(rcMessage.text){rcMessage.box.rotate(current_rotation, window.innerWidth/2, window.innerHeight/2);}
    rcMessage.html.rotate(current_rotation, window.innerWidth/2, (window.innerHeight-button_size)/2);
    rcMessage.html.center(window.innerWidth/2, (window.innerHeight-button_size)/2);
    
}

rcMessage.postHTML = function(html, miliseconds_up) {
    if (!rcMessage.html_contaner) {rcMessage.initalize();}
    rcMessage.clearHTML();
    rcMessage.rotate();

    if (!draw.node.contains(rcMessage.html_contaner.node)) {draw.node.appendChild(rcMessage.html_contaner.node);}
    rcMessage.html.appendChild("div", {id: 'mydiv', innerHTML: html});

    window.setTimeout( rcMessage.clearHTML, miliseconds_up);
}

rcMessage.post = function (message, miliseconds_up) {
    if (!rcMessage.contaner || !rcMessage.text) {rcMessage.initalize();}
    rcMessage.clear();
    if (!draw.node.contains(rcMessage.contaner.node)) {draw.node.appendChild(rcMessage.contaner.node);}
    rcMessage.text = rcMessage.text.text(message)
    
    rcMessage.rotate();
    
    window.setTimeout( rcMessage.clear, miliseconds_up);
}

rcMessage.fadeOut = function () {

}

rcMessage.clearHTML = function(){
    rcMessage.html.remove();
    rcMessage.html = rcMessage.html_contaner.foreignObject(100,100).attr({id: 'fobj'});
    if (draw.node.contains(rcMessage.html_contaner.node)) {draw.node.removeChild(rcMessage.html_contaner.node);}
}

rcMessage.clear = function() {
    if (rcMessage.text) {rcMessage.text.text(' ');} //already clear
    if (draw.node.contains(rcMessage.contaner.node)) {draw.node.removeChild(rcMessage.contaner.node);}
}