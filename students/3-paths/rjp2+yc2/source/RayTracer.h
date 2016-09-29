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
    TriTree tris;
    Array<shared_ptr<Light>> lights;



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

    void findIntersections(const Array<Ray>& rayBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, const Array<Color3>& modulationBuffer) const;

    /** Main ray tracing method. Finds radiance along ray coming from first intersecting object (looped over).
        Sums L_lights and recursive trace at location to find radiance
    */
    virtual Radiance3 trace(const Ray& ray, const Array<shared_ptr<Light>>& lightArray, const float& indirectCount, const int& recursionsLeft = 0, const bool isEyeRay = true) const;


    /** Constructor */
    RayTracer(Array<shared_ptr<Surface>> surfaces);

};




