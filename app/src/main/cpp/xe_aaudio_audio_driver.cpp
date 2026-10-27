/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */
#include "xe_aaudio_audio_driver.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

#include "xenia/apu/apu_flags.h"
#include "xenia/apu/conversion.h"
#include "xenia/base/assert.h"
#include "xenia/base/cvar.h"
#include "xenia/base/logging.h"
#include "xenia/base/profiling.h"

// 缓冲深度（设备 burst 数）。低延迟流默认只给设备允许的最浅缓冲，
// 模拟器一抖动就欠载断音；加深到 4 个 burst 可以扛住抖动。
DEFINE_uint32(
    apu_aaudio_buffer_bursts, 4,
    "Depth of the Android audio buffer, in device bursts. Higher rides out "
    "emulator slowdowns without dropping audio, at the cost of latency.",
    "APU");
DEFINE_bool(apu_aaudio_log_stats, false,
            "Log Android audio callback statistics (gaps, queue depth, "
            "underruns) once a second.",
            "APU");

namespace xe {
namespace apu {
namespace aaudio {

static constexpr uint32_t kStatsIntervalMs = 1000;

AAudioAudioDriver::AAudioAudioDriver(Memory* memory,
                                     xe::threading::Semaphore* semaphore,
                                     uint32_t frequency, uint32_t channels,
                                     bool need_format_conversion)
    : semaphore_(semaphore),
      frame_frequency_(frequency),
      frame_channels_(channels),
      need_format_conversion_(need_format_conversion),
      channel_samples_(channels == 6 ? 256 : 768),
      submit_samples_(channels * (channels == 6 ? 256 : 768)),
      host_block_samples_(host_frame_channels_ *
                          (channels == 6 ? 256 : 768)) {
  assert_true(channels == 6 || channels == 2);
  last_block_.resize(host_block_samples_, 0.0f);
}

AAudioAudioDriver::~AAudioAudioDriver() {
  assert_true(frames_queued_.empty());
  assert_true(frames_unused_.empty());
}

bool AAudioAudioDriver::Initialize() {
  {
    std::unique_lock<std::mutex> guard(frames_mutex_);
    for (int i = 0; i < 2; i++) {
      float* buffer = new float[submit_samples_];
      frames_unused_.push(buffer);
    }
  }

  {
    std::unique_lock<std::mutex> stream_guard(stream_mutex_);
    if (!BuildStream()) {
      return false;
    }
  }

  // 负责在输出设备变化时重建流的工作线程。
  recovery_thread_ = std::thread(&AAudioAudioDriver::RecoveryThreadMain, this);
  return true;
}

bool AAudioAudioDriver::BuildStream() {
  // 在当前默认设备上打开（不 setDeviceId）；新设备不给独占 MMAP 流时
  // 回退到 SHARED。
  const aaudio_sharing_mode_t modes[] = {AAUDIO_SHARING_MODE_EXCLUSIVE,
                                         AAUDIO_SHARING_MODE_SHARED};
  for (aaudio_sharing_mode_t mode : modes) {
    aaudio_result_t result = AAudio_createStreamBuilder(&builder_);
    if (result != AAUDIO_OK) {
      XELOGE("AAudio_createStreamBuilder failed: {}", result);
      return false;
    }

    AAudioStreamBuilder_setFormat(builder_, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setSampleRate(builder_, frame_frequency_);
    AAudioStreamBuilder_setChannelCount(builder_, host_frame_channels_);
    AAudioStreamBuilder_setFramesPerDataCallback(builder_, channel_samples_);
    AAudioStreamBuilder_setDataCallback(builder_, AudioCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder_, AudioErrorCallback, this);
    AAudioStreamBuilder_setPerformanceMode(builder_,
                                           AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setSharingMode(builder_, mode);
    // 低延迟流否则会被授予设备允许的最浅缓冲。AAudio 要求容量 >= 回调
    // 大小的两倍。
    AAudioStreamBuilder_setBufferCapacityInFrames(
        builder_, channel_samples_ * cvars::apu_aaudio_buffer_bursts);

    result = AAudioStreamBuilder_openStream(builder_, &stream_);
    if (result != AAUDIO_OK) {
      XELOGE("AAudioStreamBuilder_openStream ({}) failed: {}",
             mode == AAUDIO_SHARING_MODE_EXCLUSIVE ? "exclusive" : "shared",
             result);
      AAudioStreamBuilder_delete(builder_);
      builder_ = nullptr;
      continue;
    }

    result = AAudioStream_requestStart(stream_);
    if (result != AAUDIO_OK) {
      XELOGE("AAudioStream_requestStart failed: {}", result);
      AAudioStream_close(stream_);
      stream_ = nullptr;
      AAudioStreamBuilder_delete(builder_);
      builder_ = nullptr;
      continue;
    }

    // 以 burst 与实际回调大小的较大者为准：设备可能 burst 只有 96 却按
    // 256 回调，只用 burst 会剩不到两个回调的余量。
    const int32_t burst = AAudioStream_getFramesPerBurst(stream_);
    const int32_t callback_frames =
        AAudioStream_getFramesPerDataCallback(stream_);
    const int32_t drain = std::max(
        burst, callback_frames > 0 ? callback_frames
                                   : static_cast<int32_t>(channel_samples_));
    if (drain > 0) {
      AAudioStream_setBufferSizeInFrames(
          stream_, drain * cvars::apu_aaudio_buffer_bursts);
    }

    // 请求与实际获得的配置经常不一致，打印出来便于排查。
    XELOGI(
        "AAudio: {} stream, rate {}, {} ch, burst {}, callback {} (asked {}), "
        "buffer {}/{} frames, perf mode {}",
        mode == AAUDIO_SHARING_MODE_EXCLUSIVE ? "exclusive" : "shared",
        AAudioStream_getSampleRate(stream_),
        AAudioStream_getChannelCount(stream_), burst,
        AAudioStream_getFramesPerDataCallback(stream_), channel_samples_,
        AAudioStream_getBufferSizeInFrames(stream_),
        AAudioStream_getBufferCapacityInFrames(stream_),
        AAudioStream_getPerformanceMode(stream_));

    stream_initialized_ = true;
    return true;
  }
  return false;
}

void AAudioAudioDriver::Pause() {
  std::unique_lock<std::mutex> stream_guard(stream_mutex_);
  if (stream_initialized_ && stream_) {
    AAudioStream_requestPause(stream_);
  }
}

void AAudioAudioDriver::Resume() {
  std::unique_lock<std::mutex> stream_guard(stream_mutex_);
  if (stream_initialized_ && stream_) {
    AAudioStream_requestStart(stream_);
  }
}

void AAudioAudioDriver::SetVolume(float volume) {
  // AAudio 无每流音量；在回调里对样本施加。
  driver_volume_.store(std::clamp(volume, 0.0f, 1.0f),
                       std::memory_order_relaxed);
}

aaudio_data_callback_result_t AAudioAudioDriver::AudioCallback(
    AAudioStream* stream,
    void* userdata,
    void* audioData,
    int32_t numFrames) {
  SCOPE_profile_cpu_f("apu");

  auto driver = static_cast<AAudioAudioDriver*>(userdata);
  float* output_buffer = reinterpret_cast<float*>(audioData);

  // setFramesPerDataCallback 只是一个请求，在 shared 回退和每次重建时
  // 都可能被重新协商。下面的转换以 channel_samples_ 为源步长，因此无论
  // 本次回调给多少帧都必须按该尺寸运行。
  const int32_t out_samples = numFrames * host_frame_channels_;
  if (numFrames != static_cast<int32_t>(driver->channel_samples_)) {
    driver->stat_unexpected_frames_.store(numFrames, std::memory_order_relaxed);
  }

  driver->stat_callbacks_.fetch_add(1, std::memory_order_relaxed);

  // 填满整个请求，需要多少个 guest 块都行：回调大小不等于
  // channel_samples_ 时否则会以静音填充或丢弃块的剩余部分。
  int32_t frames_done = 0;
  uint32_t releases = 0;
  bool gapped = false;
  while (frames_done < numFrames) {
    if (driver->last_block_pos_ >= driver->channel_samples_) {
      float* buffer = nullptr;
      uint32_t depth = 0;
      {
        // 只在出队期间持有。
        std::unique_lock<std::mutex> guard(driver->frames_mutex_);
        depth = static_cast<uint32_t>(driver->frames_queued_.size());
        if (!driver->frames_queued_.empty()) {
          buffer = driver->frames_queued_.front();
          driver->frames_queued_.pop();
        }
      }
      driver->stat_queue_depth_sum_.fetch_add(depth, std::memory_order_relaxed);
      if (depth > driver->stat_queue_depth_max_.load(std::memory_order_relaxed)) {
        driver->stat_queue_depth_max_.store(depth, std::memory_order_relaxed);
      }
      if (!buffer) {
        // 只掩蔽还欠着的部分。
        driver->stat_gaps_.fetch_add(1, std::memory_order_relaxed);
        gapped = true;
        // ConcealGap 重复 last_block_，因此最多只能提供一个块的源。
        const int32_t done_samples = frames_done * host_frame_channels_;
        const int32_t owed_samples = out_samples - done_samples;
        driver->ConcealGap(
            output_buffer + done_samples, owed_samples,
            std::min(owed_samples, static_cast<int32_t>(driver->host_block_samples_)));
        break;
      }
      if (driver->frame_channels_ == 6) {
        conversion::sequential_6_BE_to_interleaved_2_LE(
            driver->last_block_.data(), buffer, driver->channel_samples_);
      } else {
        // 媒体播放器：已经是交织的主机端序立体声。
        std::memcpy(driver->last_block_.data(), buffer,
                    driver->host_block_samples_ * sizeof(float));
      }
      driver->ApplyGainAndClamp();
      driver->ApplyFadeIn();
      driver->last_block_valid_ = true;
      driver->last_block_pos_ = 0;
      driver->gap_blocks_ = 0;
      {
        std::unique_lock<std::mutex> guard(driver->frames_mutex_);
        driver->frames_unused_.push(buffer);
      }
      ++releases;
    }
    const int32_t chunk = std::min<int32_t>(
        numFrames - frames_done,
        static_cast<int32_t>(driver->channel_samples_ -
                             driver->last_block_pos_));
    std::memcpy(
        output_buffer + frames_done * host_frame_channels_,
        driver->last_block_.data() +
            driver->last_block_pos_ * host_frame_channels_,
        chunk * host_frame_channels_ * sizeof(float));
    driver->last_block_pos_ += static_cast<uint32_t>(chunk);
    frames_done += chunk;
  }

  // 每消费一个块释放一次节拍，这样回调大小不是整块时节拍仍然成立。
  // 欠载时也释放一次，保持 guest 音频引擎继续运转。
  if (releases == 0 && gapped) {
    releases = 1;
  }
  for (uint32_t i = 0; i < releases; ++i) {
    driver->semaphore_->Release(1, nullptr);
  }

  return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AAudioAudioDriver::ConcealGap(float* output, int32_t out_samples,
                                   int32_t copy_samples) {
  // 尚无可重复的块：刚启动，或刚静音过。
  if (!last_block_valid_) {
    std::memset(output, 0, out_samples * sizeof(float));
    return;
  }

  // 重复上一个块并衰减：持平会在块率上嗡嗡作响，衰减则是渐隐而不是
  // 突然砸到静音。
  const float start_gain = std::pow(0.6f, static_cast<float>(gap_blocks_));
  if (start_gain < 0.002f) {
    std::memset(output, 0, out_samples * sizeof(float));
    last_block_valid_ = false;
    gap_blocks_++;
    fade_in_pending_ = true;
    return;
  }
  const float end_gain = start_gain * 0.6f;
  const int32_t frames = copy_samples / host_frame_channels_;
  const float step = frames > 0 ? (end_gain - start_gain) / frames : 0.0f;

  for (int32_t f = 0; f < frames; ++f) {
    const float g = start_gain + step * f;
    output[f * 2 + 0] = last_block_[f * 2 + 0] * g;
    output[f * 2 + 1] = last_block_[f * 2 + 1] * g;
  }
  if (out_samples > copy_samples) {
    std::memset(output + copy_samples, 0,
                (out_samples - copy_samples) * sizeof(float));
  }
  gap_blocks_++;
  fade_in_pending_ = true;
}

void AAudioAudioDriver::ApplyGainAndClamp() {
  const float gain =
      driver_volume_.load(std::memory_order_relaxed);

  // 5.1->2.0 折叠的峰值增益约 2.9，响的内容会超出满刻度。在这里限幅，
  // 保证每台设备上结果一致；统计它：持续削波的修法是降增益而非更狠的限。
  uint32_t clipped = 0;
  for (uint32_t i = 0; i < host_block_samples_; ++i) {
    float s = last_block_[i] * gain;
    if (s > 1.0f) {
      s = 1.0f;
      ++clipped;
    } else if (s < -1.0f) {
      s = -1.0f;
      ++clipped;
    }
    last_block_[i] = s;
  }
  if (clipped) {
    stat_clipped_.fetch_add(clipped, std::memory_order_relaxed);
  }
}

void AAudioAudioDriver::ApplyFadeIn() {
  if (!fade_in_pending_) {
    return;
  }
  fade_in_pending_ = false;
  // 第一个真实块渐入，恢复沿是斜坡而不是台阶。64 帧约 1.3ms：作为电平
  // 变化不可闻，足以消除咔哒声。
  const int32_t ramp = 64;
  for (int32_t f = 0; f < ramp; ++f) {
    const float g = static_cast<float>(f) / ramp;
    last_block_[f * 2 + 0] *= g;
    last_block_[f * 2 + 1] *= g;
  }
}

void AAudioAudioDriver::LogAndResetStats() {
  const uint64_t callbacks = stat_callbacks_.exchange(0, std::memory_order_relaxed);
  if (!callbacks) {
    return;
  }
  const uint64_t gaps = stat_gaps_.exchange(0, std::memory_order_relaxed);
  const uint64_t depth_sum =
      stat_queue_depth_sum_.exchange(0, std::memory_order_relaxed);
  const uint32_t depth_max =
      stat_queue_depth_max_.exchange(0, std::memory_order_relaxed);
  const int32_t odd_frames =
      stat_unexpected_frames_.exchange(0, std::memory_order_relaxed);
  const uint64_t clipped = stat_clipped_.exchange(0, std::memory_order_relaxed);
  const uint64_t played = (callbacks - gaps) * host_block_samples_;

  int32_t xruns = -1;
  {
    std::unique_lock<std::mutex> stream_guard(stream_mutex_);
    if (stream_initialized_ && stream_) {
      xruns = AAudioStream_getXRunCount(stream_);
    }
  }

  XELOGI(
      "AAudio: {} cb, {} gaps ({:.1f}%), queue avg {:.2f} max {}, xruns {}, "
      "clipped {} ({:.3f}%){}",
      callbacks, gaps, 100.0 * double(gaps) / double(callbacks),
      double(depth_sum) / double(callbacks), depth_max, xruns, clipped,
      played ? 100.0 * double(clipped) / double(played) : 0.0,
      odd_frames ? fmt::format(", UNEXPECTED framesPerCallback {}", odd_frames)
                 : "");
}

void AAudioAudioDriver::AudioErrorCallback(
    AAudioStream* stream,
    void* userdata,
    aaudio_result_t error) {
  auto driver = static_cast<AAudioAudioDriver*>(userdata);
  // 路由变化（耳机/蓝牙）会断开流；必须在新默认设备上重开。AAudio 禁止
  // 在本回调里 close 流，所以只给 recovery_thread_ 置一个请求。
  XELOGW("AAudio stream error: {} - requesting stream rebuild", error);
  {
    std::lock_guard<std::mutex> lk(driver->recovery_mutex_);
    driver->restart_requested_ = true;
  }
  driver->recovery_cv_.notify_one();
}

void AAudioAudioDriver::RecoveryThreadMain() {
  bool retry_pending = false;
  for (;;) {
    {
      std::unique_lock<std::mutex> lk(recovery_mutex_);
      auto wake = [this] { return restart_requested_ || recovery_quit_; };
      // 重建失败后没有流再来触发我们了，所以轮询重试。
      // 回调线程不能打日志，由它发布、这里打。
      if (retry_pending) {
        recovery_cv_.wait_for(lk, std::chrono::milliseconds(250), wake);
      } else {
        recovery_cv_.wait_for(lk, std::chrono::milliseconds(kStatsIntervalMs),
                              wake);
      }
      if (recovery_quit_) {
        return;
      }
      if (!restart_requested_) {
        lk.unlock();
        if (cvars::apu_aaudio_log_stats) {
          LogAndResetStats();
        }
        continue;
      }
      restart_requested_ = false;
    }
    retry_pending = !RestartStream();
  }
}

bool AAudioAudioDriver::RestartStream() {
  std::unique_lock<std::mutex> stream_guard(stream_mutex_);
  XELOGW("AAudio: rebuilding stream on the current default output device");
  // 在这里（而非回调线程）是安全的：close() 会阻塞到数据回调返回。
  if (stream_) {
    AAudioStream_requestStop(stream_);
    AAudioStream_close(stream_);
    stream_ = nullptr;
  }
  if (builder_) {
    AAudioStreamBuilder_delete(builder_);
    builder_ = nullptr;
  }
  stream_initialized_ = false;

  // 正在关停：只拆除，不重开。
  if (shutting_down_.load(std::memory_order_acquire)) {
    return true;
  }

  // 新流的数据回调会自行恢复节拍信号量，AudioSystem 工作线程无需任何
  // 改动就能恢复。
  if (BuildStream()) {
    XELOGI("AAudio: stream rebuilt; audio output restored");
    return true;
  }
  XELOGE("AAudio: stream rebuild failed; will retry");
  return false;
}

void AAudioAudioDriver::SubmitFrame(float* samples) {
  float* output_frame;
  {
    std::unique_lock<std::mutex> guard(frames_mutex_);
    if (frames_unused_.empty()) {
      output_frame = new float[submit_samples_];
    } else {
      output_frame = frames_unused_.top();
      frames_unused_.pop();
    }
  }

  std::memcpy(output_frame, samples, submit_samples_ * sizeof(float));

  {
    std::unique_lock<std::mutex> guard(frames_mutex_);
    frames_queued_.push(output_frame);
  }
}

void AAudioAudioDriver::Shutdown() {
  // 先停掉并 join 恢复工作线程再拆流，确保没有重建在途。
  shutting_down_.store(true, std::memory_order_release);
  {
    std::lock_guard<std::mutex> lk(recovery_mutex_);
    recovery_quit_ = true;
  }
  recovery_cv_.notify_one();
  if (recovery_thread_.joinable()) {
    recovery_thread_.join();
  }

  {
    std::unique_lock<std::mutex> stream_guard(stream_mutex_);
    if (stream_) {
      AAudioStream_requestStop(stream_);
      AAudioStream_close(stream_);
      stream_ = nullptr;
    }

    if (builder_) {
      AAudioStreamBuilder_delete(builder_);
      builder_ = nullptr;
    }

    stream_initialized_ = false;
  }

  std::unique_lock<std::mutex> guard(frames_mutex_);
  while (!frames_unused_.empty()) {
    delete[] frames_unused_.top();
    frames_unused_.pop();
  }

  while (!frames_queued_.empty()) {
    delete[] frames_queued_.front();
    frames_queued_.pop();
  }
}

}  // namespace aaudio
}  // namespace apu
}  // namespace xe
