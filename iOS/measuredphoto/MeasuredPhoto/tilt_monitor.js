// code to handle events when not in webview in app



// the below monitors the tilt of the device in an atempt to predict the orientation change animation, and hide the menu bar when it happens so we don't get a choppy experiance.
//    window.addEventListener("devicemotion", handleMotion, true);
//    function handleMotion(e) {
//        var xy_angle = Math.atan(Math.abs(e.accelerationIncludingGravity.y)/Math.abs(e.accelerationIncludingGravity.x));
//        // looks like if the sum of the absolute values of x and y acceleration w/ gravit is greater than 10 we want to hide menu bar. show otherwise
//        if (Math.abs(e.accelerationIncludingGravity.z) > 9) {
//            if ( ! document.body.contains(menu)) {document.body.appendChild(menu);}
//       }
//        else if ( xy_angle < .55 && !orientation_drawn_landsacep ) { //about to transition to landscape
//            if (document.body.contains(menu)) {document.body.removeChild(menu);}
//        }
//        else if ( xy_angle > .75 && orientation_drawn_landsacep) { //about to transition to portrait
//            if (document.body.contains(menu)) {document.body.removeChild(menu);}
//        }
//        else {
//            if ( ! document.body.contains(menu)) {document.body.appendChild(menu);}
//        }
//    }
