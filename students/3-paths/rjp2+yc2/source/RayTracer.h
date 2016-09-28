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
    //std::vector<OurSphere> spheres;


        /**
            checks if individual light is illuminating point using intersection
            called from getDirectLight
        */
    virtual bool isLightVisible(const Ray ray, const shared_ptr<Light> light) const;

    /**
        Calculates direct illumination term at a surfel.
    */
    Radiance3 getDirectLight(const Ray& ray, const shared_ptr<Surfel> surfel, const Array<shared_ptr<Light>>& lightArray) const;


    /**
        Finds if \a sphere intersects \a ray and updates \a surfel with relevant information.
        \return true if \a sphere is intersected.
        Note that \a surfel will not have updated values if \a sphere is not intersected, and its
        member variables will be non-deterministic.
    */
    //bool sphereIntersect(const OurSphere& sphere, const Ray& ray, shared_ptr<UniversalSurfel>& surfel) const;

    

    /**
        Finds if \a tri intersects \a ray and updates \a hit with relevant information.
        \return true if \a sphere is intersected.
        Note that \a hit will not have updated its member variables if \a tri is not intersected, and its
        values will be non-deterministic.
    */
    bool triangleIntersect(const Tri& tri, const Ray &ray, TriTree::Hit& hit) const;





    /**
        Returns the first surfel hit by the given ray. \return nullptr if no scene objects lie in the ray's path
    */
    const shared_ptr<Surfel> intersectRay(const Ray& ray) const;


public:

    /** Main ray tracing method. Finds radiance along ray coming from first intersecting object (looped over).
        Sums L_lights and recursive trace at location to find radiance
    */
    virtual Radiance3 trace(const Ray& ray, const Array<shared_ptr<Light>>& lightArray, const float& indirectCount, const int& recursionsLeft = 0, const bool isEyeRay = true) const;

    /** Constructor */
    RayTracer(Array<shared_ptr<Surface>> surfaces);

};


/** Custom sphere class. Most likely broken */
class OurSphere {
public:
    Vector3 m_center;
    float m_radius;

    OurSphere(Vector3 center, float radius) {
        m_center = center;
        m_radius = radius;
    }
};


