#include "PathTracer.h"
#include "App.h"


/** constructs TriTree object used in PathTracer::intersectRay and PathTracer::triangleIntersect */
PathTracer::PathTracer(shared_ptr<Scene> scene) {
    setScene(scene);
}

void PathTracer::setScene(shared_ptr<Scene> scene) {
    m_scene = scene;
    // Set up tri-tree representing the scene 
    Array<shared_ptr<Surface>> surfaces;
    scene->onPose(surfaces);
    m_tris.setContents(surfaces);
}

void PathTracer::renderScene(const shared_ptr<Image>& image, Stopwatch& stopWatch, int raysPerPixel, bool multithreading, int scatteringEvents) const {
    //Scene changed
    if (false) {
        // extract surfaces from the scene
        // rebuild the tree
    }

    // Grab light array for the scene
    Array<shared_ptr<Light>> lightArray;
    m_scene->getTypedEntityArray(lightArray);

    // Start timing the actual rendering process (so dont take time to build data structures into account)
    stopWatch.tick();

    const int height = image->height();
    const int width = image->width();
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
        generateRays(rayBuffer, width, height, multithreading);

        // Iterate over num scattering events
        for (int i = 0; i < scatteringEvents + 1; ++i) {

            // Find intersected surfels
            traceIntersections(rayBuffer, surfelBuffer, numPixels, multithreading);

            // Get radiance from direct lights
            if (lightArray.size() > 0) {
                // Get biradiance values and shadow rays from randomly chosen light
                chooseLights(lightArray, surfelBuffer, biradianceBuffer, shadowRayBuffer, numPixels, multithreading);

                // Test whether light is actually visible
                testVisibility(shadowRayBuffer, surfelBuffer, lightShadowedBuffer, numPixels, multithreading);
            }

            writeToImage(image, biradianceBuffer, lightShadowedBuffer, shadowRayBuffer, surfelBuffer, rayBuffer, modulationBuffer, multithreading);

            // Generate recursive rays and update modulationBuffer
            generateRecursiveRays(rayBuffer, modulationBuffer, surfelBuffer, numPixels, multithreading);
        }
    }
}

void PathTracer::writeToImage(const shared_ptr<Image>& image, const Array<Biradiance3>& biradianceBuffer, const Array<bool>& lightShadowedBuffer, const Array<Ray>& shadowRayBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, Array<Ray>& rayBuffer, Array<Color3>& modulationBuffer, const bool& multithreading) const {
    int width = image->width();
    int height = image->height();

    Thread::runConcurrently(G3D::Point2int32(0, 0), G3D::Point2int32(width, height), [&](G3D::Point2int32 coord) {
        int i = coord.y * width + coord.x;
        const Point2int32 pixel = Point2int32(coord.x, coord.y);
        Radiance3 radiance;
        image->get(pixel, radiance);

        if (!lightShadowedBuffer[i]) {
   
            const Radiance3& emittedLight = surfelBuffer[i]->emittedRadiance(-rayBuffer[i].direction());
            const Radiance3& directLight = biradianceBuffer[i];
            Color3 scatteringDensity = surfelBuffer[i]->finiteScatteringDensity(-shadowRayBuffer[i].direction(), -rayBuffer[i].direction());

            radiance += emittedLight + (directLight * modulationBuffer[i] * scatteringDensity * abs(surfelBuffer[i]->geometricNormal.dot(-shadowRayBuffer[i].direction())));

            image->set(pixel, radiance);
        } 
    }, !multithreading);
}


void PathTracer::generateRecursiveRays(Array<Ray>& rayBuffer, Array<Color3>& modulationBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, const int& numPixels, const bool& multithreading) const {
    Thread::runConcurrently(0, numPixels, [&](int i) {
        const shared_ptr<Surfel>& surfel = surfelBuffer[i];
        //YAAAAAK
        if (notNull(surfel)) {
            // Use scatter to populate new ray direction, and our scatter weight
            const Vector3& directionOut = -rayBuffer[i].direction();
            Vector3 directionIn;
            Color3 weight;
            surfel->scatter(PathDirection::EYE_TO_SOURCE, directionOut, false, Random::threadCommon(), weight, directionIn);

            // Calculated bumped point
            Point3 bumpedPoint = surfel->position + (EPSILON * surfel->geometricNormal * (-sign(surfel->geometricNormal.dot(-directionIn))));

            // Store recursive ray
            rayBuffer[i] = Ray(bumpedPoint, -directionIn);

            // Store modulation?
            modulationBuffer[i] = weight * modulationBuffer[i];
        }
        //const Color3 scatter = surfel->finiteScatteringDensity(recursiveRay.direction(), -ray.direction());
    }, !multithreading);
}


void PathTracer::testVisibility(const Array<Ray>& shadowRayBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, Array<bool>& lightShadowedBuffer, const int& numPixels, const bool& multithreading) const {
    Thread::runConcurrently(0, numPixels, [&](int i) {
        // TODO Should we worry about not having this??
        //if (!light->castsShadows()) return true;

        const Ray& shadowRay = shadowRayBuffer[i];

        // intersectedSurfel closest surfel to light along shadowRay, chosenSurfel surfel we originally hit
        const shared_ptr<Surfel>& intersectedSurfel = m_tris.intersectRay(shadowRay);
        const shared_ptr<Surfel>& chosenSurfel = surfelBuffer[i];

        if (notNull(chosenSurfel) && isNull(intersectedSurfel)) {
            printf("h");        }

        if (isNull(chosenSurfel)) {
            lightShadowedBuffer[i] = true;
        } else if (isNull(intersectedSurfel)) {
            lightShadowedBuffer[i] = false;
        } else {
            // Calculate distances
            const float intersectDist = (intersectedSurfel->position - shadowRay.origin()).squaredLength();
            const float chosenDist = (chosenSurfel->position - shadowRay.origin()).squaredLength();

            // Check whether first surfel intersected is our surfel
            const bool isBlocked = (intersectDist < chosenDist - EPSILON);
            lightShadowedBuffer[i] = isBlocked;
        }

    }, !multithreading);
}




void PathTracer::chooseLights(const Array<shared_ptr<Light>>& lightArray, const Array<shared_ptr<Surfel>>& surfelBuffer, Array<Biradiance3>& biradianceBuffer, Array<Ray>& shadowRayBuffer, const int& numPixels, const bool& multithreading) const {

    // Calculate biradiance from each light source
    Thread::runConcurrently(0, numPixels, [&](int i) {
        const shared_ptr<Surfel>& surfel = surfelBuffer[i];
        if (notNull(surfel)) {
            Point3 surfelPos = surfel->position;

            // Calculate total biradiance
            int sum = 0;
            for (int j = 0; j < lightArray.size(); ++j) {
                const Biradiance3& B = lightArray[j]->biradiance(surfelPos);
                sum += B.sum();
            }

            int lightPos = 0;
            // Select random light
            // TODO make random bewteen 0 and sum
            int counter = Random::threadCommon().integer(0, sum);
            for (int j = 0; j < lightArray.size(); ++j) {
                counter -= lightArray[j]->biradiance(surfelPos).sum();
                if (counter < 0) {
                    lightPos = j;
                    break;
                }
            }

            const shared_ptr<Light>& light = lightArray[lightPos];

            // Store biradiance from light
            biradianceBuffer[i] = light->biradiance(surfelPos);

            // Create shadow ray from light to surfel
            const Vector3 directionFromLight = (surfelPos - light->position().xyz()).direction();
            // const Point3 bumpedPoint = surfelPos + (EPSILON * surfel->geometricNormal * (-sign(surfel->geometricNormal.dot(directionFromLight))));
            Ray shadowRay = Ray(light->position().xyz(), directionFromLight);

            // Store shadow Ray
            shadowRayBuffer[i] = shadowRay;
        }
    }, !multithreading);
}


void PathTracer::generateRays(Array<Ray>& rayBuffer, const int& width, const int& height, const bool& multithreading) const {
    Thread::runConcurrently(G3D::Point2int32(0, 0), G3D::Point2int32(width, height), [&](G3D::Point2int32 coord) {
        // TODO bump these around a bit
        Ray ray = m_scene->defaultCamera()->worldRay(coord.x, coord.y, Rect2D(Vector2(width, height)));
        rayBuffer[width * coord.y + coord.x] = ray;
    }, !multithreading);
}



void PathTracer::traceIntersections(const Array<Ray>& rayBuffer, Array<shared_ptr<Surfel>>& surfelBuffer, const int& numPixels, const bool& multithreading) const {
    // Find intersection
    m_tris.intersectRays(rayBuffer, surfelBuffer);

    //Thread::runConcurrently(0, numPixels, [&](int i) {
    //    const shared_ptr<Surfel>& surfel = tris.intersectRay(rayBuffer[i]);
    //    surfelBuffer[i] = surfel;

    //    //// Add emmited light from objects / scene
    //    //if (notNull(surfel)) {
    //    //    // Emmited light from surfel
    //    //    modulationBuffer[i] += surfel->emittedRadiance(rayBuffer[i].direction());
    //    //}
    //    //else {
    //    //    // Emmited light from skybox
    //    //    // TODO change this so it actualy works
    //    //    modulationBuffer[i] += Color3::black();
    //    //}
    //}, !multithreading);
}



Radiance3 PathTracer::trace(const Ray& ray, const Array<shared_ptr<Light>>& lightArray, const float& indirectCount, const int& recursionsLeft, const bool isEyeRay) const {

    // Find intersection between ray and scene
    const shared_ptr<Surfel> surfel = m_tris.intersectRay(ray);

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


Radiance3 PathTracer::getDirectLight(const Ray& ray,
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




bool PathTracer::isLightVisible(const Ray ray, const shared_ptr<Light> light) const {

    if (!light->castsShadows()) return true;

    const shared_ptr<Surfel> surfel = m_tris.intersectRay(ray);

    if (notNull(surfel)) {
        const float intersectDist = (surfel->position - ray.origin()).squaredLength();
        const float lightDist = (light->position().xyz() - ray.origin()).squaredLength();

        const bool isBlocked = (intersectDist < lightDist - EPSILON);
        return !isBlocked;
    }

    return true;
}