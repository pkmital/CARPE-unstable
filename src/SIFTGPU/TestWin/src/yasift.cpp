// Original SiftGPU from Changchang Wu: http://www.cs.unc.edu/~ccwu/siftgpu/
// Originally ported to Matlab with mex version from Adam Chapman
// Modified to include support for running SiftGPU processes and
// the abillity to send the image data (as row-major) rather than 
// send the filename.
// Parag K. Mital, Aug. 2009

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
using namespace std;


#include "GL/glew.h"

//SiftGPU library dynamically
#define SIFTGPU_DLL_DYNAMIC

#ifdef _WIN32
#ifdef SIFTGPU_DLL_DYNAMIC
#include <windows.h>
#else
//dll import definition for win32
#define SIFTGPU_DLL
#ifdef _DEBUG
#pragma comment(lib, "../lib/siftgpu_d.lib")
#else
#pragma comment(lib, "../lib/SIFTGPU.lib")
#endif
#endif
#else
#ifdef SIFTGPU_DLL_DYNAMIC
#include <dlfcn.h>
#endif
#endif


#include "../../SiftGPU/src/SiftGPU.h"


#include "mex.h"
#include "matrix.h"
#include "nr3matlab.h"

#ifndef mwsize
#define mwsize int
#endif

#ifndef mwIndex
#define mwIndex int
#endif

SiftGPU* (*pCreateNewSiftGPU)(int) = NULL;
#ifdef _WIN32
    HMODULE hsiftgpu = NULL;
#else // LINUX
    void * hsiftgpu = NULL;
#endif
SiftGPU *sift = NULL;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    //if(sift)
    //    mexPrintf("sift already initialized\n");
    if( nlhs == 0 && nrhs == 2 && mxScalar<char>(prhs[0]) == 'p' ) {   // mysift('params', {params})
        if(sift == NULL)
            mexErrMsgTxt("run mysift('open') before passing the image parameters");
        /////////////////// prhs[0] = {'e', '10'...}
        int argc = 0;
        char **argv = NULL;;
        mwIndex I;
        int k, ncell;
        int j = 0;
        
        // Count inputs and check for char type
        k=1;
        
        if( mxIsCell( prhs[k] ) ) {
            argc += ncell = mxGetNumberOfElements( prhs[k] );
            for( I=0; I<ncell; I++ )
                if( !mxIsChar( mxGetCell( prhs[k], I ) ) )
                    mexErrMsgTxt("Input cell element is not char");
        }
        else {
            argc++;
            if( !mxIsChar( prhs[k] ) )
                mexErrMsgTxt("Input argument is not char");
        }
        
        // Construct argv
        argv = (char **) mxCalloc( argc, sizeof(char *) );
        
        for( k=1; k<nrhs; k++ ) {
            if( mxIsCell( prhs[k] ) ) {
                ncell = mxGetNumberOfElements( prhs[k] );
                for( I=0; I<ncell; I++ )
                    argv[j++] = mxArrayToString( mxGetCell( prhs[k], I ) );
            }
            else {
                argv[j++] = mxArrayToString( prhs[k] );
            }
        }
        
        // int argc = sizeof(argv)/sizeof(char*);
        sift->ParseParam(argc, argv);
        
        if( argc ) {
            for( j=argc-1; j>=0; j-- )
                mxFree( argv[j] );
            mxFree( argv );
        }
        
        argc = NULL;
        argv = NULL;
        
        //create an OpenGL context for computation
        if(sift->CreateContextGL() != SiftGPU::SIFTGPU_FULL_SUPPORTED)
            mexErrMsgTxt("SiftGPU not fully supported");
        
        return;
    }
    else if( nlhs == 2 && nrhs == 1 && mxIsChar(prhs[0]) ) {     // [desc key] = mysift('filename')
        char* str = mxArrayToString(prhs[0]);
        
        //get feature vector 
        if(sift->RunSIFT(str)) 
        {
            // allocate memory
            vector<float > descriptors;
            vector<SiftGPU::SiftKeypoint> keys;
            
            //get feature count
            int num = sift->GetFeatureNum();
            
            //mexPrintf("found %d features\n", num);
            
            //allocate memory
            keys.resize(num);
            descriptors.resize(128*num);
            //read back a feature vector
            //faster than reading and writing files
            //if you dont need keys/descriptors, just put a NULL here
            sift->GetFeatureVector(&keys[0], &descriptors[0]);
            //this can be used to write your own binary sift file, which is faster
      
       
            // Descriptors
            mwsize mm = 128;
            mwsize nn = num;
            plhs[0] = mxCreateNumericMatrix(mm, nn, mxSINGLE_CLASS, mxREAL);
            int i = 0;
            for(vector<float>::iterator it = descriptors.begin();
            it != descriptors.end();
            it++, i++) {
                float *ptr = (float*)mxGetPr(plhs[0]);
                ptr[i] = *it;
            }

            // Keys
            mwsize mmm = 4;
            mwsize nnn = num;
            plhs[1] = mxCreateNumericMatrix(mmm, nnn, mxSINGLE_CLASS, mxREAL);
            i = 0;
            for(vector<SiftGPU::SiftKeypoint>::iterator it = keys.begin();
            it != keys.end();
            i += 4, it++) {
                float *ptr = (float*)mxGetPr(plhs[1]);
                SiftGPU::SiftKeypoint pt = *it;
                ptr[i] = pt.x;
                ptr[i+1] = pt.y;
                ptr[i+2] = pt.s;
                ptr[i+3] = pt.o;
            }
        }
        else {
            mexErrMsgTxt("Error running SiftGPU");
        }                
        return;  
    }
    else if( nlhs == 2 && nrhs == 1 && !mxIsChar(prhs[0])) {     // [desc, keys] = mysift(img);
        if(sift == NULL)
            mexErrMsgTxt("run mysift('open') before passing the image");
        
        // pass in the image data
        
        unsigned char *img;
        const mwSize *dims;
        int width, height, numdims;
        if ( !mxIsUint8(prhs[0]) ) {
            mexErrMsgTxt("Image must be uint8");
        }
        // get the input image
        img = (unsigned char *)mxGetData(prhs[0]);
        
        //figure out dimensions
        dims = mxGetDimensions(prhs[0]);
        numdims = mxGetNumberOfDimensions(prhs[0]);
        height = (int)dims[0];
        width = (int)dims[1];
        //mexPrintf("%d, %d, %d\n", width, height, numdims);
        
        unsigned int gl_format;
        if(numdims == 2)
            gl_format = GL_LUMINANCE;
        else if(numdims == 3)
            gl_format = GL_RGB;
        else
            mexErrMsgTxt("Image must be 2D grayscale or 3D RGB in row-major format");
                
        
        if( sift->RunSIFT( width, height, img, gl_format ) ) {
            // allocate memory
            vector<float > descriptors;
            vector<SiftGPU::SiftKeypoint> keys;
            
            //get feature count
            int num = sift->GetFeatureNum();
            
            //mexPrintf("found %d features\n", num);
            
            //allocate memory
            keys.resize(num);
            descriptors.resize(128*num);
            //read back a feature vector
            //faster than reading and writing files
            //if you dont need keys/descriptors, just put a NULL here
            sift->GetFeatureVector(&keys[0], &descriptors[0]);
            //this can be used to write your own binary sift file, which is faster
      
       
            // Descriptors
            mwsize mm = 128;
            mwsize nn = num;
            plhs[0] = mxCreateNumericMatrix(mm, nn, mxSINGLE_CLASS, mxREAL);
            int i = 0;
            for(vector<float>::iterator it = descriptors.begin();
            it != descriptors.end();
            it++, i++) {
                float *ptr = (float*)mxGetPr(plhs[0]);
                ptr[i] = *it;
            }

            // Keys
            mwsize mmm = 4;
            mwsize nnn = num;
            plhs[1] = mxCreateNumericMatrix(mmm, nnn, mxSINGLE_CLASS, mxREAL);
            i = 0;
            for(vector<SiftGPU::SiftKeypoint>::iterator it = keys.begin();
            it != keys.end();
            i += 4, it++) {
                float *ptr = (float*)mxGetPr(plhs[1]);
                SiftGPU::SiftKeypoint pt = *it;
                ptr[i] = pt.x;
                ptr[i+1] = pt.y;
                ptr[i+2] = pt.s;
                ptr[i+3] = pt.o;
            }
        }
        else {
            mexErrMsgTxt("Error running SiftGPU");
        }                
        return;
    }
    else if (nrhs == 1 && mxScalar<char>(prhs[0]) == 'd') {        // mysift('delete')
        if(sift)
        {
            delete sift;
            sift = NULL;
        }
        #ifdef _WIN32
                FreeLibrary(hsiftgpu);
        #else
                dlclose(hsiftgpu);// linux
        #endif
        return;
    }
    else if(nrhs == 1 && mxScalar<char>(prhs[0]) == 'o') {        // mysift('open')
        //mexPrintf("Initializing SiftGPU\n");
        #ifdef _WIN32
             #ifdef _DEBUG
                hsiftgpu = LoadLibrary("siftgpu_d.dll");
            #else
                hsiftgpu = LoadLibrary("siftgpu.dll");
            #endif

            if(hsiftgpu == NULL)
                mexErrMsgTxt("Unable to load library siftgpu");

            pCreateNewSiftGPU = (SiftGPU* (*) (int)) GetProcAddress(hsiftgpu, "CreateNewSiftGPU");
            if(pCreateNewSiftGPU == NULL)
                mexErrMsgTxt("Unable to create new SiftGPU");
        #else // LINUX
            hsiftgpu = dlopen("libsiftgpu.so", RTLD_LAZY);

            if(hsiftgpu == NULL)
                mexErrMsgTxt("Unable to load library siftgpu");
        
            pCreateNewSiftGPU =  (SiftGPU* (*) (int)) dlsym(hsiftgpu, "CreateNewSiftGPU");
            if(pCreateNewSiftGPU == NULL)
                mexErrMsgTxt("Unable to create new SiftGPU");
        #endif
        if(sift)
        {
            delete sift;
            sift = NULL;
        }
        sift = pCreateNewSiftGPU(1);
        return;
    }
    else {
        mexErrMsgTxt("USAGE: \nmysift('open')\n[desc keys] = mysift('process', {params}, img)\nmysift('delete)\n");
    }
    return;
}


