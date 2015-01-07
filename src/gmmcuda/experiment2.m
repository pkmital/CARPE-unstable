%EXPERIMENT2 - tests increasing numbers of clusters and data dimensionality, cpu vs gpu
%
% Andrew Harp (andrew.harp@gmail.com)
% http://andrewharp.com/gmmcuda

num_clusts = 1:16;
num_dims = 1:16;

[x, y] = meshgrid(num_clusts, num_dims);

num_data = 50000;
num_steps = 5;

gtimes = zeros(size(x));
ctimes = zeros(size(x));

for i=1:size(gtimes, 1)
  for j=1:size(gtimes, 2)
    [ctimes(i, j), gtimes(i, j)] = gmm_test(x(i, j), y(i, j), num_data, num_steps);
  end
end

ratios = ctimes ./ gtimes


%% Plotting Stuff
figure
surf(ratios)
title(sprintf('Performance comparison with %d data points', num_data));

set(gca, 'XLim', [0, max(num_clusts)]); hold on;
set(gca, 'XTick', 1:max(num_clusts)); hold on;
xlabel('Number of clusters');

set(gca, 'YLim', [0, max(num_dims)]); hold on;
set(gca, 'YTick', 1:max(num_dims)); hold on;
ylabel('Dimensionality of data');

set(gca, 'ZTick', 0:20:200); hold on;
zlabel('Ratio of CPU to GPU elapsed time');