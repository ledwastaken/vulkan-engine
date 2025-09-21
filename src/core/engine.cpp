#include "engine.h"

#include <cstring>
#include <fstream>
#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace core
{
  void Engine::init(int argc, char* argv[])
  {
    create_window();
    create_instance();
  }

  void Engine::loop()
  {
    bool running = true;
    while (running)
    {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
        if (event.type == SDL_EVENT_QUIT)
          running = false;
      }
    }
  }

  void Engine::quit()
  {
    vkDestroyInstance(instance_, nullptr);

    SDL_DestroyWindow(window_);
    SDL_Quit();
  }

  void Engine::create_window()
  {
    if (!SDL_Init(SDL_INIT_VIDEO))
      throw std::runtime_error(SDL_GetError());

    auto flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_UTILITY;
    window_ = SDL_CreateWindow("vulkan-engine", 800, 600, flags);

    if (!window_)
      throw std::runtime_error(SDL_GetError());
  }

  void Engine::create_instance()
  {
    // Query Vulkan extensions required by SDL
    uint32_t extensionCount = 0;
    auto extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    const VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "vulkan-engine",
      .applicationVersion = 1,
      .pEngineName = "vulkan-engine",
      .engineVersion = 1,
      .apiVersion = VK_MAKE_VERSION(1, 0, 0),
    };

    const VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = extensionCount,
      .ppEnabledExtensionNames = extensionNames,
    };

    // Create the instance.
    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
      throw std::runtime_error("failed to create vulkan instance");
  }
} // namespace core
