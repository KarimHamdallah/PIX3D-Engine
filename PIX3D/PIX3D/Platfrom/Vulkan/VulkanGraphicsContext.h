#pragma once
#include <Graphics/GraphicsContext.h>
#include <Core/Core.h>
#include <vector>
#include <vulkan/vulkan.h>
#include "VulkanTexture.h"

namespace
{
	static void PrintImageUsageFlags(const VkImageUsageFlags& flags)
	{
		if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
			PIX_DEBUG_INFO("Image usage transfer src is supported\n");
		}

		if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
			PIX_DEBUG_INFO("Image usage transfer dest is supported\n");
		}

		if (flags & VK_IMAGE_USAGE_SAMPLED_BIT) {
			PIX_DEBUG_INFO("Image usage sampled is supported\n");
		}

		if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
			PIX_DEBUG_INFO("Image usage color attachment is supported\n");
		}

		if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			PIX_DEBUG_INFO("Image usage depth stencil attachment is supported\n");
		}

		if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
			PIX_DEBUG_INFO("Image usage transient attachment is supported\n");
		}

		if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
			PIX_DEBUG_INFO("Image usage input attachment is supported\n");
		}
	}


	static void PrintMemoryProperty(VkMemoryPropertyFlags PropertyFlags)
	{
		if (PropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
			PIX_DEBUG_INFO("DEVICE LOCAL ");
		}

		if (PropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
			PIX_DEBUG_INFO("HOST VISIBLE ");
		}

		if (PropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
			PIX_DEBUG_INFO("HOST COHERENT ");
		}

		if (PropertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
			PIX_DEBUG_INFO("HOST CACHED ");
		}

		if (PropertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
			PIX_DEBUG_INFO("LAZILY ALLOCATED ");
		}

		if (PropertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) {
			PIX_DEBUG_INFO("PROTECTED ");
		}
	}

	// Helper function to convert VkResult to string
	inline const char* vkResultToString(VkResult result)
	{
		switch (result) {
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		default: return "UNKNOWN_ERROR";
		}
	}

	uint32_t GetBytesPerPixel(VkFormat Format)
	{
		switch (Format)
		{
			// 8-bit formats
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
			return 1;

			// 16-bit formats
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
			return 2;

			// 24-bit formats
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
			return 3;

			// 32-bit formats
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
			return 4;

			// 64-bit formats
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_SFLOAT:
			return 8;

			// 128-bit formats
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16;

			// Depth/stencil formats
		case VK_FORMAT_D16_UNORM:
			return 2;
		case VK_FORMAT_D32_SFLOAT:
			return 4;
		case VK_FORMAT_D16_UNORM_S8_UINT:
			return 3;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return 4;
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return 5;

			// Compressed formats would go here, but they require block-based calculations
			// BC1/DXT1: 8 bytes per 4x4 block (0.5 bytes per pixel)
			// BC2/DXT3: 16 bytes per 4x4 block (1 byte per pixel)
			// BC3/DXT5: 16 bytes per 4x4 block (1 byte per pixel)
			// etc.

		default:
			throw std::runtime_error("Unsupported Vulkan format!");
		}
	}

	inline bool IsDepthFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return true;

		default:
			return false;
		}
	}

#define VK_CHECK_RESULT(f) \
        do { \
            VkResult __res = (f); \
            if (__res != VK_SUCCESS) { \
                std::cout << rang::fg::red << rang::style::bold << "Fatal : VkResult is \"" \
                         << vkResultToString(__res) << "\" in " << __FILE__ << " at line " << __LINE__ \
                         << rang::style::reset << std::endl; \
                PIX_ASSERT(__res == VK_SUCCESS); \
            } \
        } while(0)
}



namespace PIX3D
{
	namespace VK
	{
		struct PhysicalDeviceData
		{
			VkPhysicalDevice m_physDevice;
			VkPhysicalDeviceProperties m_devProps;
			std::vector<VkQueueFamilyProperties> m_qFamilyProps;
			std::vector<VkBool32> m_qSupportsPresent;
			std::vector<VkSurfaceFormatKHR> m_surfaceFormats;
			VkSurfaceCapabilitiesKHR m_surfaceCaps;
			VkPhysicalDeviceMemoryProperties m_memProps;
			std::vector<VkPresentModeKHR> m_presentModes;
			VkPhysicalDeviceFeatures m_features;
		};


		class VulkanPhysicalDevice
		{
		public:
			VulkanPhysicalDevice() = default;
			~VulkanPhysicalDevice() = default;

			void Init(const VkInstance& Instance, const VkSurfaceKHR& Surface);

			uint32_t SelectDevice(VkQueueFlags RequiredQueueType, bool SupportsPresent);

			const PhysicalDeviceData& GetSelected() const;

		private:
			std::vector<PhysicalDeviceData> m_Devices;
			int m_devIndex = -1;
		};

		class VulkanQueue
		{

		public:
			VulkanQueue() {}
			~VulkanQueue() {}

			void Init(VkDevice Device, VkSwapchainKHR SwapChain, uint32_t QueueFamily, uint32_t QueueIndex);

			void UpdateSwapchain(VkSwapchainKHR newSwapchain);

			void Destroy();

			uint32_t AcquireNextImage();

			void SubmitSync(VkCommandBuffer CmbBuf);

			void SubmitAsync(VkCommandBuffer CmbBuf);

			void Present(uint32_t ImageIndex);

			void WaitIdle();

			VkQueue GetVkQueue() { return m_Queue; }

		public:

			void CreateSemaphores();

			VkDevice m_Device = VK_NULL_HANDLE;
			VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
			VkQueue m_Queue = VK_NULL_HANDLE;
			VkSemaphore m_RenderCompleteSem;
			VkSemaphore m_PresentCompleteSem;

			enum
			{
				MAX_FRAMES_IN_FLIGHT = 3
			};

			std::vector<VkSemaphore> m_ImageAvailableSemaphores;
			std::vector<VkSemaphore> m_RenderFinishedSemaphores;
			std::vector<VkFence> m_InFlightFences;
			std::vector<VkFence> m_ImagesInFlight;
			size_t m_CurrentFrame = 0;
		};


		class VulkanGraphicsContext : public GraphicsContext
		{
		public:
			VulkanGraphicsContext();
			~VulkanGraphicsContext();

			virtual void Init(void* window_handle) override;
			virtual void SwapBuffers(void* window_handle) override;
			void Destroy();

			void Resize(uint32_t width, uint32_t height);

		private:
			void RecreateSwapChain(uint32_t width, uint32_t height);

		public:
			void* m_NativeWindowHandle = nullptr;

			VkInstance m_Instance = nullptr;
			VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
			VkSurfaceKHR m_Surface = nullptr;
			VulkanPhysicalDevice m_PhysDevice;
			uint32_t m_QueueFamily = 0;
			VkDevice m_Device = nullptr;

			VkSurfaceFormatKHR m_SwapChainSurfaceFormat;
			VkSwapchainKHR m_SwapChain;
			std::vector<VkImage> m_SwapChainImages;
			std::vector<VkImageView> m_SwapChainImageViews;
			
			VkFormat m_SupportedDepthFormat;
			VulkanTexture* m_DepthAttachmentTexture;

			VkCommandPool m_CommandPool = nullptr;

			VulkanQueue m_Queue;
			uint32_t m_QueueIndex;

			VkCommandBuffer m_CopyCommandBuffer = nullptr;

			enum
			{
				MAX_UNIFORM_BUFFERS = 20,
				MAX_COMBINED_IMAGE_SAMPLERS = 500,
				MAX_STORAGE_BUFFERS = 20,

				MAX_DESCRIPTOR_SETS = 500
			};

			VkDescriptorPool m_DescriptorPool = nullptr;
		};
	}
}
