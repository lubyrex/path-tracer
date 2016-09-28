/**
  \file App.h

  The G3D 10.00 default starter app is configured for OpenGL 4.1 and
  relatively recent GPUs.
 */
#pragma once
#include <G3D/G3DAll.h>

 /** \brief Application framework. */
class App : public GApp {
protected:

    bool fixedPrimitives = false;
    bool multithreading = true;
    float numIndirectRays = 0.0f;
    Pointer<int> resolutionChoice = int(0);
    int pixelWidth = 320;
    int pixelHeight = 200;

    /** Called by GUI to load a scene image. Invokes ray tracing performed by RayTracer class */
    void render(shared_ptr<Scene> &scene, const shared_ptr<Camera> &camera, shared_ptr<Image> &image,  StopWatch& stopWatch, float indirectCount = 10, bool multithreading = true);

    /** Called from onInit */
    void makeGUI();

    // variables for cylinder gui
    //float m_cylinderRadius;
    //float m_cylinderHeight;
    //bool m_cylinderHollow;

    //// variables for height field gui
    //float m_heightfieldYScale;
    //float m_heightfieldXZScale;
    //String m_heightfieldSource;

    //// variables for glass gui
    //float m_glassSlicesNumber;

    // gui addition methods
   // void addCylinderGUI();
    //void addHeightFieldGUI();
    void addRenderGUI();

    void generateHeightField(int numTriangles);

    // object generation methods
    //void generateCylinder(int radius, int height, bool hollow);
    //void generateHeightField(float yScale, float xzScale, shared_ptr<Image> image);
    //void generateDrinkingGlass(int numSlices);

public:

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;

};
