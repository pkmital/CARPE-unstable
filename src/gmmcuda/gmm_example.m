%GMM_EXAMPLE - simple test to show GMM EM running on GPU, plotting 
% distributions afterwards.
%
% Andrew Harp (andrew.harp@gmail.com)
% http://andrewharp.com/gmmcuda

num_clusts = 3
num_dims = 2
num_data = 3000
num_steps = 10
do_cpu = 0       % Don't bother running CPU version, just do GPU.
show_plot = 1

% Run the example clustering!
gmm_test(num_clusts, num_dims, num_data, num_steps, do_cpu, show_plot);
