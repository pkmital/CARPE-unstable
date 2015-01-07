%EXPERIMENT3 - tests increasing numbers of data points, GPU only
%
% Andrew Harp (andrew.harp@gmail.com)
% http://andrewharp.com/gmmcuda

num_clusts = 10;
num_dims = 8;
num_steps = 10;
num_samples = (10.^(3:.1:6))'
num_tries = 10

totimes = zeros(size(num_samples));
gtimes = zeros(size(num_samples));
fromtimes = zeros(size(num_samples));

for i=1:numel(num_samples)
  num_data = num_samples(i);
  [totimes(i), gtimes(i), fromtimes(i)] = ...
      gmm_test(num_clusts, num_dims, num_samples(i), num_steps, num_tries, 0);
end


%% Plotting Stuff
figure
semilogx(0, 0); hold on; 
h = area(num_samples, [gtimes, totimes, fromtimes]); hold on; 

legend(h, 'GPU Computation', 'Transfer to GPU Overhead', 'Transfer from GPU Overhead', 'Location', 'NorthWest');
title(sprintf('Time allocation breakdown with %d clusters, %d dimensionality, %d EM iteration', num_clusts, num_dims, num_steps));

ylabel('Elapsed Time (seconds)');
xlabel('Number of Data Points')