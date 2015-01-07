function [x y] = errorellipse(mu,sigma,stdev,n)
L = chol(sigma,'lower');
circle = [cos(2*pi*(0:n)/n); sin(2*pi*(0:n)/n)].*stdev;
ellipse = L*circle + repmat(mu,[1,n+1]);
x = ellipse(1,:);
y = ellipse(2,:);