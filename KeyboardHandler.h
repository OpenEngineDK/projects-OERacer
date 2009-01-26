#ifndef _KEYBOARD_HANDLER_
#define _KEYBOARD_HANDLER_

#include <Core/IListener.h>
#include <Core/IEngine.h>
#include <Devices/IKeyboard.h>
#include <Devices/Symbols.h>
#include <Display/Camera.h>
#include <Physics/FixedTimeStepPhysics.h>
#include <Physics/RigidBox.h>
#include <Math/Matrix.h>
#include <Utils/Timer.h>

using OpenEngine::Core::IModule;
using OpenEngine::Core::IListener;
using OpenEngine::Core::IEngine;
using OpenEngine::Core::InitializeEventArg;
using OpenEngine::Core::ProcessEventArg;
using OpenEngine::Core::DeinitializeEventArg;
using OpenEngine::Devices::KeyboardEventArg;
using OpenEngine::Display::Camera;
using OpenEngine::Physics::RigidBox;
using OpenEngine::Physics::FixedTimeStepPhysics;
using OpenEngine::Math::Matrix;
using OpenEngine::Utils::Timer;

namespace keys = OpenEngine::Devices;

class KeyboardHandler : public IModule, public IListener<KeyboardEventArg> {
private:
    bool up, down, left, right, mod;
    float step;
    Camera* camera;
    RigidBox* box;
    FixedTimeStepPhysics* physics;
    IEngine& engine;
    Timer timer;

public:
    KeyboardHandler(IEngine& engine,
                    Camera* camera,
                    RigidBox* box,
                    FixedTimeStepPhysics* physics)
        : up(false)
        , down(false)
        , left(false)
        , right(false)
        , camera(camera)
        , box(box)
        , physics(physics)
        , engine(engine)
    {}

    void Handle(InitializeEventArg arg) {
        step = 0.0f;
        timer.Start();
    }
    void Handle(DeinitializeEventArg arg) {}
    void Handle(ProcessEventArg arg) {

        float delta = (float) timer.GetElapsedTimeAndReset().AsInt() / 100000;

        if (box == NULL || !( up || down || left || right )) return;

        static float speed = 1750.0f;
        static float turn = 550.0f;
        Matrix<3,3,float> m(box->GetRotationMatrix());

        // Forward 
        if( up ){
            Vector<3,float> dir = m.GetRow(0) * delta;
            box->AddForce(dir * speed, 1);
            box->AddForce(dir * speed, 2);
            box->AddForce(dir * speed, 3);
            box->AddForce(dir * speed, 4);
        }
        if( down ){
            Vector<3,float> dir = -m.GetRow(0) * delta;
            box->AddForce(dir * speed, 5);
            box->AddForce(dir * speed, 6);
            box->AddForce(dir * speed, 7);
            box->AddForce(dir * speed, 8);
        }
        if( left ){
            Vector<3,float> dir = -m.GetRow(2) * delta;
            box->AddForce(dir * turn, 2);
            box->AddForce(dir * turn, 4);
        }
        if( right ) {
            Vector<3,float> dir = m.GetRow(2) * delta;
            box->AddForce(dir * turn, 1);
            box->AddForce(dir * turn, 3);
        }
    }

    void Handle(KeyboardEventArg arg) {
        (arg.type == OpenEngine::Devices::EVENT_PRESS) ? KeyDown(arg) : KeyUp(arg);
    }

    void KeyDown(KeyboardEventArg arg) {
        switch ( arg.sym ) {
        case keys::KEY_r: {
            physics->Handle(InitializeEventArg());
            if( physics != NULL ){
                if( box != NULL ) {
                    box->ResetForces();
                    box->SetCenter( Vector<3,float>(2, 1, 2) );
                    logger.info << "Reset Physics" << logger.end;
                }
            }
            break;
        }

        case keys::KEY_SPACE:{
            if( physics != NULL ){
                physics->TogglePause();
            }
            break;
        }
        // Move the car forward
        case keys::KEY_UP:    up    = true; break;
        case keys::KEY_DOWN:  down  = true; break;
        case keys::KEY_LEFT:  left  = true; break;
        case keys::KEY_RIGHT: right = true; break;

        // Log Camera position 
        case keys::KEY_c: {
            Vector<3,float> camPos = camera->GetPosition();
            logger.info << "Camera Position: " << camPos << logger.end;
            break;
        }

        // Increase/decrease time in Physic
        case keys::KEY_PLUS:  mod = true; step =  0.001f; break;
        case keys::KEY_MINUS: mod = true; step = -0.001f; break;
        

        // Quit on Escape
        case keys::KEY_ESCAPE:
            engine.Stop();
            break;
        default: break;
        }
    }

    void KeyUp(KeyboardEventArg arg) {
        switch ( arg.sym ) {
        case keys::KEY_UP:    up    = false; break;
        case keys::KEY_DOWN:  down  = false; break;
        case keys::KEY_LEFT:  left  = false; break;
        case keys::KEY_RIGHT: right = false; break;
        case keys::KEY_PLUS:  mod   = false; break;
        case keys::KEY_MINUS: mod   = false; break;

        default: break;
        }
    }
};

#endif
