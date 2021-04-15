#pragma once
#include <cstdint>
#include <optional>
#include <vector>
//
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class application {
 public:
  application();
  ~application();

  void run();

 private:
  void init_window();
  void init_vulkan();
  void create_vulkan_instance();
  void check_vulkan_extensions();
  bool vulkan_validation_layers_supported();
  auto required_vulkan_extensions();
  void setup_debug_messenger_create_info(
      VkDebugUtilsMessengerCreateInfoEXT& create_info);
  void setup_debug_messenger();
  void create_surface();
  bool is_device_suitable(VkPhysicalDevice device);
  void pick_physical_device();

  struct queue_family_indices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() {
      return graphics_family.has_value() && present_family.has_value();
    }
  };

  queue_family_indices find_queue_families(VkPhysicalDevice device);
  void create_logical_device();
  bool device_extensions_supported(VkPhysicalDevice device);

  struct swapchain_support_details {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
  };

  swapchain_support_details query_swapchain_support(VkPhysicalDevice device);

 private:
  GLFWwindow* window;
  uint32_t width = 800;
  uint32_t height = 450;
  const char* title = "Vulkan Test";

  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphics_queue;
  VkSurfaceKHR surface;
  VkQueue present_queue;

  static constexpr const char* validation_layers[] = {
      "VK_LAYER_KHRONOS_validation"};
  static constexpr size_t validation_layers_count =
      sizeof(validation_layers) / sizeof(char*);
  static constexpr bool validation_layers_enabled =
#ifdef NDEBUG
      false;
#else
      true;
#endif

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                 void* pUserData);

  static constexpr const char* device_extensions[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  static constexpr size_t device_extensions_count =
      sizeof(device_extensions) / sizeof(const char*);
};