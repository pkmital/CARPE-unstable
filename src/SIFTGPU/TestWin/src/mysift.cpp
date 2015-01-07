#include <string.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <vector> 
using namespace std; 


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


#ifndef mwsize 
#define mwsize int 
#endif 

#ifndef mwIndex 
#define mwIndex int 
#endif 


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, 
                 const mxArray *prhs[]) 
{ 
    
#ifdef SIFTGPU_DLL_DYNAMIC 


        SiftGPU* (*pCreateNewSiftGPU)(int) = NULL; 
        #ifdef _WIN32 
                #ifdef _DEBUG 
                        HMODULE  hsiftgpu = LoadLibrary("siftgpu_d.dll"); 
                #else 
                        HMODULE  hsiftgpu = LoadLibrary("siftgpu.dll");
                #endif 
                if(hsiftgpu == NULL) 
                mexPrintf("Unable to load library siftgpu"); 


                pCreateNewSiftGPU = (SiftGPU* (*) (int)) 
                GetProcAddress(hsiftgpu,"CreateNewSiftGPU"); 
                if(pCreateNewSiftGPU == NULL) 
                mexPrintf("Unable to create new SiftGPU"); 

        #else 
                // 
                void * hsiftgpu = dlopen("libsiftgpu.so", RTLD_LAZY); 
                if(hsiftgpu == NULL) return 0; 
                pCreateNewSiftGPU =  (SiftGPU* (*) (int)) 
                dlsym(hsiftgpu,"CreateNewSiftGPU"); 
        #endif 
        SiftGPU &sift = *pCreateNewSiftGPU(1); 
#else 
        SiftGPU sift; 
#endif 
        // allocate memory
        vector<float > descriptors; 
        vector<SiftGPU::SiftKeypoint> keys; 
        int num; 


        mexPrintf("%d \n", mxGetClassID(*prhs));
        
////////***************************************************************************************************************************************
        
        
         int argc = 0; 
    char **argv; 
    mwIndex I; 
    int k, ncell; 
    int j = 0; 


// Count inputs and check for char type 


    k=0;
         mexPrintf("k = %d \n",k); 
        if( mxIsCell( prhs[k] ) ) 
        { 
            argc += ncell = mxGetNumberOfElements( prhs[k] ); 
            for( I=0; I<ncell; I++ ) 
                if( !mxIsChar( mxGetCell( prhs[k], I ) ) ) 
                    mexErrMsgTxt("Input cell element is not char"); 
        } 
        else 
        { 
            argc++; 
            if( !mxIsChar( prhs[k] ) ) 
                mexErrMsgTxt("Input argument is not char"); 
        } 
     


// Construct argv     


    argv = (char **) mxCalloc( argc, sizeof(char *) ); 


    for( k=0; k<nrhs; k++ ) 
    { 
        if( mxIsCell( prhs[k] ) ) 
        { 
            ncell = mxGetNumberOfElements( prhs[k] ); 
            for( I=0; I<ncell; I++ ) 
                argv[j++] = mxArrayToString( mxGetCell( prhs[k], I ) ); 
        } 
        else 
        { 
            argv[j++] = mxArrayToString( prhs[k] ); 
        } 
    } 


// Your code goes here to use argc and argv. I have just printed the 
   // results out 
// to demonstrate that it works. 


    for( j=0; j<argc; j++ ) 
        mexPrintf("Input %d = <%s>\n",j,argv[j]); 
     // char * argv[j] = {"-m", "-s", "-v", "1"};


// Free the dynamic memory, each individual string and the argv array 
//itself 


   

    
    ////////////////////*************************************************************************************************************************************************
        
        //xValues = mxGetPr(in);
        //rowlLen
        //process parameters 
         // i need to use mxGetCell sice my input is a cell array ********************************************************
        //-m,    up to 2 orientations for each feature 
        //-s     enable subpixel subscale 
        //-v 1   will invoke calling SiftGPU::SetVerbose(1),(only print out #feature and overal time) 
        //sift.EnableOutput(); //SiftGPU now downloads result to CPU by default 


       // int argc = sizeof(argv)/sizeof(char*); 
        sift.ParseParam(argc, argv); 


        //create an OpenGL context for computation 
        if(sift.CreateContextGL() != SiftGPU::SIFTGPU_FULL_SUPPORTED) 
        mexErrMsgTxt("SiftGPU not fully supported"); 
       
//////////////////******************************************************************************************************
        
        k=1;
         mexPrintf("k = %d \n",k); 
        if( mxIsCell( prhs[k] ) ) 
        { 
            argc += ncell = mxGetNumberOfElements( prhs[k] ); 
            for( I=0; I<ncell; I++ ) 
                if( !mxIsChar( mxGetCell( prhs[k], I ) ) ) 
                    mexErrMsgTxt("Input cell element is not char"); 
        } 
        else 
        { 
            argc++; 
            if( !mxIsChar( prhs[k] ) ) 
                mexErrMsgTxt("Input argument is not char"); 
        } 
        
        // Construct imname     
         
j=0;
         
char **imname; 
    imname = (char **) mxCalloc( argc, sizeof(char *) ); 


    for( k=1; k<nrhs; k++ ) 
    { 
        if( mxIsCell( prhs[k] ) ) 
        { 
            ncell = mxGetNumberOfElements( prhs[k] ); 
            for( I=0; I<ncell; I++ ) 
                imname[j++] = mxArrayToString( mxGetCell( prhs[k], I ) ); 
        } 
        else 
        { 
            imname[j++] = mxArrayToString( prhs[k] ); 
        } 
    } 
         //for( j=0; j<1; j++ )
    j=0;
        mexPrintf("Image name %d= <%s>\n",j,imname[j]); 
  //  mexPrintf("j = %d",j);
        
//////////////////******************************************************************************************************        
        
        
        
        
        
        
        
        //get feature vector 
        if(sift.RunSIFT(imname[j]))
       // { 
                //get feature count 
                num = sift.GetFeatureNum(); 


                //allocate memory 
                keys.resize(num);       descriptors.resize(128*num); 


                //read back a feature vector 
                //faster than reading and writing files 
                //if you dont need keys/descriptors, just put a NULL here 
                sift.GetFeatureVector(&keys[0], &descriptors[0]);        
                //this can be used to write your own binary sift file, which is faster 
      //  } 

        
         if( argc ) 
    { 
        for( j=argc-1; j>=0; j-- ) 
            mxFree( argv[j] ); 
        mxFree( argv ); 
    } 
        
        

    mwsize mm = 128; 
    mwsize nn = num; 
    plhs[0] = mxCreateNumericMatrix(mm, nn, mxSINGLE_CLASS, mxREAL);
    int i = 0;
    for(vector<float>::iterator it = descriptors.begin();
        it != descriptors.end();
        it++, i++)
    {
        float *ptr = (float*)mxGetPr(plhs[0]);
        ptr[i] = *it;
    }

    
    mwsize mmm = 4; 
    mwsize nnn = num; 
    plhs[1] = mxCreateNumericMatrix(mmm, nnn, mxSINGLE_CLASS, mxREAL);
    i = 0;
    for(vector<SiftGPU::SiftKeypoint>::iterator it = keys.begin();
        it != keys.end();
        i += 4, it++)
    {
      float *ptr = (float*)mxGetPr(plhs[1]);
      SiftGPU::SiftKeypoint pt = *it;
      ptr[i] = pt.x;
      ptr[i+1] = pt.y;
      ptr[i+2] = pt.s;
      ptr[i+3] = pt.o;
    }
       
#ifdef SIFTGPU_DLL_DYNAMIC 
                        delete &sift; 
                #ifdef _WIN32 
                        FreeLibrary(hsiftgpu); 
                #else 
                        dlclose(hsiftgpu);// linux 
                #endif 
#endif 

}

//      return 1; 


