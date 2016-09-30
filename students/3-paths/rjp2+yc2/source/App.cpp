/** \file App.cpp */
#include "App.h"
#include "RayTracer.h"

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


void App::onRender(shared_ptr<Image> &image) {
    message("Rendering...");

    StopWatch stopWatch = Stopwatch();
    RayTracer tracer = RayTracer();
    tracer.renderScene(scene(), image, stopWatch, m_raysPerPixel, m_multiThreading, m_scatteringEvents);

    // Show / save raw image 
    // Set window caption to amount of time rendering took (not including data structure initialization)
    double time = stopWatch.elapsedTime();
    String caption = (String)("Time: " + std::to_string(time));
    debugPrintf("%s\n", caption);
    show(image, caption);
    image->save("CustomScene.png");

    // Post-process image
    // Why does the saved image look so weird???
    const shared_ptr<Texture>& src = Texture::fromImage("Source", image);
    shared_ptr<Texture> resultTexture;
    m_film->exposeAndRender(renderDevice, m_debugCamera->filmSettings(), src, 0/* settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x*/, 0 /*settings().hdrFramebuffer.depthGuardBandThickness.x*/, resultTexture);
    resultTexture->toImage()->save("result.png");

    //if (m_resultTexture) {
    //    m_resultTexture->resize(image->width(), image->height());
    //};
}

/// Adds gui pane to let the user create a height field from an image and specified xz and y scaling amounts
void App::addRenderGUI() {

    shared_ptr<GuiWindow> renderWindow = GuiWindow::create("Render", debugWindow->theme(), Rect2D::xywh(1025, 175, 0, 0), GuiTheme::TOOL_WINDOW_STYLE);
    GuiPane* renderPane = renderWindow->pane();

    Array<String> resolutionOptions = { "1x1", "320x200", "640x400" };

    renderPane->addDropDownList("Resolution", resolutionOptions, &m_resolutionChoice);
    renderPane->addNumberBox("Rays Per Pixel", &m_raysPerPixel, "", GuiTheme::LINEAR_SLIDER, 0, 2048, 1);
    renderPane->addNumberBox("Scattering Events", &m_scatteringEvents, "", GuiTheme::LINEAR_SLIDER, 0, 2048, 1);
    renderPane->addCheckBox("Multithreading", &m_multiThreading);

    renderPane->addButton("Render", [&]() {
        shared_ptr<Image> image;
        try {
            switch (m_resolutionChoice) {
            case 0:image = (Image::create(1, 1, ImageFormat::RGB8()));
                break;
            case 1:image = (Image::create(320, 200, ImageFormat::RGB8()));
                break;
            case 2:image = (Image::create(640, 400, ImageFormat::RGB8()));
                break;
            }
        }
        catch (...) {
            msgBox("Unable to render the image.");
        }

        onRender(image);

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

