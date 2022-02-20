#ifndef NEN_HPP
#define NEN_HPP
#ifdef ANDROID
#define MOBILE
#endif
#include "Actor/Actor.hpp"
#include "Audio/MusicSystem.hpp"
#include "Audio/SoundEvent.hpp"
#include "Audio/SoundSystem.hpp"
#include "Component/BillboardComponent.hpp"
#include "Component/CircleComponent.hpp"
#include "Component/Component.hpp"
#include "Component/Draw2DComponent.hpp"
#include "Component/Draw3DComponent.hpp"
#include "Component/InputComponent.hpp"
#include "Component/MoveComponent.hpp"
#include "Component/ScriptComponent.hpp"
#include "Component/SoundComponent.hpp"
#include "Component/TextComponent.hpp"
#include "DrawObject/DrawObject.hpp"
#include "Font/Font.hpp"
#include "GUI/Button.hpp"
#include "GUI/DialogBox.hpp"
#include "GUI/HUD.hpp"
#include "GUI/PauseMenu.hpp"
#include "GUI/UIScreen.hpp"
#include "IO/AssetReader.hpp"
#include "Input/InputSystem.hpp"
#include "Logger/Logger.hpp"
#include "Math/Math.hpp"
#include "Math/Matrix3.hpp"
#include "Math/Matrix4.hpp"
#include "Math/Quaternion.hpp"
#include "Math/Random.hpp"
#include "Math/Transform.hpp"
#include "Math/Vector2.hpp"
#include "Math/Vector3.hpp"
#include "Network/TCPClient.hpp"
#include "Network/TCPSocket.hpp"
#include "Physics/Collision.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "Script/Script.hpp"
#include "Shader/Shader.hpp"
#include "Texture/Texture.hpp"
#include "Time/Time.hpp"
#include "Time/Timer.hpp"
#include "Utility/NonCopyable.hpp"
#include "Utility/Singleton.hpp"
#include "Utility/dynamic_handler.hpp"
#include "Utility/static_handler.hpp"
#include "Vertex/Vertex.hpp"
#include "Vertex/VertexArray.hpp"
#include "Window/Window.hpp"
#include "exception/exception.hpp"
#include "instancing/instancing.hpp"
#include "manager/manager.hpp"

#endif
#ifdef NO_USE_NEN_NAMESPACE
using namespace nen;
#endif
