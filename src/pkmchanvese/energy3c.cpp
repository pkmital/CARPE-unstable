/*********************************************************************
 * energy3c.cpp
 *
 * This file holds functions that initialize image means, compute the 
 * curve flow for the chanvese energy, and updates the statistics.
 * 
 * written by: Shawn Lankton (4/17/2009) - www.shawnlankton.com
 *
 ********************************************************************/

#include "energy3c.h"

static double uin, uout, sumin, sumout, ain, aout; // means

double *en_chanvese_compute(LL *Lz, double *phi, double *img, long *dims, double *scale, double lam)
{
  int x,y,z,idx,n;
  double *F, *kappa;
  double a,I,Fmax;
  // allocate space for F
  F = (double*)malloc(Lz->length*sizeof(double));
  if(F == NULL) return NULL;
  
  kappa = (double*)malloc(Lz->length*sizeof(double));
  if(kappa == NULL) return NULL;

  ll_init(Lz);n=0;Fmax=0.0001; //begining of list;
  while(Lz->curr != NULL){     //loop through list
    x = Lz->curr->x; y = Lz->curr->y; z = Lz->curr->z;
    idx = (int)(z*DIMXY+x*DIMY+y);
    I = img[idx];
    a = (I-uin)*(I-uin)-(I-uout)*(I-uout);
    if(fabs(a)>Fmax) Fmax = fabs(a);
    F[n] = a;
    kappa[n] = en_kappa_pt(Lz->curr, phi, dims); //compute kappa
    ll_step(Lz); n++;       //next point
  }
  if(scale[0] == 0) scale[0] = Fmax;

  for(int j=0;j<Lz->length;j++){
    F[j] = F[j]/scale[0]+lam*kappa[j];
  }
  free(kappa);
  return F;
}

void en_chanvese_init(double* img, double* phi, long *dims){
  double I;
  sumin = 0; sumout = 0; ain = 0; aout = 0; 
  uin = 0; uout = 0;

  for(int i=0; i<NUMEL; i++){
    I = img[i];
    if(phi[i]<=0){
      sumin  = sumin  + I; 
      ain++;
    }
    else{
      sumout = sumout + I; 
      aout++;
    }
  }
  if(ain>0)  uin = sumin/ain;
  if(aout>0) uout = sumout/aout;  
  //mexPrintf("uin=%f uout=%f\n",uin,uout);
}

void en_chanvese_update(double* img, long *dims, LL *Lin2out, LL *Lout2in){
  int x,y,z,idx;
  ll_init(Lin2out);
  while(Lin2out->curr != NULL){
    x = Lin2out->curr->x; y = Lin2out->curr->y; z = Lin2out->curr->z;
    idx = (int)(z*DIMXY+x*DIMY+y);
    
    sumin  -= img[idx]; ain--;
    sumout += img[idx]; aout++;
    ll_remcurr_free(Lin2out);
  }
  ll_init(Lout2in);
  while(Lout2in->curr != NULL){
    x = Lout2in->curr->x; y = Lout2in->curr->y; z = Lout2in->curr->z;
    idx = (int)(z*DIMXY+x*DIMY+y);
    
    sumout -= img[idx]; aout--;
    sumin  += img[idx]; ain++;
    ll_remcurr_free(Lout2in);
  }
  if(uin>0)  uin  = sumin/ain;
  if(uout>0) uout = sumout/aout;
}

double en_kappa_pt(PT* p, double *phi, long *dims){
  double dx,dy,dz;
  return en_kappa_norm_pt(p, phi, dims, &dx, &dy, &dz);
}

double en_kappa_norm_pt(PT* p, double *phi, long *dims, double *pdx, double *pdy, double *pdz){
  double kappa;
  double dx,dy,dz,dxx,dyy,dzz,dxy,dxz,dyz,dx2,dy2,dz2;
  int idx,x,y,z,n;
  int xok,yok,zok;

  x = p->x; y = p->y; z = p->z;
  idx = (int)(z*DIMXY+x*DIMY+y);

  dx=dy=dz=dxx=dyy=dzz=dxy=dxz=dyz=dx2=dy2=dz2=0;
  xok = yok = zok = 0;

  if((x+1)<DIMX && (x-1)>=0) xok = 1;
  if((y+1)<DIMY && (y-1)>=0) yok = 1;
  if((z+1)<DIMZ && (z-1)>=0) zok = 1;

  if(xok){
    dx  = (phi[idx-OFFX]-phi[idx+OFFX])/2;// (l-r)/2
    dxx = (phi[idx-OFFX]-2*phi[idx]+phi[idx+OFFX]); // l-2c+r
    dx2 = dx*dx;
  }
  if(yok){
    dy  = (phi[idx-OFFY]-phi[idx+OFFY])/2;// (u-d)/2
    dyy = (phi[idx-OFFY]-2*phi[idx]+phi[idx+OFFY]);// u-2c+d
    dy2 = dy*dy;
  }
  if(zok){
    dz  = (phi[idx-OFFZ]-phi[idx+OFFZ])/2;// (b-f)/2
    dzz = (phi[idx-OFFZ]-2*phi[idx]+phi[idx+OFFZ]);// b-2c+f
    dz2 = dz*dz;
  }
  if(xok && yok){// (ul+dr-ur-dl)/4
    dxy = (phi[idx-OFFY-OFFX]+phi[idx+OFFY+OFFX]-phi[idx-OFFY+OFFX]-phi[idx+OFFY-OFFX])/4;
  }
  if(xok && zok){// (lf+rb-rf-lb)/4
    dxz = (phi[idx+OFFZ-OFFX]+phi[idx-OFFZ+OFFX]-phi[idx+OFFZ+OFFX]-phi[idx+OFFZ+OFFX])/4;
  }
  if(zok && yok){// (uf+db-df-ub)/4
    dyz = (phi[idx-OFFY+OFFZ]+phi[idx+OFFY-OFFZ]-phi[idx+OFFY+OFFZ]-phi[idx-OFFY-OFFZ])/4;
  }
  
  kappa = (dxx*(dy2+dz2)+dyy*(dx2+dz2)+dzz*(dx2+dy2)-
           2*dx*dy*dxy-2*dx*dz*dxz-2*dy*dz*dyz)/
           (dx2+dy2+dz2+.00000001);

  pdx[0] = dx;
  pdy[0] = dy;
  pdz[0] = dz;
  return kappa;
}
