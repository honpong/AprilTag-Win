function check_decomposition(basename, p1, p2)

info = stereo_load_debug_info(basename);

%[I1 I2] = stereo_load_images(basename);
%[Fgt X1 X2] = stereo_ransac_F(I1, I2);
K = [info.focal_length 0 info.center_x; 0 info.focal_length info.center_y; 0 0 1];

Tscale = norm(info.dT);

%[Rgt Tgt] = decompose_F(Fgt, K, X1, X2);

[Re Te] = decompose_F(info.F_eight_point(1:3,1:3), K, p1, p2);

keyboard;
%[Rm Tm] = decompose_F(info.F_motion(1:3,1:3), K, X1, X2);
