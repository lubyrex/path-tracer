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
    int m_raysPerPixel = 1;
    int m_scatteringEvents = 0;
    int m_resolutionChoice = 1;


    float m_gamma = 2.0f;

    // Path tracer
    //PathTracer m_pathTracer;

    /** Called by GUI to load a scene image. Invokes ray tracing performed by RayTracer class */
    void onRender(shared_ptr<Image> &image);

    /** Called from onInit */
    void makeGUI();

    void addRenderGUI();

    void message(const String& msg) const;

    void runTests1();
    void runTests2();
    void runTests3();
    void processAndSaveImage(shared_ptr<Image> image, String name, float gamma, Stopwatch watch);

public:

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;
    void onAfterLoadScene(const Any & any, const String & sceneName);
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;
};
