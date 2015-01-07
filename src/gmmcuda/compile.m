% Compiles the Mex interface.
% See http://developer.nvidia.com/object/matlab_cuda.html for more information.
%
% Andrew Harp (andrew.harp@gmail.com)
% http://andrewharp.com/gmmcuda
mex cpugmm.cpp
nvmex -f nvmexopts.bat gmm.cu -IC:\cuda\include -LC:\cuda\lib -lcudart