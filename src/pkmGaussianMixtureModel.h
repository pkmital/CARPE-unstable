// Parag K. Mital
// Nov. 2008
// This uses the waffles source library
// even though there exists a GMixtureOfGaussians
// code, it is only for a 1-dimensional model.
// This library is for a 2D model.

#ifndef __pkmGaussianMixtureModel
#define __pkmGaussianMixtureModel

#include <cv.h>			// computer vision library
#include <cxcore.h>		// opencv core
#include <ml.h>			// machine learning library
#include <iostream>
#include <fstream>

class pkmGaussianMixtureModel
{
enum {COV_SPHERICAL, COV_DIAGONAL, COV_GENERIC};
public:
	// setup the mixture model (variables has to be 2)
	pkmGaussianMixtureModel(double *inputData, int observations, int variables, int map_scalar = 1, int cov_type = COV_SPHERICAL);

	// release CvMats stuff...
	~pkmGaussianMixtureModel();

	// the actual modeling step takes the min and max number of kernels,
	// a regularizing factor for the covariance matrix (necessary for small data)
	// and the stopping threshold for the increase in likelihood
	void modelData(int minComponents, int maxComponents, double regularizingFactor,
			double stoppingThreshold);

	void getLikelihoodMap(int rows, int cols, unsigned char *map, std::ofstream &filePtr);

	double multinormalDistribution(const CvMat *pts, const CvMat *mean, const CvMat *covar);

	// Accessor functions as simple dynamic arrays
	int		getNumberOfClusters	();
	float*	getClusterMean		(int clusterNum);
	float	getClusterWeight	(int clusterNum);
	float** getClusterCov		(int clusterNum);

	// write the best model's data to a give file stream
	int		writeToFile(std::ofstream &fileStream, bool writeClusterNums = true, 
						bool writeWeights = true, bool writeMeans = true, 
						bool writeCovs = true, bool verbose = false);


private:
	// number of parameters for the free covariance matrix
	int			m_nPars;
	int			m_nParsOver2;

	// ptr to inputData
	double		*m_pData;

	// CvEM Model array
	CvEM		*emModel;

	// OpenCvMat's for Input Data, Covariance, and Means
	CvMat		*m_pCvData;

	// Labels of each sample to the most probable mixture
	CvMat		*m_pCvLabels;

	// Mixing Probabilities
	//CvMat		*m_pCvMixProb;

	// Dimensions of input data
	int		m_nObservations;
	int		m_nVariables;
	int		m_nScale;

	double	m_Likelihood;
	double	m_BIC;

	// best number of kernels based on MLE
	int		bestModel;

	// Number of kernels
	int		m_nKernels;

	// type of covariance matrix
	int		m_covType;
	/*
	int m_nKernelCount;
	int m_nAttribute;
	double* m_pArrMeanVarWeight;
	double* m_pCatLikelihoods;
	double* m_pTemp;
	//GData* m_pData;
	GNormalDistribution m_dist;
	double m_dMinVariance;
	*/
};

#endif