%EXPERIMENT1 - tests increasing numbers of data points, CPU vs GPU
%
% Andrew Harp (andrew.harp@gmail.com)
% http://andrewharp.com/gmmcuda

num_clusts = 10;
num_dims = 8;
num_steps = 5;
num_samples = (10.^(3:0.25:6))';
num_tries = 2;

ctimes = zeros(size(num_samples));
gtimes = zeros(size(num_samples));
for i=1:numel(num_samples)
   num_data = num_samples(i);
   [ctimes(i), gtimes(i)] = gmm_test(num_clusts, num_dims, num_samples(i), num_steps, num_tries);
end

ratios = ctimes./gtimes


%% Plotting Stuff
figure

max_val = ceil(max([max(ratios), max(ctimes)]));
max_tick = 900;
[AX,H1,H2] = plotyy(num_samples, ratios, num_samples, ctimes, 'semilogx'); hold on; 

for ax=AX
  axes(ax); hold on;
  set(gca, 'YLim', [0, max_val]); hold on;

  set(gca, 'YTick', 0:1:max_tick); hold on;

  labelnums = 0:10:max_tick;
  lb = char(repmat(' ', max_tick, 3));
  labels = num2str(labelnums', '%d');
  
  num_digs = 3;
  if size(labels, 2) < num_digs
    labels = [repmat(' ', size(labels, 1), num_digs - size(labels, 2)), ...
              labels];
  end
  
  lb(labelnums+1, :) = labels;
  set(gca, 'YTickLabel', lb); hold on;
end

axes(AX(2)); hold on;
H3 = plot(num_samples, gtimes, 'r'); hold on;

HS = [H1, H2, H3];
for H=HS
  set(H,'LineWidth', 2.0)
  set(H,'Marker', 'o')
  set(H,'MarkerFaceColor', get(H, 'Color'))
  set(H,'LineStyle', '-')
end

legend(HS, 'CPU/GPU ratio', 'CPU Time', 'GPU Time', 'Location', 'NorthWest');
title(sprintf('Performance comparison with %d clusters, %d dimensionality, %d EM iterations, %d restarts', num_clusts, num_dims, num_steps, num_tries));

set(get(AX(1),'Ylabel'),'String','Ratio'); hold on;
set(get(AX(2),'Ylabel'),'String','Elapsed Time (seconds)'); hold on;

xlabel('Number of Data Points')
linkaxes(AX,'xy');