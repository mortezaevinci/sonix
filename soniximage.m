%%% IMAGING 
function [OBJECT, object_image, imageseries] = soniximage(folder, numofframes,numoftra,numofpoints,delay,pta,xsize,ysize,threshcutoff,pseudoinv)
% Parameters
% numofframes=1; 
% numofpoints=1000;
% numoftra=128;
% pta=1; %points to average
% delay =0;
% xsize = 40; ysize = 40;
% threshcutoff = 10;

tic
% load sonixdata (all voxels per channel)
k = delay;
RF = zeros(1,numofpoints*numofframes);
for i=0:numoftra-1
    [S,ERRMSG]=sprintf('%sCH%03d.daq',folder, i);
    fid = fopen(S, 'r');
    header = fread(fid,19,'int32');
    A= (fread(fid, [1, header(2)*header(3)], 'int16'));
    RF(1,:)=A(1,(1+k):((numofframes*numofpoints)+k));
    fclose(fid);
    clear A;

    %Threshold RF
    RF(abs(RF)<=threshcutoff)=0; 
    
    NM1=RF(1,:);
    NM2=reshape(NM1,numofpoints,numofframes);
     
    %Downsample
    C1 = reshape(NM2,pta,(numofpoints*numofframes/pta));
    C2 = mean(C1,1);
    C2a = reshape(C2,numofpoints/pta,numofframes); 

    %Assemble Matrix
    D2c(:,i*numofpoints/pta+1:(i+1)*numofpoints/pta) = C2a'; 
%     clear RF;
end
toc ;
% return;

for j = 1:numoftra
%Subtract average per transducer
E2a = mean(D2c(:,(j-1)*numofpoints/pta+1:j*numofpoints/pta),2);
E2a = repmat(E2a,1,numofpoints/pta);
D2d(:,(j-1)*numofpoints/pta+1:j*numofpoints/pta) = D2c(:,(j-1)*numofpoints/pta+1:j*numofpoints/pta)-E2a;
end
clear D2c

%Subtract average over all transducers
D2e = detrend(D2d,'constant');
clear D2d

%Rectify
E2c = abs(D2e); %check where to threshold
clear D2e

%Threshold cutoff
E2c(E2c<threshcutoff)=0;

OBJECT = zeros(xsize,ysize,1,numofframes);

%%% Plot image
object=(pseudoinv)*E2c'; %load proper pseudoinv
object(object<0)=0;
object_physical=reshape(object,[xsize ysize]); % reshape to object space dimension

object_image=object_physical(:,:); % extracting a plane for plotting
OBJECT(:,:,1,numofframes)=object_image;
imageseries(:,numofframes) = object;

% end

% figure; colormap(gray);
% imagesc(object_image); axis off;
% figure, imdisp(OBJECT); % use arrow keys to scroll through image set with single image
% figure, imdisp(OBJECT, 'Size', 1);
