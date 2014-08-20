function info = stereo_load_debug_info(basename)

% Total hack because run() doesn't work when the filename starts with
% a number (i.e. is not a valid function name)
debugFilename = sprintf('%s-debug-info.m', basename);
debugfid = fopen(debugFilename);
contents = fread(debugfid, inf, 'char=>char');
eval(contents); % loads debug variables including F
fclose(debugfid);
clear debugfid;
vars = who;

info = struct();
for i = 1:length(vars)
    info = setfield(info, vars{i}, eval(vars{i}));
end
