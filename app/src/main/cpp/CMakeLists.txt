
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)
# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("ax360e")

add_compile_options(-march=armv8-a)
add_compile_options(-stdlib=libc++)
add_compile_options(-fPIC)

add_library(hardware_ProcessorInfo SHARED hardware_ProcessorInfo.cpp)
target_link_libraries(hardware_ProcessorInfo PRIVATE vulkan)

add_compile_options(-fno-omit-frame-pointer)
add_compile_options(-funwind-tables)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_link_options(-Wl,-z,max-page-size=16384)

set(CMAKE_CXX_STANDARD 20)

project(ax360e LANGUAGES C CXX ASM)

add_library(emu SHARED)
set_target_properties(emu PROPERTIES OUTPUT_NAME "e")

target_link_options(emu PRIVATE -Wl,--exclude-libs,ALL)

#target_compile_options(emu PRIVATE -flto)
#target_link_options(emu PRIVATE -flto)

target_sources(emu
        PRIVATE
        aarch64_disasm.cpp
        ax360e.cpp
        ax360e_emu.cpp
        emulator.cpp
        emulator_ax360e.cpp
        document_file.cpp
        xe_aaudio_audio_driver.cpp
        xe_aaudio_audio_system.cpp
        xe_android_hid.cpp
        xe_android_input_driver.cpp
        xe_android_util.cpp
        xe_opensles_audio_driver.cpp
        xe_opensles_audio_system.cpp
        xe_saf_disc_image_device.cpp
        xe_saf_disc_image_entry.cpp
        xe_saf_disc_image_file.cpp
        xe_saf_disc_archive_file.cpp
        xe_saf_disc_archive_device.cpp
        xe_saf_disc_archive_entry.cpp
        xe_saf_stfs_device.cpp
        xe_saf_stfs_entry.cpp
        xe_saf_stfs_file.cpp
        xe_saf_xex_device.cpp
        xe_saf_xex_entry.cpp
        xe_saf_xex_file.cpp
        xe_system_ax360e.cpp
        xenia-canary/src/xenia/app/emulator_window.cc
        xenia-canary/src/xenia/app/profile_dialogs.cc
        xenia-canary/src/xenia/ui/file_picker_android.cc
        xenia-canary/src/xenia/ui/surface_android.cc
        cpuinfo.cpp
        vkapi.cpp
        vkutil.cpp
)
#xe_a64_code_cache_posix.cpp
#emulator.cpp
#emulator_ax360e.cpp
# python xenia-build premake --target_os=android --devenv=cmake
# Generated CMake files are in xenia/build/
add_subdirectory(xenia-canary EXCLUDE_FROM_ALL)

add_subdirectory(libadrenotools EXCLUDE_FROM_ALL)

target_include_directories(emu PRIVATE
        xenia-canary
        xenia-canary/src
        xenia-canary/third_party
        xenia-canary/..
)

target_link_libraries(emu
        PRIVATE
        xenia-apu
        xenia-apu-nop
        xenia-base
        xenia-core
        xenia-cpu
        xenia-cpu-backend-a64
        xenia-gpu
        xenia-gpu-null
        xenia-gpu-vulkan
        xenia-hid
        xenia-hid-nop
        xenia-kernel
        xenia-ui
        xenia-ui-vulkan
        xenia-vfs
        aes_128
        capstone
        fmt
        dxbc
        discord-rpc
        glslang-spirv
        imgui
        libavcodec
        libavutil
        libavformat
        mspack
        snappy
        xxhash
        android
        dl
        log
        vulkan
        OpenSLES
        aaudio
        z
        pugixml
        xenia-patcher
        adrenotools
)

#xenia-patcher