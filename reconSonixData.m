
% run jc_sonix in a loop - provides a way to repetitively read the daq 
%load('pinv_1600pt_pta4_15psingval.mat','pseudoinv')
root_folder = 'c:\data\';

del_cmd = 'del /Q C:\data\*.*';

acq_cmd = 'C:\sonix\Release\jc_sonix divisor=15 numSamples=2000 lineDuration=10 verbose=1 externalTrigger=1 data=';

% change to the directory first
% md_cmd = ['cd ', root_folder]
% [status,result] = dos(md_cmd);
% status
% result

% figure; colormap(gray);

for i=0:20
    
%     [status,result] = dos(del_cmd);
%     status
%     result

    md_cmd = [root_folder, num2str(i)]
    mkdir(md_cmd);

    temp_cmd = strcat(acq_cmd, root_folder, num2str(i), ' acquire=0')
    [status,result] = dos(temp_cmd);

    %status
    %result
    subfolder = [root_folder, num2str(i), '\']
    [OBJECT, object_image, imageseries] = soniximage(subfolder, 1,128,1000,0,4,40,40,10,pseudoinv);

    imagesc(object_image); axis off;

    
end