#include "engine.h"

#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace core
{
  Engine& Engine::singleton()
  {
    static Engine engine;

    return engine;
  }

  void Engine::init()
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw std::runtime_error(SDL_GetError());

    m_window = SDL_CreateWindow("Vulkan", 800, 600, SDL_WINDOW_VULKAN);
    if (!m_window)
      throw std::runtime_error(SDL_GetError());

    uint32_t extensionCount = 0;
    auto extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    VkResult result = VK_SUCCESS;
    VkApplicationInfo appInfo = {};
    VkInstanceCreateInfo instanceCreateInfo = {};

    // A generic application info structure
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Application";
    appInfo.applicationVersion = 1;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    // Create the instance.
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = extensionNames;

    result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);

    if (result != VK_SUCCESS)
      throw std::runtime_error(std::string("Failed to create vulkan instance"));

    // First figure out how many devices are in the system.
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

    if (physicalDeviceCount == 0)
      throw std::runtime_error("No physical device available");

    // Size the device array appropriately and get the physical device handles.
    m_physicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount,
                               &m_physicalDevices[0]);

    VkPhysicalDeviceFeatures supportedFeatures;
    VkPhysicalDeviceFeatures requiredFeatures = {};

    vkGetPhysicalDeviceFeatures(m_physicalDevices[0], &supportedFeatures);

    requiredFeatures.multiDrawIndirect = supportedFeatures.multiDrawIndirect;
    requiredFeatures.tessellationShader = VK_TRUE;
    requiredFeatures.geometryShader = VK_TRUE;

    // clang-format off
    const VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
      nullptr,                                    // pNext
      0,                                          // flags
      0,                                          // queueFamilyIndex
      1,                                          // queueCount
      nullptr                                     // pQueuePriorities
    };

    const char* const swapChainExtension[] = {
      "VK_KHR_swapchain",
    };

    const VkDeviceCreateInfo deviceCreateInfo = {
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // sType
      nullptr,                              // pNext
      0,                                    // flags
      1,                                    // queueCreateInfoCount
      &deviceQueueCreateInfo,               // pQueueCreateInfos
      0,                                    // enabledLayerCount
      nullptr,                              // ppEnabledLayerNames
      1,                                    // enabledExtensionCount
      swapChainExtension,                   // ppEnabledExtensionNames
      &requiredFeatures                     // pEnabledFeatures
    };
    // clang-format on

    result = vkCreateDevice(m_physicalDevices[0], &deviceCreateInfo, nullptr,
                            &m_logicalDevice);

    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create logical device");

    if (!SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface))
      throw std::runtime_error("Failed to create vulkan surface");

    // clang-format off
    const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, // sType
      nullptr,                                     // pNext
      0,                                           // flags
      m_surface,                                   // surface
      2,                                           // minImageCount
      VK_FORMAT_R8G8B8A8_UNORM,                    // imageFormat
      VK_COLORSPACE_SRGB_NONLINEAR_KHR,            // imageColorSpace
      { 1920, 1080 },                              // imageExtent
      1,                                           // imageArrayLayers
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,         // imageUsage
      VK_SHARING_MODE_EXCLUSIVE,                   // imageSharingMode
      0,                                           // queueFamilyIndexCount
      nullptr,                                     // pQueueFamilyIndices
      VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR,        // preTransform
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,           // compositeAlpha
      VK_PRESENT_MODE_FIFO_KHR,                    // presentMode
      VK_TRUE,                                     // clipped
      m_swapchain                                  // oldSwapchain
    };
    // clang-format on

    result = vkCreateSwapchainKHR(m_logicalDevice, &swapchainCreateInfo,
                                  nullptr, &m_swapchain);

    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create swapchain");

    // Next, we query the swap chain for the number of images it actually
    // contains.
    uint32_t swapChainImageCount = 0;
    result = vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain,
                                     &swapChainImageCount, nullptr);

    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to retrieve swapchain images count");

    // Now we resize our image array and retrieve the image handles from the
    // swap chain.
    m_swapchainImages.resize(swapChainImageCount);
    result =
        vkGetSwapchainImagesKHR(m_logicalDevice, m_swapchain,
                                &swapChainImageCount, m_swapchainImages.data());

    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to retrieve swapchain images");

    m_swapchainImageViews.resize(swapChainImageCount);
    for (size_t i = 0; i < swapChainImageCount; i++)
    {
      VkImageViewCreateInfo viewInfo = {};
      viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      viewInfo.image = m_swapchainImages[i];
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

      viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      viewInfo.subresourceRange.baseMipLevel = 0;
      viewInfo.subresourceRange.levelCount = 1;
      viewInfo.subresourceRange.baseArrayLayer = 0;
      viewInfo.subresourceRange.layerCount = 1;

      result = vkCreateImageView(m_logicalDevice, &viewInfo, nullptr,
                                 &m_swapchainImageViews[i]);

      if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create image view");
    }

    // clang-format off

    // This is our color attachment. It's an R8G8B8A8_UNORM single sample image.
    // We want to clear it at the start of the renderpass and save the contents
    // when we're done. It starts in UNDEFINED layout, which is a key to
    // Vulkan that it's allowed to throw the old content away, and we want to
    // leave it in COLOR_ATTACHMENT_OPTIMAL state when we're done.
    static const VkAttachmentDescription attachments[] =
    {
      {
        0,                                       // flags
        VK_FORMAT_R8G8B8A8_UNORM,                // format
        VK_SAMPLE_COUNT_1_BIT,                   // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,             // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,            // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,         // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,        // stencilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,               // initialLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // finalLayout
      }
    };

    // This is the single reference to our single attachment.
    static const VkAttachmentReference attachmentReferences[] =
    {
      {
        0,                                       // attachment
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout
      }
    };

    // There is one subpass in this renderpass, with only a reference to the
    // single output attachment.
    static const VkSubpassDescription subpasses[] =
    {
      {
        0,                               // flags
        VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
        0,                               // inputAttachmentCount
        nullptr,                         // pInputAttachments
        1,                               // colorAttachmentCount
        &attachmentReferences[0],        // pColorAttachments
        nullptr,                         // pResolveAttachments
        nullptr,                         // pDepthStencilAttachment
        0,                               // preserveAttachmentCount
        nullptr                          // pPreserveAttachments
      }
    };

    // Finally, this is the information that Vulkan needs to create the
    // renderpass object.
    static VkRenderPassCreateInfo renderpassCreateInfo =
    {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, // sType
      nullptr,                                   // pNext
      0,                                         // flags
      1,                                         // attachmentCount
      attachments,                               // pAttachments
      1,                                         // subpassCount
      subpasses,                                 // pSubpasses
      0,                                         // dependencyCount
      nullptr                                    // pDependencies
    };
    // clang-format on

    result = vkCreateRenderPass(m_logicalDevice, &renderpassCreateInfo, nullptr,
                                &m_renderpass);

    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create renderpass");

    m_framebuffers.resize(swapChainImageCount);

    for (size_t i = 0; i < swapChainImageCount; i++)
    {
      VkImageView attachments[] = { m_swapchainImageViews[i] };

      VkFramebufferCreateInfo framebufferCreateInfo = {};
      framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferCreateInfo.pNext = nullptr;
      framebufferCreateInfo.flags = 0;
      framebufferCreateInfo.renderPass = m_renderpass;
      framebufferCreateInfo.attachmentCount = 1;
      framebufferCreateInfo.pAttachments = attachments;
      framebufferCreateInfo.width = 1920;
      framebufferCreateInfo.height = 1080;
      framebufferCreateInfo.layers = 1;

      result = vkCreateFramebuffer(m_logicalDevice, &framebufferCreateInfo,
                                   nullptr, &m_framebuffers[i]);

      if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create framebuffer");
    }

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = 0;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    result = vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr,
                                 &m_commandPool);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create command pool");

    m_commandBuffers.resize(swapChainImageCount);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = m_commandBuffers.size();

    result = vkAllocateCommandBuffers(m_logicalDevice, &allocInfo,
                                      m_commandBuffers.data());
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to allocate command buffer");

    VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };

    for (size_t i = 0; i < m_commandBuffers.size(); i++)
    {
      VkCommandBufferBeginInfo beginInfo = {};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = 0;
      beginInfo.pInheritanceInfo = nullptr;

      result = vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo);

      if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording command buffer");

      VkRenderPassBeginInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = m_renderpass;
      renderPassInfo.framebuffer = m_framebuffers[i];
      renderPassInfo.renderArea.offset = { 0, 0 };
      renderPassInfo.renderArea.extent = { 1920, 1080 };
      renderPassInfo.clearValueCount = 1;
      renderPassInfo.pClearValues = &clearColor;

      vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo,
                           VK_SUBPASS_CONTENTS_INLINE);

      vkCmdEndRenderPass(m_commandBuffers[i]);
      result = vkEndCommandBuffer(m_commandBuffers[i]);
      if (result != VK_SUCCESS)
        throw std::runtime_error("failed to record command buffer!");
    }

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    result = vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr,
                               &m_imageAvailableSemaphore);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create semaphore");

    result = vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr,
                               &m_renderFinishedSemaphore);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create semaphore");

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    result =
        vkCreateFence(m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFence);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create fence");

    struct Vertex
    {
      float pos[2];
      float color[3];
    };

    const std::vector<Vertex> vertices = {
      { { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } }, // bottom red
      { { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } }, // right green
      { { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } }, // left blue
    };

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Vertex) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result =
        vkCreateBuffer(m_logicalDevice, &bufferInfo, nullptr, &m_vertexBuffer);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to create vertex buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_logicalDevice, m_vertexBuffer,
                                  &memRequirements);

    VkMemoryAllocateInfo memoryAlloInfo = {};
    memoryAlloInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAlloInfo.allocationSize = memRequirements.size;
    memoryAlloInfo.memoryTypeIndex =
        chooseHeapFromFlags(memRequirements,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    result = vkAllocateMemory(m_logicalDevice, &memoryAlloInfo, nullptr,
                              &m_vertexBufferMemory);
    if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to allocate vertex buffer memory");

    vkBindBufferMemory(m_logicalDevice, m_vertexBuffer, m_vertexBufferMemory,
                       0);
  }

  void Engine::loop()
  {
    VkResult result;

    bool running = true;
    while (running)
    {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
        if (event.type == SDL_EVENT_QUIT)
          running = false;

        uint32_t imageIndex;
        vkWaitForFences(m_logicalDevice, 1, &m_inFlightFence, VK_TRUE,
                        UINT64_MAX);
        vkResetFences(m_logicalDevice, 1, &m_inFlightFence);

        vkAcquireNextImageKHR(m_logicalDevice, m_swapchain, UINT64_MAX,
                              m_imageAvailableSemaphore, VK_NULL_HANDLE,
                              &imageIndex);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Wait until the swapchain image is ready
        VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = {
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        // The command buffer to execute
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

        // Signal when rendering is finished
        VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkQueue queue;
        vkGetDeviceQueue(m_logicalDevice, 0, 0, &queue);

        result = vkQueueSubmit(queue, 1, &submitInfo, m_inFlightFence);
        if (result != VK_SUCCESS)
          throw std::runtime_error("Failed to submit draw command buffer");

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = { m_swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // optional

        vkQueuePresentKHR(queue, &presentInfo);
      }
    }
  }

  void Engine::quit()
  {
    vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);
    vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);

    vkDestroyFence(m_logicalDevice, m_inFlightFence, nullptr);

    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphore, nullptr);

    vkFreeCommandBuffers(m_logicalDevice, m_commandPool,
                         m_commandBuffers.size(), m_commandBuffers.data());

    vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);

    for (size_t i = 0; i < m_framebuffers.size(); i++)
      vkDestroyFramebuffer(m_logicalDevice, m_framebuffers[i], nullptr);

    for (size_t i = 0; i < m_swapchainImageViews.size(); i++)
      vkDestroyImageView(m_logicalDevice, m_swapchainImageViews[i], nullptr);

    vkDestroyRenderPass(m_logicalDevice, m_renderpass, nullptr);
    vkDestroySwapchainKHR(m_logicalDevice, m_swapchain, nullptr);

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    SDL_DestroyWindow(m_window);
    SDL_Quit();
  }

  uint32_t
  Engine::chooseHeapFromFlags(const VkMemoryRequirements& memoryRequirements,
                              VkMemoryPropertyFlags requiredFlags,
                              VkMemoryPropertyFlags preferredFlags) const
  {
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

    vkGetPhysicalDeviceMemoryProperties(m_physicalDevices[0],
                                        &deviceMemoryProperties);

    uint32_t selectedType = ~0u;
    uint32_t memoryType;

    for (memoryType = 0; memoryType < 32; ++memoryType)
    {
      if (memoryRequirements.memoryTypeBits & (1 << memoryType))
      {
        const VkMemoryType& type =
            deviceMemoryProperties.memoryTypes[memoryType];
        // If it exactly matches my preferred properties, grab it.
        if ((type.propertyFlags & preferredFlags) == preferredFlags)
          return memoryType;
      }
    }

    if (selectedType != ~0u)
    {
      for (memoryType = 0; memoryType < 32; ++memoryType)
      {
        if (memoryRequirements.memoryTypeBits & (1 << memoryType))
        {
          const VkMemoryType& type =
              deviceMemoryProperties.memoryTypes[memoryType];
          // If it has all my required properties, it'll do.
          if ((type.propertyFlags & requiredFlags) == requiredFlags)
            return selectedType;
        }
      }
    }

    return selectedType;
  }
} // namespace core
