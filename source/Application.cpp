#include "Application.h"

#include <stdexcept>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>

Application::Application() :
	instance(VkInstance{}),
	window(nullptr),
	debugMode(false),
	debugMessenger(VkDebugUtilsMessengerEXT{}),
	physicalDevice(VK_NULL_HANDLE),
	device(VK_NULL_HANDLE),
	gQueue(VK_NULL_HANDLE),
	surface(VK_NULL_HANDLE),
	pQueue(VK_NULL_HANDLE),
	swapchain(VK_NULL_HANDLE),
	swapchainImages({}),
	swapchainImageFormat(),
	swapchainExtent(VkExtent2D()),
	swapchainImageViews({})
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
	CreateSwapchain();
	CreateImageViews();
}

void Application::Destroy()
{
	DestroyImageViews();
	DestroySwapchain();
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
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		instanceInfo.ppEnabledLayerNames = layers.data();
	}

	//Extensions.
	std::vector<const char*> extensions = GetInstanceExtensions();
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
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
	//Select first discrete GPU with adequate extensions and properties that found.
	std::vector<VkPhysicalDevice> devices = GetPhysicalDevices();

	for (auto& candicateDevice : devices)
	{
		//Get device features and properties.
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(candicateDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(candicateDevice, &deviceFeatures);

		std::cout << "INFO: Checking " << deviceProperties.deviceName << " for suitability.\n";
		
		if (!(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
		{
			continue;
		}

		//Device extensions.
		if (!QueryDeviceExtensions(candicateDevice, deviceProperties.deviceName))
		{
			continue;
		}

		//Swapchain properties. NOTE: Swapchain support already queried above.
		if (!QuerySwapchainProperties(candicateDevice))
		{
			continue;
		}

		physicalDevice = candicateDevice;
		std::cout << "INFO: " << deviceProperties.deviceName << " is selected.\n";
		break;
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("ERROR: There is no appropriate physical device found.\n");
	}
}

bool Application::QueryDeviceExtensions(VkPhysicalDevice device, std::string deviceName)
{
	bool suitable = true;

	//Device extensions.
	std::vector<VkExtensionProperties> supported = GetSupportedDeviceExtensions(device);
	std::vector<const char*> requested = GetRequestedDeviceExtensions();

	for (auto& r : requested)
	{
		bool found = false;
		for (auto& s : supported)
		{
			if (std::string(s.extensionName) == r)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			std::cout << "INFO: " << deviceName << " has not required device extension.\n";
			suitable = false;
		}
	}

	return suitable;
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

std::vector<VkExtensionProperties> Application::GetSupportedDeviceExtensions(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0u;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
	return extensions;
}

std::vector<const char*> Application::GetRequestedDeviceExtensions()
{
	std::vector<const char*> requested = {
		"VK_KHR_swapchain"
	};

	return requested;
}

bool Application::QuerySwapchainProperties(VkPhysicalDevice device)
{
	bool suitable = true;

	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
	
	uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0u)
	{
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());
	}

	uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (formatCount != 0u)
	{
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data());
	}

	if (formats.empty() || presentModes.empty())
	{
		suitable = false;
	}

	return suitable;
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

	std::vector<const char*> extensions = GetRequestedDeviceExtensions();

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
	createInfo.pEnabledFeatures = &features;
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledExtensionCount = 1u;

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

			if (presentationSupport == VK_TRUE)
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

void Application::CreateSwapchain()
{
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

	uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount != 0u)
	{
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
	}

	uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (formatCount != 0u)
	{
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
	}

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapchainSurfaceFormat(formats);
	VkPresentModeKHR presentMode = ChooseSwapchainPresentationMode(presentModes);
	VkExtent2D extent = ChooseSwapchainExtend(capabilities);

	uint32_t desiredImageCount = capabilities.minImageCount + 2;

	VkSwapchainCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.minImageCount = desiredImageCount;
	info.imageFormat = surfaceFormat.format;
	info.imageColorSpace = surfaceFormat.colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
	//We are assuming that pQueues and gQueues are same.
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = nullptr;

	info.preTransform = capabilities.currentTransform;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = presentMode;
	info.clipped = VK_TRUE;
	info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device,&info,nullptr,&swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to create swapchain.\n");
	}

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);

	swapchainImages.resize(imageCount);

	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
}

void Application::DestroySwapchain()
{
	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

VkSurfaceFormatKHR Application::ChooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	for (auto& format : formats)
	{
		if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats[0];
}

VkPresentModeKHR Application::ChooseSwapchainPresentationMode(const std::vector<VkPresentModeKHR>& presentModes)
{
	for (auto& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::ChooseSwapchainExtend(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
	{
		return capabilities.currentExtent;
	}
	else 
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void Application::CreateImageViews()
{
	swapchainImageViews.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainImageViews.size(); i++)
	{
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = swapchainImages[i];
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = swapchainImageFormat;
		info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(device, &info, nullptr, &swapchainImageViews[i]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("ERROR: Could not create ImageView.\n");
		}
	}
}

void Application::DestroyImageViews()
{
	for (size_t i = 0; i < swapchainImageViews.size(); i++)
	{
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
	}
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

