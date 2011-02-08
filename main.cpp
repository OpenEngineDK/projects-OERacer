// Main setup for the OpenEngine Racer project.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

// OpenEngine stuff
#include <Meta/Config.h>

// Simple setup of a rendering engine
#include <Utils/SimpleSetup.h>

// Display structures
#include <Display/IFrame.h>
#include <Display/OpenGL/SplitScreenCanvas.h>
#include <Display/OpenGL/ColorStereoCanvas.h>
#include <Display/FollowCamera.h>
#include <Display/TrackingCamera.h>
#include <Display/InterpolatedViewingVolume.h>
#include <Display/ViewingVolume.h>
#include <Display/RenderCanvas.h>
#include <Display/OpenGL/TextureCopy.h>

// Rendering structures
#include <Renderers/OpenGL/RenderingView.h>
#include <Renderers/OpenGL/Renderer.h>
// Resources
#include <Resources/IModelResource.h>
#include <Resources/ResourceManager.h>

// Scene structures
#include <Scene/SceneNode.h>
#include <Scene/GeometryNode.h>
#include <Scene/TransformationNode.h>
#include <Scene/VertexArrayTransformer.h>
#include <Scene/DisplayListTransformer.h>
#include <Scene/PointLightNode.h>
// AccelerationStructures extension
#include <Scene/CollectedGeometryTransformer.h>
#include <Scene/QuadTransformer.h>
#include <Scene/BSPTransformer.h>
#include <Scene/ASDotVisitor.h>
#include <Renderers/AcceleratedRenderingView.h>

#include <Utils/MoveHandler.h>
#include <Utils/Statistics.h>
#include <Utils/RenderStateHandler.h>

// FixedTimeStepPhysics extension
#include <Physics/FixedTimeStepPhysics.h>
#include <Physics/RigidBox.h>

// Serialization utilities
#include <Resources/BinaryStreamArchive.h>
#include <fstream>

// OERacer utility files
#include "KeyboardHandler.h"

// Additional namespaces
using namespace OpenEngine::Core;
using namespace OpenEngine::Logging;
using namespace OpenEngine::Devices;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Utils;
using namespace OpenEngine::Physics;

using OpenEngine::Display::RenderCanvas;
using OpenEngine::Display::OpenGL::TextureCopy;
using OpenEngine::Display::OpenGL::SplitScreenCanvas;
using OpenEngine::Display::OpenGL::ColorStereoCanvas;

// Configuration structure to pass around to the setup methods
struct Config {
    SimpleSetup           setup;
    FollowCamera*         camera;
    TrackingCamera*       cam_br;
    TrackingCamera*       cam_tr;
    TrackingCamera*       cam_tl;
    ISceneNode*           renderingScene;
    ISceneNode*           dynamicScene;
    ISceneNode*           staticScene;
    ISceneNode*           physicScene;
    RigidBox*             physicBody;
    FixedTimeStepPhysics* physics;
    bool                  serialize;
    Config()
        : setup(SimpleSetup("<<OpenEngine Racer>>"))
                            //, new Viewport(0,0,400,300)))
                            // , new SDLEnvironment(800,600)
                            // , new RenderingView()
                            // , new Engine()
                            // , new Renderer())
        , camera(NULL)
        , renderingScene(NULL)
        , dynamicScene(NULL)
        , staticScene(NULL)
        , physicScene(NULL)
        , physics(NULL)
        , serialize(true)
    {
        
    }
};

// Forward declaration of the setup methods
void SetupResources(Config&);
void SetupDisplay(Config&);
void SetupScene(Config&);
void SetupPhysics(Config&);
void SetupRendering(Config&);
void SetupDevices(Config&);
void SetupDebugging(Config&);

int main(int argc, char** argv) {

    Config config;

    // Print usage info.
    logger.info << "========= Running The OpenEngine Racer Project =========" << logger.end;
    logger.info << "This project is a simple testing project for OpenEngine." << logger.end;
    logger.info << "It uses an old physics system and is not very stable." << logger.end;
    logger.info << logger.end;
    logger.info << "Vehicle controls:" << logger.end;
    logger.info << "  drive forwards:  up-arrow" << logger.end;
    logger.info << "  drive backwards: down-arrow" << logger.end;
    logger.info << "  turn left:       left-arrow" << logger.end;
    logger.info << "  turn right:      right-arrow" << logger.end;
    logger.info << logger.end;
    logger.info << "Camera controls:" << logger.end;
    logger.info << "  move forwards:   w" << logger.end;
    logger.info << "  move backwards:  s" << logger.end;
    logger.info << "  move left:       a" << logger.end;
    logger.info << "  move right:      d" << logger.end;
    logger.info << "  rotate:          mouse" << logger.end;
    logger.info << logger.end;

    // Setup the engine
    SetupResources(config);
    SetupDisplay(config);
    SetupScene(config);
    SetupPhysics(config);
    SetupRendering(config);
    SetupDevices(config);

    // Possibly add some debugging stuff
    //config.setup.EnableDebugging();
    //SetupDebugging(config);

    // Start up the engine.
    config.setup.GetEngine().Start();

    // Return when the engine stops.
    return EXIT_SUCCESS;
}

void SetupResources(Config& config) {
    config.setup.AddDataDirectory("projects/OERacer/data/");
}

void SetupDisplay(Config& config) {
    config.camera  = new FollowCamera( *(new InterpolatedViewingVolume(*config.setup.GetCamera() )));
    config.setup.SetCamera(*config.camera);
    
    // config.cam_br = new TrackingCamera(*(new InterpolatedViewingVolume(*(new ViewingVolume()))));
    // config.cam_tr = new TrackingCamera(*(new InterpolatedViewingVolume(*(new ViewingVolume()))));
    // config.cam_tl = new TrackingCamera(*(new InterpolatedViewingVolume(*(new ViewingVolume()))));

    config.cam_br = new TrackingCamera(*(new ViewingVolume()));
    config.cam_tr = new TrackingCamera(*(new ViewingVolume()));
    config.cam_tl = new TrackingCamera(*(new ViewingVolume()));

}

void SetupRendering(Config& config) {
    // Add rendering initialization tasks
    // DisplayListTransformer* dlt =
    //     new DisplayListTransformer(
    //         new RenderingView(
    //             *(new Viewport(config.setup.GetFrame()))));
    //config.renderer->InitializeEvent().Attach(*tl);
    // config.setup.GetRenderer().InitializeEvent().Attach(*dlt);

    // Transform the scene to use vertex arrays
    VertexArrayTransformer vaT;
    vaT.Transform(*config.renderingScene);

    // Supply the scene to the renderer
    delete config.setup.GetScene();
    config.setup.SetScene(*config.renderingScene);

    // bottom left
    IRenderCanvas* _c1 = config.setup.GetCanvas();
    IRenderer* r = _c1->GetRenderer();

    IRenderCanvas* c1 = new ColorStereoCanvas(new TextureCopy());
    c1->SetRenderer(r);
    c1->SetScene(_c1->GetScene());
    c1->SetViewingVolume(_c1->GetViewingVolume());

    // bottom right
    IRenderCanvas* c2 = new RenderCanvas(new TextureCopy());
    c2->SetViewingVolume(config.cam_br);
    c2->SetRenderer(r);
    c2->SetScene(config.renderingScene);
    config.cam_br->SetPosition(Vector<3,float>(1000,1000,1000));
    config.cam_br->LookAt(0,0,0);

    // top right
    IRenderCanvas* c3 = new RenderCanvas(new TextureCopy());
    c3->SetViewingVolume(config.cam_tr);
    c3->SetRenderer(r);
    c3->SetScene(config.renderingScene);
    config.cam_tr->SetPosition(Vector<3,float>(0,2000,0));
    config.cam_tr->LookAt(0,0,0);


    // top left
    IRenderCanvas* c4 = new RenderCanvas(new TextureCopy());
    c4->SetViewingVolume(config.cam_tl);
    c4->SetRenderer(r);
    c4->SetScene(config.renderingScene);
    config.cam_tl->SetPosition(Vector<3,float>(0,1000,0));
    config.cam_tl->LookAt(0,0,0);

    SplitScreenCanvas* left = new SplitScreenCanvas(new TextureCopy(), *c4, *c1, SplitScreenCanvas::HORIZONTAL);
    SplitScreenCanvas* right = new SplitScreenCanvas(new TextureCopy(), *c3, *c2, SplitScreenCanvas::HORIZONTAL);
    SplitScreenCanvas* canvas = new SplitScreenCanvas(new TextureCopy(), *left, *right);
    config.setup.GetFrame().SetCanvas(canvas);
}

void SetupDevices(Config& config) {
    //Register movement handler to be able to move the camera
    MoveHandler* move_h = new MoveHandler(*config.camera, config.setup.GetMouse());
    config.setup.GetKeyboard().KeyEvent().Attach(*move_h);

    // Keyboard bindings to the rigid box and camera
    KeyboardHandler* keyHandler = new KeyboardHandler(config.setup.GetEngine(),
                                                      config.camera,
                                                      config.physicBody,
                                                      config.physics);
    config.setup.GetKeyboard().KeyEvent().Attach(*keyHandler);
    config.setup.GetJoystick().JoystickAxisEvent().Attach(*keyHandler);

    config.setup.GetEngine().InitializeEvent().Attach(*keyHandler);
    config.setup.GetEngine().ProcessEvent().Attach(*keyHandler);
    config.setup.GetEngine().DeinitializeEvent().Attach(*keyHandler);

    config.setup.GetEngine().InitializeEvent().Attach(*move_h);
    config.setup.GetEngine().ProcessEvent().Attach(*move_h);
    
    config.setup.GetMouse().MouseMovedEvent().Attach(*move_h);

    config.setup.GetEngine().DeinitializeEvent().Attach(*move_h);
}

void SetupPhysics(Config& config) {
    if (config.physicBody  == NULL ||
        config.physicScene == NULL)
        throw Exception("Physics dependencies are not satisfied.");

    if (config.serialize) {
        ifstream isf("oeracer-physics-scene.bin", ios::binary);
        if (isf.is_open()) {
            logger.info << "Loading the physics tree from file: started"
                        << logger.end;
            delete config.physicScene;
            BinaryStreamArchiveReader reader(isf);
            config.physicScene = reader.ReadScene("physics");
            isf.close();
            logger.info << "Loading the physics tree from file: done"
                        << logger.end;
        } else {
            isf.close();
            logger.info << "Creating and serializing the physics tree: started"
                        << logger.end;
            // transform the object tree to a hybrid Quad/BSP
            CollectedGeometryTransformer collT;
            QuadTransformer quadT;
            BSPTransformer bspT;
            collT.Transform(*config.physicScene);
            quadT.Transform(*config.physicScene);
            bspT.Transform(*config.physicScene);
            // serialize the scene
            ofstream of("oeracer-physics-scene.bin");
            BinaryStreamArchiveWriter writer(of);
            writer.WriteScene("physics", config.physicScene);
            of.close();
            logger.info << "Creating and serializing the physics tree: done"
                        << logger.end;
        }
    } else {
        // transform the object tree to a hybrid Quad/BSP
        CollectedGeometryTransformer collT;
        QuadTransformer quadT;
        BSPTransformer bspT;
        collT.Transform(*config.physicScene);
        quadT.Transform(*config.physicScene);
        bspT.Transform(*config.physicScene);
    }
    
    config.physics = new FixedTimeStepPhysics(config.physicScene);

    // Add physic bodies
    config.physics->AddRigidBody(config.physicBody);

    // Add to engine for processing time (with its timer)
    FixedTimeStepPhysicsTimer* ptimer = new FixedTimeStepPhysicsTimer(*config.physics);
    config.setup.GetEngine().InitializeEvent().Attach(*config.physics);
    config.setup.GetEngine().ProcessEvent().Attach(*ptimer);
    config.setup.GetEngine().DeinitializeEvent().Attach(*config.physics);
}

void SetupScene(Config& config) {
    if (config.dynamicScene    != NULL ||
        config.staticScene     != NULL ||
        config.physicScene     != NULL ||
        config.renderingScene  != NULL )
        throw Exception("Setup scene dependencies are not satisfied.");

    // Create scene nodes
    RenderStateNode* rn = new RenderStateNode();

    rn->DisableOption(RenderStateNode::WIREFRAME);
    rn->DisableOption(RenderStateNode::SOFT_NORMAL);
    rn->DisableOption(RenderStateNode::HARD_NORMAL);
    rn->DisableOption(RenderStateNode::BINORMAL);
    rn->DisableOption(RenderStateNode::TANGENT);
    rn->DisableOption(RenderStateNode::BACKFACE);
                      
    rn->EnableOption(RenderStateNode::TEXTURE);
    rn->EnableOption(RenderStateNode::SHADER);
    

    config.renderingScene = rn;
    
    RenderStateHandler* rh = new RenderStateHandler(*rn);
    config.setup.GetKeyboard().KeyEvent().Attach(*rh);

    config.dynamicScene = new SceneNode();
    config.staticScene = new SceneNode();
    config.physicScene = new SceneNode();

    config.renderingScene->AddNode(config.dynamicScene);
    config.renderingScene->AddNode(config.staticScene);

    ISceneNode* current = config.dynamicScene;

    // Position of the vehicle
    Vector<3,float> position(2, 100, 2);

    // Add models from models.txt to the scene
    ifstream* mfile = File::Open(DirectoryManager::FindFileInPath("projects/OERacer/models.txt"));
    
    bool dynamic = false;
    while (!mfile->eof()) {
        string mod_str;
        getline(*mfile, mod_str);

        // Check the string
        if (mod_str[0] == '#' || mod_str == "") continue;

        // switch to static elements
        if (mod_str == "dynamic") {
            dynamic = true;
            current = config.dynamicScene;
            continue;
        }
        else if (mod_str == "static") {
            dynamic = false;
            current = config.staticScene;
            continue;
        }
        else if (mod_str == "physic") {
            dynamic = false;
            current = config.physicScene;
            continue;
        }
        
        // Load the model
        IModelResourcePtr mod_res = ResourceManager<IModelResource>::Create(mod_str);
        mod_res->Load();
        if (mod_res->GetSceneNode() == NULL) continue;

        ISceneNode* mod_node = mod_res->GetSceneNode();
        mod_res->Unload();

        TransformationNode* mod_tran = new TransformationNode();
        mod_tran->AddNode(mod_node);
        if (dynamic) {
            // Load riget-box
            config.physicBody = new RigidBox( Box(*mod_node));
            config.physicBody->SetCenter( position );
            config.physicBody->SetTransformationNode(mod_tran);
            config.physicBody->SetGravity(Vector<3,float>(0, -9.82*20, 0));
            // Bind the follow camera
            config.camera->SetPosition(position + Vector<3,float>(-150,40,0));
            config.camera->LookAt(position - Vector<3,float>(0,30,0));
            config.camera->Follow(mod_tran);
            // bind the tracking cameras
            config.cam_br->Track(mod_tran);
            config.cam_tr->Track(mod_tran);
            config.cam_tl->Track(mod_tran);

            // Set up a light node
            PointLightNode* pln = new PointLightNode();
            pln->constAtt = 0.5;
            pln->linearAtt = 0.001;
            pln->quadAtt = 0.0001;
            mod_tran->AddNode(pln);
        }
        current->AddNode(mod_tran);
        logger.info << "Successfully loaded " << mod_str << logger.end;
    }
    mfile->close();
    delete mfile;

    QuadTransformer quadT;
    quadT.SetMaxFaceCount(500);
    quadT.SetMaxQuadSize(100);
    quadT.Transform(*config.staticScene);
}

void SetupDebugging(Config& config) {

    // add the RigidBox to the scene, for debugging
    if (config.physicBody != NULL) {
        config.renderingScene->AddNode(config.physicBody->GetRigidBoxNode());
    }

    // Add Statistics module
    config.setup.GetEngine().ProcessEvent().Attach(*(new OpenEngine::Utils::Statistics(100000)));

    // Create dot graphs of the various scene graphs
    map<string, ISceneNode*> scenes;
    scenes["dynamicScene"] = config.dynamicScene;
    scenes["staticScene"]  = config.staticScene;
    scenes["physicScene"]  = config.physicScene;

    map<string, ISceneNode*>::iterator itr;
    for (itr = scenes.begin(); itr != scenes.end(); itr++) {
        ofstream dotfile((itr->first+".dot").c_str(), ofstream::out);
        if (!dotfile.good()) {
            logger.error << "Can not open '"
                         << itr->first << ".dot' "
                         << "for output" << logger.end;
        } else {
            ASDotVisitor dot;
            dot.Write(*itr->second, &dotfile);
            logger.info << "Saved physics graph to '"
                        << itr->first << ".dot'" << logger.end;
        }
    }
}
