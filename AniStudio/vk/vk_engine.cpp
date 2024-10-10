#include "vk_engine.h"

//#include <SDL.h>
//#include <SDL_vulkan.h>
#include <GLFW/glfw3.h>

#include <vk_initializers.h>
#include <vk_types.h>

#include <chrono>
#include <thread>

#include <vk_types.h>
#include <vk_initializers.h>

//bootstrap library
#include "VkBootstrap.h"
constexpr bool bUseValidationLayers = false;

VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }
void VulkanEngine::init()
{
    // only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

/*TODO:replace with GLFW
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        _windowExtent.width,
        _windowExtent.height,
        window_flags);

    // everything went fine
    */
    _isInitialized = true;
}

void VulkanEngine::cleanup()
{
    if (_isInitialized) {

        //SDL_DestroyWindow(_window);
        glfwDestroyWindow(_window);
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

void VulkanEngine::draw()
{
    // nothing yet
}

void VulkanEngine::run()
{
    /* TODO: replace with own Event handling, window handling etc.
    SDL_Event e;
    
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    stop_rendering = false;
                }
            }
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
    */
}

constexpr bool bUseValidationLayers = false;

void VulkanEngine::init()
{
    /*TODO: replace with GLFW
	// We initialize SDL and create a window with it. 
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
	
	_window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_windowExtent.width,
		_windowExtent.height,
		window_flags
	);
    */
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(1700, 900, "Vulkan Engine", nullptr, nullptr);

	init_vulkan();

	init_swapchain();

	init_commands();

	init_sync_structures();

	//everything went fine
	_isInitialized = true;
}

void VulkanEngine::init_vulkan()
{
    vkb::InstanceBuilder builder;

	//make the vulkan instance, with basic debug features
	auto inst_ret = builder.set_app_name("Example Vulkan Application")
		.request_validation_layers(bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//grab the instance 
	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	//vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features.dynamicRendering = true;
	features.synchronization2 = true;

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;


	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(_surface)
		.select()
		.value();


	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;
}

void VulkanEngine::init_commands()
{
    //nothing yet
}
void VulkanEngine::init_sync_structures()
{
    //nothing yet
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

	_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	_swapchainExtent = vkbSwapchain.extent;
	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::init_swapchain()
{
	create_swapchain(_windowExtent.width, _windowExtent.height);
}

void VulkanEngine::destroy_swapchain()
{
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < _swapchainImageViews.size(); i++) {

		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
}

void VulkanEngine::cleanup()
{
	if (_isInitialized) {

		destroy_swapchain();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyDevice(_device, nullptr);
		
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);
		SDL_DestroyWindow(_window);
	}
}
