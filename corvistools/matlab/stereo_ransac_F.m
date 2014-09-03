%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Compute ground truth homography
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [Fgt X1 X2] = stereo_ransac_F(I1, I2)

[fs1 ds1] = vl_sift(im2single(I1));
[fs2 ds2] = vl_sift(im2single(I2));
[matches, scores] = vl_ubcmatch(ds1,ds2) ;
X1 = fs1(1:2,matches(1,:))' ;
X2 = fs2(1:2,matches(2,:))' ;

[Fgt inliers]=estimateFundamentalMatrix(X1, X2,...
       'Method', 'RANSAC', 'NumTrials', 2000, 'DistanceThreshold', 0.5);

ind = find(inliers == 1);
X1 = X1(ind,:);
X2 = X2(ind,:);
