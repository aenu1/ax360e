/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */
#include "xe_opensles_audio_system.h"
#include "xe_opensles_audio_driver.h"
namespace xe {
namespace apu {
namespace opensles {
OpenSLESAudioSystem::OpenSLESAudioSystem(cpu::Processor *processor)
    : AudioSystem(processor) {}


    OpenSLESAudioSystem::~OpenSLESAudioSystem() = default;


    std::string OpenSLESAudioSystem::name() const{
        return "OpenSLES";
    }

    X_STATUS OpenSLESAudioSystem::CreateDriver(size_t index,
                                          xe::threading::Semaphore* semaphore,
                                          AudioDriver** out_driver) {
        // return X_STATUS_NOT_IMPLEMENTED;
        auto driver = std::make_unique<OpenSLESAudioDriver>(memory_, semaphore);
        if (!driver->Initialize()) {
            driver->Shutdown();
            return X_STATUS_UNSUCCESSFUL;
        }

        *out_driver = driver.release();
        return X_STATUS_SUCCESS;
    }

    AudioDriver* OpenSLESAudioSystem::CreateDriver(xe::threading::Semaphore* semaphore,
                                                 uint32_t frequency, uint32_t channels,
                                                 bool need_format_conversion){
        //FIXME
        return new OpenSLESAudioDriver(memory_, semaphore);
    }

    void OpenSLESAudioSystem::DestroyDriver(AudioDriver* driver) {
        assert_not_null(driver);
        auto opensles_driver = dynamic_cast<OpenSLESAudioDriver*>(driver);
        assert_not_null(opensles_driver);
        opensles_driver->Shutdown();
        delete opensles_driver;
    }
}
}
}