#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <SDL.h>
#include "Math.hpp"
#include "Window.hpp"
#include <VKRenderer.h>
#include <Engine/include/OpenGL/GLRenderer.h>

namespace nen
{

	enum class GraphicsAPI
	{
		OpenGL,
		Vulkan
	};

	class Renderer
	{
	public:
		Renderer(GraphicsAPI api);
		~Renderer() = default;

		void SetGraphicsAPI(GraphicsAPI &api)
		{
			RendererAPI = api;
		}
		GraphicsAPI GetGraphicsAPI()
		{
			return RendererAPI;
		}

		bool Initialize(std::shared_ptr<class Scene> scene, std::shared_ptr<class Transition> transition);
		void Shutdown();
		void UnloadData();

		void Draw();

		void AddSprite2D(std::shared_ptr<class Sprite> sprite, std::shared_ptr<Texture> texture);
		void RemoveSprite2D(std::shared_ptr<class Sprite> sprite);

		void AddSprite3D(std::shared_ptr<class Sprite> sprite, std::shared_ptr<Texture> texture);
		void RemoveSprite3D(std::shared_ptr<class Sprite> sprite);

		void AddEffectComp(class EffectComponent *effect);
		void RemoveEffectComp(class EffectComponent *effect);

		void AddVertexArray(const VertexArray &vArray, std::string_view name);

		vk::VKRenderer &GetVK() { return *(vkRenderer.get()); }
		gl::GLRenderer &GetGL() { return *(glRenderer.get()); }

		class Texture *GetTexture(std::string_view fileName);
		class Texture *GetTextureFromMemory(const unsigned char *const buffer, const std::string &key);
		class Effect *GetEffect(const std::u16string &fileName);

		void SetViewMatrix(const Matrix4 &view) { mView = view; }
		Matrix4 GetViewMatrix() { return mView; }
		const Matrix4 &GetProjectionMatrix() { return mProjection; }

		SDL_Window *GetWindow()
		{
			if (!mWindow)
				return nullptr;
			return mWindow;
		}

	private:
		class Sprite2DComponent *transPic;

		std::shared_ptr<Transition> mTransition;
		// Map of textures loaded
		std::unordered_map<std::string, class Texture *> mTextures3D;
		// Map of meshes loaded
		// Map of effects loaded
		std::unordered_map<std::string, class Effect *> mEffects;

		// All the sprite components drawn
		std::vector<std::shared_ptr<class Sprite>> mSprite3Ds;
		std::vector<std::shared_ptr<class Sprite>> mSprite2Ds;

		// All effects components drawn
		std::vector<class EffectComponent *> mEffectComp;

		// GameHandler
		std::shared_ptr<Scene> mScene;

		// Sprite vertex array
		class VertexArray *mSpriteVerts;

		// View/projection for 3D shaders
		Matrix4 mView;
		Matrix4 mProjection;

		// Window
		SDL_Window *mWindow;
		// Renderer
		std::unique_ptr<vk::VKRenderer> vkRenderer;
		std::unique_ptr<gl::GLRenderer> glRenderer;

		GraphicsAPI RendererAPI;
	};
}