#pragma once

#include <types.hpp>
#include <scripting.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"

JPH_SUPPRESS_WARNINGS

namespace physics3d {
    class CharacterController : public JPH::CharacterContactListener {
    private:
        JPH::CharacterVirtual* controller;
        JPH::CharacterVirtualSettings settings;
    public:
        Vec3f pos;

        static CharacterController make(Vec3f pos);
        void destroy();

        void move_and_slide(Vec3f vel);
        void set_pos(Vec3f new_pos);

        bool is_grounded();
    };

    class Shape {
    private:
        JPH::ShapeSettings* settings;
        Vec3f scale;
    public:
        static Shape Cube(Vec3f scale);

        friend class Rigidbody;
    };

    class Rigidbody {
    private:
        bool is_static = false;

        JPH::BodyID id;
        JPH::ShapeRefC ref;

        Shape shape;
        
        Vec3f pos, scale;
        f32 friction = 1.0f;
    public:
        static Rigidbody make(Shape shape, Vec3f pos, bool is_static);
        void destroy();

        void SetPos(Vec3f pos);
        void SetSize(Vec3f size);
        void SetFriction(f32 friction);
        Mat4 GetMatrix();
    };

    void init();
    void shutdown();
    void update();

    JPH::PhysicsSystem* GetPhysicsSystem();
    JPH::BodyInterface& GetBodyInterface();
    JPH::TempAllocatorImpl* GetPhysicsAllocator();

    void RegisterLuaModule(sol::state& state);
};
