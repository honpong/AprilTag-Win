//Copyright (c) 2014 Caterpillar

//RC messages places text content at the front of the top level SVG node for a fixed period of time

rcMessage = { contaner : null, text : null, box : null, message_color : '#fff', box_padding : 10 };


rcMessage.initalize = function () {
    rcMessage.contaner = draw.nested('message');
    rcMessage.contaner.size(window.innerWidth, window.innerHeight)
    rcMessage.box = rcMessage.box = rcMessage.contaner.rect(10, 10);
    rcMessage.text = rcMessage.contaner.text(' ').font({
                                                      family: rcMeasurements.font_family,
                                                      size: 18
                                                      , anchor: 'middle'
                                                      , leading: 1
                                                      }).fill({ color: rcMessage.message_color, opacity: 1 });
    if (draw.node.contains(rcMessage.contaner.node)) {draw.node.removeChild(rcMessage.contaner.node);}

}

rcMessage.post = function (message, miliseconds_up) {
    if (!rcMessage.contaner) {rcMessage.initalize();}
    rcMessage.clear();
    if (!draw.node.contains(rcMessage.contaner.node)) {draw.node.appendChild(rcMessage.contaner.node);}
    rcMessage.text = rcMessage.text.text(message)
    

    //rotate and center correctly
    var current_rotation = draw_g.trans.rotation;
    
    if ((current_rotation >= -45 && current_rotation <= 45) || (current_rotation >= 320 && current_rotation <= 400)) {
        rcMessage.text.center(window.innerWidth/2 + rcMessage.text.node.offsetWidth/2 + 5, draw_g.cy() + window.innerHeight/2 - rcMessage.text.node.offsetHeight);
    }
        
    else if (current_rotation >= 140 && current_rotation <= 220) {
        rcMessage.text.center(window.innerWidth/2 + rcMessage.text.node.offsetWidth/2 + 5, draw_g.cy() + window.innerHeight/2 - rcMessage.text.node.offsetHeight );
    }
    else if (current_rotation >= 50 && current_rotation <= 135) {
        rcMessage.text.center(window.innerWidth/2 + rcMessage.text.node.offsetWidth/2, draw_g.cy() + window.innerWidth/2 - rcMessage.text.node.offsetHeight/2);
    }
    else {
        rcMessage.text.center(window.innerWidth/2 + rcMessage.text.node.offsetWidth/2, draw_g.cy() + window.innerWidth/2 - rcMessage.text.node.offsetHeight/2);
    }

    // draw dark border box with rounded edges
    rcMessage.box.size(rcMessage.text.node.offsetWidth+rcMessage.box_padding*2, rcMessage.text.node.offsetHeight+rcMessage.box_padding).fill('#000').radius(rcMessage.box_padding).opacity(0.5).center(rcMessage.text.cx()-rcMessage.box_padding/1.5, rcMessage.text.cy());

    
    rcMessage.text.rotate(current_rotation, window.innerWidth/2, window.innerHeight/2);
    rcMessage.box.rotate(current_rotation, window.innerWidth/2, window.innerHeight/2);
    
    
    window.setTimeout(function () {
                        rcMessage.clear();
                      },
                      miliseconds_up
    );
}

rcMessage.fadeOut = function () {

}

rcMessage.clear = function() {
    if (rcMessage.text) {rcMessage.text.text(' ');} //already clear
    if (draw.node.contains(rcMessage.contaner.node)) {draw.node.removeChild(rcMessage.contaner.node);}
}