/*
 *  Copyright 2023 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 */

#import <Foundation/Foundation.h>
#import "RTCMacros.h"

NS_ASSUME_NONNULL_BEGIN

RTC_OBJC_EXPORT
@interface RTC_OBJC_TYPE(RTCCustomAudioProcessing) : NSObject

/** 创建自定义音频处理模块 */
+ (instancetype)createWithTrack:(int)track;

/** 获取共享实例 */
+ (instancetype)sharedInstance;

/** 设置当前活跃音轨 */
- (void)setTrack:(int)track;

/** 设置启用状态 */
- (void)setEnable:(BOOL)enable;

/** 设置音频处理参数 */
- (void)setupTrack:(int)track
          channels:(int)channels
      samplingRate:(int)samplingRate
   bytesPerSample:(int)bytesPerSample
             tempo:(float)tempo
         pitchSemi:(float)pitchSemi;

/** 设置音高（半音） */
- (void)setPitchSemiTones:(int)track pitchSemi:(float)pitchSemi;

/** 设置音高 */
- (void)setPitch:(int)track pitch:(float)pitch;

/** 设置速度 */
- (void)setTempo:(int)track tempo:(float)tempo;

/** 设置速度变化 */
- (void)setTempoChange:(int)track tempoChange:(float)tempoChange;

/** 设置播放速率 */
- (void)setRate:(int)track rate:(float)rate;

/** 设置播放速率变化 */
- (void)setRateChange:(int)track rateChange:(float)rateChange;

/** 设置音量 */
- (void)setVolumeRate:(int)track volumeRate:(float)volumeRate;

/** 设置音量变化 */
- (void)setVolumeChange:(int)track volumeChange:(float)volumeChange;

/** 设置语音模式 */
- (void)setSpeech:(int)track speech:(BOOL)speech;

/** 清除缓冲区 */
- (void)clearBytes:(int)track;

/** 重置所有效果 */
- (void)resetEffects:(int)track;

/** 获取原生 AudioProcessing 对象（用于 Builder） */
- (nullable void*)nativeAudioProcessingModule;

@end

NS_ASSUME_NONNULL_END

