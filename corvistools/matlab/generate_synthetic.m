P1 = [1 -0.5 -5 1]';
P2 = [1  1.5 -5 1]';

Tcw = [0 0 0 1]';
Twc = -Tcw;
Twc(4) = 1;

% R and T in the world frame
% from the origin to the camera
R1 = eye(4);
T1 = [0 0 0 1]';

% from the origin to the camera
angle = 0;
R2 = [cos(angle) -sin(angle) 0 0;
      sin(angle) cos(angle) 0 0;
      0 0 1 0;
      0 0 0 1;];
T2 = [0.1 0.5 0 1]';

%R world => camera
Rcw = [0 -1 0 0;
      -1 0 0 0;
      0 0 -1 0 ;
      0 0 0 1 ];

Rwc = Rcw';

% camera parameters
f = 620
center = [320 240]';

distance = norm(P1 - P2)

p11 = Rcw * R1 * (P1 - T1 + Tcw);
p11 = p11 ./ p11(3);
p11 = p11 * f;
p1_1 = p11(1:2) + center

p12 = Rcw * R2 * (P1 - T2 + Tcw);
p12 = p12 ./ p12(3);
p12 = p12 * f
p1_2 = p12(1:2) + center

p21 = Rcw * R1 * (P2 - T1 + Tcw);
p21 = p21 ./ p21(3);
p21 = p21 * f
p2_1 = p21(1:2) + center

p22 = Rcw * R2 * (P2 - T2 + Tcw);
p22 = p22 ./ p22(3);
p22 = p22 * f
p2_2 = p22(1:2) + center

close all
figure(1); 
hold on;
plot(0, 0, 'g.');
plot(640, 480, 'g.');
plot(p1_1(1), p1_1(2), 'rx'); 
plot(p2_1(1), p2_1(2), 'bx');
axis ij
axis image

figure(2); 
hold on;
plot(0, 0, 'g.');
plot(640, 480, 'g.');
plot(p1_2(1), p1_2(2), 'rx'); 
plot(p2_2(1), p2_2(2), 'bx'); 
axis ij
axis image
