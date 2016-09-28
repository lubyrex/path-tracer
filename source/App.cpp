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
        "Test Scene");
    //developerWindow->sceneEditorWindow->selectedScend

    // Generate heightmaps with right numbers of triangles
    //generateHeightField(1);
    //generateHeightField(10);
    //generateHeightField(100);
    //generateHeightField(1000);
    //generateHeightField(10000);
}


/// Generate a greyscale heightfield object with a specific number of triangles
void App::generateHeightField(int numTriangles) {
    String name = String("model/heightfield" + std::to_string(numTriangles) + ".off");
    G3D::TextOutput objectFile = G3D::TextOutput(name);

    int numSquares = numTriangles / 2;

    int width = 2;
    int height = numSquares + 1;

    int numPoints = width * height;
    int numFaces = (width - 1) * (height - 1) * 2;

    // OFF file header
    objectFile.printf(STR(
        OFF\n
        %d %d 0\n\n
    ), numPoints, numFaces);

    // Generate points
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {

            float x = i;
            float y = 1;
            float z = j;

            objectFile.printf(STR(
                %f %f %f\n
            ), x, y, z);
        }
    }

    objectFile.writeNewline();

    // Generate faces from triangle mesh
    // Each group of four has orientation
    // a c
    // b d
    for (int i = 0; i < height - 1; ++i) {
        for (int j = 0; j < width - 1; ++j) {
            int a = i *  width + j;
            int b = (i + 1) * width + j;
            int c = a + 1;
            int d = b + 1;

            objectFile.printf(STR(
                3 % d %d %d\n
                3 % d %d %d\n
            ), a, c, b, b, c, d);
        }
    }

    objectFile.commit();
}




void App::render(shared_ptr<Scene> &scene, const shared_ptr<Camera> &camera, shared_ptr<Image> &image,  StopWatch& stopWatch, float indirectCount, bool multithreading) {
    // If we had gotten primitives working, we would have passed the value of the chekbox through this method to the the ray tracer to enable or disable the check for sphere

    int height = image->height();
    int width = image->width();


    // Set up a tri-tree representing the scene 
    Array<shared_ptr<Surface>> surfaces;
    scene->onPose(surfaces);

    RayTracer tracer(surfaces);

    // Grab light array for the scene
    Array<shared_ptr<Light>> lightArray;
    scene->getTypedEntityArray(lightArray);

    stopWatch.tick();

   

    Thread::runConcurrently(Point2int32(0, 0), Point2int32(width, height), [&](Point2int32 coord) {
        Ray eyeRay = camera->worldRay(coord.x, coord.y, Rect2D(Vector2(image->width(), image->height())));

        // This loop would let us expand the rendering algorithm to use multiple ray samples per pixel
        Radiance3 sum = Radiance3::black();
        int raysPerPixel = 5;
        int recursionDepth = 1;
        for (int i = 0; i < raysPerPixel; ++i) {
            sum += tracer.trace(eyeRay, lightArray, indirectCount, recursionDepth);
        }
        sum /= raysPerPixel;

        image->set(Point2int32(coord.x, coord.y), sum);
    }, !multithreading);

    // Here is where we would post-process the image

    stopWatch.tock();
}

/// Adds gui pane to let the user create a height field from an image and specified xz and y scaling amounts
void App::addRenderGUI() {
    // resolution dropdown box
    // fixed primitives check box
    // multithreading check box
    // integer number box 0 - 2048
    // Render button

    Array<String> resolutionOptions = { "1x1", "320x200", "640x400" };


    GuiPane* renderPane = debugPane->addPane("Render");

    renderPane->addDropDownList("Resolution", resolutionOptions, resolutionChoice, [&]() {

    });

    renderPane->addCheckBox("Fixed Primitives", &fixedPrimitives);

    renderPane->addCheckBox("Multithreading", &multithreading);

    renderPane->setNewChildSize(240);
    renderPane->addNumberBox("Indirect Rays", &numIndirectRays, "m",
        GuiTheme::NO_SLIDER, 0.0f, 2048.0f)->setUnitsSize(30);

    renderPane->addButton("Render", [&]() {
        // This code would have made the resolution box actually change the resolution (were having pointer errors)
        //if (*resolutionChoice == 0) {
        //    pixelWidth = 1;
        //    pixelHeight = 1;
        //}
        //else if (*resolutionChoice == 1) {
        //    pixelWidth = 320;
        //    pixelHeight = 200;
        //}
        //else {
        //    pixelWidth = 640;
        //    pixelHeight = 400;
        //}

        // TESTING
        // Set pixel sizes. Temporary fix until above commented out code is working
        if (true) {
            pixelHeight = 400;
            pixelWidth = 640;
        }

        shared_ptr<Image> image = Image::create(pixelWidth, pixelHeight, ImageFormat::RGB8());
        StopWatch stopWatch = StopWatch();
        render(scene(), G3D::GApp::activeCamera(), image, stopWatch, numIndirectRays, multithreading);

        image->save("CustomScene.png");

        // Set window caption to amount of time rendering took (not including data structure initialization
        double time = stopWatch.elapsedTime();
        String caption = (String)("Time: " + std::to_string(time));
        debugPrintf("%s\n", caption);
        show(image, caption);

        // Code to display 'Rendering' message would go here
        ArticulatedModel::clearCache();
        loadScene(scene()->name());
    });
}


void App::makeGUI() {
    // Initialize the developer HUD
    createDeveloperHUD();

    debugWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);

    //GuiPane* infoPane = debugPane->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    //// Example of how to add debugging controls
    //infoPane->addLabel("You can add GUI controls");
    //infoPane->addLabel("in App::onInit().");
    //infoPane->addButton("Exit", [this]() { m_endProgram = true; });
    //infoPane->pack();

    // More examples of debugging GUI controls:
    // debugPane->addCheckBox("Use explicit checking", &explicitCheck);
    // debugPane->addTextBox("Name", &myName);
    // debugPane->addNumberBox("height", &height, "m", GuiTheme::LINEAR_SLIDER, 1.0f, 2.5f);
    // button = debugPane->addButton("Run Simulator");
    // debugPane->addButton("Generate Heightfield", [this](){ generateHeightfield(); });
    // debugPane->addButton("Generate Heightfield", [this](){ makeHeightfield(imageName, scale, "model/heightfield.off"); });

    debugWindow->pack();
    debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));

    // addCylinderGUI();
    // addHeightFieldGUI();
     //addGlassGUI();
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

//
//bool App::onEvent(const GEvent& event) {
//    // Handle super-class events
//    if (GApp::onEvent(event)) { return true; }
//
//    // If you need to track individual UI events, manage them here.
//    // Return true if you want to prevent other parts of the system
//    // from observing this specific event.
//    //
//    // For example,
//    // if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_button)) { ... return true; }
//    // if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::TAB)) { ... return true; }
//    // if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == 'p')) { ... return true; }
//
//    return false;
//}
//
//
//void App::onUserInput(UserInput* ui) {
//    GApp::onUserInput(ui);
//    (void)ui;
//    // Add key handling here based on the keys currently held or
//    // ones that changed in the last frame.
//}
//
//
//void App::onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) {
//    GApp::onPose(surface, surface2D);
//
//    // Append any models to the arrays that you want to later be rendered by onGraphics()
//}
//
//
//void App::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& posed2D) {
//    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction.
//    Surface2D::sortAndRender(rd, posed2D);
//}
//
//
//void App::onCleanup() {
//    // Called after the application loop ends.  Place a majority of cleanup code
//    // here instead of in the constructor so that exceptions can be caught.
//}
