#include "VulkanDeviceFactory.hpp"

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <GLFW/glfw3.h>

inline const std::vector<const char*> deviceExtensions = {vk::KHRSwapchainExtensionName, vk::KHRSpirv14ExtensionName,
                                                          vk::KHRSynchronization2ExtensionName,
                                                          vk::KHRCreateRenderpass2ExtensionName};

void VulkanDeviceFactory::createVulkanDevice(Window& window, VulkanDevice& vulkanDevice)
{
	createInstance(window, vulkanDevice);
	createSurface(window, vulkanDevice);
	pickPhysicalDevice(vulkanDevice);
	createLogicalDevice(vulkanDevice);
	createCommandPool(vulkanDevice);
}

void VulkanDeviceFactory::createInstance(Window& window, VulkanDevice& vulkanDevice)
{
	constexpr vk::ApplicationInfo appInfo("Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "No Engine",
	                                      VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_4);

	const std::vector<char const*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
#if defined(_DEBUG) || !defined(NDEBUG)
	constexpr bool enableValidationLayers = true;
#else
	constexpr bool enableValidationLayers = false;
#endif

	// Check if the required validation layers are supported by the Vulkan implementation.
	std::vector<char const*> requiredLayers;
	if (enableValidationLayers)
	{
		requiredLayers.assign(validationLayers.begin(), validationLayers.end());
	}

	auto layerProperties = vulkanDevice.context.enumerateInstanceLayerProperties();
	if (std::ranges::any_of(requiredLayers,
	                        [&layerProperties](auto const& requiredLayer)
	                        {
		                        return std::ranges::none_of(
		                            layerProperties, [requiredLayer](auto const& layerProperty)
		                            { return std::strcmp(layerProperty.layerName, requiredLayer) == 0; });
	                        }))
	{
		throw std::runtime_error("One or more required layers are not supported!");
	}

	auto extensions = window.getRequiredExtensions();
	uint32_t extensionCount = static_cast<uint32_t>(extensions.size());

	auto extensionProperties = vulkanDevice.context.enumerateInstanceExtensionProperties();
	for (uint32_t i = 0; i < extensionCount; ++i)
	{
		if (std::ranges::none_of(extensionProperties, [extension = extensions[i]](auto const& extensionProperty)
		                         { return std::strcmp(extensionProperty.extensionName, extension) == 0; }))
		{
			throw std::runtime_error("Required GLFW extension not supported: " + std::string(extensions[i]));
		}
	}

	vk::InstanceCreateInfo createInfo{};
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
	createInfo.ppEnabledLayerNames = requiredLayers.empty() ? nullptr : requiredLayers.data();
	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

	vulkanDevice.instance = vk::raii::Instance(vulkanDevice.context, createInfo);
}

void VulkanDeviceFactory::pickPhysicalDevice(VulkanDevice& vulkanDevice)
{
	auto devices = vulkanDevice.instance.enumeratePhysicalDevices();
	const auto devIter = std::ranges::find_if(
	    devices,
	    [&](auto const& device)
	    {
		    auto queueFamilies = device.getQueueFamilyProperties();
		    bool isSuitable = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

		    // Check for graphics queue support
		    const auto qfpIter =
		        std::ranges::find_if(queueFamilies, [](vk::QueueFamilyProperties const& qfp)
		                             { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags{}; });
		    isSuitable = isSuitable && (qfpIter != queueFamilies.end());

		    // Check for required device extensions
		    auto extensions = device.enumerateDeviceExtensionProperties();
		    bool found = true;
		    for (auto const& extension : deviceExtensions)
		    {
			    auto extensionIter = std::ranges::find_if(extensions, [extension](auto const& ext)
			                                              { return std::strcmp(ext.extensionName, extension) == 0; });
			    found = found && (extensionIter != extensions.end());
		    }
		    isSuitable = isSuitable && found;

		    // Check for sampler anisotropy support
		    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();
		    isSuitable = isSuitable && (supportedFeatures.samplerAnisotropy == VK_TRUE);

		    if (isSuitable)
		    {
			    vulkanDevice.physicalDevice = device;
		    }
		    return isSuitable;
	    });

	if (devIter == devices.end())
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void VulkanDeviceFactory::createLogicalDevice(VulkanDevice& vulkanDevice)
{
	auto queueFamilyProperties = vulkanDevice.physicalDevice.getQueueFamilyProperties();
	vulkanDevice.graphicsIndex = static_cast<uint32_t>(queueFamilyProperties.size());
	vulkanDevice.presentIndex = static_cast<uint32_t>(queueFamilyProperties.size());

	// First, try to find a queue family that supports both graphics and present
	for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
	{
		bool supportsGraphics = (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags{};
		bool supportsPresent =
		    vulkanDevice.physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *vulkanDevice.surface);

		if (supportsGraphics && supportsPresent)
		{
			vulkanDevice.graphicsIndex = static_cast<uint32_t>(i);
			vulkanDevice.presentIndex = static_cast<uint32_t>(i);
			break;
		}
	}

	// If no queue family supports both graphics and present, find separate ones
	if (vulkanDevice.graphicsIndex == queueFamilyProperties.size())
	{
		for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
		{
			if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags{})
			{
				vulkanDevice.graphicsIndex = static_cast<uint32_t>(i);
			}
			if (vulkanDevice.physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *vulkanDevice.surface))
			{
				vulkanDevice.presentIndex = static_cast<uint32_t>(i);
			}
		}
	}

	if (vulkanDevice.graphicsIndex == queueFamilyProperties.size() ||
	    vulkanDevice.presentIndex == queueFamilyProperties.size())
	{
		throw std::runtime_error("Could not find suitable queue family!");
	}

	float queuePriority = 0.5f;
	std::vector<float> queuePriorities;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

	// Primary graphics queue
	queuePriorities.push_back(queuePriority);
	vk::DeviceQueueCreateInfo queueInfo{};
	queueInfo.queueFamilyIndex = vulkanDevice.graphicsIndex;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &queuePriorities[0];
	queueCreateInfos.push_back(queueInfo);

	// If graphics and present queues are different, add another queue create info
	if (vulkanDevice.graphicsIndex != vulkanDevice.presentIndex)
	{
		queuePriorities.push_back(queuePriority); // second element
		vk::DeviceQueueCreateInfo presentQueueInfo{};
		presentQueueInfo.queueFamilyIndex = vulkanDevice.presentIndex;
		presentQueueInfo.queueCount = 1;
		presentQueueInfo.pQueuePriorities = &queuePriorities[1];
		queueCreateInfos.push_back(presentQueueInfo);
	}

	vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
	                   vk::PhysicalDeviceVulkan12Features,
	                   vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT, vk::PhysicalDeviceVulkan11Features>
	    featureChain{};
	featureChain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = true;
	featureChain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 = true;
	featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState = true;
	featureChain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters = true;
	featureChain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().descriptorIndexing = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().descriptorBindingPartiallyBound = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().runtimeDescriptorArray = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().descriptorBindingSampledImageUpdateAfterBind = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().shaderSampledImageArrayNonUniformIndexing = true;

	vk::DeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	vulkanDevice.device = vk::raii::Device(vulkanDevice.physicalDevice, deviceCreateInfo);

	vulkanDevice.graphicsQueue = vk::raii::Queue(vulkanDevice.device, vulkanDevice.graphicsIndex, 0);
	vulkanDevice.presentQueue = vk::raii::Queue(vulkanDevice.device, vulkanDevice.presentIndex, 0);
}

void VulkanDeviceFactory::createSurface(Window& window, VulkanDevice& vulkanDevice)
{
	auto surfaceHandle = window.createSurface(vulkanDevice.instance);
	VkSurfaceKHR rawSurface = static_cast<VkSurfaceKHR>(surfaceHandle);
	vulkanDevice.surface = vk::raii::SurfaceKHR(vulkanDevice.instance, rawSurface);
}

void VulkanDeviceFactory::createCommandPool(VulkanDevice& vulkanDevice)
{
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = vulkanDevice.graphicsIndex;
	vulkanDevice.commandPool = vk::raii::CommandPool(vulkanDevice.device, poolInfo);
}