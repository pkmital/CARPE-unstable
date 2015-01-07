// Mex Wrapper for reference CPU implementation.
//
// Based on Mex Code from lecture slides here:
// http://www.nr.com/CS395T/lectures2008/11-GaussianMixtureModelsAndEM.pdf

#include "mex.h"
#include "nr3matlab.h"
#include "cholesky.h"
#include "cpugaumixmod.h"

Gaumixmod *gmmcpu = NULL;

/* Matlab usage:
  gmmcpu('construct',data,means)
  
  // Run computation on CPU.
  loglike = gmmcpu('step',nsteps)
  
  [mean sig] = gmmcpu(k)
  resp = gmmcpu('response')
  gmmcpu('delete')
*/
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  int i, j, nn, kk, mm;
  if (gmmcpu) {
    nn=gmmcpu->nn; kk=gmmcpu->kk; mm=gmmcpu->mm;
  }
  
  if (gmmcpu && nrhs == 1 && mxT(prhs[0]) == mxT<Doub>()) { // [mean sig] = gmmcpu(k)
    Int k = Int( mxScalar<Doub>(prhs[0]) );
    if (nlhs > 0) {
      VecDoub mean(mm, plhs[0]);
      for (i=0;i<mm;i++) {
        mean[i] = gmmcpu->means[k-1][i]; // k argument comes in 1-based
      }
    }
    
    if (nlhs > 1) {
      MatDoub sig(mm, mm, plhs[1]);
      for (i=0;i<mm;i++) {
        for (j=0;j<mm;j++) {
          sig[i][j] = gmmcpu->sig[k-1][i][j];
        }
      }
    }
  } else if (nrhs == 1 && mxScalar<char>(prhs[0]) == 'd') {        // gmmcpu('delete')
    if (gmmcpu) {
      delete gmmcpu;
      gmmcpu = NULL;
    }
  } else if (gmmcpu && nrhs == 1 && mxScalar<char>(prhs[0]) == 'r') { // gmmcpu('response')
    if (nlhs > 0) {
      MatDoub resp(nn, kk, plhs[0]);
      for (i=0;i<nn;i++) for (j=0;j<kk;j++) resp[i][j] = gmmcpu->resp[i][j];
    }
  } else if (gmmcpu && nrhs == 2 && mxT(prhs[1]) == mxT<Doub>() && mxScalar<char>(prhs[0]) == 's') { // deltaloglike = gmmcpu('step',nsteps)
    //printf("CPU method.\n");
    
    Int nstep = Int(mxScalar<Doub>(prhs[1]));
    Doub tmp;
    for (i=0;i<nstep;i++) {
      tmp = gmmcpu->estep();
      gmmcpu->mstep();
    }

    if (nlhs > 0) {
      Doub &deltaloglike = mxScalar<Doub>(plhs[0]);
      deltaloglike = tmp;
    }
  } else if (nrhs == 3 && mxT(prhs[0]) == mxT<char>()) { // gmmcpu('construct',data,means)
    MatDoub data(prhs[1]), means(prhs[2]);
    if (means.ncols() != data.ncols()) throw("wrong dims in gmmcpu 1");
    if (means.nrows() >= data.nrows()) throw("wrong dims in gmmcpu 2");
    // user probably didn't transpose
    
    if (gmmcpu) {
      delete gmmcpu;
      gmmcpu = NULL;
    }
    
    gmmcpu = new Gaumixmod(data, means);
  } else {
    throw("bad call to gmmcpu");
  }
  return;
}