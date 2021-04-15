#include <application.hpp>
//
#include <cstring>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

using namespace std;

VkResult vkCreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

application::application() {
  init_window();
  init_vulkan();
}

application::~application() {
  // No custom allocator.
  vkDestroyDevice(device, nullptr);

  // The physical device will be destroyed automatically.

  if (validation_layers_enabled)
    vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);

  vkDestroySurfaceKHR(instance, surface, nullptr);

  // Destroy Vulkan instance. No custom allocator was used.
  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);
  glfwTerminate();
}

void application::init_window() {
  glfwInit();

  // Do not create an OpenGL context.
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // No resizing for now.
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  // Create window without specifying monitor.
  // Last parameter only relevant for OpenGL.
  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

void application::init_vulkan() {
  // check_vulkan_extensions();
  create_vulkan_instance();
  setup_debug_messenger();
  create_surface();
  pick_physical_device();
  create_logical_device();
}

auto application::required_vulkan_extensions() {
  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  vector<const char*> extensions(glfw_extensions,
                                 glfw_extensions + glfw_extension_count);

  if (validation_layers_enabled)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return extensions;
}

void application::create_vulkan_instance() {
  // Check support for required Vulkan validation layers.
  if (validation_layers_enabled && !vulkan_validation_layers_supported())
    throw runtime_error(
        "Requested Vulkan validation layers are not supported!");

  // Create application information structure.
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Vulkan Test";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  // Initialize information for creating the Vulkan instance.
  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  // Set required Vulkan extensions.
  auto extensions = required_vulkan_extensions();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  // Set validation layers if used.
  VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
  if (validation_layers_enabled) {
    create_info.enabledLayerCount = validation_layers_count;
    create_info.ppEnabledLayerNames = validation_layers;

    setup_debug_messenger_create_info(debug_create_info);
    create_info.pNext = &debug_create_info;
  } else {
    create_info.enabledLayerCount = 0;
    create_info.pNext = nullptr;
  }

  // Create Vulkan instance.
  // We do not use a custom allocator.
  VkResult result = vkCreateInstance(&create_info, nullptr, &instance);
  if (result != VK_SUCCESS)
    throw runtime_error("Failed to create Vulkan instance!");
}

void application::check_vulkan_extensions() {
  // Get available Vulkan extension count.
  uint32_t extension_count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

  // Get available Vulkan extensions.
  vector<VkExtensionProperties> extensions(extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                         extensions.data());

  // Print available Vulkan extensions.
  cout << "\nAvailable Vulkan Extensions:\n";
  for (const auto& extension : extensions) {
    cout << '\t' << extension.extensionName << '\n';
  }
}

bool application::vulkan_validation_layers_supported() {
  // Get layer count.
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  // Get layer information.
  vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  // Check if required validation layers are supported.
  for (size_t i = 0; i < validation_layers_count; ++i) {
    bool found = false;
    for (const auto& layer : available_layers) {
      if (strcmp(validation_layers[i], layer.layerName) == 0) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL application::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}

void application::setup_debug_messenger_create_info(
    VkDebugUtilsMessengerCreateInfoEXT& create_info) {
  create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debug_callback;
  create_info.pUserData = nullptr;  // Optional
}

void application::setup_debug_messenger() {
  if (!validation_layers_enabled) return;

  VkDebugUtilsMessengerCreateInfoEXT create_info{};
  setup_debug_messenger_create_info(create_info);

  if (vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr,
                                     &debug_messenger) != VK_SUCCESS)
    throw std::runtime_error("Failed to set up Vulkan debug messenger!");
}

void application::create_surface() {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS)
    throw runtime_error("Failed to create window surface!");
}

bool application::is_device_suitable(VkPhysicalDevice device) {
  queue_family_indices indices = find_queue_families(device);

  auto extensions_supported = device_extensions_supported(device);

  bool swapchain_adequate = false;
  if (extensions_supported) {
    auto swapchain_support = query_swapchain_support(device);
    swapchain_adequate = !swapchain_support.formats.empty() &&
                         !swapchain_support.present_modes.empty();
  }

  return indices.is_complete() && extensions_supported && swapchain_adequate;
}

bool application::device_extensions_supported(VkPhysicalDevice device) {
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                       nullptr);

  vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                       available_extensions.data());

  set<string> required_extensions(device_extensions,
                                  device_extensions + device_extensions_count);

  for (const auto& extension : available_extensions) {
    required_extensions.erase(extension.extensionName);
  }

  return required_extensions.empty();
}

void application::pick_physical_device() {
  // Get device count.
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

  if (device_count == 0)
    throw runtime_error("Failed to find GPUs with Vulkan support!");

  // Get device information.
  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  // Check device suitability and choose.
  for (const auto& device : devices) {
    if (is_device_suitable(device)) {
      physical_device = device;
      break;
    }
  }

  if (physical_device == VK_NULL_HANDLE)
    throw runtime_error("Failed to find a suitable GPU!");
}

auto application::find_queue_families(VkPhysicalDevice device)
    -> queue_family_indices {
  queue_family_indices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           nullptr);

  vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families.data());

  int i = 0;
  for (const auto& queue_family : queue_families) {
    // Graphics Support
    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphics_family = i;

    // Presentation Support
    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
    if (present_support) indices.present_family = i;

    i++;
  }

  return indices;
}

void application::create_logical_device() {
  queue_family_indices indices = find_queue_families(physical_device);

  VkPhysicalDeviceFeatures device_features{};

  vector<VkDeviceQueueCreateInfo> queue_create_infos{};
  set<uint32_t> unique_queue_families = {indices.graphics_family.value(),
                                         indices.present_family.value()};

  float queue_priority = 1.0f;
  for (auto queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount =
      static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.pEnabledFeatures = &device_features;

  // Enable device extensions.
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(device_extensions_count);
  create_info.ppEnabledExtensionNames = device_extensions;

  // Device-specific validation layers are deprecated and typically ignored.
  // However, we set them regardless.
  if (validation_layers_enabled) {
    create_info.enabledLayerCount =
        static_cast<uint32_t>(validation_layers_count);
    create_info.ppEnabledLayerNames = validation_layers;
  } else {
    create_info.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physical_device, &create_info, nullptr, &device) !=
      VK_SUCCESS)
    throw runtime_error("Vulkan Error: Failed to create logical device!");

  vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
  vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

auto application::query_swapchain_support(VkPhysicalDevice device)
    -> swapchain_support_details {
  swapchain_support_details details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);
  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

  if (format_count != 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                         details.formats.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                            &present_mode_count, nullptr);

  if (present_mode_count != 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &present_mode_count, details.present_modes.data());
  }

  return details;
}

void application::run() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
}