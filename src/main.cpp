#include "Print.hpp"

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>

#include <Magnum/Timeline.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <cstring>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#include <GLFW/glfw3native.h>

auto get_native_platform() -> daxa::NativeWindowPlatform
{
	switch(glfwGetPlatform())
	{
		case GLFW_PLATFORM_WIN32: return daxa::NativeWindowPlatform::WIN32_API;
		case GLFW_PLATFORM_X11: return daxa::NativeWindowPlatform::XLIB_API;
		case GLFW_PLATFORM_WAYLAND: return daxa::NativeWindowPlatform::WAYLAND_API;
		default: return daxa::NativeWindowPlatform::UNKNOWN;
	}
}

auto get_native_handle(GLFWwindow * glfw_window_ptr) -> daxa::NativeWindowHandle
{
	daxa::NativeWindowHandle handle;
#if defined(_WIN32)
	handle.windows = {glfwGetWin32Window(glfw_window_ptr)};
#elif defined(__linux__)
	switch (get_native_platform())
	{
		case daxa::NativeWindowPlatform::WAYLAND_API:
			handle.wayland = {glfwGetWaylandDisplay(), glfwGetWaylandWindow(glfw_window_ptr)};
			break;
		case daxa::NativeWindowPlatform::XLIB_API:
		default:
			handle.x11 = {glfwGetX11Display(), glfwGetX11Window(glfw_window_ptr)};
	}
#endif
	return handle;
}

using namespace Magnum;

class TriangleExample: public Platform::Application 
{
public:
    explicit TriangleExample(const Arguments& arguments);
    ~TriangleExample();

    void tickEvent() override;
private:
    void draw();
    void drawEvent() override {}
    void viewportEvent(ViewportEvent& event) override;
    void exitEvent(ExitEvent& event) override;
    

    Timeline _timeline;

    bool swapchain_out_of_date = false;

    daxa::Instance instance;
    daxa::Device device;
    daxa::Swapchain swapchain;
    daxa::PipelineManager pipeline_manager;
    std::shared_ptr<daxa::RasterPipeline> pipeline;

    daxa::BufferId vertexBuffer;
};

TriangleExample::TriangleExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Triangle Example")
        .setWindowFlags(Configuration::WindowFlag::Resizable | Configuration::WindowFlag::Contextless)}
{
    using namespace Math::Literals;

    auto native_window_handle = get_native_handle(window());
    auto native_window_platform = get_native_platform();

    instance = daxa::create_instance({});
    device = instance.create_device_2(instance.choose_device({},{}));
    swapchain = device.create_swapchain({
        .native_window = native_window_handle,
        .native_window_platform = native_window_platform,
        .window_size = {(uint32_t)windowSize().x(), (uint32_t)windowSize().y()},
        .surface_format_selector = [](daxa::Format format)
        {
            switch(format)
            {
                case daxa::Format::R8G8B8A8_UNORM: return 100;
                default: return daxa::default_format_score(format);
            }
        },
        .present_mode = daxa::PresentMode::FIFO,
        .image_usage = daxa::ImageUsageFlagBits::TRANSFER_DST,
        .name = "swapchain"
    });

    pipeline_manager = daxa::PipelineManager({
        .device = device,
        .shader_compile_options = {
            .root_paths = {
                DAXA_SHADER_INCLUDE_DIR
            },
            .language = daxa::ShaderLanguage::GLSL,
            .enable_debug_info = true,
        },
        .name = "my pipeline manager",
    });


    struct TriangleVertex {
        Vector3 position;
        Color3 color;
    };
    const TriangleVertex vertices[]{
        {{-0.5f, +0.5f, 0.0f}, 0xff0000_rgbf},    /* Left vertex, red color */
        {{ +0.5f, +0.5f, 0.0f}, 0x00ff00_rgbf},    /* Right vertex, green color */
        {{+0.0f, -0.5f, 0.0f}, 0x0000ff_rgbf}     /* Top vertex, blue color */
    };

    const char* shaderSource = R"(
    #include <daxa/daxa.inl>

    struct TriangleVertex
    {
        daxa_f32vec3 position;
        daxa_f32vec3 color;
    };
    DAXA_DECL_BUFFER_PTR(TriangleVertex)

    struct MyPushConstant
    {
        daxa_BufferPtr(TriangleVertex) vertices;
    };

    DAXA_DECL_PUSH_CONSTANT(MyPushConstant, push)

    #if DAXA_SHADER_STAGE == DAXA_SHADER_STAGE_VERTEX

    layout(location = 0) out daxa_f32vec3 v_col;
    void main()
    {
        TriangleVertex vert = deref_i(push.vertices, gl_VertexIndex);
        gl_Position = daxa_f32vec4(vert.position, 1);
        v_col = vert.color;
    }

    #elif DAXA_SHADER_STAGE == DAXA_SHADER_STAGE_FRAGMENT

    layout(location = 0) in daxa_f32vec3 v_col;
    layout(location = 0) out daxa_f32vec4 color;
    void main()
    {
        color = daxa_f32vec4(v_col, 1);
    }

    #endif
    )";

    {
        auto result = pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = daxa::ShaderCompileInfo{.source = daxa::ShaderCode{shaderSource}},
            .fragment_shader_info = daxa::ShaderCompileInfo{.source = daxa::ShaderCode{shaderSource}},
            .color_attachments = {{.format = swapchain.get_format()}},
            .raster = {},
            .push_constant_size = sizeof(daxa::DeviceAddress),
            .name = "my pipeline",
        });
        if (result.is_err())
        {
            std::cerr << result.message() << std::endl;
            exit(-1);
        }
        pipeline = result.value();
    }

    vertexBuffer = device.create_buffer({
        .size = sizeof(TriangleVertex) * 3,
        .allocate_info = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
        .name = "vertex data",
    });
    std::memcpy(device.buffer_host_address(vertexBuffer).value(),vertices, sizeof(TriangleVertex) * 3);

    _timeline.start();
}

TriangleExample::~TriangleExample()
{
    pipeline_manager.~PipelineManager();
    swapchain.~Swapchain();
    device.~Device();
    instance.~Instance();
}

void TriangleExample::tickEvent()
{
    auto deltaTime = _timeline.previousFrameDuration();
    if(glfwWindowShouldClose(window())) return;
    draw();
    _timeline.nextFrame();
}

void TriangleExample::draw() 
{
    daxa::ImageId swapchainImage;
    auto result = swapchain.acquire_next_image(swapchainImage);
    if (result == daxa::DaxaResult::ERROR_OUT_OF_DATE_KHR || result == daxa::DaxaResult::SUBOPTIMAL_KHR || swapchain_out_of_date)
    {
        swapchain.resize({(uint32_t)windowSize().x(), (uint32_t)windowSize().y()});
        swapchain_out_of_date = false;
        return;
    }

    if (swapchainImage.is_empty())
    {
        return;
    }

    daxa::CommandRecorder recorder = device.create_command_recorder({.name = "render command recorder"});
    
    daxa::RenderCommandRecorder render_recorder = std::move(recorder).begin_renderpass({
        .color_attachments = std::array{
            daxa::RenderAttachmentInfo{
                .image_view = swapchainImage.default_view(),
                .load_op = daxa::AttachmentLoadOp::CLEAR,
                .clear_value = std::array<daxa::f32, 4>{0.0f, 0.0f, 0.0f, 1.0f},
            },
        },
        .render_area = {.width = (uint32_t)windowSize().x(), .height = (uint32_t)windowSize().y()},
    });

    render_recorder.set_pipeline(*pipeline);
    render_recorder.push_constant(device.buffer_device_address(vertexBuffer).value());
    render_recorder.draw({.vertex_count = 3});

    recorder = std::move(render_recorder).end_renderpass();

    auto executalbe_commands = recorder.complete_current_commands();
    recorder.~CommandRecorder();

    auto const & acquire_semaphore = swapchain.current_acquire_semaphore();
    auto const & present_semaphore = swapchain.current_present_semaphore();
    device.submit_commands(daxa::CommandSubmitInfo{
        .command_lists = std::array{executalbe_commands},
        .wait_binary_semaphores = std::array{acquire_semaphore},
        .signal_binary_semaphores = std::array{present_semaphore},
        .signal_timeline_semaphores = std::array{swapchain.current_timeline_pair()},
    });
    device.present_frame({
        .wait_binary_semaphores = std::array{present_semaphore},
        .swapchain = swapchain,
    });


    device.collect_garbage();
}

void TriangleExample::viewportEvent(ViewportEvent& event)
{
    swapchain_out_of_date = true;
}

void TriangleExample::exitEvent(ExitEvent& event)
{
    device.wait_idle();
    device.collect_garbage();

    device.destroy(vertexBuffer);
    pipeline.reset();

    event.setAccepted();
}

MAGNUM_APPLICATION_MAIN(TriangleExample)