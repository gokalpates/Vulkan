#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

class Application
{
public:
	Application();
	~Application();

	virtual void Run() = 0;
protected:
	VkInstance instance;
	GLFWwindow* window;
	bool debugMode;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue gQueue;
	VkSurfaceKHR surface;
private:
	void Initialise();
	void Destroy();

	void CreateWindow();
	void DestroyWindow();
	void CreateInstance();
	void DestroyInstance();
	std::vector<const char*> GetInstanceLayers();
	std::vector<VkLayerProperties> GetSupportedInstanceLayers();
	std::vector<const char*> GetRequestedInstanceLayers();
	std::vector<const char*> GetInstanceExtensions();
	std::vector<VkExtensionProperties> GetSupportedInstanceExtensions();
	std::vector<const char*> GetRequestedInstanceExtensions();
	void CreateDebugCallback();
	void DestroyDebugCallback();
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	static VkResult ProxyCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void ProxyDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void CreateSurface();
	void DestroySurface();
	void SelectPhysicalDevice();
	std::vector<VkPhysicalDevice> GetPhysicalDevices();
	void CreateDevice();
	void DestroyDevice();
	uint32_t GetQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlagBits bit);
	std::vector<VkQueueFamilyProperties> GetQueueFamilies(VkPhysicalDevice device);
};

