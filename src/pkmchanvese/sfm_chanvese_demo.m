%-- A demo script for sfm_chanvese.m
%--
%-- The script opens and image, prepares an initialization
%   and shows the final output after segmentation.

% load image
img = imread('airplane.png');

% prepare initialization
mask = zeros(size(img));
mask(86:218,109:238) = 1;

% set input parameters
lambda = 0.3;
iterations = 600;

% perform segmentation
[seg] = sfm_chanvese(img,mask,iterations,lambda);

% display results
subplot(2,2,1)
imagesc(img); axis image; colormap gray;
title('The original image');

subplot(2,2,2)
imagesc(mask); axis image; colormap gray;
title('The initialization');

subplot(2,2,3)
imagesc(seg); axis image; colormap gray;
title('The final segmenatation output');

subplot(2,2,4)
imagesc(img); axis image; colormap gray;
hold on;
contour(seg,[0 0],'r','linewidth',3);
hold off;
title('The image with the segmentation shown in red');

