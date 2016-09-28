#include "RayTracer.h"
#include "App.h"


/** constructs TriTree object used in RayTracer::intersectRay and RayTracer::triangleIntersect */
RayTracer::RayTracer(Array<shared_ptr<Surface>> surfaces) {
    tris.setContents(surfaces);
//spheres.append(_Sphere(Vector3(0.0f, 1.0f, 0.0f), 1.0f));
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


Radiance3 RayTracer::getDirectLight(const Ray& ray, const shared_ptr<Surfel> surfel, const Array<shared_ptr<Light>>& lightArray) const {
    const Point3 surfelPosition = surfel->position;
    const Vector3 normal = surfel->shadingNormal;

    Radiance3 directLight;

    int numLights = lightArray.length();
    for (int i = 0; i < numLights; ++i) {
        const shared_ptr<Light> light = lightArray[i];

        // TODO figure out how to maked bumped ray code condensed
        Vector3 directionToLight = (light->position().xyz() - surfelPosition).direction();

        //Vector3 directionFromLight = (surfelPosition - light->position().xyz()).direction();
        Point3 bumpedPoint = surfelPosition + (EPSILON * surfel->geometricNormal * (-sign(surfel->geometricNormal.dot(-directionToLight))));

        //directionToLight = (light->position().xyz() - bumpedPoint).direction();
        Ray lightCheckRay(bumpedPoint, directionToLight);

        // shading
        if (isLightVisible(lightCheckRay, light)) {
            Biradiance3 biradiance = light->biradiance(surfelPosition);
            Color3 scatter = surfel->finiteScatteringDensity(lightCheckRay.direction(), -ray.direction());

            directLight += biradiance * scatter * abs(normal.dot(lightCheckRay.direction()));
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

const shared_ptr<Surfel> RayTracer::intersectRay(const Ray& ray) const {
    TriTree::Hit hit = TriTree::Hit::Hit();

    float triDistance = finf();
    // finds closest triangle to source ray
    for (int i = 0; i < tris.size(); ++i) {
        if (triangleIntersect(tris[i], ray, hit) && hit.distance < (triDistance + EPSILON)) {
            triDistance = hit.distance;
            hit.triIndex = i;
        }
    }

    //// This section of our code wont compile for reasons we cannot figure out. However if it did, we would use it to make sphere primitives appear in our scenes
    //OurSphere sphere(Vector3(0.0f, 1.0f, 0.0f), 1.0f);
    shared_ptr<UniversalSurfel> sphereSurfel = nullptr;
    float sphereDistance = finf();
    //// finds closest sphere to source ray
    //for (int i = 0; i < 1; ++i) {
    //    if (sphereIntersect(sphere, ray, sphereSurfel)
    //        && (sphereSurfel->position - ray.origin()).length() < (sphereDistance + EPSILON)) {
    //        sphereDistance = (sphereSurfel->position - ray.origin()).length();
    //    }
    //}

    // assumes that finf() always is represented with the same bits. Therefore, if both are infinite, it 
    // will go to the second case

    if (triDistance < sphereDistance) {
        return tris.sample(hit);
    }
    else {
        return sphereSurfel;
    }

}


bool RayTracer::triangleIntersect(const Tri& tri, const Ray &ray, TriTree::Hit& hit) const {
    // If we had implemented alpha testing, it would work by knowing if this was a case where it mattered
    // then if alphatesting would tell us that light should pass through this triangle, we would ust return false
    // Basically, decide whether or not to ignore the triangle based on the alpha-testing


    const CPUVertexArray& vertices = tris.vertexArray();

    // Edge vectors
    const Vector3& e_1 = tri.e1(vertices);
    const Vector3& e_2 = tri.e2(vertices);

    // Face normal
    const Vector3& n = e_1.cross(e_2).direction();
    const Vector3& q = ray.direction().cross(e_2);
    const float a = e_1.dot(q);

    // Backfacing / nearly parallel, or close to the limit of precision?
    if (abs(n.dot(ray.direction())) <= EPSILON || (abs(a) <= EPSILON)) return false;
    const Vector3& s = (ray.origin() - tri.vertex(vertices, 0).position) / a;
    const Vector3& r = s.cross(e_1);

    Array<float> b;
    b.append(s.dot(q));
    b.append(r.dot(ray.direction()));
    b.append(1.0f - b[0] - b[1]);


    // Intersected outside triangle?
    if ((b[0] < 0.0f) || (b[1] < 0.0f) || (b[2] < 0.0f)) return false;

    if (e_2.dot(r) >= 0.0f) {
        hit.distance = s.length();
        hit.backface = n.dot(ray.direction()) > 0;
        hit.u = b[0];
        hit.v = b[1];
        return true;
    }
    else {
        return false;
    }
}
//
//bool RayTracer::sphereIntersect(const OurSphere& sphere, const Ray& ray, shared_ptr<UniversalSurfel>& surfel) const {
//    const Vector3 center = sphere.m_center;
//    const float radius = -sphere.m_radius;
//    Vector3 distance = ray.origin() - center;
//
//    float a = 1.0f;
//    float b = (2 * ray.direction()).dot(distance);
//    float c = distance.length() * distance.length() - radius * radius;
//
//    float t = 0.0f;
//
//    if ((4 * a * c) < b*b) {
//
//        float t_1 = (-b + sqrt((b * b) - (4 * a * c))) / (2 * a);
//        float t_2 = (-b - sqrt((b * b) - (4 * a * c))) / (2 * a);
//
//
//        // This assumes a is positive
//        if (t_1 > 0) {
//            if (t_2 > 0) {
//                t = t_2;
//            }
//            else {
//                t = t_1;
//            }
//            
//            // Update surfel info
//            Vector3 position = ray.origin() + t * ray.direction();
//            Vector3 normal = (position - center).direction();
//            surfel->position = position;
//            surfel->shadingNormal = normal;
//            surfel->geometricNormal = normal;
//            surfel->lambertianReflectivity = Color3(0.58f, 0.0f, 0.83f);
//            return true;
//        }
//        else {
//            return false;
//        }
//    }
//
//}
//
