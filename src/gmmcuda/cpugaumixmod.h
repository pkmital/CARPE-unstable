// Reference implementation of EM for GMMs taken from Numerical Recipes 3.
// Reformatted and annotated by Andrew Harp (andrew.harp@gmail.com).
// http://andrewharp.com/gmmcuda

struct preGaumixmod {
  static Int mmstat;
  struct Mat_mm : 
    MatDoub {
      Mat_mm() : MatDoub(mmstat, mmstat) {} 
    };
  
  preGaumixmod(Int mm) {mmstat = mm;}
};

Int preGaumixmod::mmstat = -1;

struct Gaumixmod : preGaumixmod {
  Int nn;              // num data
  Int kk;              // num clusters
  Int mm;              // data dimensionality
  MatDoub data;        // the actual data
  MatDoub means;       // Cluster centroids
  MatDoub resp;        // Data assignment weights?  Add to 1?
  VecDoub frac;        // Percentage of weight assigned to each cluster.
  VecDoub lndets;      // log determinants?
  vector<Mat_mm> sig;  // vector of covariance matrices for clusters
  Doub loglike;        // log likelihood of data given model.

///////////////////////////////////////////////////////////////////////////  

  Gaumixmod(MatDoub &ddata, MatDoub &mmeans) :
      preGaumixmod(ddata.ncols()),
      nn(ddata.nrows()),
      kk(mmeans.nrows()),
      mm(mmstat),
      data(ddata),
      means(mmeans),
      resp(nn, kk),
      frac(kk),
      lndets(kk),
      sig(kk) {
    
    loglike = 0.0f;
        
    Int i, j, k;
    
    // For every cluster.
    for (k=0;k<kk;k++) {
      
      // Every cluster has an equal prior probability.
      frac[k] = 1./kk;
      
      // Step through and create the sigma matrix.
      for (i=0;i<mm;i++) {
        for (j=0;j<mm;j++) {
          sig[k][i][j] = 0.;
        }
        sig[k][i][i] = 1.0e-10;
      }
    }
  }
  
///////////////////////////////////////////////////////////////////////////  
      
  // Expectation step.
  Doub estep() {
    Int k, m, n;
    
    Doub tmp, sum, max, oldloglike;
    
    // mm is dimensionality of data.
    VecDoub u(mm), v(mm);
    
    oldloglike = loglike;
    
    // Loop through clusters.
    for (k=0; k<kk; k++) {
      Cholesky choltmp(sig[k]);
      
      // What does this mean?
      lndets[k] = choltmp.logdet();
      
      // Loop through data.
      for (n=0; n<nn; n++) {
        
        // Loop through dimensions.
        for (m=0;m<mm;m++) {
          // u is delta from cluster center.
          u[m] = data[n][m]-means[k][m];
        }
        
        // Describe delta in terms of covariance of elipse?
        choltmp.elsolve(u, v);
        
        // Loop through dimensions.
        for (sum=0., m=0; m<mm; m++) {
          sum += SQR(v[m]);
        }
        
        // Assign likelihood of this data being in this cluster.
        resp[n][k] = -0.5*(sum + lndets[k]) + log(frac[k]);
      }
    }
    
    loglike = 0;
    // Loop through data.
    for (n=0;n<nn;n++) {
      max = -99.9e30;
      
      // Loop through clusters.
      for (k=0;k<kk;k++) {
        
        // Find cluster with maximum likelihood for this data point.
        if (resp[n][k] > max) {
          max = resp[n][k];
        }
      }
      
      // Sum marginal probabilities.
      for (sum=0., k=0; k<kk; k++) {
        sum += exp(resp[n][k]-max);
      }
      
      // Assign probabilities of point belonging to each cluster.
      tmp = max + log(sum);
      for (k=0; k<kk; k++) {
        resp[n][k] = exp(resp[n][k] - tmp);
      }
      
      loglike +=tmp;
      //printf("!%d! %0.2f\n", n, tmp);
    }
    
    return loglike; // - oldloglike;
  }
  
///////////////////////////////////////////////////////////////////////////  
  
  // Maximization step.
  void mstep() {
    Int j, n, k, m;
    Doub wgt, sum;
    
    // For every cluster...
    for (k=0;k<kk;k++) {
      wgt=0.;
      
      // Sum response to cluster.
      for (n=0;n<nn;n++) {
        wgt += resp[n][k];
      }
      
      // Find fraction of weight in each cluster.
      frac[k] = wgt/nn;
      
      // For every dimension.
      for (m=0;m<mm;m++) {
        for (sum=0., n=0; n<nn; n++) {
          sum += resp[n][k] * 
                 data[n][m];
        }
        
        // Mean is easy to compute as the average of weighted constituent points.
        means[k][m] = sum/wgt;
        
        // Compute new covaraniance matrix for cluster.
        for (j=0;j<mm;j++) {
          
          // Go compute every entry in covaraiance matrix.
          for (sum=0., n=0; n<nn; n++) {
            sum += resp[n][k]*
                    (data[n][m]-means[k][m]) * (data[n][j]-means[k][j]);
          }
          
          sig[k][m][j] = sum/wgt;
        
        } // j
      } // m
    } // k
  } // mstep
};
