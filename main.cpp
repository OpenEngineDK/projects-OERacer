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

// Include the serialization header
// The archives must be defined before any serializable objects so
// this must be first.
#include <Utils/Serialization.h>
#include <fstream>

// Simple setup of a rendering engine
#include <Utils/SimpleSetup.h>

// Display structures
#include <Display/FollowCamera.h>

// Rendering structures
#include <Renderers/OpenGL/RenderingView.h>

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

// FixedTimeStepPhysics extension
#include <Physics/FixedTimeStepPhysics.h>
#include <Physics/RigidBox.h>

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

using OpenEngine::Renderers::OpenGL::RenderingView;

// Configuration structure to pass around to the setup methods
struct Config {
    SimpleSetup           setup;
    FollowCamera*         camera;
    ISceneNode*           renderingScene;
    ISceneNode*           dynamicScene;
    ISceneNode*           staticScene;
    ISceneNode*           physicScene;
    RigidBox*             physicBody;
    FixedTimeStepPhysics* physics;
    Config()
        : setup(SimpleSetup("<<OpenEngine Racer>>"))
        , camera(NULL)
        , renderingScene(NULL)
        , dynamicScene(NULL)
        , staticScene(NULL)
        , physicScene(NULL)
        , physics(NULL)
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
    config.camera        = new FollowCamera( *config.setup.GetCamera() );
    config.setup.SetCamera(*config.camera);
}

void SetupRendering(Config& config) {
    // Add rendering initialization tasks
    DisplayListTransformer* dlt =
        new DisplayListTransformer(
            new RenderingView(
                *(new Viewport(config.setup.GetFrame()))));
    //config.renderer->InitializeEvent().Attach(*tl);
    config.setup.GetRenderer().InitializeEvent().Attach(*dlt);

    // Transform the scene to use vertex arrays
    VertexArrayTransformer vaT;
    vaT.Transform(*config.renderingScene);

    // Supply the scene to the renderer
    delete config.setup.GetScene();
    config.setup.SetScene(*config.renderingScene);
}

void SetupDevices(Config& config) {
    // Register movement handler to be able to move the camera
    MoveHandler* move_h = new MoveHandler(*config.camera, config.setup.GetMouse());
    config.setup.GetKeyboard().KeyEvent().Attach(*move_h);

    // Keyboard bindings to the rigid box and camera
    KeyboardHandler* keyHandler = new KeyboardHandler(config.setup.GetEngine(),
                                                      config.camera,
                                                      config.physicBody,
                                                      config.physics);
    config.setup.GetKeyboard().KeyEvent().Attach(*keyHandler);

    config.setup.GetEngine().InitializeEvent().Attach(*keyHandler);
    config.setup.GetEngine().ProcessEvent().Attach(*keyHandler);
    config.setup.GetEngine().DeinitializeEvent().Attach(*keyHandler);

    config.setup.GetEngine().InitializeEvent().Attach(*move_h);
    config.setup.GetEngine().ProcessEvent().Attach(*move_h);
    config.setup.GetEngine().DeinitializeEvent().Attach(*move_h);
}

void SetupPhysics(Config& config) {
    if (config.physicBody  == NULL ||
        config.physicScene == NULL)
        throw Exception("Physics dependencies are not satisfied.");

    ifstream isf("oeracer-physics-scene.bin", ios::binary);
    if (isf.is_open()) {
        logger.info << "Loading the physics tree from file: started" << logger.end;
        delete config.physicScene;
        config.physicScene = new SceneNode();
        Serialization::Deserialize(*config.physicScene, &isf);
        isf.close();
        logger.info << "Loading the physics tree from file: done" << logger.end;
    } else {
        isf.close();
        logger.info << "Creating and serializing the physics tree: started" << logger.end;
        // transform the object tree to a hybrid Quad/BSP
        CollectedGeometryTransformer collT;
        QuadTransformer quadT;
        BSPTransformer bspT;
        collT.Transform(*config.physicScene);
        quadT.Transform(*config.physicScene);
        bspT.Transform(*config.physicScene);
        // serialize the scene
        const ISceneNode& tmp = *config.physicScene;
        ofstream of("oeracer-physics-scene.bin");
        Serialization::Serialize(tmp, &of);
        of.close();
        logger.info << "Creating and serializing the physics tree: done" << logger.end;
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
    config.renderingScene = new SceneNode();
    config.dynamicScene = new SceneNode();
    config.staticScene = new SceneNode();
    config.physicScene = new SceneNode();

    config.renderingScene->AddNode(config.dynamicScene);
    config.renderingScene->AddNode(config.staticScene);

    ISceneNode* current = config.dynamicScene;

    // Position of the vehicle
    Vector<3,float> position(2, 100, 2);

    // Add models from models.txt to the scene
    ifstream* mfile = File::Open("projects/OERacer/models.txt");
    
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
    //config.engine.ProcessEvent().Attach(*(new OpenEngine::Utils::Statistics(1000)));

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
