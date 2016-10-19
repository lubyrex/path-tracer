#include "PathTracer.h"
#include "App.h"


/** constructs TriTree object used in PathTracer::intersectRay and PathTracer::triangleIntersect */
PathTracer::PathTracer(shared_ptr<Scene> scene) {
  
    setScene(scene);

}

void PathTracer::setScene(shared_ptr<Scene> scene) {
    m_scene = scene;

    Array<shared_ptr<Surface>> surfaces;
    m_scene->onPose(surfaces);
    m_tris.setContents(surfaces);
   
}

void PathTracer::renderScene(const shared_ptr<Image>& image, Stopwatch& stopWatch, int raysPerPixel, bool multithreading, int scatteringEvents, shared_ptr<Camera> camera) {

    m_camera=camera;


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
    rayBuffer.resize(numPixels);
    surfelBuffer.resize(numPixels);
    biradianceBuffer.resize(numPixels);
    shadowRayBuffer.resize(numPixels);
    lightShadowedBuffer.resize(numPixels);

    // Iterate over num rays per pixel
    for (int i = 0; i < raysPerPixel; ++i) {
        const String& caption = format("Iteration: %i of %i", i, raysPerPixel - 1);
        debugPrintf("%s\n", caption.c_str());

        // Generate all rays
        generateRays(rayBuffer, width, height, multithreading);
        modulationBuffer.setAll(Color3(1.0f / (float)raysPerPixel));

        // Iterate over num scattering events
        for (int j = 0; j < scatteringEvents + 1; ++j) {


            // Find intersected surfels
            traceIntersections(rayBuffer, surfelBuffer, multithreading);

            // Get radiance from direct lights
            if (lightArray.size() > 0) {
                // Get biradiance values and shadow rays from randomly chosen lights
                chooseLights(lightArray, surfelBuffer, biradianceBuffer, shadowRayBuffer, multithreading);

                // Test whether lights are actually visible
                testVisibility(shadowRayBuffer, surfelBuffer, lightShadowedBuffer, multithreading);

                writeToImage(image, biradianceBuffer, lightShadowedBuffer, shadowRayBuffer, surfelBuffer, rayBuffer, modulationBuffer, multithreading);
            }

            // Generate recursive rays and update modulationBuffer
            generateRecursiveRays(rayBuffer, modulationBuffer, surfelBuffer, multithreading);
            //debugPrintf("%d raysPerPixel %d scatteringEvents",i,j);
        }

    }
    stopWatch.tock();
}

void PathTracer::writeToImage(const shared_ptr<Image>& image, const Array<Biradiance3>& biradianceBuffer, const Array<bool>& lightShadowedBuffer, const Array<Ray>& shadowRayBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, Array<Ray>& rayBuffer, Array<Color3>& modulationBuffer, const bool& multithreading) const {
    int height = image->height();
    int width = image->width();

    Thread::runConcurrently(G3D::Point2int32(0, 0), G3D::Point2int32(width, height), [&](G3D::Point2int32 coord) {
        int i = coord.y * width + coord.x;
        const Point2int32 pixel = Point2int32(coord.x, coord.y);

        if (m_eyeRayTest) {
            Vector3 r = rayBuffer[i].direction();
            Radiance3 radiance = Radiance3(r.x + 1, r.y + 1, r.z + 1) / 2.0f;
            image->increment(pixel, radiance);
        }
        else if (m_hitsTest) {
            if (notNull(surfelBuffer[i])) {
                Point3 p = surfelBuffer[i]->position;
                Radiance3 radiance = Radiance3(p.x*0.3f + 0.5f, p.y*0.3f + 0.5f, p.z*0.3f + 0.5f);
                image->increment(pixel, radiance);
            }
        }
        else if (m_geoNormalsTest) {
            if (notNull(surfelBuffer[i])) {

                Point3 n = surfelBuffer[i]->geometricNormal;
                Radiance3 radiance = Radiance3(n.x + 1, n.y + 1, n.z + 1) / 2.0f;
                image->increment(pixel, radiance);
            }
        }
        else {

            if (notNull(surfelBuffer[i])) {

                if (!lightShadowedBuffer[i]) {
                    const Biradiance3 B = biradianceBuffer[i];
                    const Vector3 n = surfelBuffer[i]->geometricNormal;
                    const Vector3 w_i = shadowRayBuffer[i].direction();
                    const Vector3 w_o = rayBuffer[i].direction();
                    const Color3 f = surfelBuffer[i]->finiteScatteringDensity(w_i, w_o);
                    const Color3 mod = modulationBuffer[i];

                    const Radiance3& emittedLight = surfelBuffer[i]->emittedRadiance(w_o) * mod;

                    Radiance3 radiance = emittedLight + B * mod * f * abs(n.dot(w_i));

                    image->increment(pixel, radiance);
                }
                else {
                    const Vector3 w_o = rayBuffer[i].direction();
                    const Color3 mod = modulationBuffer[i];

                    const Radiance3& emittedLight = surfelBuffer[i]->emittedRadiance(w_o) * mod;

                    image->increment(pixel, emittedLight);
                }
            }
        }
    }, !multithreading);
}


void PathTracer::generateRecursiveRays(Array<Ray>& rayBuffer, Array<Color3>& modulationBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, const bool& multithreading) const {
    Thread::runConcurrently(0, rayBuffer.size(), [&](int i) {
        const shared_ptr<Surfel>& surfel = surfelBuffer[i];
        //YAAAAAK
        if (notNull(surfel)) {
            // Use scatter to populate new ray direction, and our scatter weight
            const Vector3& w_o = -rayBuffer[i].direction();
            Vector3 w_i;
            Color3 weight;
            surfel->scatter(PathDirection::EYE_TO_SOURCE, w_o, false, Random::threadCommon(), weight, w_i);

            // Calculated bumped point
            // Should directionIn be negated?
            Point3 bumpedPoint = surfel->position + (EPSILON * surfel->shadingNormal) * (-sign(surfel->geometricNormal.dot(-w_i)));

            // Store recursive ray
            rayBuffer[i] = Ray(bumpedPoint, w_i);

            // Store modulation?
            modulationBuffer[i] = weight * modulationBuffer[i];
        }
    }, !multithreading);
}


void PathTracer::testVisibility(const Array<Ray>& shadowRayBuffer, const Array<shared_ptr<Surfel>>& surfelBuffer, Array<bool>& lightShadowedBuffer, const bool& multithreading) const {
    m_tris.intersectRays(shadowRayBuffer, lightShadowedBuffer, TriTree::COHERENT_RAY_HINT | TriTree::DO_NOT_CULL_BACKFACES | TriTree::OCCLUSION_TEST_ONLY);
}




void PathTracer::chooseLights(const Array<shared_ptr<Light>>& lightArray, const Array<shared_ptr<Surfel>>& surfelBuffer, Array<Biradiance3>& biradianceBuffer, Array<Ray>& shadowRayBuffer, const bool& multithreading) const {

    // Calculate biradiance from each light source
    Thread::runConcurrently(0, surfelBuffer.size(), [&](int i) {
        const shared_ptr<Surfel>& surfel = surfelBuffer[i];

        if (notNull(surfel)) {
            Point3 surfelPos = surfel->position;
            shared_ptr<Light> light;
            float totalBiradiance = 0.0f;

            
                // Calculate total biradiance
                // int total = 0;
                for (int j = 0; j < lightArray.size(); ++j) {
                    const Biradiance3& B = lightArray[j]->biradiance(surfelPos);
                    totalBiradiance += B.sum();
                }

                int lightPos = 0;
                // Select random light
             
                float counter = Random::threadCommon().uniform(0.0f, totalBiradiance);
                for (int j = 0; j < lightArray.size(); ++j) {
                    counter -= lightArray[j]->biradiance(surfelPos).sum();
                    if (counter < 0) {
                        lightPos = j;
                        break;
                    }
                }
                light = lightArray[lightPos];

            Biradiance3 biradiance;
            if (totalBiradiance==0.0f){
            biradiance = Color3(0.0f,0.0f,0.0f);
            }
            else{
            biradiance = light->biradiance(surfelPos)*totalBiradiance/(light->biradiance(surfelPos).average());
            }
         

            // Store biradiance from light
            biradianceBuffer[i] = biradiance;

            // Create shadow ray from light to surfel
       
            const Vector3 lightToSurfel = surfelPos - light->position().xyz();
            const float distanceToLight = lightToSurfel.length();
            Ray shadowRay = Ray(light->position().xyz(), lightToSurfel.direction(), 0.01f, distanceToLight - EPSILON);

            
            
            // Store shadow Ray
            shadowRayBuffer[i] = shadowRay;
        }
        
    }, !multithreading);
}


void PathTracer::generateRays(Array<Ray>& rayBuffer, const int& width, const int& height, const bool& multithreading) const {
    Thread::runConcurrently(G3D::Point2int32(0, 0), G3D::Point2int32(width, height), [&](G3D::Point2int32 coord) {
        // TODO bump these around a bit
        //const float x_off = Random::threadCommon().integer();
        Ray ray = m_camera->worldRay(coord.x, coord.y, Rect2D(Vector2(width, height)));
        
        rayBuffer[width * coord.y + coord.x] = ray;
    }, !multithreading);
}



void PathTracer::traceIntersections(const Array<Ray>& rayBuffer, Array<shared_ptr<Surfel>>& surfelBuffer, const bool& multithreading) const {
    // Find intersections
    m_tris.intersectRays(rayBuffer, surfelBuffer, TriTree::COHERENT_RAY_HINT);
}

