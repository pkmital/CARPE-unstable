/*
 * Copyright (c) ICG. All rights reserved.
 *
 * Institute for Computer Graphics and Vision
 * Graz University of Technology / Austria
 *
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the above copyright notices for more information.
 *
 *
 * Project     : vmgpu
 * Module      : FlowLib
 * Class       : $RCSfile$
 * Language    : C++/CUDA
 * Description : Definition of FlowLib
 *
 * Author     : Manuel Werlberger
 * EMail      : werlberger@icg.tugraz.at
 *
 */

#ifndef FLOWLIB_H
#define FLOWLIB_H

// include definitions
#include <vector>
#include <map>
#include "FlowLibDefs.h"

// forward declarations
class FlowField;
namespace CudaFlowLib {
  class BaseData;
}

class FLOWLIB_DLLAPI FlowLib
{
public:
  /** Constructor of the Flow library. 
   * @param verbose Defines level of additional information displayed.
   */
  FlowLib(const int verbose = 0);

  /** Destructor of the Flow library. */
  ~FlowLib();

  /** Does the major cleanup of all FlowLib Variables. */
  void cleanup();

  /** Resets the Flow library to an 'initial' stage. */
  void reset();


  /** Enum to define the current tracking mode. */
  enum TRACKING_MODE {TMpoint, TMrectangle, TMoff};
  /** Rectangle that can be used for tracking. */
  struct TrackingRectangle {
    TrackingRectangle() : lu_x(0.0f), lu_y(0.0f), ru_x(0.0f), ru_y(0.0f), 
                          ll_x(0.0f), ll_y(0.0f), rl_x(0.0f), rl_y(0.0f), 
                          x_mov(0.0f), y_mov(0.0f) {}
    void reset() {
      lu_x = 0.0f;
      lu_y = 0.0f;
      ru_x = 0.0f;
      ru_y = 0.0f;
      ll_x = 0.0f;
      ll_y = 0.0f;
      rl_x = 0.0f;
      rl_y = 0.0f;
      x_mov = 0.0f;
      y_mov = 0.0f;
    }
      
    
    void move(bool reset = true) {
      lu_x += x_mov;
      lu_y += y_mov;
      ru_x += x_mov;
      ru_y += y_mov;
      ll_x += x_mov;
      ll_y += y_mov;
      rl_x += x_mov;
      rl_y += y_mov;

      if(reset)
      {
        x_mov = 0.0f;
        y_mov = 0.0f;
      }
    }

    float width() {
      return max(lu_x, ru_x) - min(lu_x, ru_x);
    }

    float height() {
      return max(lu_y, ll_y) - min(lu_y, ll_y);
    }
    
    // left upper point
    float lu_x;
    float lu_y;

    // right upper point
    float ru_x;
    float ru_y;

    // left lower point
    float ll_x;
    float ll_y;

    // right lower point
    float rl_x;
    float rl_y;

    // tracking movement (only translation).
    float x_mov;
    float y_mov;
  };

  /** Sets level of additional information. */
  void setVerbose(const int verbose = 0){verbose_ = verbose;}

  /** Initializes rainbow color images to visualize flow-field output. */
  void initColorGradient();

  /** Returns if FlowLib is ready or not. */
  inline bool ready(){return ready_;};

  /** Returns the number of images needed with the current algorithm. */
  unsigned int numImagesNeeded();

  /** Calculates the optimal memory size.
   * This is important for the efficiency of the algorithm! Make sure to use this
   * size to create the input device memory as else the memory has to be copied
   * @param size    size of input image
   * @return        optimal size for memory
   */
  Cuda::Size<2> getOptimalMemSize(Cuda::Size<2> size);

  /** Calculates the optimal memory size for color (planar) color input images.
   * This is important for the efficiency of the algorithm! Make sure to use this
   * size to create the input device memory as else the memory has to be copied
   * @param size    size of input image
   * @return        optimal size for memory
   */
  Cuda::Size<3> getOptimalMemSize(Cuda::Size<3> size);

  /** Returns the effective image size (region_size). Actually the flow field size is used here!
   * @param[in] scale Desired scale.
   */
  Cuda::Size<2> getRegionSize(const unsigned int scale);

  /** Sets two images as initial input to calculate 2-frame-flow
   * @param first First input image.
   * @param second Second input image.
   * @return TRUE if enough images are set an flow can be calculated and FALSE if not.
   */
  bool setInput(Cuda::DeviceMemory<float,2>* first, 
                Cuda::DeviceMemory<float,2>* second);

  /** Sets a new input image to calculate flow on this consecutive frame(s). If no image
   * was set previously the Library does nothing, returns FALSE and waits for another image. If enough
   * images are available to start the algorithm the function returns TRUE.
   * @param image Input image.
   * @param ptamDispU For model HL1FEATURES: Image of feature displacements from ptam to support OF calculation.
   * @param ptamDispV For model HL1FEATURES: Image of feature displacements from ptam to support OF calculation.
   * @return TRUE if enough images are set an flow can be calculated and FALSE if not.
   */
  bool setInput(Cuda::DeviceMemory<float,2>* image, Cuda::DeviceMemory<float,2>* ptamDispU,
                Cuda::DeviceMemory<float,2>* ptamDispV);

  /** Sets a new COLOR input image to calculate flow on this consecutive frame(s). If no
   * image was set previously the Library does nothing, returns FALSE and waits for
   * another image. If enough images are available to start the algorithm the function
   * returns TRUE.
   * @param image Color input image.
   * @return TRUE if enough images are set an flow can be calculated and FALSE if not.
   */
  bool setInput(Cuda::DeviceMemory<float,3>* image);

  /** Alternative input method.
   * @param buffer Float buffer containing image data.
   * @param width Image width.
   * @param height Image height.
   * @param stride Image pitch. (Number of _bytes_ in a row)
   * @param on_device Flag if buffer resides on the device (TRUE) or on the host (FALSE).
   */
  bool setInput(float* buffer, int width, int height, size_t pitch, bool on_device = true);

  /** Returns the specified input image. 
   * @param[in] num Image number.
   * @param[in] scale Desired scale.
   */
  Cuda::DeviceMemoryPitched<float,2>* getImage(int num, int scale);

  /** Returns the first input image
   * @param[in] scale Desired scale.
   */
  inline Cuda::DeviceMemoryPitched<float,2>* getImageOne(int scale){return this->getImage(1,scale);};

  /** Returns the second input image.
   * @param[in] scale Desired scale.
   */
  inline Cuda::DeviceMemoryPitched<float,2>* getImageTwo(int scale){return this->getImage(2,scale);};
  
  /** Returns the original input image number \a num of the given scale \a scale. */
  Cuda::DeviceMemoryPitched<float,2>* getOriginalImage(unsigned int num, unsigned int scale);

  
  /** Getter for the corresponding original image pyramid.
   * @param[in] index Index of the returned original image pyramid.
   * @returns "Vector" with the corresponding image pyramid.
   * @throw std::runtime_error If something went wrong (e.g. no initialization done yet).
   */
  Cuda::HostMemoryHeap<Cuda::DeviceMemoryPitched<float,2>*,1>* getOriginalImagePyramid(unsigned int index);

  /** Getter for the corresponding original color image pyramid.
   * @param[in] index Index of the returned original image pyramid.
   * @returns "Vector" with the corresponding color image pyramid.
   * @throw std::runtime_error If something went wrong (e.g. no initialization done yet).
   */
  Cuda::HostMemoryHeap<Cuda::DeviceMemoryPitched<float,3>*,1>* getOriginalColorImagePyramid(unsigned int index);

  /** Getter for the corresponding filtered image pyramid.
   * @param[in] index Index of the returned image pyramid.
   * @returns "Vector" with the corresponding image pyramid.
   * @throw std::runtime_error If something went wrong (e.g. no initialization done yet).
   */
  Cuda::HostMemoryHeap<Cuda::DeviceMemoryPitched<float,2>*,1>* getFilteredImagePyramid(unsigned int index);

  /** Getter for the corresponding image pyramid.
   * @param[in] index Index of the returned image pyramid.
   * @returns "Vector" with the corresponding image pyramid.
   * @throw std::runtime_error If something went wrong (e.g. no initialization done yet).
   */
  Cuda::HostMemoryHeap<Cuda::DeviceMemoryPitched<float,2>*,1>* getImagePyramid(unsigned int index);
 
  /** Getter for the ptam image pyramid in u
   * @param[in] what Which ptam pyramid (u or v) to retrieve, 0==U, 1==V 
   * @returns "Vector" with the corresponding image pyramid.
   * @throw std::runtime_error If something went wrong (e.g. model is not HL1FEATURES).
   */
  Cuda::HostMemoryHeap<Cuda::DeviceMemoryPitched<float,2>*,1>* getImagePyramidPtam(unsigned int what);

  /** Returns u (the disparities in x-direction) of the given scale. */
  Cuda::DeviceMemoryPitched<float,2>* getFlowU(const unsigned int scale = 0);

  /** Copies u (the disparities in x-direction)  to the float buffer \a buffer of the given scale [Alternative with pure buffers].
   * @param buffer Float buffer.
   * @param pitch Number of _bytes_ in a row
   * @param on_device Flag if buffer resides on the device (TRUE) or on the host (FALSE).
   */
  void getFlowU(float* buffer, size_t pitch, bool on_device = true, unsigned int scale = 0);

  /** Returns v (the disparities in x-direction) of the given scale. */
  Cuda::DeviceMemoryPitched<float,2>* getFlowV(const unsigned int scale = 0);

  /** Copies v (the disparities in y-direction)  to the float buffer \a buffer of the given scale [Alternative with pure buffers].
   * @param buffer Float buffer.
   * @param pitch Number of _bytes_ in a row
   * @param on_device Flag if buffer resides on the device (TRUE) or on the host (FALSE).
   */
  void getFlowV(float* buffer, size_t pitch, bool on_device = true, unsigned int scale = 0);

  /** Returns u for display purpose (128+u) (the disparities in x-direction) of the given scale. */
  Cuda::DeviceMemoryPitched<unsigned char,2>* getDisplayableFlowU(const unsigned int scale = 0);

  /** Returns v for display purpose (128+v) (the disparities in y-direction) of the given scale. */
  Cuda::DeviceMemoryPitched<unsigned char,2>* getDisplayableFlowV(const unsigned int scale = 0);

  /** Returns a planar color coded flow field of the given scale. */
  Cuda::DeviceMemoryPitched<float,3>* getColorFlowField(const unsigned int scale);

  /** Copies color coded flow field of the given scale to the given variable \a c_flow. */
  void getColorFlowField(Cuda::DeviceMemory<float,3>* c_flow, const unsigned int scale);

  /** Returns the flow field pyramid. */
  //inline Cuda::HostMemoryHeap1D<FlowField*>* getFlowFieldPyramid(){return data_->getFlowFieldPyramid();};

  /** Writes the current flow field to a .flo file. */
  bool writeFlowField(std::string filename, const unsigned int scale = 0) const;

  /** Warps the given image according the given flow-field.
   * @param[in] image Image that should be warped.
   * @param[out] warped_image Warped output image.
   * @param[in] flow Flow field that is used for the warping process.
   */ 
  bool warpImage(Cuda::DeviceMemoryPitched<float,2> *image, 
                 Cuda::DeviceMemoryPitched<float,2> *warped_image, 
                 Cuda::DeviceMemoryPitched<float,2> *flow_u,
                 Cuda::DeviceMemoryPitched<float,2> *flow_v,
                 float uv_weight = 1.0f);

  /** Returns warped image. Currently second image is warped back to the second one. The
   * memory allocation is done internally but no memory management is done
   * afterwards. The ownership of the memory is passed to the caller of this fcn.
   * @param[out] w_image warped image
   */ 
  bool getWarpedImage(Cuda::DeviceMemoryPitched<float,2>** w_image);

  /** Returns warped image (currently first image is warped to second one. 
   * @param[out] w_image warped image
   */ 
  bool getWarpedImage(Cuda::DeviceMemory<float,2>* w_image);

  /** Returns warped image HOST buffer. The buffer has to be allocated with the correct size before usage!
   * @param buffer Float buffer.
   * @param pitch Number of _bytes_ in a row
   */ 
  bool getWarpedImage(float* host_buffer, size_t pitch);

  /** Determines the number of available models
   * @return        number of available models
   */
  int getNumModels(){ return model_abrv_map_.size(); };

  /** Retruns string with description for model num
  * @param model     selected model
  * @return        string containing description
  */
  const std::string getModelDescription(const CudaFlowLib::Model& model);

  /** Retruns string with abbreviation for model num
  * @param num     selected model
  * @return        string containing abbreviation
  */
  const std::string getModelAbbreviation(const CudaFlowLib::Model& mod);

  /** Retruns  map of strings with abbreviations for all available models.  */
  const std::map<CudaFlowLib::Model, std::string>& getModelAbbreviationsMap(){return model_abrv_map_; };

  /** Retruns  vector of strings with abbreviations for all available models.  */
  const std::vector<std::string>& getModelAbbreviations();

  /** Sets Parameter structure for upcoming calculations. */
  void setParameters(const FlowParameters& parameters);

  /** Selects a model that will be minimized.
   * @todo See CUDA_defs.h for typedefs !!
   * @param mod      selected model
   */
  void setModel(const CudaFlowLib::Model& model);
  
  /** Selects a model that will be minimized.
   * @todo See CUDA_defs.h for typedefs !!
   * @param mod      selected model as string -- will be mapped to a Model internally
   */
  void setModel(const std::string& model);

  /** Returns number of images. */
  size_t getNumImages();

  /** Sets number of images used for flow calculations. */
  void setNumImages(const size_t& num_images);

  /** Swaps image pyramids (e.g. used to calcualte flow in different direction) 
   * for 2 images this means 1 - 2 -> 2 - 1;
   * for 3 images this does not really make sense - nothing is done here
   */
  void swapImages();

  /** Sets new smoothing parameter lambda. */
  void setLambda(const float& lambda);

  /** Sets parameter for gauss TV. */
  void setGaussTVEpsilon(const float& value);

  /** Sets new iteration amount (external). */
  void setNumIter(const unsigned int& num_iter);
  
  /** Sets new amount of iterations between a warping step. */
  void setNumWarps(const unsigned int& num_warps);

  /** Activates non-local search (TRUE) or thresholding scheme (FALSE) within primal update. */
  void useNonlocalSearch(const bool& nls);      

  /** (De-)Activates diffusion tensor. */
  void useDiffusionTensor(const bool& flag);

  /** Set smoothing amount for calculating diffusion tensor. */
  void setDiffusionSigma(const float& value);

  /** Set parameter alpha for calculating diffusion tensor. */
  void setDiffusionAlpha(const float& value);

  /** Set parameter q for calculating diffusion tensor. */
  void setDiffusionQ(const float& value);

  /** Sets number of used scales to build the image pyramid.
   * @param scales Number of scales.
   * @param returns number of scales really used. Might change to a smaller number if given number of scales does not fit the current layout.
   */
  unsigned int setNumScales(const unsigned int& scales);

  /** Sets scale factor for building the image pyramids.
   * @param factor Scale factor used to scale one pyramidal level to the next one.
   * @todo: Posibility to change scale factor on the fly
   */
  void setScaleFactor(const float& factor);

  /**
  Sets flag to indicate wether only information from ptam should be used for calcultaing the OF
  @param[in] val Use ptam features exclusively (true) or as hard constraint (false) in OF calculation
  */
  void setHL1FeaturesPtamOnly(bool val);

  /** Sets max flow value -- mainly used for normalization while displaying the color flow field. */
  void setMaxFlow(const float& f);

  /** Generic set method for Structure-Texture Decomposition.
   * @param[in] filter Filter method that is used for str.-tex. decomp. (please use all uppercase letters!!)
   * @param[in] weight Defines weighting between structure and texture component.
   * @param[in] smoothing amount of smoothing for str.-tex. decomposition.
   * @return TRUE if state was changed. Means if activate switches the filters state. 
   */
  void setStructureTextureDecomposition(const std::string& filter,
                                        const float& weight, const float& smoothing);

  /** Generic set method for Structure-Texture Decomposition.
   * @param[in] filter Filter method that is used for str.-tex. decomp. (please use all uppercase letters!!)
   * @param[in] weight Defines weighting between structure and texture component.
   * @param[in] smoothing amount of smoothing for str.-tex. decomposition.
   * @return TRUE if state was changed. Means if activate switches the filters state. 
   */
  void setStructureTextureDecomposition(CudaFlowLib::FilterMethod_t filter,
                                        const float& weight, const float& smoothing);

  /** Deactivates Structure-Texture Decomposition */
  void deactivateStructureTextureDecomposition();

  /** Returns filter method for structure-texture decomposition. */
  CudaFlowLib::FilterMethod_t getStrTexFilterMethod();

  /** Returns proportion of structure-texture decomposition. */
  float getStrTexWeight();

  /** Returns smoothing amount of denoising for structure-texture decomposition. */
  float getStrTexSmoothing();

  /** Returns number of scales. */
  size_t getNumScales();

  /** Returns the calculated x-gradient of scale \a scale if available for the used model. */
  Cuda::DeviceMemoryPitched<float,2>* getGradXImage(size_t scale);

  /** Returns the calculated y-gradient of scale \a scale if available for the used model. */
  Cuda::DeviceMemoryPitched<float,2>* getGradYImage(size_t scale);

  /** Sets the search range if a model with non-local search is used. */
  void setSearchRange(float range);
  /** Returns the search range if a model with non-local search is used. */
  float getSearchRange();
  /** Sets the search step if a model with non-local search is used. */
  void setSearchStep(float range);
  /** Returns the search step if a model with non-local search is used. */
  float getSearchStep();

  /** Deactivates the tracking stuff. */
  void deactivateTracking();

  /** Returns the tracked rectangle (therefore the current tracking position).
   * @parameter[out] rect rectangle struct that defines a rectangle that should be tracked in an affine way.
   */
  void getTrackingMovement(float& x, float& y);

  /** Returns the current tracking rectangle (movement is processed before data is set)
   * @param[out] rect TrackingRectangle struct with all 4 corner point parameters
   */
  void getTrackingRectangle(TrackingRectangle& rect);


  /** Sets a rectangle that is used for tracking. 
   * @param[in] rect rectangle struct that defines a rectangle that should be tracked in an affine way.
   * @param[in] enabled Flag if tracking is enabled right away (TRUE, default) or if only the rectangle is set (FALSE).
   */
  void setTrackingRectangle(TrackingRectangle& rect, bool enabled = true);

  /** Sets a point cloud that is used for 'point-wise' tracking. 
   * @param[in] buffer Device memory with point-cloud to track. Buffer is not
   * handeled internally and be aware that the tracking is done directly with this
   * buffer - so the data gets modified
   * @param[in] enabled Flag if tracking is enabled right away (TRUE, default) or if only the tracking buffer is set (FALSE).
   */
  void setTrackingBuffer(Cuda::DeviceMemoryPitched<float,2>* buffer, bool enabled = true);

  /** Returns the cutout regions according the given motion u/v.
   * @param[out] motionseg Resultant motion segmentation. Not moving parts are set to black.
   * @param[in] u_thresh Motion threshold in x direction.
   * @param[in] v_thresh Motion threshold in y direction.
   */
  bool getMotionSegmentation(Cuda::DeviceMemoryPitched<float,2>** motionseg, float threshold);
  bool getMotionSegmentation(Cuda::DeviceMemoryPitched<float,2>* motionseg, float threshold);

  /** Returns sort of a confidence map on how good the estimated flow is in each
   * reagion. The measurement is done via geometric similarities of warped flow
   * vectors. Be aware that therefore the flow has to be calculated again in the
   * opposite direction. Therefore this can (and will) decrease performance when using
   * in real-time settings.
   * @param[out] confidence Confidence measure for every pixel.
   */
  void getGeometricConfidence(Cuda::DeviceMemoryPitched<float,2>** confidence);
  void getGeometricConfidence(Cuda::DeviceMemoryPitched<float,2>* confidence);

  /** Runs optical flow calculations with the specified amount of iterations.
   * If the number of iterations is set to 0, the algorithm will run for the last selected number of iterations.
   * @param iterations number of iterations to calculate
   * @return TRUE if everything was ok and FALSE if not.
   */
  bool runAlgorithm(const int iterations=0);

protected:
  /** Builds corresponding data structures for the used pyramidal representations.
   * @param first First input image.
   * @param second Second input image.
   * @param third Third input image. This one is ignored if a 2-frame model is selected.
   * @return TRUE if everything was ok and FALSE if not.
   */
  bool initData(Cuda::DeviceMemory<float,2>* first=0, 
                Cuda::DeviceMemory<float,2>* second=0,
                Cuda::DeviceMemory<float,2>* third=0);

  /** Builds corresponding data structures for the used pyramidal representations for color images.
   * @param first First color input image.
   * @param second Second color input image.
   * @return TRUE if everything was ok and FALSE if not.
   */
  bool initData(Cuda::DeviceMemory<float,3>* first=0, 
                Cuda::DeviceMemory<float,3>* second=0);

  private:
  /** Helper functions to run the correct calculations for the selected model. */
  bool run();

  /** Helper function that handles the 'pointwise' tracking itself. */
  bool updateTracking();

  /** Helper function that handles tracking estimating an affince transformation of a rectangle. */
  bool updateTrackingRectangle();

  /** Helper function that handles tracking estimating an affince transformation of a rectangle. */
  bool updateTrackingRectangleHomography();

  int verbose_; /**< Defines level of additional information. */
  bool ready_; /**< Ready to process data. */

  Cuda::DeviceMemoryLinear<float,1>* gradient_; /** Color gradient for creating colored flow field. */

  FlowParameters flow_params_; /**< Parameters to initialize flow calculation. This is not used if a data structure is available! Then the parameters are set directly. */

  CudaFlowLib::Model model_; /**< Currently selected model. */
  CudaFlowLib::BaseData* data_; /**< Current data structure. */
  std::map<CudaFlowLib::Model, std::string> model_abrv_map_; /**< Model abbrevations list. */
  std::vector<std::string> model_abrvs_; /**< Model abbrevations list. */
  std::map<CudaFlowLib::Model, std::string> model_desc_map_; /**< Model descriptions list. */
  std::map<std::string, CudaFlowLib::Model> abrv_model_map_;  /**< Maps model abbrevations from a string to the Model enum. */

  TRACKING_MODE tracking_mode_;
  TrackingRectangle tracking_rectangle_;
  Cuda::DeviceMemoryPitched<float,2>* tracking_buffer_; /**< Buffer used for tracking. */
};


#endif //FLOWLIB_H
