/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xe_android_input_driver.h"
#include "xenia/ui/virtual_key.h"
#include "xenia/base/logging.h"

namespace xe {
    namespace hid {
        namespace android {
            AndroidInputDriver::AndroidInputDriver(xe::ui::Window* window, size_t window_z_order)
                    : InputDriver(window, window_z_order)
                    ,key_status_({
                                         {ui::VirtualKey::kXInputPadDpadLeft,false,0},
                                         {ui::VirtualKey::kXInputPadDpadUp,false,0},
                                         {ui::VirtualKey::kXInputPadDpadRight,false,0},
                                         {ui::VirtualKey::kXInputPadDpadDown,false,0},
                                         {ui::VirtualKey::kXInputPadA,false,0},
                                         {ui::VirtualKey::kXInputPadB,false,0},
                                         {ui::VirtualKey::kXInputPadX,false,0},
                                         {ui::VirtualKey::kXInputPadY,false,0},
                                         {ui::VirtualKey::kXInputPadBack,false,0},
                                         {ui::VirtualKey::kXInputPadStart,false,0},

                                         {ui::VirtualKey::kXInputPadLShoulder,false,0},
                                         {ui::VirtualKey::kXInputPadRShoulder,false,0},
                                         {ui::VirtualKey::kXInputPadLThumbPress,false,0},
                                         {ui::VirtualKey::kXInputPadRThumbPress,false,0},
                                         {ui::VirtualKey::kXInputPadLTrigger,false,0},
                                         {ui::VirtualKey::kXInputPadRTrigger,false,0},

                                         {ui::VirtualKey::kXInputPadLThumbLeft,false,0},
                                         {ui::VirtualKey::kXInputPadLThumbUp,false,0},
                                         {ui::VirtualKey::kXInputPadLThumbRight,false,0},
                                         {ui::VirtualKey::kXInputPadLThumbDown,false,0},
                                         {ui::VirtualKey::kXInputPadRThumbLeft,false,0},
                                         {ui::VirtualKey::kXInputPadRThumbUp,false,0},
                                         {ui::VirtualKey::kXInputPadRThumbRight,false,0},
                                         {ui::VirtualKey::kXInputPadRThumbDown,false,0},
                                 }),prev_key_status_(this->key_status_){
            }

            AndroidInputDriver::~AndroidInputDriver()=default;

            X_STATUS AndroidInputDriver::Setup() { return X_STATUS_SUCCESS; }

            X_RESULT AndroidInputDriver::GetCapabilities(uint32_t user_index, uint32_t flags,X_INPUT_CAPABILITIES* out_caps) {
                //XELOGI("AID:GetCapabilities {} {}",user_index, flags);
                if (user_index != 0) {
                    return X_ERROR_DEVICE_NOT_CONNECTED;
                }

                // TODO(benvanik): confirm with a real XInput controller.
                out_caps->type = 0x01;      // XINPUT_DEVTYPE_GAMEPAD
                out_caps->sub_type = 0x01;  // XINPUT_DEVSUBTYPE_GAMEPAD
                out_caps->flags = 0;
                out_caps->gamepad.buttons = 0xFFFF;
                out_caps->gamepad.left_trigger = 0xFF;
                out_caps->gamepad.right_trigger = 0xFF;
                out_caps->gamepad.thumb_lx = (int16_t)0xFFFFu;
                out_caps->gamepad.thumb_ly = (int16_t)0xFFFFu;
                out_caps->gamepad.thumb_rx = (int16_t)0xFFFFu;
                out_caps->gamepad.thumb_ry = (int16_t)0xFFFFu;
                out_caps->vibration.left_motor_speed = 0;
                out_caps->vibration.right_motor_speed = 0;
                return X_ERROR_SUCCESS;
            }

            X_RESULT AndroidInputDriver::GetState(uint32_t user_index,X_INPUT_STATE* out_state) {
                //XELOGI("AID:GetState {}",user_index);
                if (user_index != 0) {
                    return X_ERROR_DEVICE_NOT_CONNECTED;
                }
                //XELOGI("AID:window()->HasFocus() is_active() {} {}",window()->HasFocus(),is_active());
                packet_number_++;

                uint16_t buttons = 0;
                uint8_t left_trigger = 0;
                uint8_t right_trigger = 0;
                int16_t thumb_lx = 0;
                int16_t thumb_ly = 0;
                int16_t thumb_rx = 0;
                int16_t thumb_ry = 0;


                if (/*window()->HasFocus() && */1) {
                    //XELOGI("AID:GetState:HasFocus");
                    for (const KeyStatus& ks : key_status_) {
                        if (!ks.pressed) continue;
                        switch (ks.id) {
                            case ui::VirtualKey::kXInputPadA:
                                buttons |= 0x1000;  // XINPUT_GAMEPAD_A
                                break;
                            case ui::VirtualKey::kXInputPadY:
                                buttons |= 0x8000;  // XINPUT_GAMEPAD_Y
                                break;
                            case ui::VirtualKey::kXInputPadB:
                                buttons |= 0x2000;  // XINPUT_GAMEPAD_B
                                break;
                            case ui::VirtualKey::kXInputPadX:
                                buttons |= 0x4000;  // XINPUT_GAMEPAD_X
                                break;
                            case ui::VirtualKey::kXInputPadDpadLeft:
                                buttons |= 0x0004;  // XINPUT_GAMEPAD_DPAD_LEFT
                                break;
                            case ui::VirtualKey::kXInputPadDpadRight:
                                buttons |= 0x0008;  // XINPUT_GAMEPAD_DPAD_RIGHT
                                break;
                            case ui::VirtualKey::kXInputPadDpadDown:
                                buttons |= 0x0002;  // XINPUT_GAMEPAD_DPAD_DOWN
                                break;
                            case ui::VirtualKey::kXInputPadDpadUp:
                                buttons |= 0x0001;  // XINPUT_GAMEPAD_DPAD_UP
                                break;
                            case ui::VirtualKey::kXInputPadRThumbPress:
                                buttons |= 0x0080;  // XINPUT_GAMEPAD_RIGHT_THUMB
                                break;
                            case ui::VirtualKey::kXInputPadLThumbPress:
                                buttons |= 0x0040;  // XINPUT_GAMEPAD_LEFT_THUMB
                                break;
                            case ui::VirtualKey::kXInputPadBack:
                                buttons |= 0x0020;  // XINPUT_GAMEPAD_BACK
                                break;
                            case ui::VirtualKey::kXInputPadStart:
                                buttons |= 0x0010;  // XINPUT_GAMEPAD_START
                                break;
                            case ui::VirtualKey::kXInputPadLShoulder:
                                buttons |= 0x0100;  // XINPUT_GAMEPAD_LEFT_SHOULDER
                                break;
                            case ui::VirtualKey::kXInputPadRShoulder:
                                buttons |= 0x0200;  // XINPUT_GAMEPAD_RIGHT_SHOULDER
                                break;
                            case ui::VirtualKey::kXInputPadLTrigger:
                                left_trigger = 0xFF;
                                break;
                            case ui::VirtualKey::kXInputPadRTrigger:
                                right_trigger = 0xFF;
                                break;
                            case ui::VirtualKey::kXInputPadLThumbLeft:
                                thumb_lx =ks.value;//+= SHRT_MIN;
                                break;
                            case ui::VirtualKey::kXInputPadLThumbRight:
                                thumb_lx =ks.value;//+= SHRT_MAX;
                                break;
                            case ui::VirtualKey::kXInputPadLThumbDown:
                                thumb_ly =ks.value;//+= SHRT_MIN;
                                break;
                            case ui::VirtualKey::kXInputPadLThumbUp:
                                thumb_ly =ks.value;//+= SHRT_MAX;
                                break;
                            case ui::VirtualKey::kXInputPadRThumbUp:
                                thumb_ry =ks.value;//+= SHRT_MAX;
                                break;
                            case ui::VirtualKey::kXInputPadRThumbDown:
                                thumb_ry =ks.value;//+= SHRT_MIN;
                                break;
                            case ui::VirtualKey::kXInputPadRThumbRight:
                                thumb_rx =ks.value;//+= SHRT_MAX;
                                break;
                            case ui::VirtualKey::kXInputPadRThumbLeft:
                                thumb_rx =ks.value;//+= SHRT_MIN;
                                break;
                        }
                    }
                }

                out_state->packet_number = packet_number_;
                out_state->gamepad.buttons = buttons;
                out_state->gamepad.left_trigger = left_trigger;
                out_state->gamepad.right_trigger = right_trigger;
                out_state->gamepad.thumb_lx = thumb_lx;
                out_state->gamepad.thumb_ly = thumb_ly;
                out_state->gamepad.thumb_rx = thumb_rx;
                out_state->gamepad.thumb_ry = thumb_ry;

                return X_ERROR_SUCCESS;
            }

            X_RESULT AndroidInputDriver::SetState(uint32_t user_index,X_INPUT_VIBRATION* vibration) {
                //XELOGI( "AID:SetState {}", user_index );
                if (user_index != 0) {
                    return X_ERROR_DEVICE_NOT_CONNECTED;
                }

                return X_ERROR_SUCCESS;
            }

            X_RESULT AndroidInputDriver::GetKeystroke(uint32_t user_index, uint32_t flags,X_INPUT_KEYSTROKE* out_keystroke) {
                //XELOGI( "AID:GetKeystroke {} {}", user_index, flags );
                if (user_index != 0) {
                    return X_ERROR_DEVICE_NOT_CONNECTED;
                }

                /*if (!is_active()) {
                    return X_ERROR_EMPTY;
                }*/

                X_RESULT result = X_ERROR_EMPTY;

                ui::VirtualKey xinput_virtual_key = ui::VirtualKey::kNone;
                uint16_t unicode = 0;
                uint16_t keystroke_flags = 0;
                uint8_t hid_code = 0;
                int key_status_index=-1;

                {
                    auto global_lock = global_critical_region_.Acquire();
                    if (key_status_mask_==0) {
                        // No keys!
                        return X_ERROR_EMPTY;
                    }
                    for(size_t i = 0; i < key_status_.size(); i++){
                        if(key_status_mask_&(1<<i)){
                            xinput_virtual_key=key_status_[i].id;
                            key_status_index=i;
                            key_status_mask_&=~(1<<i);
                        }
                    }
                }

                if (xinput_virtual_key != ui::VirtualKey::kNone) {
                    if (key_status_[key_status_index].pressed) {
                        keystroke_flags |= 0x0001;  // XINPUT_KEYSTROKE_KEYDOWN
                    } else {
                        keystroke_flags |= 0x0002;  // XINPUT_KEYSTROKE_KEYUP
                    }

                    if (prev_key_status_[key_status_index].pressed == key_status_[key_status_index].pressed) {
                        keystroke_flags |= 0x0004;  // XINPUT_KEYSTROKE_REPEAT
                    }

                    result = X_ERROR_SUCCESS;
                }

                out_keystroke->virtual_key = uint16_t(xinput_virtual_key);
                out_keystroke->unicode = unicode;
                out_keystroke->flags = keystroke_flags;
                out_keystroke->user_index = 0;
                out_keystroke->hid_code = hid_code;

                // X_ERROR_EMPTY if no new keys
                // X_ERROR_DEVICE_NOT_CONNECTED if no device
                // X_ERROR_SUCCESS if key
                return result;
            }


            void AndroidInputDriver::OnKey(int key_index, bool pressed, short value){
                /*if (!is_active()) {
                    return;
                }*/

                auto global_lock = global_critical_region_.Acquire();
                prev_key_status_[key_index] = key_status_[key_index];

                key_status_[key_index].pressed = pressed;
                key_status_[key_index].value = value;

                key_status_mask_ |= (1 << key_index);
            }


            InputType AndroidInputDriver::GetInputType() const { return InputType::Controller; }

        }
    }
}
