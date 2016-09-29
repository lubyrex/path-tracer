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

    bool m_multiThreading = true;
    int m_raysPerPixel = 1;
    float m_numIndirectRays = 0.0f;
    int m_resolutionChoice = 0;

    shared_ptr<Texture> m_resultTexture;


    int pixelWidth = 320;
    int pixelHeight = 200;

    shared_ptr<Image> m_currentImage;

    /** Called by GUI to load a scene image. Invokes ray tracing performed by RayTracer class */
    void render(shared_ptr<Image> &image, float indirectCount = 10, bool multithreading = true);

    /** Called from onInit */
    void makeGUI();


    //void addRenderGUI();

    void message(const String& msg) const;

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
