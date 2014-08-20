function [R T] = decompose_F(F, K, p1, p2)

debug = 0;

%Validate with synthesized dR dT
E = K'*F*K;
p1p = inv(K)*[p1 repmat(1, [size(p1,1) 1])]';
p2p = inv(K)*[p2 repmat(1, [size(p2,1) 1])]';
if debug
    p1
    p1p
    p2
    p2p
end

[R T] = decompose_E(E, p1p(:,1), p2p(:,1), debug);
if debug
    R
    T
end

%[R1 T1] = masks_decompose_E(E, p1p, p2p);
%if debug
%    R1
%    T1
%end
%R = R1;
%T = T1;

function [R T] = decompose_E(E, p1, p2, debug)

[U D V] = svd(E);

W = [0 -1 0; 1 0 0; 0 0 1];
Z = [0 1 0; -1 0 0; 0 0 0];

if debug
    U
    D
    V
end

Rzp = W;
Rzn = -W; Rzn(3,3) = 1;

% Determinant of U and V should be positive.
detU = det(U);
detV = det(V);
if debug
    fprintf('det(U) = %f, det(V) = %f\n', detU, detV);
end

if detU < 0 && detV < 0
    U = -U;
    V = -V;
elseif detU < 0 && detV > 0
    U = -U*Rzn;
    V = V*Rzp;
elseif detU > 0 && detV < 0
    U = U*Rzn;
    V = -V*Rzp;
end

T = U(:, 3);
R1 = U*W*V';
R2 = U*W'*V';

if debug
    fprintf('After making det = +1\n')
    U
    V
    R1
    R2
end

nvalid = 0;
[d1 d2 valid] = validate_RT(R1, T, p1, p2, debug);
if valid > 0
    R = R1;
    T = T;
    nvalid = nvalid + 1;
end
[d1 d2 valid] = validate_RT(R2, T, p1, p2, debug);
if valid > 0
    R = R2;
    T = T;
    nvalid = nvalid + 1;
end
[d1 d2 valid] = validate_RT(R1, -T, p1, p2, debug);
if valid > 0
    R = R1;
    T = -T;
    nvalid = nvalid + 1;
end
[d1 d2 valid] = validate_RT(R2, -T, p1, p2, debug);
if valid > 0
    R = R2;
    T = -T;
    nvalid = nvalid + 1;
end

if debug
    fprintf('Solution %d valid\n', nvalid);
end

function [d1 d2 valid] = validate_RT(R, T, p1, p2, debug)

if debug
    fprintf('validate\n');
    R
    T
end

o1 = T;
o2 = [0 0 0]';
% -T is That' in skew symmetric
p1p = R*p1 + o1;

[pa, pb] = line_line_intersect(o1, p1p, o2, p2);
intersection = (pa + pb) / 2;
d12 = norm(intersection - pa);
d22 = norm(intersection - pb);

if debug
    intersection
    pa
    pb
    fprintf('%f %f\n', d12, d22);
end

d1 = norm(intersection)*sign(intersection(3));
d2 = R'*(intersection - T);
d2 = norm(d2)*sign(d2(3));

valid = d1 > 0  && d2 > 0;
if valid && debug
    distances
end
