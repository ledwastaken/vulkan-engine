#include "gfx/pbr-pipeline.h"

#include "core/engine.h"

namespace gfx
{
  void PhysicallyBasedRenderPipeline::init()
  {
    create_pipeline_layout();
    create_descriptor_set();
    create_pipeline_cache();

    create_shader_module("shaders/pbr/pbr.vert.spv", &vertex_shader_);
    create_shader_module("shaders/pbr/pbr.frag.spv", &fragment_shader_);

    create_graphics_pipeline();
  }

  void PhysicallyBasedRenderPipeline::draw(VkImageView image_view, VkCommandBuffer command_buffer,
                                           const types::Matrix4& view,
                                           const types::Matrix4& projection, VkBuffer index_buffer)
  {
    // FIXME
  }

  void PhysicallyBasedRenderPipeline::free()
  {
    auto& engine = core::Engine::get_singleton();

    vkDestroyPipeline(engine.get_device(), pipeline_, nullptr);

    vkDestroyShaderModule(engine.get_device(), fragment_shader_, nullptr);
    vkDestroyShaderModule(engine.get_device(), vertex_shader_, nullptr);

    vkDestroyPipelineCache(engine.get_device(), pipeline_cache_, nullptr);
    vkFreeDescriptorSets(engine.get_device(), descriptor_pool_, 1, descriptor_sets_.data());
    vkDestroyDescriptorPool(engine.get_device(), descriptor_pool_, nullptr);
    vkDestroyPipelineLayout(engine.get_device(), pipeline_layout_, nullptr);
    vkDestroyDescriptorSetLayout(engine.get_device(), descriptor_set_layout_, nullptr);
  }

  void PhysicallyBasedRenderPipeline::create_pipeline_layout()
  {
    auto& engine = core::Engine::get_singleton();

    const VkDescriptorSetLayoutBinding layout_binding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .pImmutableSamplers = nullptr,
    };

    const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = 1,
      .pBindings = &layout_binding,
    };

    VkResult result = vkCreateDescriptorSetLayout(
        engine.get_device(), &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor set layout");

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_set_layout_,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
    };

    result = vkCreatePipelineLayout(engine.get_device(), &pipeline_layout_create_info, nullptr,
                                    &pipeline_layout_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout");
  }

  void PhysicallyBasedRenderPipeline::create_descriptor_set()
  {
    auto& engine = core::Engine::get_singleton();

    const VkDescriptorPoolSize pool_size = {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = MAX_FRAMES_IN_FLIGHT,
    };

    const VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
      .poolSizeCount = 1,
      .pPoolSizes = &pool_size,
    };

    VkResult result = vkCreateDescriptorPool(engine.get_device(), &descriptor_pool_create_info,
                                             nullptr, &descriptor_pool_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create descriptor pool");

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout_);

    const VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptor_pool_,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = layouts.data(),
    };

    descriptor_sets_.resize(MAX_FRAMES_IN_FLIGHT);
    result = vkAllocateDescriptorSets(engine.get_device(), &descriptor_set_allocate_info,
                                      descriptor_sets_.data());
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor set");
  }

  void PhysicallyBasedRenderPipeline::create_pipeline_cache()
  {
    auto& engine = core::Engine::get_singleton();
    auto device = engine.get_device();

    const VkPipelineCacheCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .initialDataSize = 0,
      .pInitialData = nullptr,
    };

    VkResult result = vkCreatePipelineCache(device, &create_info, nullptr, &pipeline_cache_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline cache");
  }

  void PhysicallyBasedRenderPipeline::create_graphics_pipeline()
  {
    auto& engine = core::Engine::get_singleton();
    auto device = engine.get_device();

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

    const VkVertexInputBindingDescription vertex_binding_description = {
      .binding = 0,
      .stride = 32,
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
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
      },
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertex_binding_description,
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
      .cullMode = VK_CULL_MODE_BACK_BIT,
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

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
      .front = {},
      .back = {},
      .minDepthBounds = 0.0f,
      .maxDepthBounds = 1.0f,
    };

    const VkFormat color_attachments[] = { engine.get_surface_format().format };

    const VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .pNext = nullptr,
      .viewMask = 0,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = color_attachments,
      .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
      .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    const VkGraphicsPipelineCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipeline_rendering_create_info,
      .flags = 0,
      .stageCount = 2,
      .pStages = shader_stage_infos,
      .pVertexInputState = &vertex_input_state,
      .pInputAssemblyState = &input_assembly_state,
      .pTessellationState = nullptr,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState = &multisample_state,
      .pDepthStencilState = &depth_stencil_state,
      .pColorBlendState = &color_blend_state,
      .pDynamicState = &dynamic_state,
      .layout = pipeline_layout_,
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0,
    };

    VkResult result =
        vkCreateGraphicsPipelines(device, pipeline_cache_, 1, &create_info, nullptr, &pipeline_);
    if (result != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline");
  }
} // namespace gfx
