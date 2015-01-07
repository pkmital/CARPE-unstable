%% Parag K. Mital
%% Aug 2009

%% test out which way is the fastest
num_iterations = 10;
times = zeros(4);
show_images = 1;

%% load an image
imname = 'Blue1.jpg';
im=imread(imname);
[r c d] = size(im);

%% initialize the thing
yasift('open');

%% set the parameters
yasift('params', {'e', '10'});

%% Try the grayscale version 
for it = 1 : num_iterations
    tic

    % get the grayscale and convert to row major
    imgray = rgb2gray(im)';
    imgray(:);
    imgray = reshape(imgray, [r,c]);

    [descriptors keys]=yasift(imgray);
    times(1) = times(1) + toc;

    x=keys(1,:);
    y=keys(2,:);
    scale=keys(3,:);
    ori=keys(4,:);

    if show_images
    figure(1); imshow(im), hold on
    plot(x,y,'ro');
    hold off
    end
end

%% Try the rgb version
for it = 1 : num_iterations
    tic

    % convert to row major
    im2 = reshape(permute(im, [3,2,1]), [r,c,d]);

    [descriptors keys]=yasift(im2);
    times(2) = times(2) + toc;

    x=keys(1,:);
    y=keys(2,:);
    scale=keys(3,:);
    ori=keys(4,:);

    if show_images
    figure(2); imshow(im), hold on
    plot(x,y,'ro');
    hold off
    end
end

%% Try the original version
for it = 1 : num_iterations
    tic
    [descriptors keys]=yasift(imname);
    times(3) = times(3) + toc;

    x=keys(1,:);
    y=keys(2,:);
    scale=keys(3,:);
    ori=keys(4,:);

    if show_images
    figure(3); imshow(im), hold on
    plot(x,y,'ro');
    hold off
    end
end
%% Try the original version after reading and writing the image
for it = 1 : num_iterations
    tic
    filename = [imname(1:length(imname)-4) '_1.jpg'];
    imwrite(im, filename);

    [descriptors keys]=yasift(filename);
    times(4) = times(4) + toc;

    x=keys(1,:);
    y=keys(2,:);
    scale=keys(3,:);
    ori=keys(4,:);

    if show_images
    figure(4); imshow(im), hold on
    plot(x,y,'ro');
    hold off
    end
end

%% plot the results

figure(5), bar(0.5:1:3.5, times)
%% close the thing (though it doesn't seem to leave until MATLAB exits)
yasift('destroy')