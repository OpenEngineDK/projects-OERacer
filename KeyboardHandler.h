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

class KeyboardHandler : public IModule, 
                        public IListener<KeyboardEventArg>,
                        public IListener<JoystickAxisEventArg> {
private:
    float up, down, left, right;
    bool mod;
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
        : up(0)
        , down(0)
        , left(0)
        , right(0)
        , camera(camera)
        , box(box)
        , physics(physics)
        , engine(engine)
    {}

    void Handle(Core::InitializeEventArg arg) {
        step = 0.0f;
        timer.Start();
    }
    void Handle(Core::DeinitializeEventArg arg) {}
    void Handle(Core::ProcessEventArg arg) {


        float delta = (float) timer.GetElapsedTimeAndReset().AsInt() / 100000;

        if (box == NULL || !( up || down || left || right )) return;

        static float speed = 1750.0f;
        static float turn = 550.0f;
        Matrix<3,3,float> m(box->GetRotationMatrix());

        

        // Forward 
        if( up > 0.0 ){
            
            Vector<3,float> dir = m.GetRow(0) * delta;
            float sp = speed * up;
            box->AddForce(dir * sp, 1);
            box->AddForce(dir * sp, 2);
            box->AddForce(dir * sp, 3);
            box->AddForce(dir * sp, 4);
        }
        if( down > 0.0){
            Vector<3,float> dir = -m.GetRow(0) * delta;
            float sp = speed * down;
            box->AddForce(dir * sp, 5);
            box->AddForce(dir * sp, 6);
            box->AddForce(dir * sp, 7);
            box->AddForce(dir * sp, 8);
        }
        if( left > 0.0 ){
            Vector<3,float> dir = -m.GetRow(2) * delta;
            float tu = turn * left;
            box->AddForce(dir * tu, 2);
            box->AddForce(dir * tu, 4);
        }
        if( right > 0.0) {
            Vector<3,float> dir = m.GetRow(2) * delta;
            float tu = turn * right;
            box->AddForce(dir * tu, 1);
            box->AddForce(dir * tu, 3);
        }
    }

    void Handle(KeyboardEventArg arg) {
        (arg.type == OpenEngine::Devices::EVENT_PRESS) ? KeyDown(arg) : KeyUp(arg);
    }

    void Handle(JoystickAxisEventArg arg) {
        float max = 1 << 15;
        float thres1 = 0.1;

        up = (-arg.state.axisState[1])/max;
        if (up < thres1) up = 0.0;
        down = (arg.state.axisState[1])/max;
        if (down < thres1) down = 0.0;

        left = (-arg.state.axisState[0])/max;
        if (left < thres1) left = 0.0;
        right = (arg.state.axisState[0])/max;
        if (right < thres1) right = 0.0;

    }

    void KeyDown(KeyboardEventArg arg) {

        switch ( arg.sym ) {
        case keys::KEY_r: {
            physics->Handle(Core::InitializeEventArg());
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
        case keys::KEY_UP:    up    = 1.0; break;
        case keys::KEY_DOWN:  down  = 1.0; break;
        case keys::KEY_LEFT:  left  = 1.0; break;
        case keys::KEY_RIGHT: right = 1.0; break;

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
        case keys::KEY_UP:    up    = 0.0; break;
        case keys::KEY_DOWN:  down  = 0.0; break;
        case keys::KEY_LEFT:  left  = 0.0; break;
        case keys::KEY_RIGHT: right = 0.0; break;
        case keys::KEY_PLUS:  mod   = false; break;
        case keys::KEY_MINUS: mod   = false; break;

        default: break;
        }
    }
};

#endif
