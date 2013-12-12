function pt = calibrate_im_point(point, center, focal_length, k1, k2, k3)

if nargin < 4
    k1 = 0;
    k2 = 0;
    k3 = 0;
end

pt = (point(1:2) - center(1:2)) ./ focal_length;

r2 = sum(pt.*pt);
r4 = r2 * r2;
r6 = r4 * r2;

kr = 1 + r2 * k1 + r4 * k2 + r6 * k3;

pt = pt ./ kr;

r2 = sum(pt.*pt);
r4 = r2 * r2;
r6 = r4 * r2;

kr = 1 + r2 * k1 + r4 * k2 + r6 * k3;

pt = pt ./ kr;

pt = [pt; 1; 1];
