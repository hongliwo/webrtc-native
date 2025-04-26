#import "RTCCustomAudioProcessing.h"
#import "SoundTouchWrapper.h"
#import "modules/audio_processing/include/audio_processing.h"
#import "rtc_base/ref_counted_object.h"
#import "rtc_base/logging.h"

// 最大音轨数量
constexpr int MAX_TRACKS = 16;

// 全局音轨数组
static std::vector<std::unique_ptr<SoundTouchWrapper>> g_sound_touch_wrappers(MAX_TRACKS);

// 全局静态变量
static int g_active_track = 0;
static bool g_enable = false;
static bool g_initialized = false;

// 自定义音频处理类，实现AudioProcessing接口
class CustomAudioProcessing : public webrtc::AudioProcessing {
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
			// 检查输入参数
			if (!src || !dest) {
				return kNullPointerError;
			}

			// 如果禁用处理，直接返回
			if (!g_enable) {
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
				memcpy(dest, src, input_config.num_samples() * sizeof(int16_t));
			}

			// 处理音频数据
			sound_touch->ProcessFrame(dest, input_config.num_samples());

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

		// 实现其他必需的方法...
		// 这里省略了大量必须实现的方法，实际应用中需要完整实现

		int ProcessReverseStream(const int16_t* const src,
				const StreamConfig& input_config,
				const StreamConfig& output_config,
				int16_t* const dest) override {
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
		}

		int recommended_stream_analog_level() const override {
			return 0;
		}

		// 延迟相关方法
		int set_stream_delay_ms(int delay) override {
			return kNoError;
		}

		int stream_delay_ms() const override {
			return 0;
		}

		void set_stream_key_pressed(bool key_pressed) override {
			// 记录日志但不做处理
		}

		void set_output_will_be_muted(bool muted) override {
			// 记录日志但不做处理
		}

		void SetRuntimeSetting(RuntimeSetting setting) override {
			// 记录日志但不做处理
		}

		bool PostRuntimeSetting(RuntimeSetting setting) override {
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
			return false;
		}

		bool CreateAndAttachAecDump(FILE* handle,
				int64_t max_log_size_bytes,
				rtc::TaskQueue* worker_queue) override {
			return false;
		}

		void AttachAecDump(std::unique_ptr<webrtc::AecDump> aec_dump) override {
			// 可以简单地忽略参数，或者根据需要处理
		}

		void DetachAecDump() override {
			// 空实现
		}

		// 统计信息
		AudioProcessingStats GetStatistics() override {
			AudioProcessingStats stats;
			// 可以根据需要设置统计信息
			stats.voice_detected = absl::optional<bool>(false);
			return stats;
		}

		AudioProcessingStats GetStatistics(bool has_remote_tracks) override {
			AudioProcessingStats stats;
			// 可以根据需要设置统计信息
			stats.voice_detected = absl::optional<bool>(false);
			return stats;
		}

		// 获取配置
		AudioProcessing::Config GetConfig() const override {
			return AudioProcessing::Config();
		}

	private:
		int sample_rate_hz_;
		int num_channels_;
};

@implementation RTCCustomAudioProcessing {
	rtc::scoped_refptr<webrtc::AudioProcessing> _audioProcessing;
	int _currentTrack;
	BOOL _isEnabled;
}

#pragma mark - 单例和工厂方法

+ (instancetype)sharedInstance {
	static RTCCustomAudioProcessing *sharedInstance = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
			sharedInstance = [[self alloc] initWithTrack:0];
			});
	return sharedInstance;
}

+ (instancetype)createWithTrack:(int)track {
	return [[RTCCustomAudioProcessing alloc] initWithTrack:track];
}

- (instancetype)initWithTrack:(int)track {
	if (self = [super init]) {
		if (track < 0 || track >= MAX_TRACKS) {
			NSLog(@"Invalid track index: %d", track);
			return nil;
		}

		_currentTrack = track;
		_isEnabled = NO;

		// 创建 CustomAudioProcessing 实例
		_audioProcessing = rtc::scoped_refptr<webrtc::AudioProcessing>(
				new rtc::RefCountedObject<CustomAudioProcessing>(track));

		// 确保所有音轨都已初始化
		for (int i = 0; i < MAX_TRACKS; i++) {
			if (!g_sound_touch_wrappers[i]) {
				g_sound_touch_wrappers[i] = std::make_unique<SoundTouchWrapper>();
			}
		}

		// 设置活跃音轨和启用状态
		g_active_track = track;
		g_enable = _isEnabled;
	}
	return self;
}

#pragma mark - 音轨和启用控制

- (void)setTrack:(int)track {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}
	_currentTrack = track;
	g_active_track = track;
	NSLog(@"Set active track to %d", track);
}

- (void)setEnable:(BOOL)enable {
	_isEnabled = enable;
	g_enable = enable;
	NSLog(@"Set enable to %@", enable ? @"YES" : @"NO");
}

#pragma mark - 音频效果控制

- (void)setupTrack:(int)track
channels:(int)channels
samplingRate:(int)samplingRate
bytesPerSample:(int)bytesPerSample
tempo:(float)tempo
pitchSemi:(float)pitchSemi {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
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

	NSLog(@"Setup track %d with channels=%d, samplingRate=%d, bytesPerSample=%d, tempo=%f, pitchSemi=%f",
			track, channels, samplingRate, bytesPerSample, tempo, pitchSemi);
}

- (void)setPitchSemiTones:(int)track pitchSemi:(float)pitchSemi {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->SetPitchSemiTones(pitchSemi);
		NSLog(@"Set pitch semitones to %f for track %d", pitchSemi, track);
	} else {
		NSLog(@"Cannot set pitch semitones: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)setPitch:(int)track pitch:(float)pitch {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->SetPitch(pitch);
		NSLog(@"Set pitch to %f for track %d", pitch, track);
	} else {
		NSLog(@"Cannot set pitch: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)setTempo:(int)track tempo:(float)tempo {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->SetTempo(tempo);
		NSLog(@"Set tempo to %f for track %d", tempo, track);
	} else {
		NSLog(@"Cannot set tempo: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)setTempoChange:(int)track tempoChange:(float)tempoChange {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->SetTempoChange(tempoChange);
		NSLog(@"Set tempo change to %f for track %d", tempoChange, track);
	} else {
		NSLog(@"Cannot set tempo change: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)setRate:(int)track rate:(float)rate {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->SetRate(rate);
		NSLog(@"Set rate to %f for track %d", rate, track);
	} else {
		NSLog(@"Cannot set rate: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)setRateChange:(int)track rateChange:(float)rateChange {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->SetRateChange(rateChange);
		NSLog(@"Set rate change to %f for track %d", rateChange, track);
	} else {
		NSLog(@"Cannot set rate change: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)setVolumeRate:(int)track volumeRate:(float)volumeRate {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->SetVolumeRate(volumeRate);
		NSLog(@"Set volume rate to %f for track %d", volumeRate, track);
	} else {
		NSLog(@"Cannot set volume rate: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)setVolumeChange:(int)track volumeChange:(float)volumeChange {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->SetVolumeChange(volumeChange);
		NSLog(@"Set volume change to %f for track %d", volumeChange, track);
	} else {
		NSLog(@"Cannot set volume change: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)setSpeech:(int)track speech:(BOOL)speech {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->SetSpeech(speech);
		NSLog(@"Set speech mode to %@ for track %d", speech ? @"true" : @"false", track);
	} else {
		NSLog(@"Cannot set speech mode: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)clearBytes:(int)track {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
		return;
	}

	SoundTouchWrapper* sound_touch = g_sound_touch_wrappers[track].get();
	if (sound_touch) {
		sound_touch->Clear();
		NSLog(@"Cleared bytes for track %d", track);
	} else {
		NSLog(@"Cannot clear bytes: SoundTouchWrapper for track %d not initialized", track);
	}
}

- (void)resetEffects:(int)track {
	if (track < 0 || track >= MAX_TRACKS) {
		NSLog(@"Invalid track index: %d", track);
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
		NSLog(@"Reset effects for track %d", track);
	} else {
		NSLog(@"Cannot reset effects: SoundTouchWrapper for track %d not initialized", track);
	}
}

#pragma mark - 原生模块访问

- (rtc::scoped_refptr<webrtc::AudioProcessing>)nativeAudioProcessingModule {
	return _audioProcessing;
}

@end

