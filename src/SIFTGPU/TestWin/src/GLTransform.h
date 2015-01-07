////////////////////////////////////////////////////////////////////////////
//	File:		GLTransform.h
//	Author:		Changchang Wu
//	Description : GLTransform tookit for opengl display
//
//
//
//	Copyright (c) 2007 University of North Carolina at Chapel Hill
//	All Rights Reserved
//
//	Permission to use, copy, modify and distribute this software and its
//	documentation for educational, research and non-profit purposes, without
//	fee, and without a written agreement is hereby granted, provided that the
//	above copyright notice and the following paragraph appear in all copies.
//	
//	The University of North Carolina at Chapel Hill make no representations
//	about the suitability of this software for any purpose. It is provided
//	'as is' without express or implied warranty. 
//
//	Please send BUG REPORTS to ccwu@cs.unc.edu
//
////////////////////////////////////////////////////////////////////////////


#include "math.h"
class GlTransform
{
public:
	double		cx, cy;
	double		q[4];
	double		t[3];
	double		sc, ds;
	double		rot[4][4];
	GlTransform()
	{
		q[0]	=	1.0;
		q[1]	=	q[2]	=	q[3]	=0;
		t[0]	=	t[1]	=	t[2]	=0;
		sc		=	1.0;
		cx		=	cy		= 0;
		for(int i = 0; i< 16; i++)
		{
			rot[0][i]	=	(i%5==0);
		}

	}
	void reset()
	{
		q[0]	=	1.0;
		q[1]	=	q[2]	=	q[3]	=0;
		t[0]	=	t[1]	=	t[2]	=0;
		sc		=	1.0;
		for(int i = 0; i< 16; i++)
		{
			rot[0][i]	=	(i%5==0);
		}
	}
	 void operator=(GlTransform& v)
	 {
		q[0]	=	v.q[0];
		q[1]	=	v.q[1];
		q[2]	=	v.q[2];
		q[3]	=	v.q[3];
		t[0]	=	v.t[0];
		t[1]	=	v.t[1];
		t[2]	=	v.t[3];
		sc		=	v.sc;	
		for(int i = 0; i< 16; i++)
		{
			rot[0][i]	=	v.rot[0][i];
		}	
	 }
	 void operator *=(double scale)
	 {
		sc	*=	scale;
		t[0]*=	scale;
		t[1]*=	scale;
		t[2]*=	scale;
	 }
	 void scaleset(double scale)
	 {
		double ds = scale/sc;
		t[0]*=	ds;
		t[1]*=	ds;
		t[2]*=	ds;	
		sc  = scale;
	 }
	 void scaleup()
	 {
		double scale;
		if(sc < 6) scale = float(int(sc))+1;
		else scale = sc * 2.0;
		scaleset(scale);
	 }
	 void scaledown()
	 {
		double scale;
		if(sc >1.0 &&sc < 2.0) scale = 1.0;
		else scale = sc*0.5;
		scaleset(scale);
	 }
	 void translate(int dx, int dy,	int dz =0)
	 {
		t[0]	+=	dx;
		t[1]	+=	dy;
		t[2]	+=	dz;
	 }
	 void setcenter(double x, double y)
	 {
		cx = x;
		cy = y;
		t[0] = t[1] = t[2] = 0;
	 }

	 //not used so far
	 void trackball(double x1, double y1, double x2, double y2, double tsize)
	 {
		double len, z1, z2, phi, dq[4];
		len = x1*x1 + y1*y1;
		if(len<tsize*0.5)
		{
			z1 = sqrt(tsize*tsize-len);
		}else
		{
			z1 = tsize*tsize/2.0/sqrt(len);
		}

		len =x2*x2+y2*y2;
		if(len<tsize*0.5)
		{
			z2 = sqrt(tsize*tsize-len);
		}else
		{
			z2 = tsize*tsize/2.0/sqrt(len);
		}

		///
		len = sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)+(z1-z2)*(z1-z2))/2/tsize;
		if(len > 1.0) len = 1.0;
		phi = (float)asin(len)*2;

		dq[1] = ((y2 * z1) - (z2 * y1));
		dq[2] = ((z2 * x1) - (x2 * z1));
		dq[3] = ((x2 * y1) - (y2 * x1));

		len = sqrt(dq[3]*dq[3]+dq[1]*dq[1]+dq[2]*dq[2]);
		if(len>0.0000001 )
		{
			dq[1]/=len;
			dq[2]/=len;
			dq[3]/=len;
			double s = sin(phi);
			dq[0] = cos(phi);
			dq[1] *= s;
			dq[2] *= s;
			dq[3] *= s;
		}else
		{
			//back to same position or
			//180 degrees.. or -180
			dq[0] = 1.0f;
		}
		rotate(dq);




	 }
	 //not used so far
	 void trackdisk(double x1, double y1, double x2, double y2)
	 {
		double len, z1, z2, phi, dq[4];
		z1	=	z2	=	0	;

		len = sqrt(x1*x1+y1*y1);
		x1/=len;
		y1/=len;

		len	=sqrt(x2*x2+y2*y2);
		x2/=len;
		y2/=len;
		///
		len = sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2))/2;
		if(len > 1.0) len = 1.0;

		phi = (float)asin(len);

		dq[1] = 0;
		dq[2] = 0;
		dq[3] = ((x2 * y1) - (y2 * x1));

		len = fabs(dq[3]);
		if(len>0.0000001 )
		{
			dq[0] = cos(phi);
			dq[3] = dq[3]>0?sin(phi):-sin(phi);
		}else
		{
			//back to same position or
			//180 degrees.. or -180
			dq[0] = 1.0f;
		}
		rotate(dq);
	 }

	 //rotation is not used so far
	 void rotate(double dq[4])
	 {

		int i;
		double newq[4];
		///
		double tt[3], a[4][4];
		newq[1]=	dq[1] *	q[0]	+	q[1] * dq[0]	+	q[2] * dq[3] - q[3] * dq[2];
		newq[2]=	dq[2] *	q[0]	+	q[2] * dq[0]	+	q[3] * dq[1] - q[1] * dq[3];
		newq[3]=	dq[3] *	q[0]	+	q[3] * dq[0]	+	q[1] * dq[2] - q[2] * dq[1];
		newq[0]=	dq[0] *	q[0]	-	q[1] * dq[1]	-	q[2] * dq[2] - q[3] * dq[3];

		Quaternion2Matrix(dq, a);

		for ( i = 0; i < 3; i++) 
		{
			tt[i]	=	a[0][i] * t[0] + a[1][i] *t[1] + a[2][i] *t[2];
		}
		for( i = 0; i< 3; i++)
		{
			t[i]	=	tt[i];
		}
		for( i=	0;	i<	4; i++)
		{
			q[i]	=	newq[i];
		}
		Quaternion2Matrix(newq, rot);
	 }

	 void transform(double es = 1.0)
	 {
	    double s = sc* es;
		glTranslated(cx*es, cy*es, 0.0);
		glTranslated(t[0] ,t[1] ,t[2]);
		glScaled(s,s,s);
		glMultMatrixd((double*)rot);
		glTranslated(-cx, - cy, 0);
	 }


	//rotation is not used so far
	static void Quaternion2Matrix( double q[], double R[4][4])
	{
		double qq = sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
		double qw, qx, qy, qz;
		if(qq>0)
		{
			qw=q[0]/qq;
			qx=q[1]/qq;
			qy=q[2]/qq;
			qz=q[3]/qq;
			q[0]/=qq;
			q[1]/=qq;
			q[2]/=qq;
			q[3]/=qq;
		}else
		{
			qw = 1;
			qx = qy = qz = 0;
		}
		R[0][0]=qw*qw + qx*qx- qz*qz- qy*qy ;
		R[0][1]=2*qx*qy -2*qz*qw ;
		R[0][2]=2*qy*qw + 2*qz*qx;
		R[1][0]=2*qx*qy+ 2*qw*qz;
		R[1][1]=qy*qy+ qw*qw - qz*qz- qx*qx;
		R[1][2]=2*qz*qy- 2*qx*qw;
		R[2][0]=2*qx*qz- 2*qy*qw;
		R[2][1]=2*qy*qz + 2*qw*qx ;
		R[2][2]=qz*qz+ qw*qw- qy*qy- qx*qx;

		R[0][3]=R[1][3]=R[2][3]=0; 	
		R[3][0]=R[3][1]=R[3][2]=0; 
		R[3][3] =1.0;

	}
};