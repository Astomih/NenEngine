#pragma once
#include <Engine.hpp>
#include <vector>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace nen
{
	class Actor : public std::enable_shared_from_this<Actor>
	{
	public:
		enum State
		{
			EActive,
			EPaused,
			EDead
		};

		Actor(std::shared_ptr<class Scene> scene);
		virtual ~Actor();

		template <class T>
		std::shared_ptr<T> NewComponent(int updateOwder = 100)
		{
			if (std::is_base_of<Component, T>::value)
			{
				auto component = std::make_shared<T>(*this, updateOwder);
				this->AddComponent(component);
				return component;
			}
			else
				static_assert(std::is_base_of<Component, T>::value);
		}
		void Update(float deltaTime);
		void UpdateComponents(float deltaTime);
		virtual void UpdateActor(float deltaTime);
		virtual std::string GetID() { return ""; }

		void ProcessInput(const struct InputState &state);
		virtual void ActorInput(const struct InputState &state);

		const Vector3f &GetPosition() const noexcept { return mPosition; }
		void SetPosition(const Vector3f &pos) noexcept
		{
			mPosition = pos;
			mRecomputeWorldTransform = true;
		}
		void Move(const float x, const float y, const float z) noexcept
		{
			mPosition.x += x;
			mPosition.y += y;
			mPosition.z += z;
			mRecomputeWorldTransform = true;
		}
		float GetScale() const { return mScale; }
		void SetScale(float scale)
		{
			mScale = scale;
			mRecomputeWorldTransform = true;
		}
		const Quaternion &GetRotation() const { return mRotation; }
		void SetRotation(const Quaternion &rotation)
		{
			mRotation = rotation;
			mRecomputeWorldTransform = true;
		}

		void ComputeWorldTransform();
		const Matrix4 &GetWorldTransform() const { return mWorldTransform; }

		Vector3f GetForward() const { return Vector3f::Transform(Vector3f::UnitX, mRotation); }

		State GetState() const { return mState; }
		void SetState(State state) { mState = state; }

		std::shared_ptr<class Scene> GetScene() { return mScene; }

		void AddComponent(std::shared_ptr<class Component> component);
		void AddComponent(std::unique_ptr<class Component> &&component, uint16_t &);
		std::unordered_map<uint16_t, std::unique_ptr<Component>> &GetComponents() { return this->mComponents2; }
		void RemoveComponent(std::shared_ptr<class Component> component);
		bool isRecompute() { return mRecomputeWorldTransform; }
		void RecomuteFinished() { mRecomputeWorldTransform = false; }

	protected:
		bool mRecomputeWorldTransform;

	private:
		State mState;
		Matrix4 mWorldTransform;
		Vector3f mPosition;
		Quaternion mRotation;
		std::vector<std::shared_ptr<Component>> mComponents;
		std::unordered_map<uint16_t, std::unique_ptr<Component>> mComponents2;
		std::shared_ptr<class Scene> mScene;
		float mScale;
		uint16_t m_index = 0;
	};
}
