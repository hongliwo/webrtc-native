#include <jni.h>
#include <memory>
#include <vector>
#include <chrono>

#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/include/aec_dump.h"
#include "api/audio/audio_frame.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/logging.h"
#include "sdk/android/src/jni/jni_helpers.h"

// 包含SoundTouch包装器
#include "third_party/soundtouch/wrapper/soundtouch_wrapper.h"

namespace webrtc {
namespace jni {

// 最大音轨数量
constexpr int MAX_TRACKS = 16;

// 全局音轨数组 - 使用智能指针管理生命周期
static std::vector<std::unique_ptr<SoundTouchWrapper>> g_sound_touch_wrappers(MAX_TRACKS);

// 全局静态变量
static int g_active_track = 0;
static bool g_enable = false;
static bool g_initialized = false;

// 全局函数，设置活跃音轨
static void SetActiveTrack(int track) {
	g_active_track = track;
	RTC_LOG(LS_INFO) << "Set active track to " << track;
}

// 全局函数，设置启用状态
static void SetEnable(bool enable) {
	g_enable = enable;
	RTC_LOG(LS_INFO) << "Set enable to " << (enable ? "true" : "false");
}

// 自定义音频处理类，实现AudioProcessing接口
class CustomAudioProcessing : public AudioProcessing {
public:
	CustomAudioProcessing(int track) 
		: sample_rate_hz_(0), 
		num_channels_(0) {
			RTC_LOG(LS_INFO) << "CustomAudioProcessing created with track " << track;

			// 确保所有音轨都已初始化
			for (int i = 0; i < MAX_TRACKS; i++) {
				if (!g_sound_touch_wrappers[i]) {
					g_sound_touch_wrappers[i] = std::make_unique<SoundTouchWrapper>();
				}
			}

			// 初始化全局变量（如果尚未初始化）
			if (!g_initialized) {
				g_active_track = track;
				g_enable = false;
				g_initialized = true;
			}
		}

	~CustomAudioProcessing() override {
		RTC_LOG(LS_INFO) << "CustomAudioProcessing destroyed";
	}

	// 初始化方法
	int Initialize() override {
		RTC_LOG(LS_INFO) << "CustomAudioProcessing::Initialize() called";
		return kNoError;
	}

	int Initialize(const ProcessingConfig& processing_config) override {
		RTC_LOG(LS_INFO) << "CustomAudioProcessing::Initialize(ProcessingConfig) called";
		sample_rate_hz_ = processing_config.input_stream().sample_rate_hz();
		num_channels_ = static_cast<int>(processing_config.input_stream().num_channels());

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[g_active_track].get();
		if (sound_touch) {
			sound_touch->Initialize(num_channels_, sample_rate_hz_, 2);
			g_initialized = true;
			RTC_LOG(LS_INFO) << "SoundTouch initialized with channels=" << num_channels_ 
				<< ", rate=" << sample_rate_hz_;
		}

		return kNoError;
	}

	void ApplyConfig(const Config& config) override {
		// 应用配置 - 简单实现
		RTC_LOG(LS_INFO) << "CustomAudioProcessing::ApplyConfig called";
	}

	int ProcessStream(const int16_t* const src,
			const StreamConfig& input_config,
			const StreamConfig& output_config,
			int16_t* const dest) override {
		// RTC_LOG(LS_INFO) << "ProcessStream(int16_t): channels=" << input_config.num_channels()
		//	<< ", frames=" << input_config.num_frames()
		//	<< ", rate=" << input_config.sample_rate_hz()
		//	<< ", g_active_track=" << g_active_track;;

		// 检查输入参数
		if (!src || !dest) {
			return kNullPointerError;
		}

		// 如果禁用处理，直接返回
		if (!g_enable) {
			//RTC_LOG(LS_INFO) << "Audio processing disabled, skipping";
			return kNoError;
		}

		// 获取当前活跃的音轨
		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[g_active_track].get();
		if (!sound_touch) {
			RTC_LOG(LS_ERROR) << "Sound touch wrapper for track " << g_active_track << " is null";
			return kUnspecifiedError;
		}

		// 初始化SoundTouch（如果需要）
		if (!g_initialized || sample_rate_hz_ != input_config.sample_rate_hz() ||
			static_cast<size_t>(num_channels_) != input_config.num_channels()) {
			sound_touch->Initialize(input_config.num_channels(), input_config.sample_rate_hz(), 2); // 2 bytes per sample

			sample_rate_hz_ = input_config.sample_rate_hz();
			num_channels_ = static_cast<int>(input_config.num_channels());
			g_initialized = true;

			RTC_LOG(LS_INFO) << "CustomAudioProcessing initialized with sample_rate=" 
				<< sample_rate_hz_ << ", channels=" << num_channels_;
		}

		// 如果源和目标不同，先复制数据
		if (src != dest) {
			//RTC_LOG(LS_INFO) << "CustomAudioProcessing  src != dest";
			memcpy(dest, src, input_config.num_samples() * sizeof(int16_t));
		}

		// 记录开始时间
		//auto start_time = std::chrono::high_resolution_clock::now();

		// 处理音频数据
		sound_touch->ProcessFrame(dest, input_config.num_samples());

		// 记录结束时间并计算耗时
		//auto end_time = std::chrono::high_resolution_clock::now();
		//auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

		//RTC_LOG(LS_INFO) << "ProcessFrame execution time: " << duration << " microseconds"
		//	<< " for " << input_config.num_samples() << " samples";

		return kNoError;
	}

	int ProcessStream(const float* const* src,
			const StreamConfig& input_config,
			const StreamConfig& output_config,
			float* const* dest) override {
		RTC_LOG(LS_INFO) << "ProcessStream(float): channels=" << input_config.num_channels()
			<< ", frames=" << input_config.num_frames()
			<< ", rate=" << input_config.sample_rate_hz();

		// 简单实现 - 复制数据
		if (src && dest) {
			for (size_t ch = 0; ch < input_config.num_channels(); ++ch) {
				if (src[ch] != dest[ch]) {
					memcpy(dest[ch], src[ch], input_config.num_frames() * sizeof(float));
				}
			}
		}
		return kNoError;
	}

#if 0
	// 反向流处理
	int ProcessReverseStream(AudioFrame* frame) override {
		RTC_LOG(LS_INFO) << "ProcessReverseStream(AudioFrame) called";
		// 简单实现 - 不做处理
		return kNoError;
	}
#endif


	int ProcessReverseStream(const int16_t* const src,
			const StreamConfig& input_config,
			const StreamConfig& output_config,
			int16_t* const dest) override {
		RTC_LOG(LS_INFO) << "ProcessReverseStream(int16_t) called";
		// 简单实现 - 复制数据
		if (src && dest && src != dest) {
			memcpy(dest, src, input_config.num_samples() * sizeof(int16_t));
		}
		return kNoError;
	}

	int ProcessReverseStream(const float* const* src,
			const StreamConfig& input_config,
			const StreamConfig& output_config,
			float* const* dest) override {
		RTC_LOG(LS_INFO) << "ProcessReverseStream(float) called";
		// 简单实现 - 复制数据
		if (src && dest) {
			for (size_t ch = 0; ch < input_config.num_channels(); ++ch) {
				if (src[ch] != dest[ch]) {
					memcpy(dest[ch], src[ch], input_config.num_frames() * sizeof(float));
				}
			}
		}
		return kNoError;
	}

	int AnalyzeReverseStream(const float* const* data,
			const StreamConfig& reverse_config) override {
		RTC_LOG(LS_INFO) << "AnalyzeReverseStream called";
		// 简单实现 - 不做处理
		return kNoError;
	}

	bool GetLinearAecOutput(
			rtc::ArrayView<std::array<float, 160>> linear_output) const override {
		// 简单实现 - 不提供线性AEC输出
		return false;
	}

	// 音量控制相关方法
	void set_stream_analog_level(int level) override {
		// 记录日志但不做处理
		RTC_LOG(LS_INFO) << "set_stream_analog_level: " << level;
	}

	int recommended_stream_analog_level() const override {
		return 0;
	}

	// 延迟相关方法
	int set_stream_delay_ms(int delay) override {
		RTC_LOG(LS_INFO) << "set_stream_delay_ms: " << delay;
		return kNoError;
			}

	int stream_delay_ms() const override {
		return 0;
		}

	void set_stream_key_pressed(bool key_pressed) override {
		// 记录日志但不做处理
		RTC_LOG(LS_INFO) << "set_stream_key_pressed: " << key_pressed;
	}

	void set_output_will_be_muted(bool muted) override {
		// 记录日志但不做处理
		RTC_LOG(LS_INFO) << "set_output_will_be_muted: " << muted;
	}

	void SetRuntimeSetting(RuntimeSetting setting) override {
		// 记录日志但不做处理
		RTC_LOG(LS_INFO) << "SetRuntimeSetting called";
	}

	bool PostRuntimeSetting(RuntimeSetting setting) override {
		RTC_LOG(LS_INFO) << "PostRuntimeSetting called";
		return true;
	}

	// 获取处理参数
	int proc_sample_rate_hz() const override {
		return sample_rate_hz_;
	}

	int proc_split_sample_rate_hz() const override {
		return sample_rate_hz_;
	}

	size_t num_input_channels() const override {
		return static_cast<size_t>(num_channels_);
	}

	size_t num_proc_channels() const override {
		return static_cast<size_t>(num_channels_);
	}

	size_t num_output_channels() const override {
		return static_cast<size_t>(num_channels_);
	}

	size_t num_reverse_channels() const override {
		return static_cast<size_t>(num_channels_);
	}

	// AecDump相关方法
	bool CreateAndAttachAecDump(absl::string_view file_name,
			int64_t max_log_size_bytes,
			rtc::TaskQueue* worker_queue) override {
		RTC_LOG(LS_INFO) << "CreateAndAttachAecDump(file_name) called";
		return false;
	}

	bool CreateAndAttachAecDump(FILE* handle,
			int64_t max_log_size_bytes,
			rtc::TaskQueue* worker_queue) override {
		RTC_LOG(LS_INFO) << "CreateAndAttachAecDump(FILE*) called";
		return false;
	}

	void AttachAecDump(std::unique_ptr<AecDump> aec_dump) override {
		RTC_LOG(LS_INFO) << "AttachAecDump called";
		// 可以简单地忽略参数，或者根据需要处理
	}

	void DetachAecDump() override {
		RTC_LOG(LS_INFO) << "DetachAecDump called";
		// 空实现
	}

	// 统计信息
	AudioProcessingStats GetStatistics() override {
		RTC_LOG(LS_INFO) << "GetStatistics() called";
		AudioProcessingStats stats;
		// 可以根据需要设置统计信息
		stats.voice_detected = absl::optional<bool>(false);
		return stats;
	}

	AudioProcessingStats GetStatistics(bool has_remote_tracks) override {
		RTC_LOG(LS_INFO) << "GetStatistics(has_remote_tracks=" << has_remote_tracks << ") called";
		AudioProcessingStats stats;
		// 可以根据需要设置统计信息
		stats.voice_detected = absl::optional<bool>(false);
		return stats;
	}

	// 获取配置
	AudioProcessing::Config GetConfig() const override {
		RTC_LOG(LS_INFO) << "GetConfig called";
		return AudioProcessing::Config();
	}

private:
	int sample_rate_hz_;
	int num_channels_;
};

// JNI方法实现

extern "C" JNIEXPORT jlong JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeCreateCustomAudioProcessing(
			JNIEnv* env, jclass, jint track) {

		RTC_LOG(LS_INFO) << "Creating CustomAudioProcessing instance for track " << track;

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return 0;
		}

		// 创建CustomAudioProcessing实例，传入音轨
		// 使用正确的方式创建rtc::scoped_refptr
		rtc::scoped_refptr<AudioProcessing> audio_processing =
			rtc::scoped_refptr<AudioProcessing>(new rtc::RefCountedObject<CustomAudioProcessing>(track));

		// 记录原始指针值（以十六进制格式）
		void* raw_ptr = audio_processing.get();
		RTC_LOG(LS_INFO) << "Raw pointer value: " << raw_ptr;

		// 释放指针并转换为jlong
		jlong j_ptr = jlongFromPointer(audio_processing.release());
		RTC_LOG(LS_INFO) << "Converted jlong value: " << j_ptr;

		void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(j_ptr));
		RTC_LOG(LS_INFO) << "Converted back to pointer: " << ptr;

		// 返回指针，WebRTC会管理其生命周期
		return j_ptr;
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetup(
			JNIEnv* env, jclass, jint track, jint channels, jint samplingRate, 
			jint bytesPerSample, jfloat tempo, jfloat pitchSemi) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (!sound_touch) {
			g_sound_touch_wrappers[track] = std::make_unique<SoundTouchWrapper>();
			sound_touch = g_sound_touch_wrappers[track].get();
		}

		sound_touch->Initialize(channels, samplingRate, bytesPerSample);
		sound_touch->SetTempo(tempo);
		sound_touch->SetPitchSemiTones(pitchSemi);

		RTC_LOG(LS_INFO) << "Setup track " << track << " with channels=" << channels 
			<< ", samplingRate=" << samplingRate 
			<< ", bytesPerSample=" << bytesPerSample
			<< ", tempo=" << tempo 
			<< ", pitchSemi=" << pitchSemi;
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetPitchSemiTones(
			JNIEnv* env, jclass, jint track, jfloat pitchSemi) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetPitchSemiTones(pitchSemi);
			RTC_LOG(LS_INFO) << "Set pitch semitones to " << pitchSemi << " for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot set pitch semitones: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetPitch(
			JNIEnv* env, jclass, jint track, jfloat pitch) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetPitch(pitch);
			RTC_LOG(LS_INFO) << "Set pitch to " << pitch << " for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot set pitch: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetTempo(
			JNIEnv* env, jclass, jint track, jfloat tempo) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetTempo(tempo);
			RTC_LOG(LS_INFO) << "Set tempo to " << tempo << " for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot set tempo: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetTempoChange(
			JNIEnv* env, jclass, jint track, jfloat tempoChange) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetTempoChange(tempoChange);
			RTC_LOG(LS_INFO) << "Set tempo change to " << tempoChange << " for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot set tempo change: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetRate(
			JNIEnv* env, jclass, jint track, jfloat rate) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetRate(rate);
			RTC_LOG(LS_INFO) << "Set rate to " << rate << " for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot set rate: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetRateChange(
			JNIEnv* env, jclass, jint track, jfloat rateChange) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetRateChange(rateChange);
			RTC_LOG(LS_INFO) << "Set rate change to " << rateChange << " for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot set rate change: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetVolumeRate(
			JNIEnv* env, jclass, jint track, jfloat volumeRate) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetVolumeRate(volumeRate);
			RTC_LOG(LS_INFO) << "Set volume rate to " << volumeRate << " for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot set volume rate: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetVolumeChange(
			JNIEnv* env, jclass, jint track, jfloat volumeChange) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetVolumeChange(volumeChange);
			RTC_LOG(LS_INFO) << "Set volume change to " << volumeChange << " for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot set volume change: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetSpeech(
			JNIEnv* env, jclass, jint track, jboolean speech) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetSpeech(speech);
			RTC_LOG(LS_INFO) << "Set speech mode to " << (speech ? "true" : "false") << " for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot set speech mode: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetTrack(
			JNIEnv* env, jclass, jint track) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}
		// 使用静态方法设置活跃音轨
		webrtc::jni::SetActiveTrack(track);
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeSetEnable(
			JNIEnv* env, jclass, jboolean enable) {

		// 使用静态方法设置使能状态
		webrtc::jni::SetEnable(enable);
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeClearBytes(
			JNIEnv* env, jclass, jint track) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->Clear();
			RTC_LOG(LS_INFO) << "Cleared bytes for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot clear bytes: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

extern "C" JNIEXPORT void JNICALL
	Java_org_webrtc_CustomAudioProcessingFactory_nativeResetEffects(
			JNIEnv* env, jclass, jint track) {

		if (track < 0 || track >= MAX_TRACKS) {
			RTC_LOG(LS_ERROR) << "Invalid track index: " << track;
			return;
		}

		SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
		if (sound_touch) {
			sound_touch->SetPitch(1.0f);
			sound_touch->SetTempo(1.0f);
			sound_touch->SetRateChange(0.0f);
			sound_touch->SetVolumeRate(1.0f);
			sound_touch->SetVolumeChange(0.0f);
			sound_touch->SetSpeech(false);
			RTC_LOG(LS_INFO) << "Reset effects for track " << track;
		} else {
			RTC_LOG(LS_WARNING) << "Cannot reset effects: SoundTouchWrapper for track " << track << " not initialized";
		}
	}

}  // namespace jni
}  // namespace webrtc

