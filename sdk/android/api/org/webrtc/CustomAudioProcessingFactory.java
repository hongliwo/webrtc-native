/*
 *  Copyright 2025 The WebRTC project authors(hongliwo@). All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.webrtc;

public class CustomAudioProcessingFactory implements AudioProcessingFactory {
	private static final String TAG = "CustomAudioProcessingFactory";
	private static final int MAX_TRACKS = 16;

	// 当前活跃的音轨
	private int currentTrack;
	// enable status
	private boolean currentEnable;

	/**
	 * Dynamically allocates a webrtc::AudioProcessing instance and returns a pointer to it.
	 * The caller takes ownership of the object.
	 */
	@Override
	public long createNative() {
		Logging.d(TAG, "Creating native audio processing with track " + currentTrack);
		long pointer = nativeCreateCustomAudioProcessing(currentTrack);
		Logging.d(TAG, "Creating native audio processing with track " + currentTrack + ", and pointer is:" + pointer);
		return pointer;
	}

	/**
	 * Sets the current active track (0-15)
	 * @param track The track index to use
	 */
	public void setTrack(int track) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		currentTrack = track;
		nativeSetTrack(track);
		Logging.d(TAG, "Set current track to " + track);
	}

	/**
	 * Sets the current enable status
	 * @param enable The enable index to use
	 */
	public void setEnable(boolean enable) {
		currentEnable = enable;
		nativeSetEnable(enable);
		Logging.d(TAG, "Set current enable to:" + enable);
	}

	/**
	 * Setup the audio processing for a specific track
	 * @param track Track index (0-15)
	 * @param channels Number of audio channels
	 * @param samplingRate Sample rate in Hz
	 * @param bytesPerSample Bytes per sample (usually 2 for 16-bit PCM)
	 * @param tempo Initial tempo
	 * @param pitchSemi Initial pitch in semitones
	 */
	public void setup(int track, int channels, int samplingRate, int bytesPerSample, float tempo, float pitchSemi) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		nativeSetup(track, channels, samplingRate, bytesPerSample, tempo, pitchSemi);
		Logging.d(TAG, "Setup track " + track + " with channels=" + channels + 
				", samplingRate=" + samplingRate + ", bytesPerSample=" + bytesPerSample + 
				", tempo=" + tempo + ", pitchSemi=" + pitchSemi);
	}

	/**
	 * Sets the pitch factor in semitones for a specific track
	 * @param track Track index (0-15)
	 * @param pitchSemi The pitch in semitones
	 */
	public void setPitchSemiTones(int track, float pitchSemi) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		nativeSetPitchSemiTones(track, pitchSemi);
		Logging.d(TAG, "Set pitch semitones to " + pitchSemi + " for track " + track);
	}

	/**
	 * Sets the pitch factor (1.0 = normal, <1.0 = lower, >1.0 = higher) for a specific track
	 * @param track Track index (0-15)
	 * @param pitch The pitch factor
	 */
	public void setPitch(int track, float pitch) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		if (pitch <= 0) {
			Logging.e(TAG, "Invalid pitch value: " + pitch);
			return;
		}
		nativeSetPitch(track, pitch);
		Logging.d(TAG, "Set pitch to " + pitch + " for track " + track);
	}

	/**
	 * Sets the tempo factor (1.0 = normal, <1.0 = slower, >1.0 = faster) for a specific track
	 * @param track Track index (0-15)
	 * @param tempo The tempo factor
	 */
	public void setTempo(int track, float tempo) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		if (tempo <= 0) {
			Logging.e(TAG, "Invalid tempo value: " + tempo);
			return;
		}
		nativeSetTempo(track, tempo);
		Logging.d(TAG, "Set tempo to " + tempo + " for track " + track);
	}

	/**
	 * Sets the tempo change in percentage for a specific track
	 * @param track Track index (0-15)
	 * @param tempoChange The tempo change percentage
	 */
	public void setTempoChange(int track, float tempoChange) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		nativeSetTempoChange(track, tempoChange);
		Logging.d(TAG, "Set tempo change to " + tempoChange + " for track " + track);
	}

	/**
	 * Sets the playback rate for a specific track
	 * @param track Track index (0-15)
	 * @param rate The playback rate
	 */
	public void setRate(int track, float rate) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		nativeSetRate(track, rate);
		Logging.d(TAG, "Set rate to " + rate + " for track " + track);
	}

	/**
	 * Sets the playback rate change in percentage (-50..+100 %) for a specific track
	 * @param track Track index (0-15)
	 * @param rateChange The rate change percentage
	 */
	public void setRateChange(int track, float rateChange) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		if (rateChange < -50 || rateChange > 100) {
			Logging.e(TAG, "Invalid rate change value: " + rateChange);
			return;
		}
		nativeSetRateChange(track, rateChange);
		Logging.d(TAG, "Set rate change to " + rateChange + " for track " + track);
	}

	/**
	 * Sets the volume rate (1.0 = normal, <1.0 = quieter, >1.0 = louder) for a specific track
	 * @param track Track index (0-15)
	 * @param volumeRate The volume rate
	 */
	public void setVolumeRate(int track, float volumeRate) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		if (volumeRate < 0) {
			Logging.e(TAG, "Invalid volume rate value: " + volumeRate);
			return;
		}
		nativeSetVolumeRate(track, volumeRate);
		Logging.d(TAG, "Set volume rate to " + volumeRate + " for track " + track);
	}

	/**
	 * Sets the volume change in decibels (-48..48 dB) for a specific track
	 * @param track Track index (0-15)
	 * @param volumeChange The volume change in dB
	 */
	public void setVolumeChange(int track, float volumeChange) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		if (volumeChange < -48 || volumeChange > 48) {
			Logging.e(TAG, "Invalid volume change value: " + volumeChange);
			return;
		}
		nativeSetVolumeChange(track, volumeChange);
		Logging.d(TAG, "Set volume change to " + volumeChange + " for track " + track);
	}

	/**
	 * Sets speech mode for better speech processing for a specific track
	 * @param track Track index (0-15)
	 * @param speech True for speech mode, false for music mode
	 */
	public void setSpeech(int track, boolean speech) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		nativeSetSpeech(track, speech);
		Logging.d(TAG, "Set speech mode to " + speech + " for track " + track);
	}

	/**
	 * Clears the audio buffer for a specific track
	 * @param track Track index (0-15)
	 */
	public void clearBytes(int track) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		nativeClearBytes(track);
		Logging.d(TAG, "Cleared bytes for track " + track);
	}

	/**
	 * Resets all audio effects to default values for a specific track
	 * @param track Track index (0-15)
	 */
	public void resetEffects(int track) {
		if (track < 0 || track >= MAX_TRACKS) {
			Logging.e(TAG, "Invalid track index: " + track);
			return;
		}
		nativeResetEffects(track);
		Logging.d(TAG, "Reset effects for track " + track);
	}

	// Native methods
	private static native long nativeCreateCustomAudioProcessing(int track);
	private static native void nativeSetup(int track, int channels, int samplingRate, 
			int bytesPerSample, float tempo, float pitchSemi);
	private static native void nativeSetPitchSemiTones(int track, float pitchSemi);
	private static native void nativeSetPitch(int track, float pitch);
	private static native void nativeSetTempo(int track, float tempo);
	private static native void nativeSetTempoChange(int track, float tempoChange);
	private static native void nativeSetRate(int track, float rate);
	private static native void nativeSetRateChange(int track, float rateChange);
	private static native void nativeSetVolumeRate(int track, float volumeRate);
	private static native void nativeSetVolumeChange(int track, float volumeChange);
	private static native void nativeSetSpeech(int track, boolean speech);
	private static native void nativeSetTrack(int track);
	private static native void nativeSetEnable(boolean enable);
	private static native void nativeClearBytes(int track);
	private static native void nativeResetEffects(int track);
}

