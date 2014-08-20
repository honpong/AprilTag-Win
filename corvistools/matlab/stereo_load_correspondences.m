function correspondences = stereo_load_correspondences(basename)

cfilename = sprintf('%s-topn-correspondences.csv', basename);
if(exist(cfilename))
    correspondences = csvread(cfilename);
    % ignore last column due to a bug
    correspondences = correspondences(:,1:end-1);
else
    cfilename = sprintf('%s-correspondences.csv', basename);
    correspondences = csvread(cfilename, 1, 0);
end

% convert to matlab coordinates
correspondences(:, 1:2) = correspondences(:,1:2) + 1;
correspondences(:, 3:4:end) = correspondences(:, 3:4:end) + 1;
correspondences(:, 4:4:end) = correspondences(:, 4:4:end) + 1;
