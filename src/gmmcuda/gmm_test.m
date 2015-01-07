function [ctime, gtime, otime] = gmm_test(num_clusts, num_dims, num_samps, num_steps, num_tries, do_cpu, show_plot)
%GMM_TEST - runs the EM algorithm on random data on the GPU 
% (and on the CPU if requested) with the parameters specified.
%
% Andrew Harp (andrew.harp@gmail.com)
% http://andrewharp.com/gmmcuda
num_samples = round(num_samps);
fprintf('==== Running test with %d clusters, %d dimensions, and %d data points ====\n', num_clusts, num_dims, num_samples);

%% we'll break out of the loop after a successful run

while 1
  try
    sigmas = {};
    for clust_ind=1:num_clusts
      sigmas{clust_ind} = eye(num_dims); % .* ((rand(num_dims) + 1) / 2);
    end
    
    spread = 5;
    means = rand(num_clusts, num_dims) * spread;
    
    nsamps = round((ones(1, num_clusts) * num_samples) / num_clusts);
    
    if ~exist('do_cpu', 'var')
      do_cpu = 1;
    end
    
    if ~exist('show_plot', 'var')
      show_plot = 0;
    end
    
    rsamps = {};
    for i = 1:size(sigmas, 2)
      rsamps{i} = mvnrnd(means(i, :), sigmas{i}, nsamps(i));
    end
    
    % Note that this is already transposed.
    % I like to think of the guesses being aligned along rows, but
    % here they are aligned along columns.  That's how the C++ interface
    % wants it.
    guesses = rand([num_dims, num_clusts, num_tries]) * spread;
    rsamp = [];
    
    if show_plot
      figure
      cols = {'k.', 'b.', 'g.', 'r.', 'g.'};
      plot(guesses(:, 1), guesses(:, 2), 'rx'); hold on;
    end
    
    for i = 1:size(sigmas, 2)
      if show_plot
        col_ind = mod(i-1, numel(cols))+1;
        plot(rsamps{i}(:, 1), rsamps{i}(:, 2), cols{col_ind}); hold on;
      end
      
      rsamp = [rsamp; rsamps{i}];
    end
    
    best_loglike = -inf;
    ctime = 0;
    %% CPU
    if do_cpu == 1
      fprintf('CPU Method Times (compute):\n');

      cpu_loglikes = zeros(1, num_tries);
      for try_num=1:num_tries
        cpugmm('construct', rsamp', guesses(:, :, try_num));

        tic;
        cpu_loglikes(try_num) = cpugmm('step', num_steps);
        ctime = ctime + toc;
                
        if cpu_loglikes(try_num) > best_loglike
          best_loglike = cpu_loglikes(try_num);
          
          cpu_mus = [];
          cpu_sigs = [];
        
          for k=1:num_clusts
            [mmu ssig] = cpugmm(k);
            cpu_sigs = [cpu_sigs; ssig];
            cpu_mus = [cpu_mus; mmu];
          end;
        end
        
        cpugmm('delete');
      end
      
      disp(ctime);
    end
    
    %% GPU
    fprintf('GPU Method (togpu, compute, fromgpu):\n');
    
    gtime = [];
    totime = [];
    fromtime = [];
    
    % Construct the GPU-based GMM.
    gmm('construct', rsamp', guesses);
    
    gpu_mus = [];
    gpu_sigs = [];
    
    num_its = 1;
    if show_plot == 1
      num_its = num_steps;
      num_steps = 1;
    end
    
    for i=1:num_its
      tic;
      gmm('togpu');
      totime(end+1) = toc;
      
      tic;
      gpu_loglikes = gmm('gstep', num_steps);

      gtime(end+1) = toc;
      
      tic;
      gmm('fromgpu');
      fromtime(end+1) = toc;
      
      disp([totime(end), gtime(end), fromtime(end)]);
      
      for k=1:num_clusts
        [mmu ssig] = gmm(k);
        
        gpu_sigs = [gpu_sigs; ssig];
        gpu_mus = [gpu_mus; mmu];
        
        if show_plot
          [x, y] = errorellipse(mmu(:), ssig, 1, 100);
          plot(x, y, 'r'); hold on;
        end;
      end
    end
    gmm('delete');
    
        
    %% Analysis
    if any(isnan(gpu_mus(:))) || any(isnan(gpu_sigs(:)))
      beep
      plot(rsamp(:, 1), rsamp(:, 2), 'r.'); hold on;
      warning('NaNs returned!');
    end
    
    if ~do_cpu
      ctime = mean(totime);
      gtime = mean(gtime);
      otime = mean(fromtime);
      
      fprintf('Average GPU total time: %.05f\n', mean(ctime + gtime + otime));
    else
      ctime = mean(ctime);
      gtime = mean(gtime + totime + fromtime);
      
      mean_mse = mean((cpu_mus(:) - gpu_mus(:)).^2);
      sigma_mse = mean((cpu_sigs(:) - gpu_sigs(:)).^2);
      
      loglike_mse = mean((cpu_loglikes - gpu_loglikes).^2);
      
      % LL MSE will be higher because the numbers in question have greater
      % magnitude.
      fprintf('LL MSE: %0.2f\n', loglike_mse);
      fprintf('Mu MSE: %0.10f\n', mean_mse);
      fprintf('Sigma MSE: %0.10f\n',  sigma_mse);
      
      epsilon = 0.001;
      ll_epsilon = 1;
      if mean_mse > epsilon || sigma_mse > epsilon || loglike_mse > ll_epsilon
        beep;
        warning('Large error!');
      end

      fprintf('Average CPU time: %.05f\n', mean(ctime));
      fprintf('Average GPU total time: %.05f\n', mean(gtime));
    end
    
    if do_cpu
      fprintf('Ratio was %.02f\n', ctime/gtime);
    end
  catch ME
    warning(ME.message);

    gmm('delete');
    warning('Probably just bad random data, restarting experiment.');
    continue
  end
  
  % We seem to have finished alright.
  break
end

fprintf('\nFinished test of %d clusters, %d dimensions, and %d data points\n\n\n', num_clusts, num_dims, num_samples);

