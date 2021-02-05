
% 128 channels by 2000 points, change the number of points if changed in
% the DAQ Demo software
[S,ERRMSG]=sprintf('C:/sonix/data/block00000.dat');
fid = fopen(S, 'r');
header = fread(fid,9,'int32')
blockNum    = header(1);
sumTriggers = header(2);
numTriggers = header(3);
numChannels = header(4);
numPoints   = header(5);
x   = header(6);
y   = header(7);
z   = header(8);
u   = header(9);

if (sumTriggers == 1)
A = fread(fid, [numPoints, numChannels], 'int32');
else
A = fread(fid, [numPoints*numTriggers, numChannels], 'int32');
end

fclose(fid);
% B = reshape(A, numPoints, numChannels);

figure(1); imagesc(A);
figure(2); plot(A(:,19)','DisplayName',blockNum,'YDataSource','A');
%plot(A,'DisplayName','A','Counts','A');figure(gcf)

return;

R = 128;
[N, C] =size(A);
RF=zeros(R,C);

for i=0:127
    [S,ERRMSG]=sprintf('C:/jcarson/sonix/data/CH%03d.daq',i);
    fid = fopen(S, 'r');
    header = fread(fid,19,'int32');
    A= (fread(fid, [1, header(2)*header(3)], 'int16'));
    RF(i+1,:)=A;
    fclose(fid);
end