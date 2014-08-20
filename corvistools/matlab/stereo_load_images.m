function [I1 I2] = stereo_load_images(basename)

if(exist(sprintf('%s-current.pgm', basename)))
    I1 = imread(sprintf('%s-current.pgm', basename));
    I2 = imread(sprintf('%s-previous.pgm', basename));
elseif(exist(sprintf('%s-reference-rectified.pgm', basename)))
    I1 = imread(sprintf('%s-reference-rectified.pgm', basename));
    I2 = imread(sprintf('%s-target-rectified.pgm', basename));
else
    I1 = imread(sprintf('%s-reference.pgm', basename));
    I2 = imread(sprintf('%s-target.pgm', basename));
end

