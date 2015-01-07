/*********************************************************************
 * sfm_chanvese_mex.cpp
 *
 * This file performs the statistical computations needed for running 
 * the chan-vese active contour segmentation energy using the Sparse 
 * Field method presented by Whitaker.
 * 
 * written by: Shawn Lankton (4/17/2009) - www.shawnlankton.com
 *
 ********************************************************************/

#include "sparse3c.h"

//[phi C] = sfm_chanvese_mex(img,mask,iterations,lam);
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

  //declare variables
  mxArray *phi_out, *C_out, *label_out;
  const mwSize *mdims;
  double *img, *phi, *mask, *C, *label, *F;
  double lambda;
  int    iter;
  long    dims[5];
  long    dimx, dimy, dimz, numdims;
  LL *Lz, *Ln1, *Ln2, *Lp1, *Lp2;
  LL *Sz, *Sn1, *Sn2, *Sp1, *Sp2;
  LL *Lin2out, *Lout2in;

  //figure out dimensions
  mdims = mxGetDimensions(prhs[0]);
  dims[2] = 1; dims[1] = 1;
  numdims = mxGetNumberOfDimensions(prhs[0]);
  switch(numdims){
  case 3: dimz = (int)mdims[2]; dims[2] = dimz;
  case 2: dimx = (int)mdims[1]; dims[1] = dimx;
  case 1: dimy = (int)mdims[0]; dims[0] = dimy;}
  dims[3] = dims[0]*dims[1]; dims[4] = dims[0]*dims[1]*dims[2];


  // associate inputs;
  img     = mxGetPr(prhs[0]);
  mask    = mxGetPr(prhs[1]);
  iter    = (int)mxGetPr(prhs[2])[0];
  if(nrhs>2) lambda = (double)(mxGetPr(prhs[3])[0]);  else lambda = .1;

  //associate outputs;
  phi_out  = plhs[0] = mxDuplicateArray(prhs[1]);
  phi  = mxGetPr(phi_out);

  label_out          = mxDuplicateArray(prhs[1]);
  label= mxGetPr(label_out);
  // C is not setup until after the segmentation


  //create linked lists
  Lz  = ll_create();
  Ln1 = ll_create();
  Ln2 = ll_create();
  Lp1 = ll_create();
  Lp2 = ll_create();
  Lin2out = ll_create();
  Lout2in = ll_create();

  //initialize lists, phi, and labels
  ls_mask2phi3c(mask,phi,label,dims,Lz,Ln1,Ln2,Lp1,Lp2);

  //compute iterations of chanvese energy
  chanvese(img,phi,label,dims,
           Lz,Ln1,Lp1,Ln2,Lp2,Lin2out,Lout2in,
           iter,lambda,plhs,0);

  //prepare "list" output
  plhs[1] = prep_C_output(Lz,dims,phi);

  //destroy linked lists
  ll_destroy(Lz);
  ll_destroy(Ln1);
  ll_destroy(Ln2);
  ll_destroy(Lp1);
  ll_destroy(Lp2);
  ll_destroy(Lin2out);
  ll_destroy(Lout2in);
  return;
}

void chanvese(double *img, double *phi, double *label, long *dims,
              LL *Lz, LL *Ln1, LL *Lp1, LL *Ln2, LL *Lp2, LL *Lin2out, LL *Lout2in,
              int iter,double lambda,  mxArray **plhs,int display){
  double *F;
  double scale[1]; scale[0] = 0;
  int countdown;

  //initialize datastructures and statistics
  en_chanvese_init(img,phi,dims);
  for(int i=0;i<iter;i++){
    //compute force
    F = en_chanvese_compute(Lz,phi,img,dims,scale,lambda);
    //perform iteration
    ls_iteration(F,phi,label,dims,Lz,Ln1,Lp1,Ln2,Lp2,Lin2out,Lout2in);
    //update statistics
    en_chanvese_update(img, dims, Lin2out, Lout2in);
  }
}

mxArray *prep_C_output(LL *Lz,long *dims,double *phi){
  mxArray *out;
  double *data, *C;
  int n = 0;
  int x,y,z,idx;

  if(Lz == NULL) return mxCreateDoubleMatrix(1,1,mxREAL);
  out = mxCreateDoubleMatrix(Lz->length,1,mxREAL);
  C = mxGetPr(out);

  ll_init(Lz);
  while(Lz->curr != NULL){
    C[n] =(double)(Lz->curr->x*DIMY+Lz->curr->y+Lz->curr->z*DIMXY);
    ll_step(Lz); n++;
  }
  return out;
}
