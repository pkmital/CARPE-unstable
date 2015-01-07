% [seg Lz] = sfm_chanvese(img,mask,iterations,lambda)
% 
% img - any image (2D or 3D).  color images will be 
%       converted to grayscale.
%
% mask - binary image representing initialization.  
%        (1's foreground, 0's background)
% 
% iterations - number of iterations to run
%
% lambda - relative weighting of curve smoothness 
%          (lambda will ususally be between [0,1])
%
% seg - binary map of resulting segmentation 
%       (1's foreground, 0's background)
%
% Lz - A list of the indexes of points on the zero level set.
%
% --------------------------------------
% written by Shawn Lankton (4/17/2009) - www.shawnlankton.com
%
function [seg Lz] = sfm_chanvese(img,mask,iterations,lam)
  
  % ensure mex is compiled
  if(isempty(which('sfm_chanvese_mex')))
    compile_sfm_chanvese
  end

  % check for color image and convert to grayscale
  if(numel(size(img))==3) %if image is 3D
    if(size(img,3)==3)     %z dim is 3... probably color
      img = rgb2gray(img);%rescale to grayscale
    end
  end
  
  % mex is expecting img to be a double
  img = double(img);      
  mask = double(mask);
  
  if(any(size(img)~=size(mask)))
    error('img and mask must be the same size');
  end
  
  % default value of lambda is 0.1
  if(~exist('lam','var')) 
    lam = 0.1; 
  end
  lam = double(lam);

  % default value of lambda is 0.1
  if(~exist('iterations','var')) 
    iterations = 100; 
  end
  iterations = double(iterations);

  % perform segmentation
  [phi c] = sfm_chanvese_mex(img,mask,iterations,lam);
  
  % get labelmap from signed distance function
  seg = phi<=0;
  Lz  = uint32(c)+1;
  
  
  
  
