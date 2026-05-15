/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */
#ifndef AX360E_XE_OPENSLES_AUDIO_SYSTEM_H
#define AX360E_XE_OPENSLES_AUDIO_SYSTEM_H

#include "xenia/apu/audio_system.h"

namespace xe {
    namespace apu {
        namespace opensles {

            class OpenSLESAudioSystem : public AudioSystem {
            public:
                explicit OpenSLESAudioSystem(cpu::Processor *processor);

                ~OpenSLESAudioSystem() override;

                std::string name() const override;
                static bool IsAvailable() { return true; }

                static std::unique_ptr<AudioSystem> Create(cpu::Processor *processor);

                X_STATUS CreateDriver(size_t index, xe::threading::Semaphore *semaphore,
                                      AudioDriver **out_driver) override;

                AudioDriver* CreateDriver(xe::threading::Semaphore* semaphore,
                                          uint32_t frequency, uint32_t channels,
                                          bool need_format_conversion) override;

                void DestroyDriver(AudioDriver *driver) override;
            };
        }
    }
}
#endif //AX360E_XE_OPENSLES_AUDIO_SYSTEM_H
