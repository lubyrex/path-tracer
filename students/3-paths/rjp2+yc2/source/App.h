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

    // Variables for render GUI
    bool m_multiThreading = true;
    int m_raysPerPixel = 5;
    int m_numIndirectRays = 1;
    int m_resolutionChoice = 1;


    void renderScene(shared_ptr<Image> image, Stopwatch& stopWatch, int raysPerPixel = 5, bool multithreading = true, int numIndirectRays = 1) const;

    /** Called by GUI to load a scene image. Invokes ray tracing performed by RayTracer class */
    void onRender(shared_ptr<Image> &image);

    /** Called from onInit */
    void makeGUI();


    void addRenderGUI();

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
