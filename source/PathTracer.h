#pragma once
#include <G3D/G3DAll.h>

/**
    Performs ray tracing on the given ray, looking through all surfaces in the scene.
*/

class RayTracer {
protected:

    /** Handling for float precision and ray bump */
    const float EPSILON = 0.0001f;

    /** TriTree used to iterate through all triangles in the scene */
    //TriTree tris;
    //Array<shared_ptr<Light>> lights;



        /**
            checks if individual light is illuminating point using intersection
            called from getDirectLight
        */
    virtual bool isLightVisible(const Ray ray, const shared_ptr<Light> light) const;

    /**
        Calculates direct illumination term at a surfel.
    */
    Radiance3 getDirectLight(const Ray& ray, const shared_ptr<Surfel>& surfel, const Array<shared_ptr<Light>>& lightArray) const;



public:

    void renderScene(const shared_ptr<Scene>& scene, const shared_ptr<Image>& image, Stopwatch& stopWatch, int raysPerPixel = 5, bool multithreading = true, int scatteringEvents = 1) const;

    void RayTracer::generateRays(Array<Ray>& rayBuffer, const shared_ptr<Scene>& scene,  const int& width, const int& height, const bool& multithreading) const;

    void traceIntersections(const Array<Ray>& rayBuffer, Array<shared_ptr<Surfel>>& surfelBuffer, const int& numPixels, const bool& multithreading) const;

    void chooseLights(const Array<shared_ptr<Light>>& lightArray, const Array<shared_ptr<Surfel>>& surfelBuffer, Array<Biradiance3>& biradianceBuffer, const Array<Ray>& shadowRayBuffer, const int& numPixels, const bool& multithreading) const;

    void testVisibility(const Array<Ray>& shadowRayBuffer, const Array<Biradiance3>& biradianceBuffer, const Array<bool>& lightShadowedBuffer, const int& numPixels, const bool& multithreading) const;
 
    void generateRecursiveRays(Array<Ray>& rayBuffer, const int& numPixels, const bool& multithreading) const;

    void updateModulation(Array<Color3>& modulationBuffer, const int& numPixels, const bool& multithreading) const;

    void writeToImage(const shared_ptr<Image>& image, const Array<Biradiance3>& biradianceBuffer, const Array<bool>& lightShadowedBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, const bool& multithreading) const;



       /** Main ray tracing method. Finds radiance along ray coming from first intersecting object (looped over).
        Sums L_lights and recursive trace at location to find radiance
    */
    virtual Radiance3 trace(const Ray& ray, const Array<shared_ptr<Light>>& lightArray, const float& indirectCount, const int& recursionsLeft = 0, const bool isEyeRay = true) const;


    /** Constructor */
    //RayTracer(Array<shared_ptr<Surface>> surfaces);
    RayTracer();
};




