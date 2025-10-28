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

#include "voice_input.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace AICopilot {

// ============================================================================
// VoiceActivityDetector Implementation
// ============================================================================

VoiceActivityDetector::VoiceActivityDetector()
    : sensitivity_(0.5f), noise_floor_(100.0), max_amplitude_(0.0) {
}

bool VoiceActivityDetector::initialize(float sensitivity) {
    sensitivity_ = std::max(0.0f, std::min(1.0f, sensitivity));
    noise_floor_ = 100.0 * (1.0 - sensitivity_);
    energy_history_.clear();
    return true;
}

std::pair<VADLevel, double> VoiceActivityDetector::processFrame(
    const std::vector<int16_t>& samples) {
    
    double energy = calculateFrameEnergy(samples);
    double confidence = 0.0;
    
    // Calculate confidence based on energy threshold
    if (energy > noise_floor_) {
        confidence = std::min(1.0, (energy - noise_floor_) / (noise_floor_ * 2.0));
    }
    
    // Store energy history for adaptive noise floor
    energy_history_.push_back(energy);
    if (energy_history_.size() > 50) {
        energy_history_.erase(energy_history_.begin());
    }
    
    // Update noise floor adaptively
    if (energy_history_.size() >= 10) {
        std::vector<double> sorted_history = energy_history_;
        std::sort(sorted_history.begin(), sorted_history.end());
        noise_floor_ = sorted_history[sorted_history.size() / 4];  // 25th percentile
    }
    
    VADLevel level = determineVADLevel(energy, confidence);
    return std::make_pair(level, confidence);
}

void VoiceActivityDetector::setNoiseFloor(double noise_floor) {
    noise_floor_ = std::max(0.0, noise_floor);
}

VoiceInputStats VoiceActivityDetector::getStatistics() const {
    VoiceInputStats stats;
    stats.current_noise_floor = noise_floor_;
    stats.max_amplitude = max_amplitude_;
    if (!energy_history_.empty()) {
        stats.average_vad_confidence = std::accumulate(
            energy_history_.begin(), energy_history_.end(), 0.0) / energy_history_.size();
    }
    return stats;
}

double VoiceActivityDetector::calculateFrameEnergy(
    const std::vector<int16_t>& samples) const {
    double energy = 0.0;
    for (const auto& sample : samples) {
        energy += sample * sample;
    }
    return std::sqrt(energy / std::max(1u, (unsigned)samples.size()));
}

VADLevel VoiceActivityDetector::determineVADLevel(double energy, double confidence) {
    if (confidence < 0.1) return VADLevel::NO_VOICE;
    if (confidence < 0.3) return VADLevel::WEAK_VOICE;
    if (confidence < 0.7) return VADLevel::MODERATE_VOICE;
    return VADLevel::STRONG_VOICE;
}

double VoiceActivityDetector::calculateEnergy(
    const std::vector<int16_t>& samples) const {
    double sum = 0.0;
    for (const auto& sample : samples) {
        sum += sample * sample;
    }
    return std::sqrt(sum / std::max(1u, (unsigned)samples.size()));
}

// ============================================================================
// AudioPreprocessor Implementation
// ============================================================================

AudioPreprocessor::AudioPreprocessor()
    : noise_reduction_level_(0.5f), high_pass_filter_state_(0.0) {
}

bool AudioPreprocessor::initialize(float noise_reduction_level) {
    noise_reduction_level_ = std::max(0.0f, std::min(1.0f, noise_reduction_level));
    return true;
}

std::vector<int16_t> AudioPreprocessor::reduceNoise(
    const std::vector<int16_t>& samples) {
    
    std::vector<int16_t> result = samples;
    
    if (previous_frame_.empty()) {
        previous_frame_ = samples;
        return result;
    }
    
    // Apply spectral subtraction using previous frame
    for (size_t i = 0; i < result.size(); ++i) {
        double current = result[i];
        double previous = (i < previous_frame_.size()) ? previous_frame_[i] : 0.0;
        
        // Spectral subtraction factor based on noise reduction level
        double alpha = 0.5 + noise_reduction_level_;  // 0.5 to 1.5
        double subtracted = current - (previous * alpha);
        
        // Soft floor to prevent over-subtraction
        if (std::abs(subtracted) < std::abs(current) * 0.1) {
            subtracted = current * 0.1;
        }
        
        result[i] = static_cast<int16_t>(std::max(-32768.0, 
                    std::min(32767.0, subtracted)));
    }
    
    previous_frame_ = samples;
    return result;
}

std::vector<int16_t> AudioPreprocessor::applyGain(
    const std::vector<int16_t>& samples, float gain_db) {
    
    float gain_linear = std::pow(10.0f, gain_db / 20.0f);
    
    std::vector<int16_t> result;
    result.reserve(samples.size());
    
    for (const auto& sample : samples) {
        double amplified = sample * gain_linear;
        result.push_back(static_cast<int16_t>(
            std::max(-32768.0, std::min(32767.0, amplified))));
    }
    
    return result;
}

std::vector<int16_t> AudioPreprocessor::normalize(
    const std::vector<int16_t>& samples) {
    
    if (samples.empty()) return samples;
    
    int16_t max_sample = 0;
    for (const auto& sample : samples) {
        max_sample = std::max(max_sample, static_cast<int16_t>(std::abs(sample)));
    }
    
    if (max_sample == 0) return samples;
    
    float scale = 30000.0f / max_sample;  // Target level
    
    std::vector<int16_t> result;
    result.reserve(samples.size());
    
    for (const auto& sample : samples) {
        result.push_back(static_cast<int16_t>(sample * scale));
    }
    
    return result;
}

std::vector<int16_t> AudioPreprocessor::highPassFilter(
    const std::vector<int16_t>& samples) {
    
    std::vector<int16_t> result;
    result.reserve(samples.size());
    
    // Simple first-order high-pass filter
    // Removes DC and very low frequencies
    const double alpha = 0.95;  // Cutoff around 100 Hz at 16 kHz
    
    for (const auto& sample : samples) {
        high_pass_filter_state_ = alpha * (high_pass_filter_state_ + sample);
        int16_t filtered = static_cast<int16_t>(sample - high_pass_filter_state_);
        result.push_back(filtered);
    }
    
    return result;
}

// ============================================================================
// VoiceCommandBuffer Implementation
// ============================================================================

VoiceCommandBuffer::VoiceCommandBuffer() : max_frames_(0) {
}

void VoiceCommandBuffer::initialize(size_t max_frames) {
    max_frames_ = max_frames;
    clear();
}

void VoiceCommandBuffer::addFrame(const AudioFrame& frame) {
    if (frames_.size() >= max_frames_) {
        frames_.pop();
    }
    frames_.push(frame);
}

std::vector<int16_t> VoiceCommandBuffer::getBufferedAudio() const {
    std::vector<int16_t> result;
    
    std::queue<AudioFrame> temp = frames_;
    while (!temp.empty()) {
        const auto& frame = temp.front();
        result.insert(result.end(), frame.samples.begin(), frame.samples.end());
        temp.pop();
    }
    
    return result;
}

void VoiceCommandBuffer::clear() {
    while (!frames_.empty()) {
        frames_.pop();
    }
}

bool VoiceCommandBuffer::hasMinimumVoiceContent(size_t min_voice_frames) const {
    size_t voice_frames = 0;
    std::queue<AudioFrame> temp = frames_;
    
    while (!temp.empty()) {
        const auto& frame = temp.front();
        if (frame.vad_level > VADLevel::NO_VOICE) {
            voice_frames++;
        }
        temp.pop();
    }
    
    return voice_frames >= min_voice_frames;
}

AudioFrame VoiceCommandBuffer::getFrame(size_t index) const {
    std::queue<AudioFrame> temp = frames_;
    for (size_t i = 0; i < index && !temp.empty(); ++i) {
        temp.pop();
    }
    
    if (temp.empty()) {
        return AudioFrame{std::vector<int16_t>(), 0, VADLevel::NO_VOICE};
    }
    
    return temp.front();
}

std::vector<int16_t> VoiceCommandBuffer::extractVoiceSegment() const {
    std::vector<int16_t> result;
    
    bool in_voice = false;
    std::queue<AudioFrame> temp = frames_;
    
    while (!temp.empty()) {
        const auto& frame = temp.front();
        
        if (frame.vad_level > VADLevel::NO_VOICE) {
            result.insert(result.end(), frame.samples.begin(), frame.samples.end());
            in_voice = true;
        } else if (in_voice) {
            // End of voice segment detected
            break;
        }
        
        temp.pop();
    }
    
    return result;
}

// ============================================================================
// VoiceInput Implementation
// ============================================================================

VoiceInput::VoiceInput()
    : is_running_(false), is_listening_(false), current_vad_level_(VADLevel::NO_VOICE),
      frame_index_(0), last_voice_timestamp_(0) {
    
    vad_ = std::make_unique<VoiceActivityDetector>();
    preprocessor_ = std::make_unique<AudioPreprocessor>();
    buffer_ = std::make_unique<VoiceCommandBuffer>();
}

VoiceInput::~VoiceInput() {
    shutdown();
}

bool VoiceInput::initialize() {
    if (is_running_) return true;
    
    if (!vad_->initialize(0.6f)) return false;
    if (!preprocessor_->initialize(0.5f)) return false;
    
    buffer_->initialize(3200);  // ~10 seconds at 16kHz with 512-sample frames
    
    is_running_ = true;
    frame_index_ = 0;
    current_frame_.clear();
    recent_sound_levels_.clear();
    
    return true;
}

void VoiceInput::shutdown() {
    is_running_ = false;
    is_listening_ = false;
    buffer_->clear();
}

void VoiceInput::processAudioData(const std::vector<int16_t>& audio_samples) {
    if (!is_running_ || !is_listening_) return;
    
    // Add to current frame
    current_frame_.insert(current_frame_.end(), 
                         audio_samples.begin(), audio_samples.end());
    
    // Process complete frames
    while (current_frame_.size() >= AudioFormat::FRAME_SIZE) {
        std::vector<int16_t> frame(
            current_frame_.begin(),
            current_frame_.begin() + AudioFormat::FRAME_SIZE);
        
        current_frame_.erase(current_frame_.begin(),
                            current_frame_.begin() + AudioFormat::FRAME_SIZE);
        
        processAccumulatedFrame();
    }
}

void VoiceInput::startListening() {
    is_listening_ = true;
    buffer_->clear();
    last_voice_timestamp_ = 0;
}

void VoiceInput::stopListening() {
    is_listening_ = false;
}

void VoiceInput::setVADSensitivity(float sensitivity) {
    if (vad_) {
        vad_->initialize(sensitivity);
    }
}

std::vector<int16_t> VoiceInput::getVoiceBuffer() const {
    if (buffer_) {
        return buffer_->getBufferedAudio();
    }
    return std::vector<int16_t>();
}

void VoiceInput::clearVoiceBuffer() {
    if (buffer_) {
        buffer_->clear();
    }
}

VoiceInputStats VoiceInput::getStatistics() const {
    return statistics_;
}

void VoiceInput::registerVoiceCallback(VoiceDetectionCallback callback) {
    voice_callback_ = callback;
}

double VoiceInput::getAverageSoundLevel() const {
    if (recent_sound_levels_.empty()) return 0.0;
    
    double sum = 0.0;
    for (double level : recent_sound_levels_) {
        sum += level;
    }
    
    return sum / recent_sound_levels_.size();
}

void VoiceInput::processAccumulatedFrame() {
    if (current_frame_.size() != AudioFormat::FRAME_SIZE) return;
    
    // Create frame
    AudioFrame frame;
    frame.samples = current_frame_;
    frame.timestamp_ms = frame_index_ * (uint64_t)AudioFormat::FRAME_DURATION_MS;
    
    // Preprocess
    std::vector<int16_t> processed = preprocessor_->highPassFilter(current_frame_);
    processed = preprocessor_->reduceNoise(processed);
    
    // Detect voice activity
    auto [vad_level, confidence] = vad_->processFrame(processed);
    frame.vad_level = vad_level;
    
    // Calculate sound level
    double max_sample = 0.0;
    for (const auto& sample : processed) {
        max_sample = std::max(max_sample, std::abs(static_cast<double>(sample)));
    }
    double sound_level = max_sample / 32768.0;  // Normalize to 0.0-1.0
    
    recent_sound_levels_.push_back(sound_level);
    if (recent_sound_levels_.size() > 100) {
        recent_sound_levels_.erase(recent_sound_levels_.begin());
    }
    
    // Store in buffer
    if (buffer_) {
        buffer_->addFrame(frame);
    }
    
    // Update state
    current_vad_level_ = vad_level;
    if (vad_level > VADLevel::NO_VOICE) {
        last_voice_timestamp_ = frame.timestamp_ms;
    }
    
    // Call voice callback if voice detected
    if (vad_level > VADLevel::NO_VOICE && voice_callback_) {
        voice_callback_(processed, confidence);
    }
    
    updateStatistics();
    frame_index_++;
    current_frame_.clear();
}

void VoiceInput::updateStatistics() {
    statistics_.total_frames_processed++;
    
    if (current_vad_level_ > VADLevel::NO_VOICE) {
        statistics_.voice_frames_detected++;
    } else {
        statistics_.silence_frames++;
    }
    
    if (buffer_) {
        auto vad_stats = vad_->getStatistics();
        statistics_.current_noise_floor = vad_stats.current_noise_floor;
        statistics_.max_amplitude = vad_stats.max_amplitude;
    }
    
    if (statistics_.total_frames_processed > 0) {
        statistics_.average_vad_confidence = 
            (double)statistics_.voice_frames_detected / statistics_.total_frames_processed;
    }
}

} // namespace AICopilot
