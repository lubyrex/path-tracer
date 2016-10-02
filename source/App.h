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

    // Path tracer
    //PathTracer m_pathTracer;

    /** Called by GUI to load a scene image. Invokes ray tracing performed by RayTracer class */
    void onRender(shared_ptr<Image> &image);

    /** Called from onInit */
    void makeGUI();

    void addRenderGUI();

    void message(const String& msg) const;

public:

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;
};
