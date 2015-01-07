#pragma once

class imageStats
{
public:
	imageStats(void);
public:
	~imageStats(void);
public:
	void defineImage(unsigned char *img);
	unsigned char * getLuminanceMap();
	unsigned char * getMotionMagnitudeMap();
	unsigned char * getEdgeWeightMap();
private:
	
};
