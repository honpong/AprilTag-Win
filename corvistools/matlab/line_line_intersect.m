function [pa pb] = line_line_intersect(p1, p2, p3, p4)

pa = [0 0 0]';
pb = [0 0 0]';

p13 = p1 - p3;
p43 = p4 - p3;
p21 = p2 - p1;
if abs(p43(1)) < eps && abs(p43(2)) < eps && abs(p43(3)) < eps
    fprintf('Ill coonditioned p43\n');
    return;
end

if abs(p21(1)) < eps && abs(p21(2)) < eps && abs(p21(3)) < eps
    fprintf('Ill coonditioned p21\n');
    return;
end

d1343 = p13' * p43; %p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
d4321 = p43' * p21; %p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
d1321 = p13' * p21; %p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
d4343 = p43' * p43; %p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
d2121 = p21' * p21; %p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

denom = d2121 * d4343 - d4321 * d4321;
if abs(denom) < eps
    fprintf('Parallel')
    return;
end

numer = d1343 * d4321 - d1321 * d4343;

mua = numer / denom;
mub = (d1343 + d4321 * mua) / d4343;

pa = p1 + mua*p21;
pb = p3 + mub*p43;
