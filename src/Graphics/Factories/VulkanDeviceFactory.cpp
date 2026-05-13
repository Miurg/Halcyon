#include "VulkanDeviceFactory.hpp"

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <GLFW/glfw3.h>

#ifdef TRACY_ENABLE
#include <tracy/TracyVulkan.hpp>
#endif

inline std::vector<const char*> buildDeviceExtensions()
{
	std::vector<const char*> ext = {
	    vk::KHRSwapchainExtensionName,
	    vk::KHRSpirv14ExtensionName,
	    vk::KHRSynchronization2ExtensionName,
	    vk::KHRCreateRenderpass2ExtensionName,
	};
	return ext;
}
inline const std::vector<const char*> deviceExtensions = buildDeviceExtensions();

void VulkanDeviceFactory::createVulkanDevice(Window& window, VulkanDevice& vulkanDevice)
{
	createInstance(window, vulkanDevice);
	createSurface(window, vulkanDevice);
	pickPhysicalDevice(vulkanDevice);
	createLogicalDevice(vulkanDevice);
	createCommandPool(vulkanDevice);
	createTracyContext(vulkanDevice);
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

	// Check if a physical device meets the requirements for our application
	auto isDeviceSuitable = [&](vk::raii::PhysicalDevice const& device) -> bool
	{
		if (device.getProperties().apiVersion < VK_API_VERSION_1_3) return false;

		auto queueFamilies = device.getQueueFamilyProperties();
		bool hasGraphicsQueue =
		    std::ranges::any_of(queueFamilies, [](vk::QueueFamilyProperties const& qfp)
		                        { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlags{}; });
		if (!hasGraphicsQueue) return false;

		auto extensions = device.enumerateDeviceExtensionProperties();
		bool hasAllExtensions =
		    std::ranges::all_of(deviceExtensions,
		                        [&](char const* required)
		                        {
			                        return std::ranges::any_of(extensions, [required](vk::ExtensionProperties const& ext)
			                                                   { return std::strcmp(ext.extensionName, required) == 0; });
		                        });
		if (!hasAllExtensions) return false;

		return device.getFeatures().samplerAnisotropy == VK_TRUE;
	};

	// Determine the maximum supported MSAA sample count for the device
	auto setMsaaSamples = [&](vk::raii::PhysicalDevice const& device)
	{
		vk::PhysicalDeviceProperties props = device.getProperties();
		vk::SampleCountFlags counts =
		    props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;

		if (counts & vk::SampleCountFlagBits::e64)
			vulkanDevice.maxMsaaSamples = vk::SampleCountFlagBits::e64;
		else if (counts & vk::SampleCountFlagBits::e32)
			vulkanDevice.maxMsaaSamples = vk::SampleCountFlagBits::e32;
		else if (counts & vk::SampleCountFlagBits::e16)
			vulkanDevice.maxMsaaSamples = vk::SampleCountFlagBits::e16;
		else if (counts & vk::SampleCountFlagBits::e8)
			vulkanDevice.maxMsaaSamples = vk::SampleCountFlagBits::e8;
		else if (counts & vk::SampleCountFlagBits::e4)
			vulkanDevice.maxMsaaSamples = vk::SampleCountFlagBits::e4;
		else if (counts & vk::SampleCountFlagBits::e2)
			vulkanDevice.maxMsaaSamples = vk::SampleCountFlagBits::e2;
		else
			vulkanDevice.maxMsaaSamples = vk::SampleCountFlagBits::e1;
	};

	// First, try to find a discrete GPU that meets the requirements
	auto devIter = std::ranges::find_if(devices,
	                                    [&](vk::raii::PhysicalDevice const& device)
	                                    {
		                                    return isDeviceSuitable(device) && device.getProperties().deviceType ==
		                                                                           vk::PhysicalDeviceType::eDiscreteGpu;
	                                    });

	// If no discrete GPU is found, look for any suitable GPU
	if (devIter == devices.end())
	{
		devIter = std::ranges::find_if(devices,
		                               [&](vk::raii::PhysicalDevice const& device) { return isDeviceSuitable(device); });
	}

	if (devIter == devices.end()) throw std::runtime_error("failed to find a suitable GPU!");

	vulkanDevice.physicalDevice = *devIter;
	setMsaaSamples(*devIter);
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
	                   vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
	                   vk::PhysicalDeviceVulkan11Features>
	    featureChain{};
	featureChain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = true;
	featureChain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 = true;
	featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState = true;
	featureChain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters = true;
	featureChain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
	featureChain.get<vk::PhysicalDeviceFeatures2>().features.multiDrawIndirect = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().descriptorIndexing = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().descriptorBindingPartiallyBound = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().runtimeDescriptorArray = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().descriptorBindingSampledImageUpdateAfterBind = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().shaderSampledImageArrayNonUniformIndexing = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().drawIndirectCount = true;
	featureChain.get<vk::PhysicalDeviceVulkan12Features>().hostQueryReset = true;

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
void VulkanDeviceFactory::createTracyContext(VulkanDevice& vulkanDevice)
{
#ifdef TRACY_ENABLE
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = *vulkanDevice.commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = 1;

	auto cmdBuffers = (*vulkanDevice.device).allocateCommandBuffers(allocInfo);
	VkCommandBuffer rawCmd = cmdBuffers[0];

	// TracyVkContextCalibrated can spin forever in Calibrate() when
	// vkGetCalibratedTimestampsEXT always reports deviation > m_deviation (seen on some Windows/GPU drivers).
	vulkanDevice.tracyContext =
	    TracyVkContext(*vulkanDevice.physicalDevice, *vulkanDevice.device, *vulkanDevice.graphicsQueue, rawCmd);

	vulkanDevice.graphicsQueue.waitIdle();
	(*vulkanDevice.device).freeCommandBuffers(*vulkanDevice.commandPool, vk::CommandBuffer(rawCmd));
#else
	(void)vulkanDevice;
#endif
}