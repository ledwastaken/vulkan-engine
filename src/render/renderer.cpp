#include "render/renderer.h"

#include <cstring>
#include <fstream>

#include "core/engine.h"
#include "scene/mesh.h"
#include "scene/scene.h"
#include "types/matrix4.h"

using namespace scene;
using namespace types;

namespace render
{
  void Renderer::init()
  {
    create_uniform_buffer();

    create_shader_module("shader.vert.spv", &vertex_shader_);
    create_shader_module("shader.frag.spv", &fragment_shader_);

    create_descriptor_set_layout();
    create_pipeline_layout();
    create_pipeline_cache();

    create_pipeline();
  }

  void Renderer::free()
  {
    auto& engine = core::Engine::get_singleton();

    vkUnmapMemory(engine.get_device(), uniform_buffer_memory_);
    vkDestroyBuffer(engine.get_device(), uniform_buffer_, nullptr);
    vkFreeMemory(engine.get_device(), uniform_buffer_memory_, nullptr);

    vkDestroyPipeline(engine.get_device(), pipeline_, nullptr);

    vkDestroyDescriptorSetLayout(engine.get_device(), descriptor_set_layout_, nullptr);
    vkDestroyPipelineLayout(engine.get_device(), pipeline_layout_, nullptr);
    vkDestroyPipelineCache(engine.get_device(), pipeline_cache_, nullptr);

    vkDestroyShaderModule(engine.get_device(), vertex_shader_, nullptr);
    vkDestroyShaderModule(engine.get_device(), fragment_shader_, nullptr);
  }

  void Renderer::operator()(scene::Scene& scene, uint32_t image_index)
  {
    auto camera = scene.current_camera;

    if (!camera)
      return;

    auto& engine = core::Engine::get_singleton();
    command_buffer_ = engine.get_command_buffer(image_index);

    const VkCommandBufferBeginInfo command_buffer_begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pInheritanceInfo = nullptr,
    };

    VkResult result = vkResetCommandBuffer(command_buffer_, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to reset command buffer");

    result = vkBeginCommandBuffer(command_buffer_, &command_buffer_begin_info);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to begin command buffer recording");

    const VkRect2D render_area = {
      .offset = { 0, 0 },
      .extent = engine.get_swapchain_extent(),
    };

    const VkClearValue clear_value = { { { 0.3f, 0.3f, 0.4f, 1.0f } } };

    const VkRenderPassBeginInfo renderpass_begin_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = engine.get_renderpass(),
      .framebuffer = engine.get_frambuffer(image_index),
      .renderArea = render_area,
      .clearValueCount = 1,
      .pClearValues = &clear_value,
    };

    vkCmdBeginRenderPass(command_buffer_, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(engine.get_swapchain_extent().width),
      .height = static_cast<float>(engine.get_swapchain_extent().height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
      .offset = { 0, 0 },
      .extent = engine.get_swapchain_extent(),
    };

    vkCmdSetViewport(command_buffer_, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer_, 0, 1, &scissor);

    static float time = 0.0f;
    time += 0.025f;
    vkCmdPushConstants(command_buffer_, pipeline_layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(float), &time);

    // TODO: Load MVP matrices into uniform_buffer_data_
    auto projection = Matrix4::perpective(camera->field_of_view, 800.0f / 600.0f, 0.1f, 100.0f);
    auto view = camera->cframe.invert().to_matrix();

    std::memcpy(uniform_buffer_data_ + 16 * sizeof(float), view.data(), 16 * sizeof(float));
    std::memcpy(uniform_buffer_data_ + 32 * sizeof(float), projection.data(), 16 * sizeof(float));

    vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0,
                            1, &descriptor_set_, 0, nullptr);

    this->operator()(scene);

    vkCmdEndRenderPass(command_buffer_);
    result = vkEndCommandBuffer(command_buffer_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to end command buffer recording");
  }

  void Renderer::operator()(Mesh& mesh)
  {
    auto vertex_buffer = mesh.get_vertex_buffer();
    auto model = mesh.cframe.to_matrix();

    std::memcpy(uniform_buffer_data_, model.data(), 16 * sizeof(float));

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffer_, 0, 1, &vertex_buffer, &offset);
    vkCmdBindIndexBuffer(command_buffer_, mesh.get_index_buffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer_, mesh.get_index_count(), 1, 0, 0, 0);

    Visitor::operator()(mesh);
  }

  void Renderer::create_shader_module(const char* path, VkShaderModule* shader_module)
  {
    auto& engine = core::Engine::get_singleton();

    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
      throw std::runtime_error("failed to open file");

    size_t file_size = file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    const VkShaderModuleCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = file_size,
      .pCode = reinterpret_cast<uint32_t*>(buffer.data()),
    };

    VkResult result =
        vkCreateShaderModule(engine.get_device(), &create_info, nullptr, shader_module);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create shader module");
  }

  void Renderer::create_descriptor_set_layout()
  {
    auto& engine = core::Engine::get_singleton();

    const VkDescriptorSetLayoutBinding bindings[] = {
      {
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .pImmutableSamplers = nullptr,
      },
    };

    const VkDescriptorSetLayoutCreateInfo descriptor_set_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = 1,
      .pBindings = bindings,
    };

    VkResult result = vkCreateDescriptorSetLayout(engine.get_device(), &descriptor_set_create_info,
                                                  nullptr, &descriptor_set_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create create descriptor set layout");

    const VkDescriptorPoolSize descriptor_pool_size = {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 3,
    };

    const VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .maxSets = 1,
      .poolSizeCount = 1,
      .pPoolSizes = &descriptor_pool_size,
    };

    result = vkCreateDescriptorPool(engine.get_device(), &descriptor_pool_create_info, nullptr,
                                    &descriptor_pool_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor pool");

    const VkDescriptorSetAllocateInfo descriptor_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = 1,
      .pSetLayouts = &descriptor_set_layout_,
    };

    result =
        vkAllocateDescriptorSets(engine.get_device(), &descriptor_allocate_info, &descriptor_set_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor set");

    const VkDescriptorBufferInfo ubo_info = {
      .buffer = uniform_buffer_,
      .offset = 0,
      .range = 48 * sizeof(float),
    };

    const VkWriteDescriptorSet descriptor_write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = descriptor_set_,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pImageInfo = nullptr,
      .pBufferInfo = &ubo_info,
      .pTexelBufferView = nullptr,
    };

    vkUpdateDescriptorSets(engine.get_device(), 1, &descriptor_write, 0, nullptr);
  }

  void Renderer::create_pipeline_layout()
  {
    auto& engine = core::Engine::get_singleton();

    // TODO: Remove those constants
    const VkPushConstantRange push_constants = {
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .offset = 0,
      .size = sizeof(float),
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout_,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constants,
    };

    VkResult result = vkCreatePipelineLayout(engine.get_device(), &pipeline_layout_create_info,
                                             nullptr, &pipeline_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout");
  }

  void Renderer::create_pipeline_cache()
  {
    auto& engine = core::Engine::get_singleton();

    const VkPipelineCacheCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .initialDataSize = 0,
      .pInitialData = nullptr,
    };

    VkResult result =
        vkCreatePipelineCache(engine.get_device(), &create_info, nullptr, &pipeline_cache_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline cache");
  }

  void Renderer::create_pipeline()
  {
    auto& engine = core::Engine::get_singleton();

    const VkPipelineShaderStageCreateInfo shader_stage_infos[] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vertex_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fragment_shader_,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
    };

    const VkVertexInputBindingDescription vertex_binding_descriptions[] = {
      {
          .binding = 0,
          .stride = 32,
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      },
    };

    const VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
      {
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0,
      },
      {
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 12,
      },
      {
          .location = 2,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = 24,
      }
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = vertex_binding_descriptions,
      .vertexAttributeDescriptionCount = 3,
      .pVertexAttributeDescriptions = vertex_attribute_descriptions,
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0.0f,
      .depthBiasClamp = 0.0f,
      .depthBiasSlopeFactor = 0.0f,
      .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisample_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE,
    };

    const VkPipelineColorBlendAttachmentState color_blend_attachment = {
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineColorBlendStateCreateInfo color_blend_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment,
      .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
    };

    const VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
    };

    const VkPipelineDynamicStateCreateInfo dynamic_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states,
    };

    const VkPipelineViewportStateCreateInfo viewport_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr,
    };

    const VkGraphicsPipelineCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stageCount = 2,
      .pStages = shader_stage_infos,
      .pVertexInputState = &vertex_input_state,
      .pInputAssemblyState = &input_assembly_state,
      .pTessellationState = nullptr,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState = &multisample_state,
      .pDepthStencilState = nullptr,
      .pColorBlendState = &color_blend_state,
      .pDynamicState = &dynamic_state,
      .layout = pipeline_layout_,
      .renderPass = engine.get_renderpass(),
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
    };

    VkResult result = vkCreateGraphicsPipelines(engine.get_device(), pipeline_cache_, 1,
                                                &create_info, nullptr, &pipeline_);
  }

  void Renderer::create_uniform_buffer()
  {
    auto& engine = core::Engine::get_singleton();
    auto buffer_size = 48 * sizeof(float);

    const VkBufferCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
    };

    VkResult result = vkCreateBuffer(engine.get_device(), &create_info, nullptr, &uniform_buffer_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create buffer");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(engine.get_device(), uniform_buffer_, &memory_requirements);

    auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const VkMemoryAllocateInfo allocate_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = engine.find_memory_type(memory_requirements.memoryTypeBits, flags),
    };

    result =
        vkAllocateMemory(engine.get_device(), &allocate_info, nullptr, &uniform_buffer_memory_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate device memory");

    result = vkBindBufferMemory(engine.get_device(), uniform_buffer_, uniform_buffer_memory_, 0);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to bind buffer memory");

    result = vkMapMemory(engine.get_device(), uniform_buffer_memory_, 0, buffer_size, 0,
                         &uniform_buffer_data_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to map buffer memory");
  }
} // namespace render
