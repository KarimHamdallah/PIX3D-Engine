#include "VulkanGraphicsContext.h"
#include "VulkanHelper.h"
#include <GLFW/glfw3.h>
#include <Engine/Engine.hpp>

namespace
{
	const char* GetDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT Severity)
	{
		switch (Severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return "Verbose";

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return "Info";

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return "Warning";

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			return "Error";

		default:
			PIX_ASSERT_MSG(false, "Invalid message severity!");
		}

		return "NO SUCH SEVERITY!";
	}

	const char* GetDebugType(VkDebugUtilsMessageTypeFlagsEXT Type)
	{
		switch (Type)
		{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			return "General";

		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			return "Validation";

		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			return "Performance";

#ifdef _WIN64 // doesn't work on my Linux for some reason
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
			return "Device address binding";
#endif
		default:
			PIX_ASSERT_MSG(false, "Invalid debug type!");
		}

		return "NO SUCH TYPE!";
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
		VkDebugUtilsMessageTypeFlagsEXT Type,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		/*
		PIX_DEBUG_INFO_FORMAT("Debug callback: {}", pCallbackData->pMessage);
		PIX_DEBUG_INFO_FORMAT("  Severity {}", GetDebugSeverityStr(Severity));
		PIX_DEBUG_INFO_FORMAT("  Type {}", GetDebugType(Type));
		PIX_DEBUG_INFO_FORMAT("  Objects ");

		for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
		{
#ifdef _WIN32
			PIX_DEBUG_INFO_FORMAT("{}llux ", pCallbackData->pObjects[i].objectHandle);
#else
			printf("%lux ", pCallbackData->pObjects[i].objectHandle);
#endif
		}
		*/

		if (Severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			PIX_DEBUG_INFO_FORMAT("Debug callback: {}", pCallbackData->pMessage);
		}
		if (Severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			PIX_DEBUG_ERROR_FORMAT("Debug callback: {}", pCallbackData->pMessage);
		}

		return VK_FALSE;  // The calling function should not be aborted
	}


	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& PresentModes)
	{
		for (int i = 0; i < PresentModes.size(); i++)
		{
			if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				return PresentModes[i];
		}

		// Fallback to FIFO which is always supported
		return VK_PRESENT_MODE_FIFO_KHR;
	}


	uint32_t ChooseNumImages(const VkSurfaceCapabilitiesKHR& Capabilities)
	{
		uint32_t RequestedNumImages = Capabilities.minImageCount + 1;

		int FinalNumImages = 0;

		if ((Capabilities.maxImageCount > 0) && (RequestedNumImages > Capabilities.maxImageCount))
			FinalNumImages = Capabilities.maxImageCount;
		else
			FinalNumImages = RequestedNumImages;

		return FinalNumImages;
	}

	VkSurfaceFormatKHR ChooseSurfaceFormatAndColorSpace(const std::vector<VkSurfaceFormatKHR>& SurfaceFormats)
	{
		for (int i = 0; i < SurfaceFormats.size(); i++)
		{
			if ((SurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB) &&
				(SurfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
			{
				return SurfaceFormats[i];
			}
		}

		return SurfaceFormats[0];
	}

	VkFormat GetSupportedDepthFormat(VkPhysicalDevice physDevice)
	{
		// Ordered from optimal to acceptable depth formats
		std::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM,
			VK_FORMAT_D16_UNORM_S8_UINT
		};

		for (VkFormat format : depthFormats)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(physDevice, format, &formatProps);

			// Check if format supports depth stencil attachment
			if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				return format;
			}
		}

		PIX_ASSERT_MSG(false, "Failed to find supported depth format!");
		return VK_FORMAT_UNDEFINED;
	}

	// Helper function to check if a format has a stencil component
	bool HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
			format == VK_FORMAT_D24_UNORM_S8_UINT ||
			format == VK_FORMAT_D16_UNORM_S8_UINT;
	}
}

namespace PIX3D
{
	namespace VK
	{

		VulkanGraphicsContext::VulkanGraphicsContext()
		{
		}

		VulkanGraphicsContext::~VulkanGraphicsContext()
		{
		}

		void VulkanGraphicsContext::Init(void* window_handle)
		{
			// set native window handle
			m_NativeWindowHandle = window_handle;


			////////////////////////////////////////
			//////////// Vulkan Instance ///////////
			////////////////////////////////////////


			// layers are injected dlls to your program to debug your vulkan code
			std::vector<const char*> Layers =
			{
			  "VK_LAYER_KHRONOS_validation"
			};

			// extentions are platform specific vulkan functionalities like plugins libraries
			std::vector<const char*> Extensions =
			{
				VK_KHR_SURFACE_EXTENSION_NAME,

				#if defined (_WIN32)
				"VK_KHR_win32_surface",
								#endif
				#if defined (__APPLE__)
				"VK_MVK_macos_surface",
								#endif
				#if defined (__linux__)
				"VK_KHR_xcb_surface",
				#endif
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME
			};

			// debug messenger for Vulkan validation layers
			VkDebugUtilsMessengerCreateInfoEXT InstanceCreationMessengerCreateInfo = {};
			InstanceCreationMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			InstanceCreationMessengerCreateInfo.pNext = NULL;
			InstanceCreationMessengerCreateInfo.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			InstanceCreationMessengerCreateInfo.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			InstanceCreationMessengerCreateInfo.pfnUserCallback = &DebugCallback;
			InstanceCreationMessengerCreateInfo.pUserData = NULL;

			// Provides basic information about your application to the Vulkan driver
			VkApplicationInfo AppInfo = {};
			AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			AppInfo.pNext = NULL;
			AppInfo.pApplicationName = "PiX 3D ENGINE APPLICATION";
			AppInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
			AppInfo.pEngineName = "PiX 3D ENGINE";
			AppInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
			AppInfo.apiVersion = VK_API_VERSION_1_0;

			// setup vulkan instance (vulkan loader)
			VkInstanceCreateInfo CreateInfo = {};
			CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			CreateInfo.pNext = &InstanceCreationMessengerCreateInfo;
			CreateInfo.flags = 0;  // reserved for future use. Must be zero
			CreateInfo.pApplicationInfo = &AppInfo;
			CreateInfo.enabledLayerCount = (uint32_t)(Layers.size());
			CreateInfo.ppEnabledLayerNames = Layers.data();
			CreateInfo.enabledExtensionCount = (uint32_t)(Extensions.size());
			CreateInfo.ppEnabledExtensionNames = Extensions.data();

			VkResult res = vkCreateInstance(&CreateInfo, NULL, &m_Instance);
			VK_CHECK_RESULT(res);
			PIX_DEBUG_SUCCESS("Vulkan instance created");


			/////////////////////////////////////////////////////
			//////////// Vulkan Debug Utils Messenger ///////////
			/////////////////////////////////////////////////////

			// Create debug utils messenger
			VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = {};
			MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			MessengerCreateInfo.pNext = NULL;
			MessengerCreateInfo.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			MessengerCreateInfo.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			MessengerCreateInfo.pfnUserCallback = &DebugCallback;
			MessengerCreateInfo.pUserData = NULL;

			// Get function pointer for debug messenger creation
			PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger =
				(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");

			PIX_ASSERT_MSG(vkCreateDebugUtilsMessenger, "Failed to get address of vkCreateDebugUtilsMessengerEXT");

			// Create the debug messenger
			res = vkCreateDebugUtilsMessenger(
				m_Instance,
				&MessengerCreateInfo,
				NULL,
				&m_DebugMessenger);

			VK_CHECK_RESULT(res);
			PIX_DEBUG_SUCCESS("Debug utils messenger created");

			/////////////////////////////////////////
			//////////// Vulkan Surface /////////////
			/////////////////////////////////////////

			// you have define surface of our window to vulkan to draw on it
			res = glfwCreateWindowSurface(m_Instance, (GLFWwindow*)m_NativeWindowHandle, NULL, &m_Surface);
			VK_CHECK_RESULT(res, "glfwCreateWindowSurface");
			PIX_DEBUG_SUCCESS("GLFW window surface created");


			//////////////////////////////////////////////////////
            //////////// Physical Device Selection ///////////////
            //////////////////////////////////////////////////////

			m_PhysDevice.Init(m_Instance, m_Surface);
			m_QueueFamily = m_PhysDevice.SelectDevice(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, true);

			//////////////////////////////////////////////////////
	        //////////// Logical Device Creation /////////////////
	        //////////////////////////////////////////////////////

	        // Queue creation info
			float QueuePriorities[] = { 1.0f };
			VkDeviceQueueCreateInfo QueueInfo = {};
			QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueInfo.pNext = NULL;
			QueueInfo.flags = 0;  // must be zero
			QueueInfo.queueFamilyIndex = m_QueueFamily;
			QueueInfo.queueCount = 1;
			QueueInfo.pQueuePriorities = &QueuePriorities[0];

			// Device extensions
			std::vector<const char*> DeviceExtensions =
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
			};

			// Check for required features
			if (m_PhysDevice.GetSelected().m_features.multiDrawIndirect == VK_FALSE)
				PIX_ASSERT_MSG(false, "Multi Draw Indirect is not supported!");
			if (m_PhysDevice.GetSelected().m_features.geometryShader == VK_FALSE)
				PIX_ASSERT_MSG(false, "The Geometry Shader is not supported!");
			if (m_PhysDevice.GetSelected().m_features.tessellationShader == VK_FALSE)
				PIX_ASSERT_MSG(false, "The Tessellation Shader is not supported!");

			// Enable device features
			VkPhysicalDeviceFeatures DeviceFeatures = {};
			DeviceFeatures.geometryShader = VK_TRUE;
			DeviceFeatures.tessellationShader = VK_TRUE;
			DeviceFeatures.samplerAnisotropy = VK_TRUE;

			// Logical device creation
			VkDeviceCreateInfo DeviceCreateInfo = {};
			DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			DeviceCreateInfo.pNext = NULL;
			DeviceCreateInfo.flags = 0;
			DeviceCreateInfo.queueCreateInfoCount = 1;
			DeviceCreateInfo.pQueueCreateInfos = &QueueInfo;
			DeviceCreateInfo.enabledLayerCount = 0;          // DEPRECATED
			DeviceCreateInfo.ppEnabledLayerNames = NULL;     // DEPRECATED
			DeviceCreateInfo.enabledExtensionCount = (uint32_t)DeviceExtensions.size();
			DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
			DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

			res = vkCreateDevice(m_PhysDevice.GetSelected().m_physDevice, &DeviceCreateInfo, NULL, &m_Device);
			VK_CHECK_RESULT(res);
			PIX_DEBUG_SUCCESS("Logical device created");


			//////////////////////////////////////////////////////
            ////////////////// Swap Chain ////////////////////////
            //////////////////////////////////////////////////////

			const VkSurfaceCapabilitiesKHR& SurfaceCaps = m_PhysDevice.GetSelected().m_surfaceCaps;

			uint32_t NumImages = ChooseNumImages(SurfaceCaps);

			const std::vector<VkPresentModeKHR>& PresentModes = m_PhysDevice.GetSelected().m_presentModes;
			VkPresentModeKHR PresentMode = ChoosePresentMode(PresentModes);

			m_SwapChainSurfaceFormat = ChooseSurfaceFormatAndColorSpace(m_PhysDevice.GetSelected().m_surfaceFormats);

			VkSwapchainCreateInfoKHR SwapChainCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.pNext = NULL,
				.flags = 0,
				.surface = m_Surface,
				.minImageCount = NumImages,
				.imageFormat = m_SwapChainSurfaceFormat.format,
				.imageColorSpace = m_SwapChainSurfaceFormat.colorSpace,
				.imageExtent = SurfaceCaps.currentExtent,
				.imageArrayLayers = 1,
				.imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT),
				.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = &m_QueueFamily,
				.preTransform = SurfaceCaps.currentTransform,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = PresentMode,
				.clipped = VK_TRUE
			};

			res = vkCreateSwapchainKHR(m_Device, &SwapChainCreateInfo, NULL, &m_SwapChain);
			VK_CHECK_RESULT(res, "vkCreateSwapchainKHR");

			PIX_DEBUG_INFO("Swap chain created");

			uint32_t NumSwapChainImages = 0;
			res = vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &NumSwapChainImages, NULL);
			VK_CHECK_RESULT(res, "vkGetSwapchainImagesKHR");
			PIX_ASSERT(NumImages <= NumSwapChainImages);

			PIX_DEBUG_INFO_FORMAT("Requested {0} images, created {1} images", NumImages, NumSwapChainImages);

			m_SwapChainImages.resize(NumSwapChainImages);
			m_SwapChainImageViews.resize(NumSwapChainImages);

			res = vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &NumSwapChainImages, m_SwapChainImages.data());
			VK_CHECK_RESULT(res, "vkGetSwapchainImagesKHR");

			int LayerCount = 1;
			int MipLevels = 1;

			for (uint32_t i = 0; i < NumSwapChainImages; i++)
			{
				m_SwapChainImageViews[i] = VulkanHelper::CreateImageView(m_Device, m_SwapChainImages[i], m_SwapChainSurfaceFormat.format,
					VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, LayerCount, MipLevels);
			}

			PIX_DEBUG_INFO("Swap chain images and views created");

			//////////////////////////////////////////////////////
			////////////////// Supported Depth Format ////////////
			//////////////////////////////////////////////////////


			m_SupportedDepthFormat = GetSupportedDepthFormat(m_PhysDevice.GetSelected().m_physDevice);
			//PIX_DEBUG_INFO_FORMAT("Selected depth format: {}", m_SupportedDepthFormat);

			//////////////////////////////////////////////////////
			////////////////// Depth Texture /////////////////////
			//////////////////////////////////////////////////////
			

			auto GetTextureFormatFromVulkanFormat = [](VkFormat format) -> TextureFormat {
				switch (format)
				{
				case VK_FORMAT_R8_UNORM: return TextureFormat::R8;
				case VK_FORMAT_R8G8_UNORM: return TextureFormat::RG8;
				case VK_FORMAT_R8G8B8_UNORM: return TextureFormat::RGB8;
				case VK_FORMAT_R8G8B8A8_UNORM: return TextureFormat::RGBA8;
				case VK_FORMAT_R16G16B16_SFLOAT: return TextureFormat::RGB16F;
				case VK_FORMAT_R16G16B16A16_SFLOAT: return TextureFormat::RGBA16F;
				case VK_FORMAT_R32G32B32A32_SFLOAT: return TextureFormat::RGBA32F;

					// Depth formats
				case VK_FORMAT_D16_UNORM: return TextureFormat::DEPTH16;
				case VK_FORMAT_X8_D24_UNORM_PACK32: return TextureFormat::DEPTH24;
				case VK_FORMAT_D32_SFLOAT: return TextureFormat::DEPTH32;
				case VK_FORMAT_D24_UNORM_S8_UINT: return TextureFormat::DEPTH24_STENCIL8;
				case VK_FORMAT_D32_SFLOAT_S8_UINT: return TextureFormat::DEPTH32_STENCIL8;

				default:
					PIX_ASSERT_MSG(false, "Unsupported Vulkan format!");
				}
			};

			auto specs = Engine::GetApplicationSpecs();

			m_DepthAttachmentTexture = new VK::VulkanTexture();
			m_DepthAttachmentTexture->Create();
			m_DepthAttachmentTexture->CreateColorAttachment(specs.Width, specs.Height, GetTextureFormatFromVulkanFormat(m_SupportedDepthFormat));

			//////////////////////////////////////////////////////
			////////////////// Command Pool //////////////////////
			//////////////////////////////////////////////////////

			VkCommandPoolCreateInfo cmdPoolCreateInfo = 
			{
		      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		      .pNext = NULL,
		      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		      .queueFamilyIndex = m_QueueFamily
			};

			res = vkCreateCommandPool(m_Device, &cmdPoolCreateInfo, NULL, &m_CommandPool);
			VK_CHECK_RESULT(res, "vkCreateCommandPool");

			PIX_DEBUG_INFO("Command buffer pool created");


			//////////////////////////////////////////////////////
            ////////////////// Queue Init ////////////////////////
            //////////////////////////////////////////////////////

			m_Queue.Init(m_Device, m_SwapChain, m_QueueFamily, 0);


			//////////////////////////////////////////////////////
            ////////////////// Command Buffer ////////////////////
            //////////////////////////////////////////////////////

			VulkanHelper::CreateCommandBuffers(m_Device, m_CommandPool, 1, &m_CopyCommandBuffer);


			//////////////////////////////////////////////////////
            ////////////////// Descriptor Pool ///////////////////
            //////////////////////////////////////////////////////

			// Descriptor pool sizes
			VkDescriptorPoolSize poolSizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_UNIFORM_BUFFERS },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_COMBINED_IMAGE_SAMPLERS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_STORAGE_BUFFERS },
			};

			// Descriptor pool creation info
			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]); // Number of pool sizes
			poolInfo.pPoolSizes = poolSizes;                                  // Pool sizes array
			poolInfo.maxSets = MAX_DESCRIPTOR_SETS;                           // Max descriptor sets

			// Create the descriptor pool
			if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
				PIX_ASSERT_MSG(false, "Failed to create descriptor pool!");
		}

		void VulkanGraphicsContext::SwapBuffers(void* window_handle)
		{
		}

		void VulkanGraphicsContext::Destroy()
		{
			// destroy swap chain

			for (int i = 0; i < m_SwapChainImageViews.size(); i++)
			{
				vkDestroyImageView(m_Device, m_SwapChainImageViews[i], NULL);
			}

			vkDestroySwapchainKHR(m_Device, m_SwapChain, NULL);

			// destroy device

			vkDestroyDevice(m_Device, NULL);

			// destroy surface

			PFN_vkDestroySurfaceKHR vkDestroySurface = VK_NULL_HANDLE;
			vkDestroySurface = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(m_Instance, "vkDestroySurfaceKHR");

			PIX_ASSERT_MSG(vkDestroySurface, "Cannot find address of vkDestroySurfaceKHR\n");

			vkDestroySurface(m_Instance, m_Surface, NULL);

			PIX_DEBUG_SUCCESS("GLFW window surface destroyed");

			// destroy debug messenger

			PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = VK_NULL_HANDLE;
			vkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");

			PIX_ASSERT_MSG(vkDestroyDebugUtilsMessenger, "Cannot find address of vkDestroyDebugUtilsMessengerEXT\n");

			vkDestroyDebugUtilsMessenger(m_Instance, m_DebugMessenger, NULL);

			PIX_DEBUG_SUCCESS("Debug callback destroyed");

			// destroy instance

			vkDestroyInstance(m_Instance, NULL);
			PIX_DEBUG_SUCCESS("Vulkan instance destroyed");
		}












		/*
			  PHYSICAL DEVICE SELECTION
		*/



		void VulkanPhysicalDevice::Init(const VkInstance& Instance, const VkSurfaceKHR& Surface)
		{
			uint32_t NumDevices = 0;

			VkResult res = vkEnumeratePhysicalDevices(Instance, &NumDevices, NULL);
			VK_CHECK_RESULT(res, "vkEnumeratePhysicalDevices error");
			PIX_DEBUG_INFO_FORMAT("Num physical devices {}", NumDevices);
			m_Devices.resize(NumDevices);

			std::vector<VkPhysicalDevice> Devices;
			Devices.resize(NumDevices);

			res = vkEnumeratePhysicalDevices(Instance, &NumDevices, Devices.data());
			VK_CHECK_RESULT(res, "vkEnumeratePhysicalDevices error");

			for (uint32_t i = 0; i < NumDevices; i++)
			{
				VkPhysicalDevice PhysDev = Devices[i];
				m_Devices[i].m_physDevice = PhysDev;

				vkGetPhysicalDeviceProperties(PhysDev, &m_Devices[i].m_devProps);

				PIX_DEBUG_INFO_FORMAT("Device name: {}", m_Devices[i].m_devProps.deviceName);

				uint32_t apiVer = m_Devices[i].m_devProps.apiVersion;

				PIX_DEBUG_INFO_FORMAT("    API version: {0}.{1}.{2}.{3}",
					VK_API_VERSION_VARIANT(apiVer),
					VK_API_VERSION_MAJOR(apiVer),
					VK_API_VERSION_MINOR(apiVer),
					VK_API_VERSION_PATCH(apiVer));

				uint32_t NumQFamilies = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &NumQFamilies, NULL);
				PIX_DEBUG_INFO_FORMAT("    Num of family queues: {}", NumQFamilies);

				m_Devices[i].m_qFamilyProps.resize(NumQFamilies);
				m_Devices[i].m_qSupportsPresent.resize(NumQFamilies);

				vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &NumQFamilies, m_Devices[i].m_qFamilyProps.data());

				for (uint32_t q = 0; q < NumQFamilies; q++)
				{
					const VkQueueFamilyProperties& QFamilyProp = m_Devices[i].m_qFamilyProps[q];

					PIX_DEBUG_INFO_FORMAT("    Family {0} Num queues: {1} ", q, QFamilyProp.queueCount);
					VkQueueFlags Flags = QFamilyProp.queueFlags;

					/*
					VK_QUEUE_SPARSE_BINDING_BIT:
					specifies that queues in this queue family support
					sparse memory management operations
					*/
					PIX_DEBUG_INFO_FORMAT("    GFX {0}, Compute {1}, Transfer {2}, Sparse binding {3}",
						(Flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
						(Flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
						(Flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
						(Flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No");

					res = vkGetPhysicalDeviceSurfaceSupportKHR(PhysDev, q, Surface, &(m_Devices[i].m_qSupportsPresent[q]));
					VK_CHECK_RESULT(res, "vkGetPhysicalDeviceSurfaceSupportKHR error");
				}

				uint32_t NumFormats = 0;
				res = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDev, Surface, &NumFormats, NULL);
				VK_CHECK_RESULT(res, "vkGetPhysicalDeviceSurfaceFormatsKHR");
				PIX_ASSERT(NumFormats > 0);

				m_Devices[i].m_surfaceFormats.resize(NumFormats);

				res = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDev, Surface, &NumFormats, m_Devices[i].m_surfaceFormats.data());
				VK_CHECK_RESULT(res, "vkGetPhysicalDeviceSurfaceFormatsKHR");

				/*
				for (uint32_t j = 0; j < NumFormats; j++)
				{
					const VkSurfaceFormatKHR& SurfaceFormat = m_Devices[i].m_surfaceFormats[j];
					PIX_DEBUG_INFO_FORMAT("    Format {0} color space {1}", SurfaceFormat.format, SurfaceFormat.colorSpace);
				}
				*/

				res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysDev, Surface, &(m_Devices[i].m_surfaceCaps));
				VK_CHECK_RESULT(res, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

				PrintImageUsageFlags(m_Devices[i].m_surfaceCaps.supportedUsageFlags);

				uint32_t NumPresentModes = 0;

				res = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDev, Surface, &NumPresentModes, NULL);
				VK_CHECK_RESULT(res, "vkGetPhysicalDeviceSurfacePresentModesKHR");

				PIX_ASSERT(NumPresentModes != 0);

				m_Devices[i].m_presentModes.resize(NumPresentModes);

				res = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDev, Surface, &NumPresentModes, m_Devices[i].m_presentModes.data());
				VK_CHECK_RESULT(res, "vkGetPhysicalDeviceSurfacePresentModesKHR");

				PIX_DEBUG_INFO_FORMAT("Number of presentation modes {}", NumPresentModes);

				vkGetPhysicalDeviceMemoryProperties(PhysDev, &(m_Devices[i].m_memProps));

				PIX_DEBUG_INFO_FORMAT("Num memory types {}", m_Devices[i].m_memProps.memoryTypeCount);
				for (uint32_t j = 0; j < m_Devices[i].m_memProps.memoryTypeCount; j++)
				{
					PIX_DEBUG_INFO_FORMAT("{0}: flags {1} heap {2} ", j,
						m_Devices[i].m_memProps.memoryTypes[j].propertyFlags,
						m_Devices[i].m_memProps.memoryTypes[j].heapIndex);

					PrintMemoryProperty(m_Devices[i].m_memProps.memoryTypes[j].propertyFlags);

					printf("\n");
				}
				PIX_DEBUG_INFO_FORMAT("Num heap types {}", m_Devices[i].m_memProps.memoryHeapCount);

				vkGetPhysicalDeviceFeatures(m_Devices[i].m_physDevice, &m_Devices[i].m_features);
			}
		}

		uint32_t VulkanPhysicalDevice::SelectDevice(VkQueueFlags RequiredQueueType, bool SupportsPresent)
		{
			for (uint32_t i = 0; i < m_Devices.size(); i++)
			{
				const VkPhysicalDeviceProperties& devProps = m_Devices[i].m_devProps;

				// Prioritize discrete GPUs
				if (devProps.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
					continue; // Skip non-discrete GPUs

				for (uint32_t j = 0; j < m_Devices[i].m_qFamilyProps.size(); j++)
				{
					const VkQueueFamilyProperties& QFamilyProp = m_Devices[i].m_qFamilyProps[j];

					if ((QFamilyProp.queueFlags & RequiredQueueType) && ((bool)m_Devices[i].m_qSupportsPresent[j] == SupportsPresent))
					{
						m_devIndex = i;
						int QueueFamily = j;
						PIX_DEBUG_INFO_FORMAT("Using GFX device {0} and queue family {1}", m_devIndex, QueueFamily);
						return QueueFamily;
					}
				}
			}

			PIX_ASSERT_MSG(false, std::format("Required queue type {0} and supports present {1} not found", RequiredQueueType, SupportsPresent));
			return 0;
		}

		const PhysicalDeviceData& VulkanPhysicalDevice::GetSelected() const
		{
			PIX_ASSERT_MSG(m_devIndex >= 0, "A physical device has not been selected!");
			return m_Devices[m_devIndex];
		}


		/*
		
		Queue

		*/



		void VulkanQueue::Init(VkDevice Device, VkSwapchainKHR SwapChain, uint32_t QueueFamily, uint32_t QueueIndex)
		{
			m_Device = Device;
			m_SwapChain = SwapChain;

			vkGetDeviceQueue(Device, QueueFamily, QueueIndex, &m_Queue);

			printf("Queue acquired\n");

			CreateSemaphores();
		}


		void VulkanQueue::Destroy()
		{
			vkDestroySemaphore(m_Device, m_PresentCompleteSem, NULL);
			vkDestroySemaphore(m_Device, m_RenderCompleteSem, NULL);
		}


		void VulkanQueue::CreateSemaphores()
		{
			m_PresentCompleteSem = VulkanHelper::CreateVulkanSemaphore(m_Device);
			m_RenderCompleteSem = VulkanHelper::CreateVulkanSemaphore(m_Device);
		}


		void VulkanQueue::WaitIdle()
		{
			vkQueueWaitIdle(m_Queue);
		}


		uint32_t VulkanQueue::AcquireNextImage()
		{
			uint32_t ImageIndex = 0;
			VkResult res = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_PresentCompleteSem, NULL, &ImageIndex);
			VK_CHECK_RESULT(res, "vkAcquireNextImageKHR");
			return ImageIndex;
		}


		void VulkanQueue::SubmitSync(VkCommandBuffer CmbBuf)
		{
			VkSubmitInfo SubmitInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.pNext = NULL,
				.waitSemaphoreCount = 0,
				.pWaitSemaphores = VK_NULL_HANDLE,
				.pWaitDstStageMask = VK_NULL_HANDLE,
				.commandBufferCount = 1,
				.pCommandBuffers = &CmbBuf,
				.signalSemaphoreCount = 0,
				.pSignalSemaphores = VK_NULL_HANDLE
			};

			VkResult res = vkQueueSubmit(m_Queue, 1, &SubmitInfo, NULL);
			VK_CHECK_RESULT(res, "vkQueueSubmit");
		}


		void VulkanQueue::SubmitAsync(VkCommandBuffer CmbBuf)
		{
			VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			VkSubmitInfo SubmitInfo = {
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.pNext = NULL,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &m_PresentCompleteSem,
				.pWaitDstStageMask = &waitFlags,
				.commandBufferCount = 1,
				.pCommandBuffers = &CmbBuf,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &m_RenderCompleteSem
			};

			VkResult res = vkQueueSubmit(m_Queue, 1, &SubmitInfo, NULL);
			VK_CHECK_RESULT(res, "vkQueueSubmit");
		}


		void VulkanQueue::Present(uint32_t ImageIndex)
		{
			VkPresentInfoKHR PresentInfo = {
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.pNext = NULL,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &m_RenderCompleteSem,
				.swapchainCount = 1,
				.pSwapchains = &m_SwapChain,
				.pImageIndices = &ImageIndex
			};

			VkResult res = vkQueuePresentKHR(m_Queue, &PresentInfo);
			VK_CHECK_RESULT(res, "vkQueuePresentKHR");

			WaitIdle();	// TODO: looks like a workaround but we're getting error messages without it
		}
	}
}
