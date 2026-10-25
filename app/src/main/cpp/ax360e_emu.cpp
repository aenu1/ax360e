/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <jni.h>

#include <android/asset_manager.h>
#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <jni.h>
#include <array>
#include <memory>
#include <sys/prctl.h>

#include "xenia/apu/nop/nop_audio_system.h"
#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/base/profiling.h"
#include "xenia/config.h"
#include "xenia/emulator.h"
#include "xenia/gpu/graphics_system.h"
#include "xenia/gpu/null/null_graphics_system.h"
#include "xenia/gpu/vulkan/vulkan_graphics_system.h"
#include "xenia/hid/nop/nop_hid.h"
#include "xenia/kernel/xam/xam_module.h"
#include "xenia/kernel/xam/xam_ui.h"
#include "xenia/vfs/devices/host_path_device.h"

#include "emulator.h"
#include "emulator_ax360e.h"

#include "xe_android_hid.h"
#include "xe_android_input_driver.h"
#include "xe_opensles_audio_system.h"
#include "xe_aaudio_audio_system.h"
#include "document_file.h"

#include "ax360e_emu.h"
//#include "nlohmann/json.hpp"

#define LOG_TAG "ax360e_native"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG,__VA_ARGS__);

DEFINE_string(apu, "aaudio", "Audio system. Use: [any, nop, opensles, aaudio]", "APU");
DEFINE_string(gpu, "vulkan", "Graphics system. Use: [vulkan, null]",
              "GPU");
DEFINE_string(hid, "android", "Input system. Use: [android, nop]",
              "HID");

DEFINE_path(
        storage_root, "",
        "Root path for persistent internal data storage (config, etc.), or empty "
        "to use the path preferred for the OS, such as the documents folder, or "
        "the emulator executable directory if portable.txt is present in it.",
        "Storage");
DEFINE_path(
        content_root, "",
        "Root path for guest content storage (saves, etc.), or empty to use the "
        "content folder under the storage root.",
        "Storage");
DEFINE_path(
        cache_root, "",
        "Root path for files used to speed up certain parts of the emulator or the "
        "game. These files may be persistent, but they can be deleted without "
        "major side effects such as progress loss. If empty, the cache folder "
        "under the storage root, or, if available, the cache directory preferred "
        "for the OS, will be used.",
        "Storage");

DEFINE_bool(mount_scratch, false, "Enable scratch mount", "Storage");
DEFINE_bool(mount_cache, false, "Enable cache mount", "Storage");
DEFINE_bool(mount_memory_unit, false, "Enable memory unit (MU) mount",
            "Storage");

DECLARE_bool(force_mount_devkit);
DEFINE_transient_path(target, "",
                      "Specifies the target .xex or .iso to execute.",
                      "General");
DEFINE_transient_bool(portable, false,
                      "Specifies if Xenia should run in portable mode.",
                      "General");

DECLARE_bool(debug);

DEFINE_bool(discord, false, "Enable Discord rich presence", "General");

extern JavaVM* g_jvm;
namespace ae{
    extern std::unique_ptr<xe::ui::WindowedApp> g_windowed_app;
}
void AndroidWindowedAppContext::NotifyUILoopOfPendingFunctions() {
    std::unique_lock<std::mutex> lock(mutex);
    bool completed = false;
    events.push({EVENT_EXECUTE_PENDING_FUNCTIONS, &completed});
    cv.notify_one();
    cv.wait(lock, [&completed] { return completed; });
}

void AndroidWindowedAppContext::PlatformQuitFromUIThread() {
    std::lock_guard<std::mutex> lock(mutex);
    events.push({EVENT_QUIT, nullptr});
    cv.notify_one();
}

void AndroidWindowedAppContext::request_paint() {
    bool expected = false;
    if (!paint_pending_.compare_exchange_strong(expected, true)) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex);
    events.push({EVENT_PAINT, nullptr});
    cv.notify_one();
}

void AndroidWindowedAppContext::request_surface_changed() {
    std::lock_guard<std::mutex> lock(mutex);
    events.push({EVENT_SURFACE_CHANGED, nullptr});
    cv.notify_one();
}

void AndroidWindowedAppContext::setup_ui_thr_id(std::thread::id id){
    WindowedAppContext::ui_thread_id_=id;
}

void AndroidWindowedAppContext::main_loop(){
    assert(WindowedAppContext::ui_thread_id_==std::this_thread::get_id());
    while(!WindowedAppContext::HasQuitFromUIThread()){
        EventItem item;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this] { return !events.empty() || WindowedAppContext::HasQuitFromUIThread(); });
            if(events.empty()) {
                continue;
            }
            item = events.front();
            events.pop();
        }

        if(item.type==EVENT_EXECUTE_PENDING_FUNCTIONS){
            WindowedAppContext::ExecutePendingFunctionsFromUIThread();
            {
                std::lock_guard<std::mutex> lock(mutex);
                *item.completed = true;
            }
            cv.notify_one();
        }
        else if(item.type==EVENT_PAINT){
            paint_pending_.store(false, std::memory_order_release);
            EmulatorApp* app=static_cast<EmulatorApp*>(ae::g_windowed_app.get());
            AndroidWindow* win=static_cast<AndroidWindow*>(app->emu_window->window());
            win->Paint();
        }
        else if(item.type==EVENT_SURFACE_CHANGED){
            EmulatorApp* app=static_cast<EmulatorApp*>(ae::g_windowed_app.get());
            AndroidWindow* win=static_cast<AndroidWindow*>(app->emu_window->window());
            win->UpdateSurface();
        }
        else if(item.type==EVENT_QUIT){
            WindowedAppContext::QuitFromUIThread();
            cv.notify_all();
            return;
        }
    }
}

AndroidWindowedAppContext::AndroidWindowedAppContext() {
}

AndroidWindowedAppContext::~AndroidWindowedAppContext(){
}

AndroidWindow::AndroidWindow(xe::ui::WindowedAppContext& app_context, const std::string_view title,
                             uint32_t desired_logical_width, uint32_t desired_logical_height)
                             : Window(app_context, title, desired_logical_width, desired_logical_height) {}

bool AndroidWindow::OpenImpl() {
    XELOGI("Opening Android window...");

    if (ae::window) {
        int w = ANativeWindow_getWidth(ae::window);
        int h = ANativeWindow_getHeight(ae::window);
        if (w > 0 && h > 0) {
            OnDesiredLogicalSizeUpdate(SizeToLogical(w), SizeToLogical(h));
            WindowDestructionReceiver destruction_receiver(this);
            OnActualSizeUpdate(uint32_t(w), uint32_t(h), WindowResizeAction::kManual, destruction_receiver);
        } else {
            XELOGW("Android window has invalid size: {}x{}", w, h);
        }
    }

    return true;
}

void AndroidWindow::RequestCloseImpl() {
    XELOGI("Requesting Android window close...");
}

std::unique_ptr<xe::ui::Surface> AndroidWindow::CreateSurfaceImpl(xe::ui::Surface::TypeFlags allowed_types) {
    XELOGI("Creating Android surface...");
    if(allowed_types&xe::ui::Surface::kTypeFlag_AndroidNativeWindow) {
        ANativeWindow *window = ae::window;
        return std::make_unique<xe::ui::AndroidNativeWindowSurface>(window);
    }
    return nullptr;
}

void AndroidWindow::RequestPaintImpl() {
    XELOGI("Requesting Android window paint...");
    static_cast<AndroidWindowedAppContext&>(app_context()).request_paint();
}

void AndroidWindow::UpdateSurface(){
    if (ae::window) {
        int w = ANativeWindow_getWidth(ae::window);
        int h = ANativeWindow_getHeight(ae::window);
        if (w > 0 && h > 0) {
            OnDesiredLogicalSizeUpdate(SizeToLogical(w), SizeToLogical(h));
            WindowDestructionReceiver destruction_receiver(this);
            OnActualSizeUpdate(uint32_t(w), uint32_t(h), WindowResizeAction::kManual, destruction_receiver);
            if (destruction_receiver.IsWindowDestroyedOrClosed()) {
                return;
            }
        }
    }
    OnSurfaceChanged(true);
}

void AndroidWindow::Paint(){
    OnPaint(false);
}

std::unique_ptr<xe::ui::Window> xe::ui::Window::Create(WindowedAppContext& app_context,
                                                       const std::string_view title,
                                                       uint32_t desired_logical_width,
                                                       uint32_t desired_logical_height) {
    return std::make_unique<AndroidWindow>(
            app_context, title, desired_logical_width, desired_logical_height);
}

android_menu_item::android_menu_item(Type type, const std::string& text, const std::string& hotkey,
                                     std::function<void()> callback)
        : MenuItem(type, text, hotkey, callback) {
    LOGW("android_menu_item: %d %s %s",static_cast<int>(type),text.c_str(),hotkey.c_str());
}

std::unique_ptr<xe::ui::MenuItem> xe::ui::MenuItem::Create(Type type,
                                                   const std::string& text,
                                                   const std::string& hotkey,
                                                   std::function<void()> callback) {
    return std::make_unique<android_menu_item>(type, text, hotkey, callback);
}


std::unique_ptr<xe::ui::WindowedApp> EmulatorApp::create(xe::ui::WindowedAppContext& app_context) {
    return std::unique_ptr<xe::ui::WindowedApp>(new EmulatorApp(app_context));
}

EmulatorApp::EmulatorApp(xe::ui::WindowedAppContext& app_context)
        : WindowedApp(app_context,"ax36e") {
}

bool EmulatorApp::OnInitialize() {

    xe::Profiler::Initialize();
    xe::Profiler::ThreadEnter("Main");

    std::filesystem::path storage_root=cvars::storage_root;

    storage_root = std::filesystem::absolute(storage_root);
    XELOGI("Storage root: {}", storage_root.c_str());

    config::SetupConfig(storage_root);

#if XE_ARCH_AMD64 == 1
    xe::amd64::InitFeatureFlags();
#elif XE_ARCH_ARM64 == 1
    xe::arm64::InitFeatureFlags();
#endif

    std::filesystem::path content_root = cvars::content_root;
    if (content_root.empty()) {
        content_root = storage_root / "content";
    } else {
        // If content root isn't an absolute path, then it should be relative to the
        // storage root.
        if (!content_root.is_absolute()) {
            content_root = storage_root / content_root;
        }
    }
    content_root = std::filesystem::absolute(content_root);
    XELOGI("Content root: {}", content_root.c_str());

    std::filesystem::path cache_root = cvars::cache_root;
    if (cache_root.empty()) {
        cache_root = storage_root / "cache";
        // TODO(Triang3l): Point to the app's external storage "cache" directory on
        // Android.
    } else {
        // If content root isn't an absolute path, then it should be relative to the
        // storage root.
        if (!cache_root.is_absolute()) {
            cache_root = storage_root / cache_root;
        }
    }
    cache_root = std::filesystem::absolute(cache_root);
    XELOGI("Host cache root: {}", cache_root);

    // Create the emulator but don't initialize so we can setup the window.
    emu =
            std::make_unique<xe::Emulator>("", storage_root, content_root, cache_root);

    // Determine window size based on user setting.
    // auto res = xe::gpu::GraphicsSystem::GetInternalDisplayResolution();

    // Main emulator display window.
    emu_window = xe::app::EmulatorWindow::Create(emu.get(), app_context(),
                                                 ae::window_width,ae::window_height);

    if (!emu_window) {
        XELOGE("Failed to create the main emulator window");
        return false;
    }

    emu_thr_quit_requested.store(false, std::memory_order_relaxed);
    emu_thr_event = xe::threading::Event::CreateAutoResetEvent(false);
    assert_not_null(emu_thr_event);
    emu_thr = std::thread(&EmulatorApp::emu_thr_main, this);

    return true;
}

std::unique_ptr<xe::apu::AudioSystem> EmulatorApp::create_audio_system(xe::cpu::Processor* processor) {
    Factory<xe::apu::AudioSystem, xe::cpu::Processor*> factory;
    factory.Add<xe::apu::nop::NopAudioSystem>("nop");
#if XE_PLATFORM_AX360E
    factory.Add<xe::apu::opensles::OpenSLESAudioSystem>("opensles");
    factory.Add<xe::apu::aaudio::AAudioAudioSystem>("aaudio");
#endif
    return factory.Create(cvars::apu, processor);
}

std::unique_ptr<xe::gpu::GraphicsSystem> EmulatorApp::create_graphics_system() {
    Factory<xe::gpu::GraphicsSystem> factory;
    factory.Add<xe::gpu::vulkan::VulkanGraphicsSystem>("vulkan");
    factory.Add<xe::gpu::null::NullGraphicsSystem>("null");
    return factory.Create(cvars::gpu);
}


std::vector<std::unique_ptr<xe::hid::InputDriver>> EmulatorApp::create_input_drivers(xe::ui::Window* window) {
    std::vector<std::unique_ptr<xe::hid::InputDriver>> drivers;
    if (cvars::hid.compare("nop") == 0) {
        drivers.emplace_back(xe::hid::nop::Create(window, xe::app::EmulatorWindow::kZOrderHidInput));
    }
    else {
        Factory<xe::hid::InputDriver, xe::ui::Window *, size_t> factory;
        factory.Add("android", xe::hid::android::Create);

        for (auto &driver: factory.CreateAll(cvars::hid, window,
                                             xe::app::EmulatorWindow::kZOrderHidInput)) {
            if (XSUCCEEDED(driver->Setup())) {
                drivers.emplace_back(std::move(driver));
            }
        }
    }
    if (drivers.empty()) {
        // Fallback to nop if none created.
        drivers.emplace_back(
                xe::hid::nop::Create(window, xe::app::EmulatorWindow::kZOrderHidInput));
    }
    return drivers;
}

void EmulatorApp::emu_thr_main() {
    assert_not_null(emu_thr_event);

    JNIEnv *env;
    g_jvm->AttachCurrentThread(&env, nullptr);

    xe::threading::set_name("Emulator");
    xe::Profiler::ThreadEnter("Emulator");

    // Setup and initialize all subsystems. If we can't do something
    // (unsupported system, memory issues, etc) this will fail early.
    xe::X_STATUS result = emu->Setup(
            emu_window->window(), emu_window->imgui_drawer(), true,
            create_audio_system, create_graphics_system, create_input_drivers);
    if (XFAILED(result)) {
        XELOGE("Failed to setup emulator: {:08X}", result);
        app_context().RequestDeferredQuit();
        return;
    }
    app_context().CallInUIThread(
            [this]() { emu_window->SetupGraphicsSystemPresenterPainting(); });
    const auto fs = emu->file_system();

    if (cvars::mount_scratch) {
        auto scratch_device = std::make_unique<xe::vfs::HostPathDevice>(
                "\\SCRATCH", emu->storage_root() / "scratch", false);
        if (!scratch_device->Initialize()) {
            XELOGE("Unable to scan scratch path");
        } else {
            if (!fs->RegisterDevice(std::move(scratch_device))) {
                XELOGE("Unable to register scratch path");
            } else {
                fs->RegisterSymbolicLink("scratch:", "\\SCRATCH");
            }
        }
    }

    if (cvars::mount_cache) {
        auto cache0_device = std::make_unique<xe::vfs::HostPathDevice>(
                "\\CACHE0", emu->storage_root() / "cache0", false);
        if (!cache0_device->Initialize()) {
            XELOGE("Unable to scan cache0 path");
        } else {
            if (!fs->RegisterDevice(std::move(cache0_device))) {
                XELOGE("Unable to register cache0 path");
            } else {
                fs->RegisterSymbolicLink("cache0:", "\\CACHE0");
            }
        }

        auto cache1_device = std::make_unique<xe::vfs::HostPathDevice>(
                "\\CACHE1", emu->storage_root() / "cache1", false);
        if (!cache1_device->Initialize()) {
            XELOGE("Unable to scan cache1 path");
        } else {
            if (!fs->RegisterDevice(std::move(cache1_device))) {
                XELOGE("Unable to register cache1 path");
            } else {
                fs->RegisterSymbolicLink("cache1:", "\\CACHE1");
            }
        }

        // Some (older?) games try accessing cache:\ too
        // NOTE: this must be registered _after_ the cache0/cache1 devices, due to
        // substring/start_with logic inside VirtualFileSystem::ResolvePath, else
        // accesses to those devices will go here instead
        auto cache_device = std::make_unique<xe::vfs::HostPathDevice>(
                "\\CACHE", emu->storage_root() / "cache", false);
        if (!cache_device->Initialize()) {
            XELOGE("Unable to scan cache path");
        } else {
            if (!fs->RegisterDevice(std::move(cache_device))) {
                XELOGE("Unable to register cache path");
            } else {
                fs->RegisterSymbolicLink("cache:", "\\CACHE");
            }
        }
    }

    if (cvars::force_mount_devkit) {
        auto devkit_device =
                std::make_unique<xe::vfs::HostPathDevice>("\\DEVKIT", "devkit", false);

        if (!devkit_device->Initialize()) {
            XELOGE("Unable to scan devkit path");
        }

        if (!fs->RegisterDevice(std::move(devkit_device))) {
            XELOGE("Unable to register devkit path");
        }

        fs->RegisterSymbolicLink("DEVKIT:", "\\DEVKIT");
        fs->RegisterSymbolicLink("e:", "\\DEVKIT");
    }

    if (cvars::mount_memory_unit) {
        auto mu_device =
                std::make_unique<xe::vfs::HostPathDevice>("\\MU", "MU", false);

        if (!mu_device->Initialize()) {
            XELOGE("Unable to scan MU path");
        }

        if (!fs->RegisterDevice(std::move(mu_device))) {
            XELOGE("Unable to register MU path");
        }

        fs->RegisterSymbolicLink("MU:", "\\MU");
    }

// Set a debug handler.
// This will respond to debugging requests so we can open the debug UI.
    /*if (cvars::debug) {
        emulator_->processor()->set_debug_listener_request_handler(
                [this](xe::cpu::Processor* processor) {
                    if (debug_window_) {
                        return debug_window_.get();
                    }
                    app_context().CallInUIThreadSynchronous([this]() {
                        debug_window_ = xe::debug::ui::DebugWindow::Create(emulator_.get(),
                                                                           app_context());
                        debug_window_->window()->AddListener(
                                &debug_window_closed_listener_);
                    });
                    // If failed to enqueue the UI thread call, this will just be null.
                    return debug_window_.get();
                });
    }*/
    emu->on_launch.AddListener([&](auto title_id, const auto& game_title) {
        XELOGI("on_launch {}",
               game_title.empty() ? "Unknown Title" : std::string(game_title));
        //app_context().CallInUIThread([this]() { emu_window->UpdateTitle(); });
        emu_thr_event->Set();
    });
    emu->on_shader_storage_initialization.AddListener(
            [this](bool initializing) {
                XELOGI("Shader storage initialization: {}", initializing);
                app_context().CallInUIThread([this, initializing]() {
                    emu_window->SetInitializingShaderStorage(initializing);

                });

            });

    emu->on_patch_apply.AddListener([this]() {
        app_context().CallInUIThread([this]() { emu_window->UpdateTitle(); });
    });

    emu->on_terminate.AddListener([]() {
        XELOGI("Emulator terminated");
    });

    // Enable emulator input now that the emulator is properly loaded.
    app_context().CallInUIThread(
            [this]() { emu_window->OnEmulatorInitialized(); });

    // Grab path from the flag or unnamed argument.
    std::string path;
    if (!cvars::target.empty()) {
        path = cvars::target;
    }

    if (!path.empty()) {
        jclass uri_class = env->FindClass("android/net/Uri");
        jmethodID parse_method = env->GetStaticMethodID(uri_class, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
        jstring uri_string = env->NewStringUTF(path.c_str());
        jobject uri = env->CallStaticObjectMethod(uri_class, parse_method, uri_string);

        std::unique_ptr<DocumentFile> file =
                DocumentFile::find(g_jvm,uri);

        std::string name = file->getName();


        if(name.ends_with(".xex")){
            result = app_context().CallInUIThread(
                    [this, &file]() { return emu->LaunchXexFile(std::move(file)); });
        }
        else if(name.ends_with(".iso")){
            result = app_context().CallInUIThread(
                    [this, &file]() { return emu->LaunchDiscImage(std::move(file)); });
        }
        else if(name.ends_with(".zar")){
            result = app_context().CallInUIThread(
                    [this, &file]() { return emu->LaunchDiscArchive(std::move(file)); });
        }
        else{
            std::string data_dir = path+".data";
            jstring data_dir_str = env->NewStringUTF(data_dir.c_str());
            jobject data_dir_uri = env->CallStaticObjectMethod(uri_class, parse_method, data_dir_str);

            std::unique_ptr<DocumentFile> data_dir_file =
                    DocumentFile::find(g_jvm,data_dir_uri);
            result = app_context().CallInUIThread(
                    [this, &file,&data_dir_file]() { return emu->LaunchStfsContainer(std::move(file), std::move(data_dir_file)); });
        }

        /*result = emu->LaunchPath(abs_path);*//*app_context().CallInUIThread(
                [this, abs_path]() { return emu_window->RunTitle(abs_path); });*/

        if (XFAILED(result)) {
            xe::FatalError(fmt::format("Failed to launch target: {:08X}", result));
            app_context().RequestDeferredQuit();
            return;
        }
    }

    auto xam = emu->kernel_state()->GetKernelModule<xe::kernel::xam::XamModule>(
            "xam.xex");

    if (xam) {
        xam->LoadLoaderData();

        if (!xam->loader_data().host_path.empty()) {
            const std::filesystem::path host_path = xam->loader_data().host_path;
            app_context().CallInUIThread([this, host_path]() {
                return emu_window->RunTitle(host_path);
            });
        }
    }

    // Now, we're going to use this thread to drive events related to emulation.
    /*while (!emu_thr_quit_requested.load(std::memory_order_relaxed)) {
        xe::threading::Wait(emu_thr_event.get(), false);
        emu->WaitUntilExit();
    }*/
    while (!emu_thr_quit_requested.load(std::memory_order_relaxed)) {
        xe::threading::Wait(emu_thr_event.get(), false);
        emu->WaitUntilExit();
    }

    XELOGI("QUIT");
    app_context().QuitFromUIThread();
}

XE_DEFINE_WINDOWED_APP(ax36e,EmulatorApp::create);

namespace ae{

    void show_soft_keyboard(const std::string& initial_text);
    void hide_soft_keyboard();

    int boot_type;

    std::string boot_game_path;
    int boot_game_fd;
    std::string boot_game_uri;

    ANativeWindow* window;
    int window_width;
    int window_height;

    std::string game_id;

     std::unique_ptr<xe::ui::WindowedApp> g_windowed_app;
     EmulatorApp* g_windowed_app_ref;

    // n->[n]
    static std::array<xe::ui::VirtualKey,24> key_maps={
            xe::ui::VirtualKey::kXInputPadDpadLeft,
            xe::ui::VirtualKey::kXInputPadDpadUp,
            xe::ui::VirtualKey::kXInputPadDpadRight,
            xe::ui::VirtualKey::kXInputPadDpadDown,
            xe::ui::VirtualKey::kXInputPadA,
            xe::ui::VirtualKey::kXInputPadB,
            xe::ui::VirtualKey::kXInputPadX,
            xe::ui::VirtualKey::kXInputPadY,
            xe::ui::VirtualKey::kXInputPadBack,
            xe::ui::VirtualKey::kXInputPadStart,

            xe::ui::VirtualKey::kXInputPadLShoulder,
            xe::ui::VirtualKey::kXInputPadRShoulder,
            xe::ui::VirtualKey::kXInputPadLThumbPress,
            xe::ui::VirtualKey::kXInputPadRThumbPress,
            xe::ui::VirtualKey::kXInputPadLTrigger,
            xe::ui::VirtualKey::kXInputPadRTrigger,

            xe::ui::VirtualKey::kXInputPadLThumbLeft,
            xe::ui::VirtualKey::kXInputPadLThumbUp,
            xe::ui::VirtualKey::kXInputPadLThumbRight,
            xe::ui::VirtualKey::kXInputPadLThumbDown,
            xe::ui::VirtualKey::kXInputPadRThumbLeft,
            xe::ui::VirtualKey::kXInputPadRThumbUp,
            xe::ui::VirtualKey::kXInputPadRThumbRight,
            xe::ui::VirtualKey::kXInputPadRThumbDown
    };

    void main_thr(){

        std::string tid=[]{
            std::stringstream ss;
            ss<<std::this_thread::get_id();
            return ss.str();
        }();
        LOGW("new thr: %s",tid.c_str());

        //prctl(PR_SET_TIMERSLACK,1,0,0,0);

        AndroidWindowedAppContext wnd_ctx;
        wnd_ctx.setup_ui_thr_id(std::this_thread::get_id());
        g_windowed_app=xe::ui::GetWindowedAppCreator()(wnd_ctx);
        g_windowed_app_ref=dynamic_cast<EmulatorApp*>(g_windowed_app.get());

        // Connect the ImGui keyboard input dialog to the Android soft keyboard.
        xe::kernel::xam::soft_keyboard_show_hook=[](const std::string& initial){
            ae::show_soft_keyboard(initial);
        };
        xe::kernel::xam::soft_keyboard_hide_hook=[](){
            ae::hide_soft_keyboard();
        };

        std::vector<char*> args;
        args.push_back(NULL);
        for(auto& i:g_launch_args){
            args.push_back((char*)i.c_str());
        }
        static std::string boot_target=std::string("--target=")+ae::boot_game_uri;
        args.push_back((char*)boot_target.c_str());

        int argc=args.size();
        char** argv=args.data();

        cvar::ParseLaunchArguments(argc, argv, g_windowed_app->GetPositionalOptionsUsage(),
                                   g_windowed_app->GetPositionalOptions());
        xe::InitializeLogging(g_windowed_app->GetName());
        if(g_windowed_app->OnInitialize()){
            wnd_ctx.main_loop();
        }

    }

    void key_event(int key_code,bool pressed,int value){
        static const bool is_android=cvars::hid=="android";
        if(is_android){
            xe::hid::android::AndroidInputDriver* driver=reinterpret_cast<xe::hid::android::AndroidInputDriver*>(g_windowed_app_ref->emu->input_system()->drivers_[0].get());
            driver->OnKey(key_code,pressed,value);
        }
    }

    // ---- Soft keyboard bridge (Java IME <-> ImGui KeyboardInputDialog) ----

    // Cached at JNI_OnLoad time: FindClass on threads attached later (like the
    // emulator UI thread) cannot see app classes through the default loader.
    static jclass g_class_EmulatorActivity=nullptr;
    static jmethodID g_mid_show_soft_input=nullptr;
    static jmethodID g_mid_hide_soft_input=nullptr;

    static JNIEnv* get_jni_env(){
        JNIEnv* env=nullptr;
        if(g_jvm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)==JNI_EDETACHED){
            // The emulator UI thread is a native thread; keep it attached.
            g_jvm->AttachCurrentThread(&env,nullptr);
        }
        return env;
    }

    void show_soft_keyboard(const std::string& initial_text){
        if(!g_class_EmulatorActivity||!g_mid_show_soft_input) return;
        JNIEnv* env=get_jni_env();
        if(!env) return;
        jstring js=env->NewStringUTF(initial_text.c_str());
        env->CallStaticVoidMethod(g_class_EmulatorActivity,g_mid_show_soft_input,js);
        env->DeleteLocalRef(js);
    }

    void hide_soft_keyboard(){
        if(!g_class_EmulatorActivity||!g_mid_hide_soft_input) return;
        JNIEnv* env=get_jni_env();
        if(!env) return;
        env->CallStaticVoidMethod(g_class_EmulatorActivity,g_mid_hide_soft_input);
    }

    void set_activity_jni_cache(jclass cls,jmethodID show,jmethodID hide){
        g_class_EmulatorActivity=cls;
        g_mid_show_soft_input=show;
        g_mid_hide_soft_input=hide;
    }

    void ime_input(const char* text,bool done){
        if(!g_windowed_app) return;
        std::string s=text?text:"";
        // The dialog lives on the emulator UI thread; marshal there and
        // re-check the receiver, the dialog may close in the meantime.
        g_windowed_app->app_context().CallInUIThread([s,done](){
            auto* dialog=xe::kernel::xam::KeyboardInputDialog::GetActiveTextInput();
            if(dialog) dialog->ApplyImeText(s,done);
        });
    }
    void surface_changed(){
        if(!g_windowed_app) return;
        auto* ctx=static_cast<AndroidWindowedAppContext*>(&g_windowed_app->app_context());
        ctx->request_surface_changed();
    }
    bool is_running(){
        return !g_windowed_app_ref->emu->is_paused();
    }
    bool is_paused(){
        return g_windowed_app_ref->emu->is_paused();
    }
    void pause(){
        if(!g_windowed_app_ref) return;
        g_windowed_app_ref->emu->Pause();
    }
    void resume(){
        if(!g_windowed_app_ref) return;
        g_windowed_app_ref->emu->Resume();
    }
    void quit(){
    }

    void init(){
    }

}

// Caches EmulatorActivity JNI references while the app class loader is still
// reachable (called from JNI_OnLoad); must run before any attached native
// thread tries to call back into the activity.
int cache_ax360e_activity_jni(JNIEnv* env){
    jclass cls=env->FindClass("aenu/ax360e/EmulatorActivity");
    if(!cls) return JNI_ERR;
    jclass gcls=(jclass)env->NewGlobalRef(cls);
    jmethodID mid_show=env->GetStaticMethodID(gcls,"show_soft_input","(Ljava/lang/String;)V");
    jmethodID mid_hide=env->GetStaticMethodID(gcls,"hide_soft_input","()V");
    if(!mid_show||!mid_hide){
        env->DeleteGlobalRef(gcls);
        return JNI_ERR;
    }
    ae::set_activity_jni_cache(gcls,mid_show,mid_hide);
    return JNI_OK;
}
