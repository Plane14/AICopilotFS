/*****************************************************************************
* Copyright 2025 AI Copilot FS Project
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef VOICE_INPUT_HPP
#define VOICE_INPUT_HPP

#include <cstdint>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <chrono>

namespace AICopilot {

/**
 * Voice Input Processing System
 * 
 * Handles audio capture, preprocessing, and voice activity detection (VAD)
 * for pilot voice commands in the cockpit environment.
 */

// Audio format specifications
struct AudioFormat {
    static constexpr int SAMPLE_RATE = 16000;  // 16 kHz
    static constexpr int CHANNELS = 1;          // Mono
    static constexpr int BIT_DEPTH = 16;        // 16-bit PCM
    static constexpr int FRAME_SIZE = 512;      // Samples per frame
    static constexpr float FRAME_DURATION_MS = (FRAME_SIZE * 1000.0f) / SAMPLE_RATE;
};

// Voice activity detection levels
enum class VADLevel {
    NO_VOICE = 0,
    WEAK_VOICE = 1,
    MODERATE_VOICE = 2,
    STRONG_VOICE = 3
};

// Audio buffer for voice frames
struct AudioFrame {
    std::vector<int16_t> samples;
    uint64_t timestamp_ms;
    VADLevel vad_level;
};

// Voice input statistics
struct VoiceInputStats {
    uint32_t total_frames_processed = 0;
    uint32_t voice_frames_detected = 0;
    uint32_t silence_frames = 0;
    double average_vad_confidence = 0.0;
    double max_amplitude = 0.0;
    double current_noise_floor = 0.0;
};

/**
 * VoiceActivityDetector - Detects voice presence in audio
 */
class VoiceActivityDetector {
public:
    VoiceActivityDetector();
    
    /**
     * Initialize VAD with sensitivity level (0.0-1.0)
     * Higher sensitivity detects quieter speech
     */
    bool initialize(float sensitivity = 0.5f);
    
    /**
     * Process audio frame and detect voice activity
     * Returns VAD level and confidence score
     */
    std::pair<VADLevel, double> processFrame(const std::vector<int16_t>& samples);
    
    /**
     * Set noise floor (for dynamic adjustment)
     */
    void setNoiseFloor(double noise_floor);
    
    /**
     * Get current noise floor estimate
     */
    double getNoiseFloor() const { return noise_floor_; }
    
    /**
     * Get VAD statistics
     */
    VoiceInputStats getStatistics() const;
    
private:
    float sensitivity_;
    double noise_floor_;
    double max_amplitude_;
    std::vector<double> energy_history_;
    
    // VAD calculation
    double calculateFrameEnergy(const std::vector<int16_t>& samples) const;
    VADLevel determineVADLevel(double energy, double confidence);
    double calculateEnergy(const std::vector<int16_t>& samples) const;
};

/**
 * AudioPreprocessor - Noise reduction and audio enhancement
 */
class AudioPreprocessor {
public:
    AudioPreprocessor();
    
    /**
     * Initialize preprocessor with noise reduction level
     */
    bool initialize(float noise_reduction_level = 0.5f);
    
    /**
     * Apply noise reduction to audio frame
     */
    std::vector<int16_t> reduceNoise(const std::vector<int16_t>& samples);
    
    /**
     * Apply gain to audio (amplification)
     */
    std::vector<int16_t> applyGain(const std::vector<int16_t>& samples, float gain_db);
    
    /**
     * Normalize audio levels
     */
    std::vector<int16_t> normalize(const std::vector<int16_t>& samples);
    
    /**
     * Apply high-pass filter (remove low-frequency noise)
     */
    std::vector<int16_t> highPassFilter(const std::vector<int16_t>& samples);
    
private:
    float noise_reduction_level_;
    std::vector<int16_t> previous_frame_;
    double high_pass_filter_state_;
    
    // Spectral subtraction state
    std::vector<double> noise_spectrum_;
};

/**
 * VoiceCommandBuffer - Buffers and manages voice frames
 */
class VoiceCommandBuffer {
public:
    VoiceCommandBuffer();
    
    /**
     * Initialize buffer with maximum size (in frames)
     */
    void initialize(size_t max_frames);
    
    /**
     * Add audio frame to buffer
     */
    void addFrame(const AudioFrame& frame);
    
    /**
     * Get buffered audio as continuous data
     */
    std::vector<int16_t> getBufferedAudio() const;
    
    /**
     * Clear buffer
     */
    void clear();
    
    /**
     * Check if buffer has sufficient voice content
     */
    bool hasMinimumVoiceContent(size_t min_voice_frames = 20) const;
    
    /**
     * Get number of frames in buffer
     */
    size_t getFrameCount() const { return frames_.size(); }
    
    /**
     * Get frame at index
     */
    AudioFrame getFrame(size_t index) const;
    
    /**
     * Extract voice segment (continuous voice frames)
     */
    std::vector<int16_t> extractVoiceSegment() const;
    
private:
    std::queue<AudioFrame> frames_;
    size_t max_frames_;
};

/**
 * VoiceInput - Main voice input processing system
 */
class VoiceInput {
public:
    VoiceInput();
    ~VoiceInput();
    
    /**
     * Initialize voice input system
     * Returns true if successful
     */
    bool initialize();
    
    /**
     * Shutdown voice input system
     */
    void shutdown();
    
    /**
     * Check if system is running
     */
    bool isRunning() const { return is_running_; }
    
    /**
     * Process raw audio data (e.g., from audio device)
     */
    void processAudioData(const std::vector<int16_t>& audio_samples);
    
    /**
     * Start listening for voice commands
     */
    void startListening();
    
    /**
     * Stop listening for voice commands
     */
    void stopListening();
    
    /**
     * Check if currently listening
     */
    bool isListening() const { return is_listening_; }
    
    /**
     * Get current VAD level
     */
    VADLevel getCurrentVADLevel() const { return current_vad_level_; }
    
    /**
     * Check if voice activity detected
     */
    bool voiceDetected() const { return current_vad_level_ > VADLevel::NO_VOICE; }
    
    /**
     * Set VAD sensitivity (0.0-1.0, higher = more sensitive)
     */
    void setVADSensitivity(float sensitivity);
    
    /**
     * Get accumulated voice command buffer
     */
    std::vector<int16_t> getVoiceBuffer() const;
    
    /**
     * Clear accumulated voice buffer
     */
    void clearVoiceBuffer();
    
    /**
     * Get input statistics
     */
    VoiceInputStats getStatistics() const;
    
    /**
     * Register callback for voice detection
     */
    using VoiceDetectionCallback = std::function<void(const std::vector<int16_t>&, double)>;
    void registerVoiceCallback(VoiceDetectionCallback callback);
    
    /**
     * Get average sound level (0.0-1.0)
     */
    double getAverageSoundLevel() const;
    
private:
    bool is_running_;
    bool is_listening_;
    VADLevel current_vad_level_;
    
    std::unique_ptr<VoiceActivityDetector> vad_;
    std::unique_ptr<AudioPreprocessor> preprocessor_;
    std::unique_ptr<VoiceCommandBuffer> buffer_;
    
    VoiceDetectionCallback voice_callback_;
    
    // Processing state
    std::vector<int16_t> current_frame_;
    size_t frame_index_;
    uint64_t last_voice_timestamp_;
    
    // Statistics
    VoiceInputStats statistics_;
    std::vector<double> recent_sound_levels_;
    
    // Helper methods
    void processAccumulatedFrame();
    void updateStatistics();
};

} // namespace AICopilot

#endif // VOICE_INPUT_HPP
