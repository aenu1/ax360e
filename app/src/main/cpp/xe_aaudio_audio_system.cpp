/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xe_aaudio_audio_system.h"
#include "xe_aaudio_audio_driver.h"
namespace xe {
    namespace apu {
        namespace aaudio {
            AAudioAudioSystem::AAudioAudioSystem(cpu::Processor *processor)
                    : AudioSystem(processor) {}


            AAudioAudioSystem::~AAudioAudioSystem() = default;

            std::string AAudioAudioSystem::name() const{
                return "AAudio";
            }
            X_STATUS AAudioAudioSystem::CreateDriver(size_t index,
                                                     xe::threading::Semaphore *semaphore,
                                                     AudioDriver **out_driver) {
                auto driver = std::make_unique<AAudioAudioDriver>(memory_, semaphore);
                if (!driver->Initialize()) {
                    driver->Shutdown();
                    return X_STATUS_UNSUCCESSFUL;
                }

                *out_driver = driver.release();
                return X_STATUS_SUCCESS;
            }

            void AAudioAudioSystem::DestroyDriver(AudioDriver *driver) {
                assert_not_null(driver);
                auto aaudio_driver = dynamic_cast<AAudioAudioDriver *>(driver);
                assert_not_null(aaudio_driver);
                aaudio_driver->Shutdown();
                delete aaudio_driver;
            }

            AudioDriver* AAudioAudioSystem::CreateDriver(xe::threading::Semaphore* semaphore,
                                      uint32_t frequency, uint32_t channels,
                                      bool need_format_conversion){
                //FIXME
                return new AAudioAudioDriver(memory_, semaphore);
            }
        }
    }
}