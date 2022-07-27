#include "Application.h"

#include <stdexcept>
#include <string>
#include <iostream>
#include <map>
#include <set>

Application::Application() :
	instance(VkInstance{}),
	window(nullptr),
	debugMode(false),
	debugMessenger(VkDebugUtilsMessengerEXT{}),
	physicalDevice(VK_NULL_HANDLE),
	device(VK_NULL_HANDLE),
	gQueue(VK_NULL_HANDLE),
	surface(VK_NULL_HANDLE),
	pQueue(VK_NULL_HANDLE)
{
	//Determine compile mode.
#ifndef NDEBUG
	debugMode = true;
#endif // NDEBUG

	Initialise();
}

Application::~Application()
{
	Destroy();
}

void Application::Initialise()
{
	CreateWindow();
	CreateInstance();
	CreateDebugCallback();
	CreateSurface();
	SelectPhysicalDevice();
	CreateDevice();
}

void Application::Destroy()
{
	DestroyDevice();
	DestroySurface();
	DestroyDebugCallback();
	DestroyInstance();
	DestroyWindow();
}

void Application::CreateWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(800, 600, "Vulkan Application", nullptr, nullptr);
}

void Application::DestroyWindow()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::CreateInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.applicationVersion = 0u;
	appInfo.pEngineName = "hpe";
	appInfo.engineVersion = 0u;

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	
	//Validation layers.
	std::vector<const char*> layers;
	if (debugMode)
	{
		layers = GetInstanceLayers();
		instanceInfo.enabledLayerCount = layers.size();
		instanceInfo.ppEnabledLayerNames = layers.data();
	}

	//Extensions.
	std::vector<const char*> extensions = GetInstanceExtensions();
	instanceInfo.enabledExtensionCount = extensions.size();
	instanceInfo.ppEnabledExtensionNames = extensions.data();

	VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create instance.\n");
	}
}

void Application::DestroyInstance()
{
	vkDestroyInstance(instance, nullptr);
}

std::vector<const char*> Application::GetInstanceLayers()
{
	std::vector<VkLayerProperties> supported = GetSupportedInstanceLayers();
	std::vector<const char*> requested = GetRequestedInstanceLayers();

	for (size_t i = 0; i < requested.size(); i++)
	{
		bool found = false;
		for (size_t j = 0; j < supported.size(); j++)
		{
			if (std::string(requested.at(i)) == std::string(supported.at(j).layerName))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			throw std::runtime_error("ERROR: Could not find instance layer named: " + std::string(requested.at(i)));
		}
	}

	return requested;
}

std::vector<VkLayerProperties> Application::GetSupportedInstanceLayers()
{
	uint32_t layerCount = 0u;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> layers(layerCount);

	vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
	return layers;
}

std::vector<const char*> Application::GetRequestedInstanceLayers()
{
	std::vector<const char*> requested = {
		"VK_LAYER_KHRONOS_validation"
	};
	return requested;
}

std::vector<const char*> Application::GetInstanceExtensions()
{
	std::vector<VkExtensionProperties> supported = GetSupportedInstanceExtensions();
	std::vector<const char*> requested = GetRequestedInstanceExtensions();

	for (size_t i = 0; i < requested.size(); i++)
	{
		bool found = false;
		for (size_t j = 0; j < supported.size(); j++)
		{
			if (std::string(requested.at(i)) == std::string(supported.at(j).extensionName))
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			throw std::runtime_error("ERROR: Could not find instance extension named: " + std::string(requested.at(i)));
		}
	}

	return requested;
}

std::vector<VkExtensionProperties> Application::GetSupportedInstanceExtensions()
{
	uint32_t extensionCount = 0u;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	return extensions;
}

std::vector<const char*> Application::GetRequestedInstanceExtensions()
{
	//GLFW requires some extensions.
	uint32_t glfwExtensionCount = 0u;
	const char** glfwExtensions = nullptr;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (debugMode)
	{
		extensions.push_back("VK_EXT_debug_utils");
	}
	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::string severity = "";
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	{
		severity = "VERBOSE: ";
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		severity = "INFO: ";
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		severity = "WARNING: ";
	}
	else
	{
		severity = "ERROR: ";
	}
	
	std::string type = "";
	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
	{
		type = "GENERAL: ";
	}
	else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
	{
		type = "VALIDATION: ";
	}
	else
	{
		type = "PERFORMANCE: ";
	}

	std::cerr << "VALIDATION LAYER: " << type << severity << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

VkResult Application::ProxyCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		throw std::runtime_error("ERROR: Could not find vkCreateDebugUtilsMessengerEXT.\n");
	}
}

void Application::ProxyDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
	else
	{
		std::cout << "WARNING: Could not find vkDestroyDebugUtilsMessengerEXT.\n";
	}
}

void Application::CreateSurface()
{
	if (glfwCreateWindowSurface(instance,window,nullptr,&surface) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create surface.\n");
	}
}

void Application::DestroySurface()
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

void Application::SelectPhysicalDevice()
{
	//For now, just select first Discrete GPU that found.

	std::vector<VkPhysicalDevice> devices = GetPhysicalDevices();

	for (auto& candicateDevice : devices)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(candicateDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(candicateDevice, &deviceFeatures);

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			physicalDevice = candicateDevice;
			std::cout << "INFO: " << deviceProperties.deviceName << " is selected.\n";
			break;
		}
	}
}

std::vector<VkPhysicalDevice> Application::GetPhysicalDevices()
{
	uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0u)
	{
		throw std::runtime_error("ERROR: Failed to find GPUs with Vulkan support.");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);

	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	return devices;
}

void Application::CreateDevice()
{
	uint32_t graphicsFamilyIndex = GetQueueFamilyIndex(physicalDevice, VK_QUEUE_GRAPHICS_BIT);
	uint32_t presentationFamilyIndex = GetQueueFamilyIndex(physicalDevice, VK_QUEUE_FLAG_BITS_MAX_ENUM);
	
	std::set<uint32_t> uniqueQueueFamilies = { graphicsFamilyIndex,presentationFamilyIndex };

	float queuePriority = 1.f;

	std::vector<VkDeviceQueueCreateInfo> queueInfos;
	for (auto& queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = queueFamily;
		info.queueCount = 1;
		info.pQueuePriorities = &queuePriority;
		
		queueInfos.push_back(info);
	}

	VkPhysicalDeviceFeatures features{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
	createInfo.pEnabledFeatures = &features;
	
	VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create device.");
	}

	vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &gQueue);
	vkGetDeviceQueue(device, presentationFamilyIndex, 0, &pQueue);
}

void Application::DestroyDevice()
{
	vkDestroyDevice(device, nullptr);
}

uint32_t Application::GetQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlagBits bit)
{
	std::vector<VkQueueFamilyProperties> families = GetQueueFamilies(device);

	//If bit is set to max value, then look for queue family with presentation support.
	if (bit & VK_QUEUE_FLAG_BITS_MAX_ENUM)
	{
		uint32_t index = 0u;
		for (auto& family : families)
		{
			VkBool32 presentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentationSupport);

			if (presentationSupport == true)
			{
				return index;
			}
			index++;
		}
	}
	else //Otherwise search for queue family specified with bit.
	{
		uint32_t index = 0u;
		for (auto& family : families)
		{
			if (family.queueFlags & bit)
			{
				return index;
			}
			index++;
		}
	}

	throw std::runtime_error("ERROR: Queue family could not found.\n");
}

std::vector<VkQueueFamilyProperties> Application::GetQueueFamilies(VkPhysicalDevice device)
{
	uint32_t queueFamilyCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	return queueFamilies;
}

void Application::CreateDebugCallback()
{
	if (!debugMode)
	{
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT messengerInfo{};
	messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messengerInfo.pfnUserCallback = &DebugCallback;
	messengerInfo.pUserData = nullptr;

	VkResult result = ProxyCreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &debugMessenger);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create debug messenger.\n");
	}
}

void Application::DestroyDebugCallback()
{
	if (!debugMode)
	{
		return;
	}

	ProxyDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
}

