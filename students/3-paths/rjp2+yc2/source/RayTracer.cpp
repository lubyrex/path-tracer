#include "RayTracer.h"
#include "App.h"


/** constructs TriTree object used in RayTracer::intersectRay and RayTracer::triangleIntersect */
RayTracer::RayTracer() {
    //tris.setContents(surfaces);
}

void RayTracer::renderScene(const shared_ptr<Scene>& scene, const shared_ptr<Image>& image, Stopwatch& stopWatch, int raysPerPixel, bool multithreading, int scatteringEvents) const {
    //Scene changed
    if (false) {
        // extract surfaces from the scene
        // rebuild the tree
    }
    int height = image->height();
    int width = image->width();

    // Why do we have to set to all black? to reset the picture??
    // Not do this since we start each pixel radiance sum at black
    //image->setAll(Color3::black());

    // Set up a tri-tree representing the scene 
    Array<shared_ptr<Surface>> surfaces;
    scene->onPose(surfaces);
    TriTree tris;
    tris.setContents(surfaces);

    // Grab light array for the scene
    Array<shared_ptr<Light>> lightArray;
    scene->getTypedEntityArray(lightArray);

    // Initalize path tracer
    //RayTracer tracer(surfaces);

    // Start timing the actual rendering process (so dont take time to build data structures into account)
    stopWatch.tick();

    int recursionDepth = 1;

    const int numPixels = width * height;

    Array<Color3> modulationBuffer;
    Array<Ray> rayBuffer;
    Array<shared_ptr<Surfel>> surfelBuffer;
    Array<Biradiance3> biradianceBuffer;
    Array<Ray> shadowRayBuffer;
    Array<bool> lightShadowedBuffer;

    modulationBuffer.resize(numPixels);
    modulationBuffer.setAll(Color3(1 / (float)raysPerPixel));

    rayBuffer.resize(numPixels);
    surfelBuffer.resize(numPixels);
    biradianceBuffer.resize(numPixels);
    shadowRayBuffer.resize(numPixels);
    lightShadowedBuffer.resize(numPixels);

    // Iterate over num rays per pixel
    for (int i = 0; i < raysPerPixel; ++i) {
        // Generate all rays
        generateRays(rayBuffer, scene, width, height, multithreading);

        // Iterate over num scattering events
        for (int i = 0; i < scatteringEvents; ++i) {

            // Put emitted light from ray intersections into modulation buffer
            traceIntersections(rayBuffer, surfelBuffer, numPixels, multithreading);

            // Get radiance from direct lights
            if (lightArray.size() > 0) {
                // Get biradiance values and shadow rays from randomly chosen light
                chooseLights(lightArray, surfelBuffer, biradianceBuffer, shadowRayBuffer, numPixels, multithreading);

                // Test whether light is actually visible
                testVisibility(shadowRayBuffer, biradianceBuffer, lightShadowedBuffer, numPixels, multithreading);
            }

            // Generate recursive rays
            generateRecursiveRays(rayBuffer, numPixels, multithreading);

            // Update modulation buffer
            updateModulation(modulationBuffer, numPixels, multithreading);

            writeToImage(image, biradianceBuffer, lightShadowedBuffer, surfelBuffer, multithreading);
        }
    }
}

void RayTracer::writeToImage(const shared_ptr<Image>& image, const Array<Biradiance3>& biradianceBuffer, const Array<bool>& lightShadowedBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, const bool& multithreading) const {
    int width = image->width();
    int height = image->height();

    Thread::runConcurrently(G3D::Point2int32(0, 0), G3D::Point2int32(width, height), [&](G3D::Point2int32 coord) {

    }, !multithreading);
}



void RayTracer::updateModulation(Array<Color3>& modulationBuffer, const int& numPixels, const bool& multithreading) const {
    Thread::runConcurrently(0, numPixels, [&](int i) {

    }, !multithreading);
}

void RayTracer::generateRecursiveRays(Array<Ray>& rayBuffer, const int& numPixels, const bool& multithreading) const {
    Thread::runConcurrently(0, numPixels, [&](int i) {

    }, !multithreading);
}


void RayTracer::testVisibility(const Array<Ray>& shadowRayBuffer, const Array<Biradiance3>& biradianceBuffer, const Array<bool>& lightShadowedBuffer, const int& numPixels, const bool& multithreading) const {
    Thread::runConcurrently(0, numPixels, [&](int i) {

    }, !multithreading);
}


void RayTracer::chooseLights(const Array<shared_ptr<Light>>& lightArray, const Array<shared_ptr<Surfel>>& surfelBuffer, const Array<Biradiance3>& biradianceBuffer, const Array<Ray>& shadowRayBuffer, const int& numPixels, const bool& multithreading) const {

    // Calculate biradiance from each light source
    Thread::runConcurrently(0, numPixels, [&](int i) {
        const shared_ptr<Surfel>& surfel = surfelBuffer[i];
        if (notNull(surfel)) {
            Point3 pos = surfel->position;

            // Calculate total biradiance
            int sum = 0;
            for (int j = 0; j < lightArray.size(); ++j) {
                const Biradiance3& B = lightArray[j]->biradiance(pos);
                sum += B.sum();
            }

            // Select random light
            // TODO make random
            int counter = 10;
            for (int j = 0; j < lightArray.size(); ++j) {
                counter -= lightArray[i]->biradiance(pos).sum();
            }
        }

    }, !multithreading);
}


void RayTracer::generateRays(Array<Ray>& rayBuffer, const shared_ptr<Scene>& scene, const int& width, const int& height, const bool& multithreading) const {
    Thread::runConcurrently(G3D::Point2int32(0, 0), G3D::Point2int32(width, height), [&](G3D::Point2int32 coord) {
        // TODO bump these around a bit
        Ray ray = scene->defaultCamera()->worldRay(coord.x, coord.y, Rect2D(Vector2(width, height)));
        rayBuffer[coord.x * coord.y + coord.x] = ray;
    }, !multithreading);
}



void RayTracer::traceIntersections(const Array<Ray>& rayBuffer, Array<shared_ptr<Surfel>>& surfelBuffer, const int& numPixels, const bool& multithreading) const {
    // Find intersection
    Thread::runConcurrently(0, numPixels, [&](int i) {
        const shared_ptr<Surfel>& surfel = tris.intersectRay(rayBuffer[i]);
        surfelBuffer[i] = surfel;

        //// Add emmited light from objects / scene
        //if (notNull(surfel)) {
        //    // Emmited light from surfel
        //    modulationBuffer[i] += surfel->emittedRadiance(rayBuffer[i].direction());
        //}
        //else {
        //    // Emmited light from skybox
        //    // TODO change this so it actualy works
        //    modulationBuffer[i] += Color3::black();
        //}
    }, !multithreading);
}



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
