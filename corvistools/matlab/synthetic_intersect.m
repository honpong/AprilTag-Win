% P1 = [1 -0.5 -5 1]';
% P2 = [1  1.5 -5 1]';

Tcw = [0 0 0 1]';
Twc = -Tcw;
Twc(4) = 1;

% R and T in the world frame
% Rotation from the origin to the camera
R1w = eye(4);
Rw1 = R1w';
T1w = [0 0 0 1]';
Tw1 = -T1w;
Tw1(4) = 1;

% Rotation from the origin to the camera
theta = 0;
R2w = [cos(theta) -sin(theta) 0 0;
      sin(theta) cos(theta) 0 0;
      0 0 1 0;
      0 0 0 1;];
%R2w = eye(4);
Rw2 = R2w';
T2w = [0.1 0.5 0 1]';
Tw2 = -T2w;

%R world => camera
Rcw = [0 -1 0 0;
      -1 0 0 0;
      0 0 -1 0 ;
      0 0 0 1 ];

Rwc = Rcw';

% camera parameters
f = 620;
center = [320 240]';

p1_gt = [382 116]';
% 45 degree rotation
%p2_gt = [328.7681 73.4056]';
p2_gt = [444 128.4]';

p1_gt_cal = calibrate_im_point(p1_gt, center, f)
p2_gt_cal = calibrate_im_point(p2_gt, center, f)

p1_gt_cal_transformed = Rw1*Rwc*p1_gt_cal + T1w + Twc;
p1_gt_cal_transformed(4) = 1;

o1_transformed = T1w + Twc;
o1 = o1_transformed;

p2_gt_cal_transformed = Rw2*Rwc*p2_gt_cal + T2w + Twc;
p2_gt_cal_transformed(4) = 1;

o2_transformed = T2w + Twc;
o2 = o2_transformed;

close all
figure(1); hold on

p1 = p1_gt_cal_transformed;
p2 = p2_gt_cal_transformed;

d1 = p1 - o1;
coords = [o1 p1];
coords(:, 2) = o1 + (p1 - o1)*10
line(coords(1,:), coords(2,:), coords(3,:));

d2 = p2 - o2;
coords = [o2 p2];
coords(:, 2) = o2 + (p2 - o2)*10
line(coords(1,:), coords(2,:), coords(3,:));


plot3(o1(1), o1(2), o1(3), '--rs','LineWidth',2,...
                       'MarkerEdgeColor','k',...
                       'MarkerFaceColor','r',...
                       'MarkerSize',10)

plot3(o2(1), o2(2), o2(3), '--rs','LineWidth',2,...
                       'MarkerEdgeColor','k',...
                       'MarkerFaceColor','g',...
                       'MarkerSize',10)

