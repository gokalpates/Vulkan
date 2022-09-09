#include "Application.h"

#include <stdexcept>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>

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
	swapchainImageViews({}),
	renderPass(VK_NULL_HANDLE),
	pipelineLayout(VK_NULL_HANDLE),
	graphicsPipeline(VK_NULL_HANDLE),
	commandPool(VK_NULL_HANDLE),
	commandBuffer(VK_NULL_HANDLE),
	sImageAvailable(VK_NULL_HANDLE),
	sRenderingDone(VK_NULL_HANDLE),
	fInFlight(VK_NULL_HANDLE)
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
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffer();
	CreateSyncPrimitives();
}

void Application::Destroy()
{
	DestroySyncPrimitives();
	DestroyCommandPool();
	DestroyFramebuffers();
	DestroyGraphicsPipeline();
	DestroyRenderPass();
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

void Application::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(device,&renderPassCreateInfo,nullptr,&renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create render pass.\n");
	}
}

void Application::DestroyRenderPass()
{
	vkDestroyRenderPass(device, renderPass, nullptr);
}

void Application::CreateGraphicsPipeline()
{
	auto vertexShaderCode = ReadFile("shader/vert.spv");
	auto fragmentShaderCode = ReadFile("shader/frag.spv");

	VkShaderModule vShaderModule = CreateShaderModule(vertexShaderCode);
	VkShaderModule fShaderModule = CreateShaderModule(fragmentShaderCode);

	VkPipelineShaderStageCreateInfo vShaderStage{};
	vShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vShaderStage.module = vShaderModule;
	vShaderStage.pName = "main";

	VkPipelineShaderStageCreateInfo fShaderStage{};
	fShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fShaderStage.module = fShaderModule;
	fShaderStage.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[2] = { vShaderStage,fShaderStage };

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.lineWidth = 1.f;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create pipeline layout.\n");
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;

	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlending;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

	pipelineCreateInfo.layout = pipelineLayout;

	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;

	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

	result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create graphics pipeline.\n");
	}

	vkDestroyShaderModule(device, fShaderModule, nullptr);
	vkDestroyShaderModule(device, vShaderModule, nullptr);
}

void Application::DestroyGraphicsPipeline()
{
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

std::vector<char> Application::ReadFile(std::string filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("ERROR: Failed to open file.\n");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(std::ios::beg);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkShaderModule Application::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = code.size();
	info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule{};
	VkResult result = vkCreateShaderModule(device, &info, nullptr, &shaderModule);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create shader module.\n");
	}

	return shaderModule;
}

void Application::CreateFramebuffers()
{
	swapchainFramebuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); i++) {
		VkImageView attachments[] = {
			swapchainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device,&framebufferInfo,nullptr,&swapchainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("ERROR: Failed to create framebuffer.\n");
		}
	}
}

void Application::DestroyFramebuffers()
{
	for (auto& framebuffer : swapchainFramebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
}

void Application::CreateCommandPool()
{
	uint32_t graphicsIndex = GetQueueFamilyIndex(physicalDevice, VK_QUEUE_GRAPHICS_BIT);

	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = graphicsIndex;

	if (vkCreateCommandPool(device,&commandPoolCreateInfo,nullptr,&commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create command pool.\n");
	}
}

void Application::DestroyCommandPool()
{
	vkDestroyCommandPool(device, commandPool, nullptr);
}

void Application::CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Failed to allocate command buffer.\n");
	}
}

void Application::CreateSyncPrimitives()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(device,&semaphoreCreateInfo,nullptr,&sImageAvailable) != VK_SUCCESS ||
		vkCreateSemaphore(device,&semaphoreCreateInfo,nullptr,&sRenderingDone) != VK_SUCCESS ||
		vkCreateFence(device,&fenceCreateInfo,nullptr,&fInFlight) != VK_SUCCESS)
	{
		throw std::runtime_error("ERROR: Could not create sync objects.\n");
	}
}

void Application::DestroySyncPrimitives()
{
	vkDestroySemaphore(device, sImageAvailable, nullptr);
	vkDestroySemaphore(device, sRenderingDone, nullptr);
	vkDestroyFence(device, fInFlight, nullptr);
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

