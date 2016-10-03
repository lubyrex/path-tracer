/** \file App.cpp */
#include "App.h"
#include "PathTracer.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
    {
        G3DSpecification g3dSpec;
        g3dSpec.audio = false;
        initGLG3D(g3dSpec);
    }

    GApp::Settings settings(argc, argv);

    // Change the window and other startup parameters by modifying the
    // settings class.  For example:
    settings.window.caption = argv[0];

    // Set enable to catch more OpenGL errors
    // settings.window.debugContext     = true;

    // Some common resolutions:
    // settings.window.width            =  854; settings.window.height       = 480;
    // settings.window.width            = 1024; settings.window.height       = 768;
    settings.window.width = 1280; settings.window.height = 720;
    //settings.window.width             = 1920; settings.window.height       = 1080;
    // settings.window.width            = OSWindow::primaryDisplayWindowSize().x; settings.window.height = OSWindow::primaryDisplayWindowSize().y;
    settings.window.fullScreen = false;
    settings.window.resizable = !settings.window.fullScreen;
    settings.window.framed = !settings.window.fullScreen;

    // Set to true for a significant performance boost if your app can't render at 60fps, or if
    // you *want* to render faster than the display.
    settings.window.asynchronous = false;

    settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(64, 64);
    settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);
    settings.dataDir = FileSystem::currentDirectory();
    settings.screenshotDirectory = "../journal/";

    settings.renderer.deferredShading = true;
    settings.renderer.orderIndependentTransparency = false;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {
}


// Called before the application loop begins.  Load data here and
// not in the constructor so that common exceptions will be
// automatically caught.
void App::onInit() {
    debugPrintf("Target frame rate = %f Hz\n", realTimeTargetDuration());
    GApp::onInit();
    setFrameDuration(1.0f / 120.0f);

    // Call setScene(shared_ptr<Scene>()) or setScene(MyScene::create()) to replace
    // the default scene here.

    showRenderingStats = false;

    makeGUI();
    // For higher-quality screenshots:
    // developerWindow->videoRecordDialog->setScreenShotFormat("PNG");
    // developerWindow->videoRecordDialog->setCaptureGui(false);
    developerWindow->cameraControlWindow->moveTo(Point2(developerWindow->cameraControlWindow->rect().x0(), 0));
    loadScene(
        "G3D Cornell Box");
    //developerWindow->sceneEditorWindow->selectedScend

    // Is this necessary to initialize?
     //m_pathTracer;
}

void App::onAfterLoadScene(const Any& any, const String& sceneName) {
    GApp::onAfterLoadScene(any, sceneName);
    Array<shared_ptr<Camera>> cameras;
    scene()->getTypedEntityArray<Camera>(cameras);
    for (int i = 0; i < cameras.length(); ++i) {
        shared_ptr<Camera> c = cameras[i];
        FilmSettings& f = c->filmSettings();
        f.setGamma(m_gamma);
        f.setAntialiasingEnabled(false);
        f.setBloomStrength(0.0f);
        f.setVignetteBottomStrength(0.0f);
        f.setVignetteTopStrength(0.0f);
    }
}

void App::message(const String& msg) const {
    renderDevice->clear();
    renderDevice->push2D();
    debugFont->draw2D(renderDevice, msg, renderDevice->viewport().center(), 12,
        Color3::white(), Color4::clear(), GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
    renderDevice->pop2D();

    // Force update so that we can see the message
    renderDevice->swapBuffers();
}

void App::processAndSaveImage(shared_ptr<Image> image, String name, float gamma, Stopwatch watch) {
    image->convert(ImageFormat::RGB8());

    const shared_ptr<Texture>& src = Texture::fromImage("Source", image);
    shared_ptr<Texture> resultTexture;


    image->save(name);
    double time = watch.elapsedTime();
    const String& caption = format("Time: %fs", time);
    debugPrintf("%s\n", caption.c_str());
    show(image, caption);


    //    Array<shared_ptr<Camera>> cameras;
    //    scene()->getTypedEntityArray<Camera>(cameras);
    //    for (int i = 0; i < cameras.length(); ++i) {
    //        shared_ptr<Camera> c = cameras[i];
    //        FilmSettings& f = c->filmSettings();
    //        f.setGamma(gamma);
    //    }
    //    m_film->exposeAndRender(renderDevice, activeCamera()->filmSettings(), m_framebuffer->texture(0), settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.depthGuardBandThickness.x, resultTexture);
    //    resultTexture->toImage()->save("eyeRayTest.png");
    //    show(resultTexture);
}

void App::runTests1() {

    PathTracer tracer(scene());
    StopWatch stopWatch;

    // Eye ray directions
    stopWatch.reset();
    //m_gamma = 0.4;
    //ArticulatedModel::clearCache();
    //loadScene(scene()->name());
    tracer.m_eyeRayTest = true;
    shared_ptr<Image> eyeRay = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(eyeRay, stopWatch);
    processAndSaveImage(eyeRay, "eyeRayTest.png", 4.4, stopWatch);
    tracer.m_eyeRayTest = false;

    // hit positions
    stopWatch.reset();
    tracer.m_hitsTest = true;
    shared_ptr<Image> hits = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(hits, stopWatch);
    processAndSaveImage(hits, "hitsTest.png", 4.4, stopWatch);
    tracer.m_hitsTest = false;

    // geo normals
    stopWatch.reset();
    tracer.m_geoNormalsTest = true;
    shared_ptr<Image> geoNormals = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(geoNormals, stopWatch);
    processAndSaveImage(geoNormals, "normalsTest.png", 2.2, stopWatch);
    tracer.m_geoNormalsTest = false;
}

void App::runTests2() {
    PathTracer tracer(scene());
    StopWatch stopWatch;
    //ArticulatedModel::clearCache();


    scene()->load("G3D Cornell Box");
    tracer.setScene(scene());
    stopWatch.reset();
    shared_ptr<Image> cornell = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(cornell, stopWatch, 1, true, 1);
    processAndSaveImage(cornell, "Cornell.png", 4.4, stopWatch);


    stopWatch.reset();
    scene()->load("G3D Cornell Box (Spheres)");
    tracer.setScene(scene());
    shared_ptr<Image> sphere = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sphere, stopWatch, 128, true, 1);
    processAndSaveImage(sphere, "Sphere.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sphere2 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sphere2, stopWatch, 128, true, 1);
    processAndSaveImage(sphere2, "Sphere2.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sphere3 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sphere3, stopWatch, 128, true, 1);
    processAndSaveImage(sphere3, "Sphere3.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sphere4 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sphere4, stopWatch, 128, true, 1);
    processAndSaveImage(sphere4, "Sphere4.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sphere10 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sphere10, stopWatch, 128, true, 1);
    processAndSaveImage(sphere10, "Sphere5.png", 4.4, stopWatch);

}

void App::runTests3() {
    PathTracer tracer(scene());
    StopWatch stopWatch;
    //ArticulatedModel::clearCache();


    scene()->load("G3D Sponza");
    tracer.setScene(scene());
    shared_ptr<Image> sponza = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sponza, stopWatch, 1, true, 1);
    processAndSaveImage(sponza, "Sponza.png", 4.4, stopWatch);


    stopWatch.reset();
    shared_ptr<Image> sponza2 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sponza2, stopWatch, 1, true, 2);
    processAndSaveImage(sponza2, "Sponza2.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sponza3 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sponza3, stopWatch, 1, true, 3);
    processAndSaveImage(sponza3, "Sponza3.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sponza4 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sponza4, stopWatch, 1, true, 4);
    processAndSaveImage(sponza4, "Sponza4.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sponza5 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sponza5, stopWatch, 4, true, 1);
    processAndSaveImage(sponza5, "Sponza5.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sponza6 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sponza6, stopWatch, 16, true, 1);
    processAndSaveImage(sponza6, "Sponza6.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sponza7 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sponza7, stopWatch, 256, true, 1);
    processAndSaveImage(sponza7, "Sponza7.png", 4.4, stopWatch);

    stopWatch.reset();
    shared_ptr<Image> sponza8 = Image::create(320, 200, ImageFormat::RGB32F());
    tracer.renderScene(sponza8, stopWatch, 1024, true, 1);
    processAndSaveImage(sponza8, "Sponza8.png", 4.4, stopWatch);


}

void App::onRender(shared_ptr<Image> &image) {
    message("Rendering...");
    ArticulatedModel::clearCache();
    loadScene(scene()->name());

    StopWatch stopWatch;
    PathTracer tracer = PathTracer(scene());
    //tracer.setScene(scene());
    //tracer.m_eyeRayTest = true;
    tracer.renderScene(image, stopWatch, m_raysPerPixel, m_multiThreading, m_scatteringEvents);

    // Show / save raw image 
    // Set window caption to amount of time rendering took (not including data structure initialization)
    double time = stopWatch.elapsedTime();
    const String& caption = format("Time: %fs", time);
    debugPrintf("%s\n", caption.c_str());
    show(image, caption);
    image->convert(ImageFormat::RGB8());
    image->save("eyeRayTest.png");

    // Post-process image
    // Why does the saved image look so weird???
   //const shared_ptr<Texture>& src = Texture::fromImage("Source", image, ImageFormat::RGB8());
   // shared_ptr<Texture> resultTexture;
   // resultTexture->resize(image->width(), image->height());
   // m_film->exposeAndRender(renderDevice, activeCamera()->filmSettings(), src, settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.depthGuardBandThickness.x, resultTexture);
   //  show(resultTexture);
   // resultTexture->toImage()->save("result.png");

     //if (m_resultTexture) {
     //    m_resultTexture->resize(image->width(), image->height());
     //};

     //m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0), settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.depthGuardBandThickness.x);
}

/// Adds gui pane to let the user create a height field from an image and specified xz and y scaling amounts
void App::addRenderGUI() {

    shared_ptr<GuiWindow> renderWindow = GuiWindow::create("Render", debugWindow->theme(), Rect2D::xywh(1025, 175, 0, 50), GuiTheme::TOOL_WINDOW_STYLE);
    GuiPane* renderPane = renderWindow->pane();

    Array<String> resolutionOptions = { "1x1", "320x200", "640x400" };

    renderPane->addDropDownList("Resolution", resolutionOptions, &m_resolutionChoice);
    renderPane->addNumberBox("Rays Per Pixel", &m_raysPerPixel, "", GuiTheme::LINEAR_SLIDER, 1, 2048, 1);
    renderPane->addNumberBox("Scatters", &m_scatteringEvents, "", GuiTheme::LINEAR_SLIDER, 0, 2048, 1);
    renderPane->addCheckBox("Multithreading", &m_multiThreading);

    renderPane->addButton("Render", [&]() {
        shared_ptr<Image> image;
        try {
            switch (m_resolutionChoice) {
            case 0:image = (Image::create(1280, 720, ImageFormat::RGB32F()));
                break;
            case 1:image = (Image::create(320, 200, ImageFormat::RGB32F()));
                break;
            case 2:image = (Image::create(640, 400, ImageFormat::RGB32F()));
                break;
            }
        }
        catch (...) {
            msgBox("Unable to render the image.");
        }
        //onRender(image);
        runTests1();

        ArticulatedModel::clearCache();
        loadScene(scene()->name());
    });

    renderWindow->pack();
    renderWindow->setVisible(true);
    addWidget(renderWindow);
}


void App::makeGUI() {

    // Initialize the developer HUD
    createDeveloperHUD();

    debugWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));

    // Adds window with reload button
    //shared_ptr<GuiWindow> controlsWindow = GuiWindow::create("Controls", debugWindow->theme(), Rect2D::xywh(1025, 175, 0, 0), GuiTheme::TOOL_WINDOW_STYLE);
    //GuiPane* controlsPane = controlsWindow->pane();
    //controlsPane->addLabel("Use WASD keys + right mouse to move");

    //controlsPane->addButton("Reload", [this]() {loadScene(
    //    developerWindow->sceneEditorWindow->selectedSceneName()  // Load the first scene encountered 
    //); });

    //controlsWindow->pack();
    //controlsWindow->setVisible(true);
    //addWidget(controlsWindow);

    addRenderGUI();
}




// This default implementation is a direct copy of GApp::onGraphics3D to make it easy
// for you to modify. If you aren't changing the hardware rendering strategy, you can
// delete this override entirely.
void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) {
    if (!scene()) {
        if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!rd->swapBuffersAutomatically())) {
            swapBuffers();
        }
        rd->clear();
        rd->pushState(); {
            rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
            drawDebugShapes();
        } rd->popState();
        return;
    }

    GBuffer::Specification gbufferSpec = m_gbufferSpecification;
    extendGBufferSpecification(gbufferSpec);
    m_gbuffer->setSpecification(gbufferSpec);
    m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());
    m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.hdrFramebuffer.depthGuardBandThickness, m_settings.hdrFramebuffer.colorGuardBandThickness);

    m_renderer->render(rd, m_framebuffer, scene()->lightingEnvironment().ambientOcclusionSettings.enabled ? m_depthPeelFramebuffer : shared_ptr<Framebuffer>(),
        scene()->lightingEnvironment(), m_gbuffer, allSurfaces);

    // Debug visualizations and post-process effects
    rd->pushState(m_framebuffer); {
        // Call to make the App show the output of debugDraw(...)
        rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
        drawDebugShapes();
        const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : shared_ptr<Entity>();
        scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings(), activeCamera());

        // Post-process special effects
        m_depthOfField->apply(rd, m_framebuffer->texture(0), m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(), m_settings.hdrFramebuffer.depthGuardBandThickness - m_settings.hdrFramebuffer.colorGuardBandThickness);

        m_motionBlur->apply(rd, m_framebuffer->texture(0), m_gbuffer->texture(GBuffer::Field::SS_EXPRESSIVE_MOTION),
            m_framebuffer->texture(Framebuffer::DEPTH), activeCamera(),
            m_settings.hdrFramebuffer.depthGuardBandThickness - m_settings.hdrFramebuffer.colorGuardBandThickness);
    } rd->popState();

    // We're about to render to the actual back buffer, so swap the buffers now.
    // This call also allows the screenshot and video recording to capture the
    // previous frame just before it is displayed.
    if (submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) {
        swapBuffers();
    }

    // Clear the entire screen (needed even though we'll render over it, since
    // AFR uses clear() to detect that the buffer is not re-used.)
    rd->clear();

    // Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
    m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0), settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.depthGuardBandThickness.x);
}



void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    GApp::onSimulation(rdt, sdt, idt);

    // Example GUI dynamic layout code.  Resize the debugWindow to fill
    // the screen horizontally.
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
}

