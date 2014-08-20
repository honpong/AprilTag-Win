function compare_decomp()

decompose('~/Desktop/meshtest/dont wait dont use motion/2014-08-13_19-38-00-stereo');
decompose('~/Desktop/meshtest/dont wait dont use motion/2014-08-13_19-38-33-stereo');
decompose('~/Desktop/meshtest/dont wait dont use motion/2014-08-13_19-39-06-stereo');
decompose('~/Desktop/meshtest/dont wait dont use motion/2014-08-13_19-39-27-stereo');
decompose('~/Desktop/meshtest/dont wait dont use motion/2014-08-13_19-39-51-stereo');
decompose('~/Desktop/meshtest/dont wait use motion/2014-08-13_19-04-21-stereo');
decompose('~/Desktop/meshtest/dont wait use motion/2014-08-13_19-04-46-stereo');
decompose('~/Desktop/meshtest/dont wait use motion/2014-08-13_19-05-13-stereo');
decompose('~/Desktop/meshtest/dont wait use motion/2014-08-13_19-24-11-stereo');
decompose('~/Desktop/meshtest/dont wait use motion/2014-08-13_19-24-31-stereo');
decompose('~/Desktop/meshtest/dont wait use motion/2014-08-13_19-24-51-stereo');
decompose('~/Desktop/meshtest/wait before start dont use motion/2014-08-13_19-41-57-stereo');
decompose('~/Desktop/meshtest/wait before start dont use motion/2014-08-13_19-42-36-stereo');
decompose('~/Desktop/meshtest/wait before start dont use motion/2014-08-13_19-43-01-stereo');
decompose('~/Desktop/meshtest/wait before start dont use motion/2014-08-13_19-43-32-stereo');
decompose('~/Desktop/meshtest/wait before start dont use motion/2014-08-13_19-43-57-stereo');
decompose('~/Desktop/meshtest/wait before start dont use motion/2014-08-13_19-44-26-stereo');
decompose('~/Desktop/meshtest/wait before start use motion/2014-08-13_18-47-48-stereo');
decompose('~/Desktop/meshtest/wait before start use motion/2014-08-13_18-48-16-stereo');
decompose('~/Desktop/meshtest/wait before start use motion/2014-08-13_18-48-40-stereo');
decompose('~/Desktop/meshtest/wait before start use motion/2014-08-13_18-49-08-stereo');

function gt_err = decompose(basename)

info = stereo_load_debug_info(basename);

[I1 I2] = stereo_load_images(basename);
[Fgt X1 X2] = stereo_ransac_F(I1, I2);
K = [info.focal_length 0 info.center_x; 0 info.focal_length info.center_y; 0 0 1];

Tscale = norm(info.dT);

[Rgt Tgt] = decompose_F(Fgt, K, X1, X2);

[Re Te] = decompose_F(info.F_eight_point(1:3,1:3), K, X1, X2);
[Rm Tm] = decompose_F(info.F_motion(1:3,1:3), K, X1, X2);

Tgts = Tgt*Tscale;
fprintf('%s\n', basename);
fprintf('ransac had %d inliers\n', size(X1, 1))
for i = 1:3
    fprintf('%g\t%g\n', Tgts(i), info.dT(i));
end

deltaRgt = Rm'*Rgt;

dT = info.dT(1:3);
[v i] = max(abs(dT));
if sign(dT(i)) ~= sign(Tgts(i))
    Tgts = -Tgts;
end

delta = Tgts - dT;
error_pct = 100*norm(Tgts - dT)/norm(dT);
theta = acos((trace(deltaRgt)-1)/2) * 180/pi;
fprintf('theta: %g degrees\n', theta);
fprintf('error: %g%% over %gcm baseline (delta %f %f %f)\n', error_pct, Tscale*100, delta(1), delta(2), delta(3));
