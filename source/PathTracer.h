#pragma once
#include <G3D/G3DAll.h>

/**
    Performs ray tracing on the given ray, looking through all surfaces in the scene.
*/

class PathTracer {
protected:

    /** Handling for float precision and ray bump */
    const float EPSILON = 0.0001f;

    /** TriTree used to iterate through all triangles in the scene */
    TriTree m_tris;
    shared_ptr<Scene> m_scene;
    //Array<shared_ptr<Light>> lights;

    RealTime m_lastTreeBuildTime;

        /**
            checks if individual light is illuminating point using intersection
            called from getDirectLight
        */
    virtual bool isLightVisible(const Ray ray, const shared_ptr<Light> light) const;

    /**
        Calculates direct illumination term at a surfel.
    */
    Radiance3 getDirectLight(const Ray& ray, const shared_ptr<Surfel>& surfel, const Array<shared_ptr<Light>>& lightArray) const;


        /***
       Pre: Scene and image size
       Post: rayBuffer will be filled with one ray for each pixel in the image
    */
    void PathTracer::generateRays(Array<Ray>& rayBuffer, const int& width, const int& height, const bool& multithreading) const;


     /***
       Pre: Filled rayBuffer and image size
       Post: surfelBuffer will have one intersected surfel for each pixel
    */
    void traceIntersections(const Array<Ray>& rayBuffer, Array<shared_ptr<Surfel>>& surfelBuffer, const bool& multithreading) const;

     /***
       Pre: Filled lightArray, filled surfelBuffer and image size
       Post: biradianceBuffer filled for each pixel, shadowRayBuffer filled for each pixel
    */
    void chooseLights(const Array<shared_ptr<Light>>& lightArray, const Array<shared_ptr<Surfel>>& surfelBuffer, Array<Biradiance3>& biradianceBuffer, Array<Ray>& shadowRayBuffer, const bool& multithreading) const;

    /***
       Pre: Filled shadowRayBuffer, filled surfelBuffer
       Post: lightShadowedBuffer contaning whether light is visible for each pixel
    */
    void testVisibility(const Array<Ray>& shadowRayBuffer,  const Array<shared_ptr<Surfel>>& surfelBuffer, Array<bool>& lightShadowedBuffer, const bool& multithreading) const;
    
     /***
       Pre: Filled rayBuffer, and filled surfelBuffer
       Post: filled rayBuffer with one recursive ray for each pixel, and updated modulationBuffer
    */
    void generateRecursiveRays(Array<Ray>& rayBuffer, Array<Color3>& modulationBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, const bool& multithreading) const;

     /***
       Pre: Filled rayBuffer, filled surfelBuffer
       Post: updated modulationBuffer constianing new scattering weight for each pixel
    */
    //void updateModulation(Array<Color3>& modulationBuffer, Array<Ray>& rayBuffer,  const Array<shared_ptr<Surfel>>& surfelBuffer, const int& numPixels, const bool& multithreading) const;

    /***
       Pre: Filled rayBuffer, filled biradianceBuffer, filled lightShadowedBuffer
       Post: Weighted biradiance data added to each pixel
    */
    void writeToImage(const shared_ptr<Image>& image, const Array<Biradiance3>& biradianceBuffer, const Array<bool>& lightShadowedBuffer, const Array<Ray>& shadowRayBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, Array<Ray>& rayBuffer, Array<Color3>& modulationBuffer, const bool& multithreading) const;


public:

      bool m_eyeRayTest = false;
      bool m_hitsTest = false;
      bool m_geoNormalsTest = false;
      

    /** Constructor */
    PathTracer(shared_ptr<Scene> scene = nullptr);

    void setScene(shared_ptr<Scene> scene);
    

    void renderScene(const shared_ptr<Image>& image, Stopwatch& stopWatch, int raysPerPixel = 1, bool multithreading = true, int scatteringEvents = 0);



     /** Main ray tracing method. Finds radiance along ray coming from first intersecting object (looped over).
        Sums L_lights and recursive trace at location to find radiance
    */
    virtual Radiance3 trace(const Ray& ray, const Array<shared_ptr<Light>>& lightArray, const float& indirectCount, const int& recursionsLeft = 0, const bool isEyeRay = true) const;

   
};





