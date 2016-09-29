#include "RayTracer.h"
#include "App.h"


/** constructs TriTree object used in RayTracer::intersectRay and RayTracer::triangleIntersect */
RayTracer::RayTracer(Array<shared_ptr<Surface>> surfaces) {
    tris.setContents(surfaces);
}
//
//void RayTracer::renderScene(shared_ptr<Scene>& scene, shared_ptr<Image> image, Stopwatch& stopWatch, int raysPerPixel = 5, bool multithreading = true, int numIndirectRays = 1) const {
//    int height = image->height();
//    int width = image->width();
//
//    //// Set up a tri-tree representing the scene 
//    //Array<shared_ptr<Surface>> surfaces;
//    //scene->onPose(surfaces);
//
//
//    //// Grab light array for the scene
//    //Array<shared_ptr<Light>> lightArray;
//    //scene->getTypedEntityArray(lightArray);
//
//    stopWatch.tick();
//
//    Thread::runConcurrently(Point2int32(0, 0), Point2int32(width, height), [&](Point2int32 coord) {
//        Ray eyeRay = scene->defaultCamera()->worldRay(coord.x, coord.y, Rect2D(Vector2(image->width(), image->height())));
//
//        // This loop would let us expand the rendering algorithm to use multiple ray samples per pixel
//        Radiance3 sum = Radiance3::black();
//        int recursionDepth = 1;
//        for (int i = 0; i < raysPerPixel; ++i) {
//            sum += trace(eyeRay, lightArray, numIndirectRays, recursionDepth);
//        }
//        sum /= raysPerPixel;
//
//        image->set(Point2int32(coord.x, coord.y), sum);
//    }, !multithreading);
//
//    stopWatch.tock();
//}


Radiance3 RayTracer::trace(const Ray& ray, const Array<shared_ptr<Light>>& lightArray, const float& indirectCount, const int& recursionsLeft, const bool isEyeRay) const {

    // Find intersection between ray and scene
    const shared_ptr<Surfel> surfel = tris.intersectRay(ray);

    if (notNull(surfel)) {
        Radiance3 emittedLight = surfel->emittedRadiance(-ray.direction());
        Radiance3 directLight = getDirectLight(ray, surfel, lightArray);
        Radiance3 indirectLight = Radiance3(0.0f, 0.0f, 0.0f);

        // indirect recursive call
        if (indirectCount > 0) {
            if (recursionsLeft > 0) {
                for (int i = 0; i < indirectCount; ++i) {
                    const Vector3 direction = G3D::Vector3::cosHemiRandom(surfel->shadingNormal);
                    const Point3 surfelPosition = surfel->position;
                    Point3 bumpedPoint = surfelPosition + (EPSILON * surfel->geometricNormal * (-sign(surfel->geometricNormal.dot(-direction))));

                    Ray recursiveRay(bumpedPoint, direction);
                    const Color3 scatter = surfel->finiteScatteringDensity(recursiveRay.direction(), -ray.direction());
                    const Radiance3 recursion = trace(recursiveRay, lightArray, indirectCount, recursionsLeft - 1, false);
                    indirectLight += recursion * scatter / indirectCount;
                }
            }
        }
        else {
            indirectLight += surfel->reflectivity(Random::threadCommon()) * 0.05f;
        }

        return emittedLight + directLight + indirectLight;
    }
    else {
        return Radiance3(0, 0, 0);
    }
}


Radiance3 RayTracer::getDirectLight(const Ray& ray,
    const shared_ptr<Surfel>& surfel,
    const Array<shared_ptr<Light>>& lightArray) const {

    const Point3& pos = surfel->position;
    const Vector3& n = surfel->shadingNormal;

    Radiance3 directLight;

    int numLights = lightArray.length();
    for (int i = 0; i < numLights; ++i) {
        const shared_ptr<Light>& light = lightArray[i];

        const Vector3 directionToLight = (light->position().xyz() - pos).direction();

        //Vector3 directionFromLight = (surfelPosition - light->position().xyz()).direction();
        const Point3 bumpedPoint = pos + (EPSILON * surfel->geometricNormal * (-sign(surfel->geometricNormal.dot(-directionToLight))));

        //directionToLight = (light->position().xyz() - bumpedPoint).direction();
        const Ray lightCheckRay(bumpedPoint, directionToLight);

        // shading
        if (isLightVisible(lightCheckRay, light)) {
            const Biradiance3& B = light->biradiance(pos);
            Color3 f = surfel->finiteScatteringDensity(lightCheckRay.direction(), -ray.direction());

            directLight += B * f * abs(n.dot(lightCheckRay.direction()));
        }
    }

    return directLight;
}



bool RayTracer::isLightVisible(const Ray ray, const shared_ptr<Light> light) const {

    if (!light->castsShadows()) return true;

    const shared_ptr<Surfel> surfel = tris.intersectRay(ray);

    if (notNull(surfel)) {
        const float intersectDist = (surfel->position - ray.origin()).squaredLength();
        const float lightDist = (light->position().xyz() - ray.origin()).squaredLength();

        const bool isBlocked = (intersectDist < lightDist - EPSILON);
        return !isBlocked;
    }

    return true;
}
