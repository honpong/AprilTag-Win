function topn_correspondences(basename)


info = stereo_load_debug_info(basename);
[I1 I2] = stereo_load_images(basename);
correspondences = stereo_load_correspondences(basename);

cfilename = sprintf('%s-topn-correspondences.csv', basename);
if(exist(cfilename))
    correspondences = csvread(cfilename);
    % ignore last column due to a bug
    correspondences = correspondences(:,1:end-1);
else
    cfilename = sprintf('%s-correspondences.csv', basename);
    correspondences = csvread(cfilename, 1, 0);
end

[scores_sorted ind] = sort(correspondences(:,5)); % sort by best match score
correspondences = correspondences(ind,:);
f1 = correspondences(:,1:2);
best_scores = correspondences(:,5);
no_matches = find(best_scores >= 1);
grid_ind = floor((f1-1)/8)+1;
depth_map = zeros(480/8, 640/8);
grid_ind = sub2ind(size(depth_map), grid_ind(:,2), grid_ind(:,1));
ind = find(best_scores < -0.5);
depth_map(grid_ind(ind)) = 1./correspondences(ind,6); % best match depth map
cvexShowDisparity(depth_map, 0);
%figure(2); clf; imagesc(depth_map);
%figure; imagesc(log(depth_map + 1))

F = info.F(1:3, 1:3);
[Fgt X1 X2] = stereo_ransac_F(I1, I2);

K = [info.focal_length 0 info.center_x; 0 info.focal_length info.center_y; 0 0 1];

figure(77); clf;
imshow(I1);
hold on
plot(X1(1, 1), X1(1, 2), 'go');
figure(78); clf;
imshow(I2);
hold on
plot(X2(1, 1), X2(1, 2), 'go');
decompose_F(Fgt, K, X1, X2);

% computes a transform to make correspondences with scanlines
[T1,T2] = estimateUncalibratedRectification(F,X1,X2,size(I1));
% disparity and reconstructScene
% rectifyStereoImages
% cvexRectifyImages(filename1, filename2)

figure(100); clf;
subplot(121); imshow(I1); 
title('Inliers and Epipolar Lines in First Image'); hold on;
plot(X1(:,1), X1(:,2), 'go')

% Compute the epipolar lines in the first image.
epiLines = epipolarLine(Fgt', X2);

% Compute the intersection points of the lines and the image border.
pts = lineToBorderPoints(epiLines, size(I1));

% Show the epipolar lines in the first image
line(pts(:, [1,3])', pts(:, [2,4])');

% Show the inliers in the second image.
subplot(122); imshow(I2);
title('Inliers and Epipolar Lines in Second Image'); hold on;
plot(X2(:,1), X2(:,2), 'go')

% Compute and show the epipolar lines in the second image.
epiLines = epipolarLine(Fgt, X1);
pts = lineToBorderPoints(epiLines, size(I2));
line(pts(:, [1,3])', pts(:, [2,4])');
title('Ground truth');
truesize;

% now do the same thing with the F we had determined
figure(101); clf;
subplot(121); imshow(I1); 
title('Inliers and Epipolar Lines in First Image'); hold on;
plot(X1(:,1), X1(:,2), 'go')

% Compute the epipolar lines in the first image.
epiLines = epipolarLine(F', X2);

% Compute the intersection points of the lines and the image border.
pts = lineToBorderPoints(epiLines, size(I1));

% Show the epipolar lines in the first image
line(pts(:, [1,3])', pts(:, [2,4])');

% Show the inliers in the second image.
subplot(122); imshow(I2);
title('Inliers and Epipolar Lines in Second Image'); hold on;
plot(X2(:,1), X2(:,2), 'go')

% Compute and show the epipolar lines in the second image.
epiLines = epipolarLine(F, X1);
pts = lineToBorderPoints(epiLines, size(I2));
line(pts(:, [1,3])', pts(:, [2,4])');

title('Estimated');
truesize;

%%%%%%%%%%%%%%%%%%%%%
plot_epipolar_topn(11, I1, I2, Fgt, F, correspondences(1,:));
plot_epipolar_topn(12, I1, I2, Fgt, F, correspondences(100,:));
plot_epipolar_topn(13, I1, I2, Fgt, F, correspondences(1000,:));

%%%%%%%%%%%%%%%%%%%%%%
% plot correspondences
if 0
figure(10); clf; imshow(I1); hold on; 
%plot(f1(:,1), f1(:,2), 'g.');
%plot(f1(no_matches,1), f1(no_matches,2), 'ro');
plot(f1(1:100,1), f1(1:100,2), 'r+');
title('no matches in red o with green . best 100 in g with a red +');

plot_topn(14, I1, I2, correspondences(1,:));
plot_topn(15, I1, I2, correspondences(100,:));
plot_topn(16, I1, I2, correspondences(1000,:));
end

keyboard;

function plot_epipolar_topn(fignum, I1, I2, Fgt, F, correspondences)

figure(fignum); clf;
dh1 = max(size(I2,1)-size(I1,1),0) ;
dh2 = max(size(I1,1)-size(I2,1),0) ;
imagesc([padarray(I1,dh1,'post') padarray(I2,dh2,'post')]) ; hold on;
o = size(I1,2) ;
plot(correspondences(1), correspondences(2), 'g.');
plot(correspondences(3:4:end)+o, correspondences(4:4:end), 'ro'); 
colormap gray
axis image off ;

epipolar_line_gt = epipolarLine(Fgt, correspondences(1:2));
pts = lineToBorderPoints(epipolar_line_gt, size(I2));
pts(1) = pts(1) + o;
pts(3) = pts(3) + o;
line(pts(:, [1,3])', pts(:, [2 4])', 'Color', 'b');

% F should be add [1 1] pixel, then subtract at the end
epipolar_line = epipolarLine(F, correspondences(1:2));
pts = lineToBorderPoints(epipolar_line, size(I2));
pts(1) = pts(1) + o;
pts(3) = pts(3) + o;
line(pts(:, [1,3])', pts(:, [2 4])', 'Color', 'r');
title('Ground truth line in blue');

truesize;


function plot_topn(fignum, I1, I2, correspondences)

dh1 = max(size(I2,1)-size(I1,1),0) ;
dh2 = max(size(I1,1)-size(I2,1),0) ;

figure(fignum) ; clf ;
imagesc([padarray(I1,dh1,'post') padarray(I2,dh2,'post')]) ; hold on;
o = size(I1,2) ;
plot(correspondences(1), correspondences(2), 'g.');
plot(correspondences(3:4:end)+o, correspondences(4:4:end), 'r.'); 
depths = correspondences(6:4:end)
colormap gray
axis image off ;
