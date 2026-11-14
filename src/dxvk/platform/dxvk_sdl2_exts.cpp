#include "../dxvk_platform_exts.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

namespace dxvk {

  DxvkPlatformExts DxvkPlatformExts::s_instance;

  std::string_view DxvkPlatformExts::getName() {
    return "SDL2 WSI";
  }


  DxvkNameSet DxvkPlatformExts::getInstanceExtensions() {
#ifdef __ANDROID__
    // SDL on Android only supports a single window, which the game already owns (or will
    // create), so a hidden dummy window cannot be used to ask SDL for the extensions.
    // These two are what SDL_Vulkan_GetInstanceExtensions returns on Android.
    DxvkNameSet androidNames;
    androidNames.add("VK_KHR_surface");
    androidNames.add("VK_KHR_android_surface");
    return androidNames;
#else
    SDL_Window* window = SDL_CreateWindow(
      "Dummy Window",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      1, 1,
      SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN);

    if (window == nullptr)
      throw DxvkError(str::format("SDL2 WSI: Failed to create dummy window. ", SDL_GetError()));

    uint32_t extensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr))
      throw DxvkError(str::format("SDL2 WSI: Failed to get instance extension count. ", SDL_GetError()));

    auto extensionNames = std::vector<const char *>(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames.data()))
      throw DxvkError(str::format("SDL2 WSI: Failed to get instance extensions. ", SDL_GetError()));

    DxvkNameSet names;
    for (const char* name : extensionNames)
      names.add(name);

    SDL_DestroyWindow(window);

    return names;
#endif
  }


  DxvkNameSet DxvkPlatformExts::getDeviceExtensions(
          uint32_t      adapterId) {
    return DxvkNameSet();
  }
  

  void DxvkPlatformExts::initInstanceExtensions() {

  }


  void DxvkPlatformExts::initDeviceExtensions(
    const DxvkInstance* instance) {

  }

}