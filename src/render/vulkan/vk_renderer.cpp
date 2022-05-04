#include <manager/manager.hpp>

#include <camera/camera.hpp>

#if !defined(EMSCRIPTEN) && !defined(MOBILE)
// general
#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

// extenal
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

// internal
#include "../../texture/texture_system.hpp"
#include "vk_base.hpp"
#include "vk_renderer.hpp"
#include "vk_shader.hpp"
#include "vk_util.hpp"

namespace nen::vk {
using namespace vkutil;

constexpr int maxpoolSize = 5000;
vk_renderer::vk_renderer(manager &_manager)
    : m_manager(_manager), m_descriptorPool(), m_descriptorSetLayout(),
      m_sampler(), m_base(std::make_unique<vk_base>(this, _manager)) {}
void vk_renderer::Initialize() { m_base->initialize(); }

void vk_renderer::Shutdown() {
  cleanup();
  m_base->terminate();
}
void vk_renderer::Render() { m_base->render(); }
void vk_renderer::AddVertexArray(const vertex_array &vArray,
                                 std::string_view name) {
  vk_vertex_array vArrayVK;
  vArrayVK.indexCount = vArray.indexCount;
  vArrayVK.indices = vArray.indices;
  vArrayVK.vertices = vArray.vertices;
  vArrayVK.materialName = vArray.materialName;
  auto vArraySize = vArray.vertices.size() * sizeof(vertex);
  vArrayVK.vertexBuffer =
      CreateBuffer(vArraySize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  vArrayVK.indexBuffer = CreateBuffer(vArray.indices.size() * sizeof(uint32_t),
                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  write_memory(vArrayVK.vertexBuffer.allocation, vArrayVK.vertices.data(),
               vArraySize);
  write_memory(vArrayVK.indexBuffer.allocation, vArrayVK.indices.data(),
               sizeof(uint32_t) * vArrayVK.indices.size());
  m_VertexArrays.insert(
      std::pair<std::string, vk_vertex_array>(name.data(), vArrayVK));
}
void vk_renderer::UpdateVertexArray(const vertex_array &vArray,
                                    std::string_view name) {
  vk_vertex_array vArrayVK;
  vArrayVK.indexCount = vArray.indexCount;
  vArrayVK.indices = vArray.indices;
  vArrayVK.vertices = vArray.vertices;
  vArrayVK.materialName = vArray.materialName;
  write_memory(m_VertexArrays[name.data()].vertexBuffer.allocation,
               vArrayVK.vertices.data(),
               vArray.vertices.size() * sizeof(vertex));
  write_memory(m_VertexArrays[name.data()].indexBuffer.allocation,
               vArrayVK.indices.data(),
               vArray.indices.size() * sizeof(uint32_t));
}

void vk_renderer::draw2d(std::shared_ptr<class draw_object> drawObject) {
  auto t = std::make_shared<vk::vk_draw_object>();
  t->drawObject = drawObject;
  registerImageObject(drawObject->texture_handle);
  registerTexture(t, texture_type::Image2D);
}

void vk_renderer::draw3d(std::shared_ptr<class draw_object> sprite) {
  auto t = std::make_shared<vk::vk_draw_object>();
  t->drawObject = sprite;
  registerImageObject(sprite->texture_handle);
  registerTexture(t, texture_type::Image3D);
}

void vk_renderer::LoadShader(const shader &shaderInfo) {
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
      vk_shader::LoadModule(m_base->get_vk_device(),
                            shaderInfo.vertName.c_str(),
                            VK_SHADER_STAGE_VERTEX_BIT),
      vk_shader::LoadModule(m_base->get_vk_device(),
                            shaderInfo.fragName.c_str(),
                            VK_SHADER_STAGE_FRAGMENT_BIT)};
  vk_pipeline pipeline;
  pipeline.Initialize(mPipelineLayout, m_base->m_renderPass, shaderStages);
  pipeline.ColorBlendFactor(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
  pipeline.AlphaBlendFactor(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
  pipeline.Prepare(m_base->get_vk_device());
  vk_shader::CleanModule(m_base->get_vk_device(), shaderStages);
  userPipelines.emplace_back(
      std::pair<shader, vk_pipeline>{shaderInfo, pipeline});
}
void vk_renderer::UnloadShader(const shader &shaderInfo) {
  std::erase_if(userPipelines, [&](auto &x) {
    if (x.first == shaderInfo) {
      x.second.Cleanup(m_base->m_device);
      return true;
    };
    return false;
  });
}

void vk_renderer::add_instancing(const instancing &_instancing) {
  auto t = std::make_shared<vk::vk_draw_object>();
  t->drawObject = _instancing.object;
  registerImageObject(_instancing.object->texture_handle);
  t->uniformBuffers.resize(m_base->mSwapchain->GetImageCount());
  for (auto &v : t->uniformBuffers) {
    VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    v = CreateBuffer(sizeof(shader_parameter),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags);
  }
  layouts.push_back(m_descriptorSetLayout);
  prepareDescriptorSet(t);

  vk_instancing vi{_instancing};
  vi.vk_draw_object = t;
  vi.instance_buffer = CreateBuffer(_instancing.size,
                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  write_memory(vi.instance_buffer.allocation, _instancing.data.data(),
               _instancing.size);
  if (_instancing.type == object_type::_2D) {
    m_instancies_2d.push_back(vi);
  } else {
    m_instancies_3d.push_back(vi);
  }
}

void vk_renderer::draw_instancing_3d(VkCommandBuffer command) {
  for (auto &_instancing : m_instancies_3d) {
    pipelineInstancingOpaque.Bind(command);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout.GetLayout(),
        0, 1, &_instancing.vk_draw_object->descripterSet[m_base->m_imageIndex],
        0, nullptr);
    auto allocation =
        _instancing.vk_draw_object->uniformBuffers[m_base->m_imageIndex]
            .allocation;
    write_memory(allocation, &_instancing.vk_draw_object->drawObject->param,
                 sizeof(shader_parameter));
    std::string index = _instancing.ins.object->vertexIndex;
    VkBuffer buffers[] = {m_VertexArrays[index].vertexBuffer.buffer,
                          _instancing.instance_buffer.buffer};
    VkDeviceSize offsets[] = {0, 0};
    vkCmdBindVertexBuffers(command, 0, 2, buffers, offsets);
    vkCmdBindIndexBuffer(command, m_VertexArrays[index].indexBuffer.buffer, 0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command, m_VertexArrays[index].indexCount,
                     _instancing.ins.data.size(), 0, 0, 0);
  }
}
void vk_renderer::draw_instancing_2d(VkCommandBuffer command) {
  for (auto &_instancing : m_instancies_2d) {
    pipelineInstancing2D.Bind(command);
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout.GetLayout(),
        0, 1, &_instancing.vk_draw_object->descripterSet[m_base->m_imageIndex],
        0, nullptr);
    auto allocation =
        _instancing.vk_draw_object->uniformBuffers[m_base->m_imageIndex]
            .allocation;
    write_memory(allocation, &_instancing.vk_draw_object->drawObject->param,
                 sizeof(shader_parameter));
    std::string index = _instancing.ins.object->vertexIndex;
    VkBuffer buffers[] = {m_VertexArrays[index].vertexBuffer.buffer,
                          _instancing.instance_buffer.buffer};
    VkDeviceSize offsets[] = {0, 0};
    vkCmdBindVertexBuffers(command, 0, 2, buffers, offsets);
    vkCmdBindIndexBuffer(command, m_VertexArrays[index].indexBuffer.buffer, 0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command, m_VertexArrays[index].indexCount,
                     _instancing.ins.data.size(), 0, 0, 0);
  }
}

void vk_renderer::prepare() {
  prepareDescriptorSetLayout();
  prepareDescriptorPool();

  m_sampler = createSampler();
  layouts.resize(m_base->mSwapchain->GetImageCount());
  for (auto &i : layouts) {
    i = m_descriptorSetLayout;
  }

  mPipelineLayout.Initialize(m_base->get_vk_device(), &m_descriptorSetLayout,
                             m_base->mSwapchain->GetSurfaceExtent());
  mPipelineLayout.Prepare(m_base->get_vk_device());

  // Opaque pipeline
  {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        vk_shader::LoadModule(m_base->get_vk_device(), "shader.vert.spv",
                              VK_SHADER_STAGE_VERTEX_BIT),
        vk_shader::LoadModule(m_base->get_vk_device(), "shaderOpaque.frag.spv",
                              VK_SHADER_STAGE_FRAGMENT_BIT)};
    pipelineOpaque.Initialize(mPipelineLayout, m_base->m_renderPass,
                              shaderStages);
    pipelineOpaque.ColorBlendFactor(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
    pipelineOpaque.AlphaBlendFactor(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
    pipelineOpaque.Prepare(m_base->get_vk_device());
    vk_shader::CleanModule(m_base->get_vk_device(), shaderStages);
  }

  // alpha pipeline
  {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        vk_shader::LoadModule(m_base->get_vk_device(), "shader.vert.spv",
                              VK_SHADER_STAGE_VERTEX_BIT),
        vk_shader::LoadModule(m_base->get_vk_device(), "shaderAlpha.frag.spv",
                              VK_SHADER_STAGE_FRAGMENT_BIT)};
    pipelineAlpha.Initialize(mPipelineLayout, m_base->m_renderPass,
                             shaderStages);
    pipelineAlpha.ColorBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA,
                                   VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    pipelineAlpha.AlphaBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA,
                                   VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    pipelineAlpha.SetDepthTest(VK_TRUE);
    pipelineAlpha.SetDepthWrite(VK_FALSE);
    pipelineAlpha.Prepare(m_base->get_vk_device());
    vk_shader::CleanModule(m_base->get_vk_device(), shaderStages);
  }
  // 2D pipeline
  {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        vk_shader::LoadModule(m_base->get_vk_device(), "shader.vert.spv",
                              VK_SHADER_STAGE_VERTEX_BIT),
        vk_shader::LoadModule(m_base->get_vk_device(), "shaderAlpha.frag.spv",
                              VK_SHADER_STAGE_FRAGMENT_BIT)};
    pipeline2D.Initialize(mPipelineLayout, m_base->m_renderPass, shaderStages);
    pipeline2D.ColorBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA,
                                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    pipeline2D.AlphaBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA,
                                VK_BLEND_FACTOR_ZERO);
    pipeline2D.SetDepthTest(VK_FALSE);
    pipeline2D.SetDepthWrite(VK_FALSE);
    pipeline2D.Prepare(m_base->get_vk_device());
    vk_shader::CleanModule(m_base->get_vk_device(), shaderStages);
  }
  // SkyBox pipeline
  {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        vk_shader::LoadModule(m_base->get_vk_device(), "shader.vert.spv",
                              VK_SHADER_STAGE_VERTEX_BIT),
        vk_shader::LoadModule(m_base->get_vk_device(), "shaderOpaque.frag.spv",
                              VK_SHADER_STAGE_FRAGMENT_BIT)};
    pipelineSkyBox.Initialize(mPipelineLayout, m_base->m_renderPass,
                              shaderStages);
    pipelineSkyBox.ColorBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA,
                                    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    pipelineSkyBox.AlphaBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA,
                                    VK_BLEND_FACTOR_ZERO);
    pipelineSkyBox.SetDepthTest(VK_FALSE);
    pipelineSkyBox.SetDepthWrite(VK_FALSE);
    pipelineSkyBox.Prepare(m_base->get_vk_device());
    vk_shader::CleanModule(m_base->get_vk_device(), shaderStages);
  }
  //
  // Instancing pipelines
  //
  // Opaque pipeline
  {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        vk_shader::LoadModule(m_base->get_vk_device(),
                              "shader_instance.vert.spv",
                              VK_SHADER_STAGE_VERTEX_BIT),
        vk_shader::LoadModule(m_base->get_vk_device(), "shaderOpaque.frag.spv",
                              VK_SHADER_STAGE_FRAGMENT_BIT)};
    pipelineInstancingOpaque.Initialize(mPipelineLayout, m_base->m_renderPass,
                                        shaderStages);
    pipelineInstancingOpaque.ColorBlendFactor(VK_BLEND_FACTOR_ONE,
                                              VK_BLEND_FACTOR_ZERO);
    pipelineInstancingOpaque.AlphaBlendFactor(VK_BLEND_FACTOR_ONE,
                                              VK_BLEND_FACTOR_ZERO);
    pipelineInstancingOpaque.Prepare(m_base->get_vk_device());
    vk_shader::CleanModule(m_base->get_vk_device(), shaderStages);
  }

  // alpha pipeline
  {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        vk_shader::LoadModule(m_base->get_vk_device(),
                              "shader_instance.vert.spv",
                              VK_SHADER_STAGE_VERTEX_BIT),
        vk_shader::LoadModule(m_base->get_vk_device(), "shaderAlpha.frag.spv",
                              VK_SHADER_STAGE_FRAGMENT_BIT)};
    pipelineInstancingAlpha.Initialize(mPipelineLayout, m_base->m_renderPass,
                                       shaderStages);
    pipelineInstancingAlpha.ColorBlendFactor(
        VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    pipelineInstancingAlpha.AlphaBlendFactor(
        VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    pipelineInstancingAlpha.SetDepthTest(VK_TRUE);
    pipelineInstancingAlpha.SetDepthWrite(VK_FALSE);
    pipelineInstancingAlpha.Prepare(m_base->get_vk_device());
    vk_shader::CleanModule(m_base->get_vk_device(), shaderStages);
  }
  // 2D pipeline
  {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        vk_shader::LoadModule(m_base->get_vk_device(),
                              "shader_instance.vert.spv",
                              VK_SHADER_STAGE_VERTEX_BIT),
        vk_shader::LoadModule(m_base->get_vk_device(), "shaderAlpha.frag.spv",
                              VK_SHADER_STAGE_FRAGMENT_BIT)};
    pipelineInstancing2D.Initialize(mPipelineLayout, m_base->m_renderPass,
                                    shaderStages);
    pipelineInstancing2D.ColorBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA,
                                          VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    pipelineInstancing2D.AlphaBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA,
                                          VK_BLEND_FACTOR_ZERO);
    pipelineInstancing2D.SetDepthTest(VK_FALSE);
    pipelineInstancing2D.SetDepthWrite(VK_FALSE);
    pipelineInstancing2D.Prepare(m_base->get_vk_device());
    vk_shader::CleanModule(m_base->get_vk_device(), shaderStages);
  }
  prepareImGUI();
}
void vk_renderer::cleanup() {
  VkDevice device = m_base->get_vk_device();

  for (auto &i : mImageObjects) {
    DestroyImage(i.second);
  }
  DestroyVulkanObject<VkSampler>(device, m_sampler, &vkDestroySampler);
  mPipelineLayout.Cleanup(device);
  pipelineOpaque.Cleanup(device);
  pipelineAlpha.Cleanup(device);
  pipeline2D.Cleanup(device);
  for (auto &i : userPipelines) {
    i.second.Cleanup(device);
  }
  for (auto &i : m_VertexArrays) {
    DestroyVulkanObject<VkBuffer>(device, i.second.vertexBuffer.buffer,
                                  &vkDestroyBuffer);
    DestroyVulkanObject<VkBuffer>(device, i.second.indexBuffer.buffer,
                                  &vkDestroyBuffer);
  }
  DestroyVulkanObject<VkDescriptorPool>(device, m_descriptorPool,
                                        &vkDestroyDescriptorPool);
  DestroyVulkanObject<VkDescriptorSetLayout>(device, m_descriptorSetLayout,
                                             &vkDestroyDescriptorSetLayout);
}

void vk_renderer::makeCommand(VkCommandBuffer command,
                              VkRenderPassBeginInfo &ri,
                              VkCommandBufferBeginInfo &ci, VkFence &fence) {
  vkBeginCommandBuffer(command, &ci);
  auto viewport = VkViewport{
      .x = 0.f,
      .y = static_cast<float>(m_base->mSwapchain->GetSurfaceExtent().height),
      .width = static_cast<float>(m_base->mSwapchain->GetSurfaceExtent().width),
      .height =
          -static_cast<float>(m_base->mSwapchain->GetSurfaceExtent().height),
      .minDepth = 0.f,
      .maxDepth = 1.f};
  VkRect2D scissor{{0, 0},
                   {(m_base->mSwapchain->GetSurfaceExtent().width),
                    (m_base->mSwapchain->GetSurfaceExtent().height)}};
  vkCmdBeginRenderPass(command, &ri, VK_SUBPASS_CONTENTS_INLINE);
  /* skybox
   */
  draw_skybox(command);
  draw3d(command);
  draw_instancing_3d(command);
  draw2d(command);
  draw_instancing_2d(command);
  renderImGUI(command);
  pipelineOpaque.Bind(command);

  for (auto &sprite : mDrawObject3D)
    unregisterTexture(sprite);
  mDrawObject3D.clear();
  for (auto &sprite : mDrawObject2D)
    unregisterTexture(sprite);
  mDrawObject2D.clear();
  for (auto &_instancing : m_instancies_2d) {
    unregisterTexture(_instancing.vk_draw_object);
    DestroyBuffer(_instancing.instance_buffer);
  }
  m_instancies_2d.clear();
  for (auto &_instancing : m_instancies_3d) {
    unregisterTexture(_instancing.vk_draw_object);
    DestroyBuffer(_instancing.instance_buffer);
  }
  m_instancies_3d.clear();

  vkCmdSetScissor(command, 0, 1, &scissor);
  vkCmdSetViewport(command, 0, 1, &viewport);
  vkWaitForFences(m_base->get_vk_device(), 1, &fence, VK_TRUE, UINT64_MAX);
}
void vk_renderer::draw_skybox(VkCommandBuffer command) {
  pipelineSkyBox.Bind(command);
  auto t = std::make_shared<vk::vk_draw_object>();
  t->drawObject = std::make_shared<draw_object>();
  t->drawObject->texture_handle = get_renderer().skybox_texture->handle;
  t->drawObject->vertexIndex = "BOX";
  shader_parameter param;
  matrix4 w = matrix4::Identity;
  w[0][0] = 5;
  w[1][1] = 5;
  w[2][2] = 5;
  param.proj = get_camera().projection;
  param.view = matrix4::LookAt(vector3(0, 0, 0),
                               get_camera().target - get_camera().position,
                               get_camera().up);
  auto &va = m_VertexArrays["BOX"];

  if (!mImageObjects.contains(t->drawObject->texture_handle)) {
    mImageObjects[t->drawObject->texture_handle] =
        vk_renderer::createTextureFromSurface(
            get_texture_system().get(t->drawObject->texture_handle));
  } else {
    update_image_object(t->drawObject->texture_handle);
  }
  t->uniformBuffers.resize(m_base->mSwapchain->GetImageCount());
  for (auto &v : t->uniformBuffers) {
    VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    v = CreateBuffer(sizeof(shader_parameter),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags);
  }
  layouts.push_back(m_descriptorSetLayout);
  prepareDescriptorSet(t);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(command, 0, 1, &va.vertexBuffer.buffer, &offset);
  vkCmdBindIndexBuffer(command, va.indexBuffer.buffer, offset,
                       VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          mPipelineLayout.GetLayout(), 0, 1,
                          &t->descripterSet[m_base->m_imageIndex], 0, nullptr);
  auto allocation = t->uniformBuffers[m_base->m_imageIndex].allocation;
  write_memory(allocation, &param, sizeof(shader_parameter));
  vkCmdDrawIndexed(command, va.indexCount, 1, 0, 0, 0);
  unregisterTexture(t);
}

void vk_renderer::draw3d(VkCommandBuffer command) {
  pipelineOpaque.Bind(command);
  VkDeviceSize offset = 0;
  for (auto &sprite : mDrawObject3D) {
    if (sprite->drawObject->shader_data.vertName == "default" &&
        sprite->drawObject->shader_data.fragName == "default")
      pipelineOpaque.Bind(command);
    else {
      for (auto &i : userPipelines) {
        if (i.first == sprite->drawObject->shader_data)
          i.second.Bind(command);
      }
    }

    std::string index = sprite->drawObject->vertexIndex;
    ::vkCmdBindVertexBuffers(
        command, 0, 1, &m_VertexArrays[index].vertexBuffer.buffer, &offset);
    ::vkCmdBindIndexBuffer(command, m_VertexArrays[index].indexBuffer.buffer,
                           offset, VK_INDEX_TYPE_UINT32);
    // Set descriptors
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout.GetLayout(),
        0, 1, &sprite->descripterSet[m_base->m_imageIndex], 0, nullptr);
    auto allocation = sprite->uniformBuffers[m_base->m_imageIndex].allocation;
    write_memory(allocation, &sprite->drawObject->param,
                 sizeof(shader_parameter));
    vkCmdDrawIndexed(command, m_VertexArrays[index].indexCount, 1, 0, 0, 0);
  }
}

void vk_renderer::draw2d(VkCommandBuffer command) {
  pipeline2D.Bind(command);
  VkDeviceSize offset = 0;
  for (auto &sprite : mDrawObject2D) {
    if (sprite->drawObject->shader_data.vertName == "default" &&
        sprite->drawObject->shader_data.fragName == "default")
      pipeline2D.Bind(command);
    else {
      for (auto &i : userPipelines) {
        if (i.first == sprite->drawObject->shader_data)
          i.second.Bind(command);
      }
    }
    vkCmdBindVertexBuffers(
        command, 0, 1,
        &m_VertexArrays[sprite->drawObject->vertexIndex].vertexBuffer.buffer,
        &offset);
    vkCmdBindIndexBuffer(
        command,
        m_VertexArrays[sprite->drawObject->vertexIndex].indexBuffer.buffer,
        offset, VK_INDEX_TYPE_UINT32);

    // Set descriptors
    vkCmdBindDescriptorSets(
        command, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout.GetLayout(),
        0, 1, &sprite->descripterSet[m_base->m_imageIndex], 0, nullptr);
    auto allocation = sprite->uniformBuffers[m_base->m_imageIndex].allocation;
    write_memory(allocation, &sprite->drawObject->param,
                 sizeof(shader_parameter));

    vkCmdDrawIndexed(command,
                     m_VertexArrays[sprite->drawObject->vertexIndex].indexCount,
                     1, 0, 0, 0);
  }
}

void vk_renderer::write_memory(VmaAllocation allocation, const void *data,
                               size_t size) {
  void *p = nullptr;
  vmaMapMemory(allocator, allocation, &p);
  memcpy(p, data, size);
  vmaUnmapMemory(allocator, allocation);
}

void vk_renderer::prepareImGUI() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
  io.IniFilename = NULL;
  io.Fonts->AddFontFromFileTTF("data/font/mplus/mplus-1p-medium.ttf", 18.0f,
                               nullptr, io.Fonts->GetGlyphRangesJapanese());

  ImGui_ImplSDL2_InitForVulkan(
      (SDL_Window *)m_base->m_manager.get_window().GetSDLWindow());

  uint32_t imageCount = m_base->mSwapchain->GetImageCount();
  ImGui_ImplVulkan_InitInfo info{};
  info.Instance = m_base->m_instance;
  info.PhysicalDevice = m_base->m_physDev;
  info.Device = m_base->get_vk_device();
  info.QueueFamily = m_base->m_graphicsQueueIndex;
  info.Queue = m_base->get_vk_queue();
  info.DescriptorPool = m_descriptorPool;
  info.MinImageCount = 2;
  info.ImageCount = imageCount;
  if (!ImGui_ImplVulkan_Init(&info, m_base->m_renderPass)) {
    std::exit(1);
  }

  // Transfar font texture
  VkCommandBufferAllocateInfo commandAI{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
      m_base->m_commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};
  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };

  VkCommandBuffer command;
  vkAllocateCommandBuffers(m_base->get_vk_device(), &commandAI, &command);
  vkBeginCommandBuffer(command, &beginInfo);
  ImGui_ImplVulkan_CreateFontsTexture(command);
  vkEndCommandBuffer(command);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr};
  submitInfo.pCommandBuffers = &command;
  submitInfo.commandBufferCount = 1;

  vkQueueSubmit(m_base->get_vk_queue(), 1, &submitInfo, VK_NULL_HANDLE);

  // Wait for transfar font texture
  vkDeviceWaitIdle(m_base->get_vk_device());
  vkFreeCommandBuffers(m_base->get_vk_device(), m_base->m_commandPool, 1,
                       &command);
}
void vk_renderer::renderImGUI(VkCommandBuffer command) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(
      (SDL_Window *)m_base->m_manager.get_window().GetSDLWindow());
  ImGui::NewFrame();

  if (m_manager.get_renderer().isShowImGui()) {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(get_window().Size().x, get_window().Size().y), ImGuiCond_Always);
    ImGui::Begin("Editor", nullptr,
                 ImGuiWindowFlags_HorizontalScrollbar |
                     ImGuiWindowFlags_MenuBar);
    for (auto &i : get_renderer().get_imgui_function()) {
      i();
    }
    ImGui::End();
  }

  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command);
}

void vk_renderer::prepareDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  VkDescriptorSetLayoutBinding bindingUBO{}, bindingTex{}, bindingInstance{};
  bindingUBO.binding = 0;
  bindingUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindingUBO.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  bindingUBO.descriptorCount = 1;
  bindings.push_back(bindingUBO);

  bindingTex.binding = 1;
  bindingTex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindingTex.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindingTex.descriptorCount = 1;
  bindings.push_back(bindingTex);

  bindingInstance.binding = 2;
  bindingInstance.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindingInstance.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  bindingInstance.descriptorCount = 1;
  bindings.push_back(bindingInstance);

  VkDescriptorSetLayoutCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  ci.bindingCount = uint32_t(bindings.size());
  ci.pBindings = bindings.data();
  vkCreateDescriptorSetLayout(m_base->get_vk_device(), &ci, nullptr,
                              &m_descriptorSetLayout);
}

void vk_renderer::prepareDescriptorPool() {

  VkDescriptorPoolSize poolSize[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxpoolSize},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxpoolSize},
  };
  VkDescriptorPoolCreateInfo descPoolCI{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      maxpoolSize * sizeof(poolSize) / sizeof(poolSize[0]), // maxSets
      sizeof(poolSize) / sizeof(poolSize[0]),
      poolSize,
  };
  vkCreateDescriptorPool(m_base->get_vk_device(), &descPoolCI, nullptr,
                         &m_descriptorPool);
}

void vk_renderer::prepareDescriptorSet(std::shared_ptr<vk_draw_object> sprite) {
  VkDescriptorSetAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  ai.descriptorPool = m_descriptorPool;
  ai.descriptorSetCount = uint32_t(m_base->mSwapchain->GetImageCount());
  ai.pSetLayouts = layouts.data();
  sprite->descripterSet.resize(m_base->mSwapchain->GetImageCount());
  vkAllocateDescriptorSets(m_base->get_vk_device(), &ai,
                           sprite->descripterSet.data());
  // Write to descriptor set.
  for (int i = 0; i < m_base->mSwapchain->GetImageCount(); i++) {
    VkDescriptorBufferInfo descUBO{};
    descUBO.buffer = sprite->uniformBuffers[i].buffer;
    descUBO.offset = 0;
    descUBO.range = VK_WHOLE_SIZE;

    VkDescriptorImageInfo descImage{};
    descImage.imageView =
        mImageObjects[sprite->drawObject->texture_handle].view;
    descImage.sampler = m_sampler;
    descImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet ubo{};
    ubo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ubo.dstBinding = 0;
    ubo.descriptorCount = 1;
    ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo.pBufferInfo = &descUBO;
    ubo.dstSet = sprite->descripterSet[i];

    VkWriteDescriptorSet tex{};
    tex.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    tex.dstBinding = 1;
    tex.descriptorCount = 1;
    tex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    tex.pImageInfo = &descImage;
    tex.dstSet = sprite->descripterSet[i];

    std::vector<VkWriteDescriptorSet> writeSets = {ubo, tex};
    vkUpdateDescriptorSets(m_base->get_vk_device(), uint32_t(writeSets.size()),
                           writeSets.data(), 0, nullptr);
  }
}
vk_buffer_object vk_renderer::CreateBuffer(uint32_t size,
                                           VkBufferUsageFlags usage,
                                           VkMemoryPropertyFlags flags) {
  vk_buffer_object obj;
  VkBufferCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  ci.usage = usage;
  ci.size = size;
  VmaAllocationCreateInfo buffer_alloc_info = {};
  buffer_alloc_info.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
  vmaCreateBuffer(allocator, &ci, &buffer_alloc_info, &obj.buffer,
                  &obj.allocation, nullptr);
  return obj;
}

VkSampler vk_renderer::createSampler() {
  VkSampler sampler;
  VkSamplerCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  ci.minFilter = VK_FILTER_LINEAR;
  ci.magFilter = VK_FILTER_LINEAR;
  ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  ci.maxAnisotropy = 1.0f;
  ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  vkCreateSampler(m_base->get_vk_device(), &ci, nullptr, &sampler);
  return sampler;
}

vk_image_object vk_renderer::create_texture(SDL_Surface *imagedata,
                                            VkFormat format) {
  vk_buffer_object stagingBuffer;
  vk_image_object texture{};
  int width = imagedata->w, height = imagedata->h;
  auto *pImage = imagedata->pixels;
  {
    // Create VkImage texture
    VkImageCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.extent = {uint32_t(width), uint32_t(height), 1};
    ci.format = format;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.arrayLayers = 1;
    ci.mipLevels = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ci.tiling = VkImageTiling::VK_IMAGE_TILING_LINEAR;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaCreateImage(allocator, &ci, &alloc_info, &texture.image,
                   &texture.allocation, nullptr);
  }

  {
    uint32_t imageSize = imagedata->h * imagedata->w * 4;
    stagingBuffer = CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    write_memory(stagingBuffer.allocation, pImage, imageSize);
  }

  VkBufferImageCopy copyRegion{};
  copyRegion.imageExtent = {uint32_t(width), uint32_t(height), 1};
  copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  VkCommandBuffer command;
  {
    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandBufferCount = 1;
    ai.commandPool = m_base->m_commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_base->get_vk_device(), &ai, &command);
  }

  VkCommandBufferBeginInfo commandBI{};
  commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(command, &commandBI);
  setImageMemoryBarrier(command, texture.image, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vkCmdCopyBufferToImage(command, stagingBuffer.buffer, texture.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  setImageMemoryBarrier(command, texture.image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vkEndCommandBuffer(command);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command;
  vkQueueSubmit(m_base->get_vk_queue(), 1, &submitInfo,
                m_base->m_fences[m_base->m_imageIndex]);

  {
    // Create view for texture reference
    VkImageViewCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.image = texture.image;
    ci.format = format;
    ci.components = {
        .r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_R,
        .g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_G,
        .b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_B,
        .a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_A,
    };

    ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCreateImageView(m_base->get_vk_device(), &ci, nullptr, &texture.view);
  }
  vkFreeCommandBuffers(m_base->get_vk_device(), m_base->m_commandPool, 1,
                       &command);
  DestroyBuffer(stagingBuffer);
  SDL_FreeSurface(imagedata);
  return texture;
}

vk_image_object
vk_renderer::createTextureFromSurface(const ::SDL_Surface &surface) {
  ::SDL_Surface surf = surface;
  ::SDL_LockSurface(&surf);
  auto *formatbuf = ::SDL_AllocFormat(SDL_PIXELFORMAT_ABGR8888);
  formatbuf->BytesPerPixel = 4;
  auto imagedata = ::SDL_ConvertSurface(&surf, formatbuf, 0);
  ::SDL_UnlockSurface(&surf);
  auto format = VK_FORMAT_R8G8B8A8_UNORM;
  SDL_FreeFormat(formatbuf);
  return create_texture(imagedata, format);
}

vk_image_object
vk_renderer::createTextureFromMemory(const std::vector<char> &imageData) {
  auto *rw = ::SDL_RWFromMem((void *)imageData.data(), imageData.size());
  ::SDL_Surface *surface;
  surface = ::IMG_Load_RW(rw, 1);
  ::SDL_LockSurface(surface);
  auto formatbuf = ::SDL_AllocFormat(SDL_PIXELFORMAT_ABGR8888);
  formatbuf->BytesPerPixel = 4;
  auto imagedata = SDL_ConvertSurface(surface, formatbuf, 0);
  SDL_UnlockSurface(surface);
  return create_texture(imagedata, VK_FORMAT_R8G8B8A8_UNORM);
}

void vk_renderer::setImageMemoryBarrier(VkCommandBuffer command, VkImage image,
                                        VkImageLayout oldLayout,
                                        VkImageLayout newLayout) {
  VkImageMemoryBarrier imb{};
  imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imb.oldLayout = oldLayout;
  imb.newLayout = newLayout;
  imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imb.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  imb.image = image;

  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

  switch (oldLayout) {
  case VK_IMAGE_LAYOUT_UNDEFINED:
    imb.srcAccessMask = 0;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  default:
    break;
  }

  switch (newLayout) {
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    imb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    break;
  }

  srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

  vkCmdPipelineBarrier(command, srcStage, dstStage, 0,
                       0, // memoryBarrierCount
                       nullptr,
                       0, // bufferMemoryBarrierCount
                       nullptr,
                       1, // imageMemoryBarrierCount
                       &imb);
}

uint32_t
vk_renderer::GetMemoryTypeIndex(uint32_t requestBits,
                                VkMemoryPropertyFlags requestProps) const {
  uint32_t result = ~0u;
  for (uint32_t i = 0; i < m_physicalMemProps.memoryTypeCount; ++i) {
    if (requestBits & 1) {
      const auto &types = m_physicalMemProps.memoryTypes[i];
      if ((types.propertyFlags & requestProps) == requestProps) {
        result = i;
        break;
      }
    }
    requestBits >>= 1;
  }
  return result;
}

VkFramebuffer vk_renderer::CreateFramebuffer(VkRenderPass renderPass,
                                             uint32_t width, uint32_t height,
                                             uint32_t viewCount,
                                             VkImageView *views) {
  VkFramebufferCreateInfo fbCI{
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      nullptr,
      0,
      renderPass,
      viewCount,
      views,
      width,
      height,
      1,
  };
  VkFramebuffer framebuffer;
  vkCreateFramebuffer(m_base->get_vk_device(), &fbCI, nullptr, &framebuffer);
  return framebuffer;
}

void vk_renderer::DestroyBuffer(vk_buffer_object &bufferObj) {
  vmaDestroyBuffer(allocator, bufferObj.buffer, bufferObj.allocation);
}

void vk_renderer::DestroyImage(vk_image_object &imageObj) {
  vkDestroyImageView(m_base->get_vk_device(), imageObj.view, nullptr);
  vmaDestroyImage(allocator, imageObj.image, imageObj.allocation);
}

void vk_renderer::DestroyFramebuffers(uint32_t count,
                                      VkFramebuffer *framebuffers) {
  for (uint32_t i = 0; i < count; ++i) {
    vkDestroyFramebuffer(m_base->get_vk_device(), framebuffers[i], nullptr);
  }
}
void vk_renderer::update_image_object(const handle_t &handle) {
  DestroyImage(mImageObjects[handle]);
  mImageObjects.erase(handle);
  ::SDL_Surface &surf = get_texture_system().get(handle);
  ::SDL_LockSurface(&surf);
  auto *formatbuf = ::SDL_AllocFormat(SDL_PIXELFORMAT_ABGR8888);
  formatbuf->BytesPerPixel = 4;
  auto *imagedata = ::SDL_ConvertSurface(&surf, formatbuf, 0);
  ::SDL_UnlockSurface(&surf);
  vk_buffer_object stagingBuffer;
  vk_image_object texture;
  int width = imagedata->w, height = imagedata->h;
  auto *pImage = imagedata->pixels;
  {
    // Create VkImage texture
    VkImageCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.extent = {uint32_t(width), uint32_t(height), 1};
    ci.format = VK_FORMAT_R8G8B8A8_UNORM;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.arrayLayers = 1;
    ci.mipLevels = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ci.tiling = VkImageTiling::VK_IMAGE_TILING_LINEAR;
    ci.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaCreateImage(allocator, &ci, &alloc_info, &texture.image,
                   &texture.allocation, nullptr);
  }

  {
    uint32_t imageSize = imagedata->h * imagedata->w * 4;
    stagingBuffer = CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    write_memory(stagingBuffer.allocation, pImage, imageSize);
  }

  VkBufferImageCopy copyRegion{};
  copyRegion.imageExtent = {uint32_t(width), uint32_t(height), 1};
  copyRegion.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  VkCommandBuffer command;
  {
    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandBufferCount = 1;
    ai.commandPool = m_base->m_commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(m_base->get_vk_device(), &ai, &command);
  }

  VkCommandBufferBeginInfo commandBI{};
  commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(command, &commandBI);
  setImageMemoryBarrier(command, texture.image,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  vkCmdCopyBufferToImage(command, stagingBuffer.buffer, texture.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  setImageMemoryBarrier(command, texture.image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  vkEndCommandBuffer(command);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command;
  vkQueueSubmit(m_base->get_vk_queue(), 1, &submitInfo,
                m_base->m_fences[m_base->m_imageIndex]);

  // Create view for texture reference
  VkImageViewCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ci.image = texture.image;
  ci.format = VK_FORMAT_R8G8B8A8_UNORM;
  ci.components = {
      .r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_R,
      .g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_G,
      .b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_B,
      .a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_A,
  };

  ci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  vkCreateImageView(m_base->get_vk_device(), &ci, nullptr, &texture.view);
  vkDeviceWaitIdle(m_base->get_vk_device());
  vkFreeCommandBuffers(m_base->get_vk_device(), m_base->m_commandPool, 1,
                       &command);
  DestroyBuffer(stagingBuffer);
  SDL_FreeFormat(formatbuf);
  SDL_FreeSurface(imagedata);
  mImageObjects.emplace(handle, texture);
}
void vk_renderer::registerImageObject(const handle_t &handle) {
  if (mImageObjects.contains(handle)) {
    update_image_object(handle);
    return;
  }
  mImageObjects[handle] =
      vk_renderer::createTextureFromSurface(get_texture_system().get(handle));
}
void vk_renderer::unregister_image_object(const handle_t &handle) {
  for (auto it = mImageObjects.begin(); it != mImageObjects.end(); ++it) {
    if (it->first == handle) {
      DestroyImage(it->second);
      mImageObjects.erase(it);
      return;
    }
  }
}
VkRenderPass vk_renderer::GetRenderPass(const std::string &name) {
  if (m_base)
    return m_base->m_renderPass;
  return 0;
}
VkDevice vk_renderer::GetDevice() {
  if (m_base)
    return m_base->get_vk_device();
  return m_base->get_vk_device();
}
void vk_renderer::registerTexture(std::shared_ptr<vk_draw_object> texture,
                                  texture_type type) {
  if (texture_type::Image3D == type) {
    mDrawObject3D.push_back(texture);
    texture->uniformBuffers.resize(m_base->mSwapchain->GetImageCount());
    for (auto &v : texture->uniformBuffers) {
      VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      v = CreateBuffer(sizeof(shader_parameter),
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags);
    }
    layouts.push_back(m_descriptorSetLayout);
    prepareDescriptorSet(texture);
  } else {
    auto iter = mDrawObject2D.begin();
    for (; iter != mDrawObject2D.end(); ++iter) {
      if (texture->drawObject->drawOrder < (*iter)->drawObject->drawOrder) {
        break;
      }
    }

    // Inserts element before position of iterator
    mDrawObject2D.insert(iter, texture);
    texture->uniformBuffers.resize(m_base->mSwapchain->GetImageCount());
    for (auto &v : texture->uniformBuffers) {
      VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      v = CreateBuffer(sizeof(shader_parameter),
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags);
    }
    layouts.push_back(m_descriptorSetLayout);
    prepareDescriptorSet(texture);
  }
}
void vk_renderer::unregisterTexture(std::shared_ptr<vk_draw_object> texture) {
  auto device = m_base->get_vk_device();
  vkFreeDescriptorSets(device, m_descriptorPool,
                       static_cast<uint32_t>(texture->descripterSet.size()),
                       texture->descripterSet.data());
  for (auto &i : texture->uniformBuffers) {
    DestroyBuffer(i);
  }
  texture->uniformBuffers.clear();
  layouts.pop_back();
}
} // namespace nen::vk
#endif
