/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */
#ifndef AX360E_XE_AAUDIO_AUDIO_DRIVER_H
#define AX360E_XE_AAUDIO_AUDIO_DRIVER_H

#include <atomic>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stack>
#include <thread>

#include <aaudio/AAudio.h>

#include "xenia/apu/audio_driver.h"
#include "xenia/base/threading.h"

namespace xe {
namespace apu {
namespace aaudio {

class AAudioAudioDriver : public AudioDriver {
 public:
  // channels 选择提交格式（与 SDLAudioDriver 对齐）：6 = 游戏路径，每声道
  // 256 个大端序顺序样本的 5.1；2 = 媒体播放器，按歌曲自身采样率交织的
  // 主机端序立体声，768 帧。两者每次 SubmitFrame 都是 1536 个 float。
  AAudioAudioDriver(Memory* memory, xe::threading::Semaphore* semaphore,
                    uint32_t frequency = 48000, uint32_t channels = 6,
                    bool need_format_conversion = true);
  ~AAudioAudioDriver() override;

  bool Initialize();
    void Pause() override;
    void Resume() override;
    void SetVolume(float volume) override;
  void SubmitFrame(float* frame) override;
  void Shutdown();

 protected:
  static aaudio_data_callback_result_t AudioCallback(
      AAudioStream* stream,
      void* userdata,
      void* audioData,
      int32_t numFrames);

  static void AudioErrorCallback(
      AAudioStream* stream,
      void* userdata,
      aaudio_result_t error);

  // 仅回调线程使用。
  void ConcealGap(float* output, int32_t out_samples, int32_t copy_samples);
  void ApplyFadeIn();
  // AAudio 没有流音量控制，只能软件实现。仅回调线程使用。
  void ApplyGainAndClamp();

  // （重新）在当前默认输出设备上打开流；调用者持有 stream_mutex_。
  bool BuildStream();
  // 在当前默认设备上重建流；有可用流（或正在关停）时返回 true。
  // 仅在 recovery_thread_ 上运行。
  bool RestartStream();
  // 处理错误回调发来的重建请求，失败时重试。
  void RecoveryThreadMain();

  xe::threading::Semaphore* semaphore_ = nullptr;

  AAudioStreamBuilder* builder_ = nullptr;
  AAudioStream* stream_ = nullptr;
  bool stream_initialized_ = false;
  // 串行化流生命周期。数据回调使用其 `stream` 参数而非 stream_，
  // 因此从不加这把锁。
  std::mutex stream_mutex_ = {};

  // 输出设备切换恢复：错误回调置位请求，recovery_thread_ 负责实际的
  // close+reopen（AAudio 禁止在回调线程里 close）。Initialize 中启动，
  // Shutdown 中 join。
  std::thread recovery_thread_ = {};
  std::mutex recovery_mutex_ = {};
  std::condition_variable recovery_cv_ = {};
  bool restart_requested_ = false;
  bool recovery_quit_ = false;
  std::atomic<bool> shutting_down_{false};

  static constexpr uint32_t host_frame_channels_ = 2;
  // 由构造契约固定，见构造函数注释。
  const uint32_t frame_frequency_;
  const uint32_t frame_channels_;
  const bool need_format_conversion_;
  const uint32_t channel_samples_;
  const uint32_t submit_samples_;
  const uint32_t host_block_samples_;
  std::queue<float*> frames_queued_ = {};
  std::stack<float*> frames_unused_ = {};
  std::mutex frames_mutex_ = {};

  // 欠载掩蔽：静音会在每个缺口的两侧都产生台阶，5.3ms 一个块相当于
  // ~187Hz 的咔哒声列。仅回调线程使用。
  std::vector<float> last_block_;
  bool last_block_valid_ = false;
  // last_block_ 已交给设备的帧数。回调大小若不恰好等于 channel_samples_，
  // 否则会丢音或重播。仅回调线程使用。
  uint32_t last_block_pos_ = channel_samples_;
  uint32_t gap_blocks_ = 0;

  bool fade_in_pending_ = false;

  // 实时回调写入，recovery_thread_ 读取：只用 relaxed 原子量，
  // 不放任何可能阻塞回调的东西。
  std::atomic<uint64_t> stat_callbacks_{0};
  std::atomic<uint64_t> stat_gaps_{0};
  std::atomic<uint64_t> stat_queue_depth_sum_{0};
  std::atomic<uint32_t> stat_queue_depth_max_{0};
  // 一个未经请求的块大小会悄悄改变排空速率。
  std::atomic<int32_t> stat_unexpected_frames_{0};
  // 降混超出满刻度的样本数。
  std::atomic<uint64_t> stat_clipped_{0};
  void LogAndResetStats();

  // 每驱动音量（XMP）：其他线程写，回调读。
  std::atomic<float> driver_volume_{1.0f};
};

}  // namespace aaudio
}  // namespace apu
}  // namespace xe

#endif //AX360E_XE_AAUDIO_AUDIO_DRIVER_H
