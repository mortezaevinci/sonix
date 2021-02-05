

% 128 channels by 2000 points, change the number of points if changed in
% the DAQ Demo software
[S,ERRMSG]=sprintf('C:/sonix/data/CH000.daq');
fid = fopen(S, 'r');
header = fread(fid,19,'int32');
A= (fread(fid, [1, header(2)*header(3)], 'int16'));
fclose(fid);
R = 128;
[N, C] =size(A);
RF=zeros(R,C);

for i=0:127
    [S,ERRMSG]=sprintf('C:/sonix/data/CH%03d.daq',i);
    %[S,ERRMSG]=sprintf('CH%03d.daq',i);
    fid = fopen(S, 'r');
    header = fread(fid,19,'int32');
    A= (fread(fid, [1, header(2)*header(3)], 'int16'));
    RF(i+1,:)=A;
    fclose(fid);
end

% max(max(RF))
% min(min(RF))

%figure;
% plot(RF(3,:));
%imagesc(RF);

 