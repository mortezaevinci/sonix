%%% IMAGING

%PARAMETERS
numofvoxels=1; 
numofpoints=1000;
numoftra=128;
pta=4; %points to average

%change delay for phocs
usstart=4;
usend=1000;

threshcutoff=10;

% tic
% % load sonixdata (all voxels per channel)
% % RF = zeros(numoftra, );
% for i=0:numoftra-1
%     [S,ERRMSG]=sprintf('CH%03d.daq',i);
%     fid = fopen(S, 'r');
%     header = fread(fid,19,'int32');
%     A= (fread(fid, [1, header(2)*header(3)], 'int16'));
%     RF(i+1,:)=A;
%     fclose(fid);
%     clear A;
% end
% toc 
close all
 for v= 480:1:500 
NM1=RF(:,v:999+v);
ab = NM1';
NM2=reshape(ab,1,128000);

I2=sum(reshape(NM2',[pta ((numofpoints*numoftra)/pta)]));


% I2b1=reshape(I2,((numofpoints)/pta)/numoftra,numoftra);
% I2b=sum(reshape(I2,((numofpoints)/pta)/numoftra,numoftra));
% I2b=I2b/((((numofpoints)/pta))/numoftra);
% 
% I2b=repmat(I2b,((numofpoints)/pta)/numoftra,1);
% I2b=I2b1-I2b;
% 
% I2b=NM2;
% %Trim unused points
% I2d=I2b((usstart/pta):(usend/pta),1:numoftra);
% I2e=reshape(I2d,1,((usend/pta)-(usstart/pta)+1)*numoftra);

%Subtract average over all transducers
I2a=I2-mean(I2);

%Rectify
I2c= abs(I2a);

%Threshold cutoff
I2c(I2c<threshcutoff)=0;

%M2c=vertcat(M2c,I2c);
%end;
%M2c(1,:)=[];

tic
object=(pseudoinv)*(I2c');
toc
object(object<0)=0;
object_physical=reshape(object,[50 50]); % reshape to object space dimension
object_image=object_physical(:,:); % extracting a plane (11) for plotting

%%% Plot image

figure; colormap(gray);
imagesc(object_image); axis off;

 end
return;



% %Reshape and subtract average per transducer
numofpoints=1000;
numoftra=128;
OBJECT = zeros(100,100,1,n);
usstart=4; usend=1000;
threshold = 30;
% thresh = 2e-6;
% pseudoinv = pseudoinv_10000plane_pta4_jan24;
% pseudoinv(abs(pseudoinv)<thresh)=0;

for d = 487; %CHANGE DELAY
NM1=RF(:,d:999+d); 
NM2=reshape(NM1',1,128000);

C2=sum(reshape(NM2',pta,((numoftra*numofpoints)/pta))); 
C2b1=reshape(C2,((numoftra*numofpoints)/pta)/numoftra,numoftra);
C2b=sum(reshape(C2,((numoftra*numofpoints)/pta)/numoftra,numoftra));
C2b=C2b/(((numoftra*numofpoints)/pta)/numoftra);
C2b=repmat(C2b,((numoftra*numofpoints)/pta)/numoftra,1);
C2b=C2b1-C2b;

%Trim unused points
C2d=C2b((usstart/pta):(usend/pta),1:numoftra);
C2e=reshape(C2d,1,((usend/pta)-(usstart/pta)+1)*numoftra);

%Subtract average over all transducers
C2a=C2e-mean(C2e);

%Rectify
D2c= abs(C2a); %check where to threshold

%Threshold cutoff
D2c(D2c<threshcutoff)=0;


%%% Plot image
object=(pseudoinv_10000plane_pta4_jan24)*D2c';
object(object<0)=0;
object_physical=reshape(object,[100 100]); % reshape to object space dimension
object_image=object_physical(:,:); % extracting a plane (11) for plotting
% OBJECT(:,:,1,d)=object_image;

figure; colormap(gray);
imagesc(object_image); axis off;
end

h = mat2gray(OBJECT);
montage(h);

