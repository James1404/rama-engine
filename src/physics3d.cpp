#include <physics3d.hpp>

#include <engine.hpp>

#include <thread>

static void TraceImpl(const char *inFMT, ...) {
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
    engine::info("Jolt: {}", buffer);
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, u32 inLine) {
	// Print to the TTY
    engine::info("Jolt: {} : {} : ({}) {}",
        inFile,
        inLine,
        inExpression,
        (inMessage != nullptr ? inMessage : "")
    );

	// Breakpoint
	return true;
};

#endif // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr u32 NUM_LAYERS(2);
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
	BPLayerInterfaceImpl() {
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual u32 GetNumBroadPhaseLayers() const override {
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
		switch ((JPH::BroadPhaseLayer::Type)inLayer) {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
                return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
                return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
private:
	JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
		switch (inLayer1) {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
		}
	}
};

namespace {
    f32 accumulator = 0;
    bool dirty = true;
    f32 cDeltaTime = 1.0f / 60.0f;
    u32 cMaxBodies = 1024;
    u32 cNumBodyMutexes = 0;
    u32 cMaxBodyPairs = 1024;
    u32 cMaxContactConstraints = 1024;

    JPH::JobSystemThreadPool* job_system;

    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    JPH::TempAllocatorImpl* temp_allocator;
    JPH::PhysicsSystem* physics_system;

    sol::table lib(sol::this_state state) {
        using namespace physics3d;

        sol::state_view luaview(state);

        auto module = luaview.create_table();

        module.set_function("Init", &physics3d::init);
        module.set_function("Shutdown", &physics3d::shutdown);
        module.set_function("Update", &physics3d::update);

        module.new_usertype<CharacterController>("CharacterController",
            "Make", &CharacterController::make,
            "Destroy", &CharacterController::destroy,
            "MoveAndSlide", &CharacterController::move_and_slide,
            "SetPos", &CharacterController::set_pos,
            "IsGrounded", &CharacterController::is_grounded,
            "pos", &CharacterController::pos
        );

        module.new_usertype<Shape>("Shape",
            "Cube", &Shape::Cube
        );

        module.new_usertype<Rigidbody>("Rigidbody",
            "Make", &Rigidbody::make,
            "Destroy", &Rigidbody::destroy,
            "GetMatrix", &Rigidbody::GetMatrix,

            "SetPos", &Rigidbody::SetPos,
            "SetSize", &Rigidbody::SetSize,
            "SetFriction", &Rigidbody::SetFriction

        );

        return module;
    }
}

namespace physics3d {
    void init() {
        JPH::RegisterDefaultAllocator();

        temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
        job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        JPH::Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

        JPH::Factory::sInstance = new JPH::Factory();

        JPH::RegisterTypes();

        physics_system = new JPH::PhysicsSystem;
        physics_system->Init(
            cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            broad_phase_layer_interface,
            object_vs_broadphase_layer_filter, 
            object_vs_object_layer_filter
        );

        JPH::BodyInterface &body_interface = physics_system->GetBodyInterface();

        physics_system->OptimizeBroadPhase();
    }

    void shutdown() {
        JPH::UnregisterTypes();

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        delete physics_system;
        delete temp_allocator;
        delete job_system;
    }

    void update() {
        accumulator += engine::DeltaTime();

        while(accumulator >= cDeltaTime) {
            if(dirty) {
                physics_system->OptimizeBroadPhase();
                dirty = false;
            }

            physics_system->Update(cDeltaTime, 1, temp_allocator, job_system);
            accumulator -= cDeltaTime;
        }
    }

    JPH::PhysicsSystem* GetPhysicsSystem() {
        return physics_system;
    }

    JPH::BodyInterface& GetBodyInterface() {
        return physics_system->GetBodyInterface();
    }

    JPH::TempAllocatorImpl* GetPhysicsAllocator() {
        return temp_allocator;
    }

    void RegisterLuaModule(sol::state& state) {
        state.require("physics3d", sol::c_call<decltype(&lib), &lib>, false);
    }

    CharacterController CharacterController::make(Vec3f pos) {
        CharacterController result;
        
        result.settings.mShape = JPH::RotatedTranslatedShapeSettings(
            JPH::Vec3(pos.x, pos.y, pos.z),
            JPH::Quat::sIdentity(),
            new JPH::CapsuleShape(1.0, 1.0)
        ).Create().Get();

        result.controller = new JPH::CharacterVirtual(
            &result.settings,
            JPH::Vec3(pos.x, pos.y, pos.z),
            JPH::Quat::sIdentity(),
            physics3d::GetPhysicsSystem()
        );
        result.controller->SetListener(&result);

        result.pos = pos;

        return result;
    }

    void CharacterController::destroy() {
        delete controller;
    }

    void CharacterController::move_and_slide(Vec3f vel) {
        controller->SetLinearVelocity(
            JPH::Vec3(
                vel.x,
                vel.y,
                vel.z
            )
        );

        auto system = physics3d::GetPhysicsSystem();

        JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
        controller->ExtendedUpdate(
            engine::DeltaTime(),
            -controller->GetUp() * system->GetGravity().Length(),
            update_settings,
            system->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
            system->GetDefaultLayerFilter(Layers::MOVING),
            {},
            {},
            *temp_allocator
        );

        JPH::Vec3 jpos = controller->GetPosition();
        pos = Vec3f(jpos.GetX(), jpos.GetY(), jpos.GetZ());
    }

    void CharacterController::set_pos(Vec3f new_pos) {
        controller->SetPosition(
            JPH::Vec3(new_pos.x, new_pos.y, new_pos.z)
        );
    }

    bool CharacterController::is_grounded() {
        return controller->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
    }

    Shape Shape::Cube(Vec3f scale) {
        Shape result;

        result.scale = scale;
        result.settings = new JPH::BoxShapeSettings(JPH::Vec3(scale.x, scale.y, scale.z));

        return result;
    }

    Rigidbody Rigidbody::make(Shape shape, Vec3f pos, bool is_static) {
        Rigidbody result;
        result.is_static = is_static;

        JPH::ShapeSettings::ShapeResult shaperesult = shape.settings->Create();
        result.ref = shaperesult.Get();

        delete shape.settings;

        JPH::BodyCreationSettings creation_settings(
            result.ref,
            JPH::Vec3(pos.x, pos.y, pos.z),
            JPH::Quat::sIdentity(),
            (result.is_static ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic),
            (result.is_static ? Layers::NON_MOVING : Layers::MOVING)
        );

        result.id = physics3d::GetBodyInterface().CreateAndAddBody(creation_settings, JPH::EActivation::Activate);

        result.SetFriction(result.friction);

        result.pos = pos;
        result.shape = shape;

        return result;
    }

    void Rigidbody::destroy() {
    }

    void Rigidbody::SetPos(Vec3f pos) {
        physics3d::GetBodyInterface().SetPosition(
            id,
            JPH::Vec3(
                pos.x,
                pos.y,
                pos.z
            ),
            JPH::EActivation::Activate
        );
    }

    void Rigidbody::SetSize(Vec3f size) {
        TODO("Rigidbody::SetSize");
    }

    void Rigidbody::SetFriction(f32 friction) {
        this->friction = friction;
        physics3d::GetBodyInterface().SetFriction(id, friction);
    }

    Mat4 Rigidbody::GetMatrix() {
        Mat4 matrix(1.0);

        JPH::Mat44 transform = physics3d::GetBodyInterface().GetWorldTransform(id);

        JPH::Vec3 pos = transform.GetTranslation();

        matrix = glm::translate(matrix, Vec3f(pos.GetX(), pos.GetY(), pos.GetZ()));
        matrix = glm::scale(matrix, shape.scale);

        return matrix;
    }
}
