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

#include <gtest/gtest.h>
#include "voice_input.hpp"
#include "speech_recognizer.hpp"
#include "voice_interpreter.hpp"
#include "voice_output.hpp"
#include <chrono>
#include <algorithm>

namespace AICopilot {

// ============================================================================
// VoiceInput Tests
// ============================================================================

class VoiceInputTest : public ::testing::Test {
protected:
    VoiceInput voice_input;
    
    std::vector<int16_t> generateTestAudio(size_t num_samples, bool with_voice = true) {
        std::vector<int16_t> audio;
        audio.reserve(num_samples);
        
        for (size_t i = 0; i < num_samples; ++i) {
            if (with_voice && i % 2 == 0) {
                audio.push_back(5000 + (i % 1000));  // Voice-like signal
            } else {
                audio.push_back(100 + (i % 10));     // Quiet noise
            }
        }
        
        return audio;
    }
};

TEST_F(VoiceInputTest, InitializeVoiceInput) {
    EXPECT_TRUE(voice_input.initialize());
    EXPECT_TRUE(voice_input.isRunning());
    EXPECT_FALSE(voice_input.isListening());
}

TEST_F(VoiceInputTest, StartStopListening) {
    EXPECT_TRUE(voice_input.initialize());
    
    voice_input.startListening();
    EXPECT_TRUE(voice_input.isListening());
    
    voice_input.stopListening();
    EXPECT_FALSE(voice_input.isListening());
}

TEST_F(VoiceInputTest, VoiceActivityDetection) {
    EXPECT_TRUE(voice_input.initialize());
    voice_input.startListening();
    
    // Process silence
    auto silence = generateTestAudio(1000, false);
    voice_input.processAudioData(silence);
    EXPECT_EQ(voice_input.getCurrentVADLevel(), VADLevel::NO_VOICE);
    
    // Process voice
    auto voice = generateTestAudio(1000, true);
    voice_input.processAudioData(voice);
    EXPECT_TRUE(voice_input.voiceDetected());
}

TEST_F(VoiceInputTest, GetAverageSoundLevel) {
    EXPECT_TRUE(voice_input.initialize());
    voice_input.startListening();
    
    auto audio = generateTestAudio(5000, true);
    voice_input.processAudioData(audio);
    
    double level = voice_input.getAverageSoundLevel();
    EXPECT_GE(level, 0.0);
    EXPECT_LE(level, 1.0);
}

TEST_F(VoiceInputTest, ClearVoiceBuffer) {
    EXPECT_TRUE(voice_input.initialize());
    voice_input.startListening();
    
    auto audio = generateTestAudio(2000, true);
    voice_input.processAudioData(audio);
    
    auto buffer = voice_input.getVoiceBuffer();
    EXPECT_GT(buffer.size(), 0);
    
    voice_input.clearVoiceBuffer();
    buffer = voice_input.getVoiceBuffer();
    EXPECT_EQ(buffer.size(), 0);
}

// ============================================================================
// SpeechRecognizer Tests
// ============================================================================

class SpeechRecognizerTest : public ::testing::Test {
protected:
    SpeechRecognizer recognizer;
    
    void SetUp() override {
        ASSERT_TRUE(recognizer.initialize());
    }
};

TEST_F(SpeechRecognizerTest, RecognizeClimbCommand) {
    RecognitionResult result = recognizer.recognizeText("climb to twenty thousand");
    
    EXPECT_EQ(result.command, "SET_ALTITUDE");
    EXPECT_GE(result.confidence, 0.85);
}

TEST_F(SpeechRecognizerTest, RecognizeDirectCommand) {
    RecognitionResult result = recognizer.recognizeText("direct to lima");
    
    EXPECT_EQ(result.command, "DIRECT_TO_WAYPOINT");
    EXPECT_GE(result.confidence, 0.85);
}

TEST_F(SpeechRecognizerTest, RecognizeRunwayCommand) {
    RecognitionResult result = recognizer.recognizeText("find best runway");
    
    EXPECT_EQ(result.command, "FIND_BEST_RUNWAY");
    EXPECT_GE(result.confidence, 0.85);
}

TEST_F(SpeechRecognizerTest, RecognizeWeatherCommand) {
    RecognitionResult result = recognizer.recognizeText("current weather");
    
    EXPECT_EQ(result.command, "CURRENT_WEATHER");
    EXPECT_GE(result.confidence, 0.85);
}

TEST_F(SpeechRecognizerTest, RecognizePreflightCommand) {
    RecognitionResult result = recognizer.recognizeText("preflight checklist");
    
    EXPECT_EQ(result.command, "PREFLIGHT_CHECKLIST");
    EXPECT_GE(result.confidence, 0.85);
    EXPECT_TRUE(recognizer.requiresConfirmation("PREFLIGHT_CHECKLIST"));
}

TEST_F(SpeechRecognizerTest, RecognizeEmergencyCommand) {
    RecognitionResult result = recognizer.recognizeText("emergency landing");
    
    EXPECT_EQ(result.command, "EMERGENCY_LANDING");
    EXPECT_GE(result.confidence, 0.85);
    EXPECT_TRUE(recognizer.requiresConfirmation("EMERGENCY_LANDING"));
}

TEST_F(SpeechRecognizerTest, RecognizeTerrainWarningCommand) {
    RecognitionResult result = recognizer.recognizeText("terrain warning");
    
    EXPECT_EQ(result.command, "TERRAIN_WARNING_CHECK");
    EXPECT_GE(result.confidence, 0.85);
}

TEST_F(SpeechRecognizerTest, CommandStatistics) {
    recognizer.recognizeText("climb to ten thousand");
    recognizer.recognizeText("set heading two seven zero");
    recognizer.recognizeText("climb to ten thousand");
    
    auto stats = recognizer.getStatistics();
    EXPECT_EQ(stats.total_attempts, 3);
    EXPECT_GE(stats.successful_recognitions, 2);
}

TEST_F(SpeechRecognizerTest, GetAvailableCommands) {
    auto commands = recognizer.getAvailableCommands();
    
    EXPECT_GT(commands.size(), 40);  // Should have 50+ commands
    
    auto it = std::find(commands.begin(), commands.end(), "SET_ALTITUDE");
    EXPECT_NE(it, commands.end());
}

TEST_F(SpeechRecognizerTest, CommandCategoryFiltering) {
    // Test that categories can be identified
    auto all_commands = recognizer.getAvailableCommands();
    EXPECT_GT(all_commands.size(), 0);
    
    // Verify some expected commands exist
    auto it = std::find(all_commands.begin(), all_commands.end(), "SET_ALTITUDE");
    EXPECT_NE(it, all_commands.end());
}

// ============================================================================
// Voice Interpreter Tests
// ============================================================================

class VoiceInterpreterTest : public ::testing::Test {
protected:
    VoiceInterpreter interpreter;
    
    void SetUp() override {
        ASSERT_TRUE(interpreter.initialize());
    }
};

TEST_F(VoiceInterpreterTest, InterpretSetAltitudeCommand) {
    std::map<std::string, std::string> params = {{"altitude", "15000"}};
    SystemAction action = interpreter.interpretCommandWithParameters("SET_ALTITUDE", params);
    
    EXPECT_EQ(action.type, SystemActionType::SET_ALTITUDE);
    EXPECT_EQ(action.parameters["altitude"], "15000");
    EXPECT_FALSE(action.requires_confirmation);
}

TEST_F(VoiceInterpreterTest, InterpretNavigationCommand) {
    std::map<std::string, std::string> params = {{"waypoint", "LIMA"}};
    SystemAction action = interpreter.interpretCommandWithParameters("DIRECT_TO_WAYPOINT", params);
    
    EXPECT_EQ(action.type, SystemActionType::DIRECT_TO_WAYPOINT);
}

TEST_F(VoiceInterpreterTest, ValidateAltitudeParameter) {
    std::map<std::string, std::string> params = {{"altitude", "55000"}};
    SystemAction action = interpreter.interpretCommandWithParameters("SET_ALTITUDE", params);
    
    // Altitude should be clamped to 50000
    double alt = std::stod(action.parameters["altitude"]);
    EXPECT_LE(alt, 50000);
}

TEST_F(VoiceInterpreterTest, ValidateHeadingParameter) {
    std::map<std::string, std::string> params = {{"heading", "380"}};
    SystemAction action = interpreter.interpretCommandWithParameters("SET_HEADING", params);
    
    // Heading should be normalized to 0-360
    double hdg = std::stod(action.parameters["heading"]);
    EXPECT_GE(hdg, 0);
    EXPECT_LT(hdg, 360);
}

TEST_F(VoiceInterpreterTest, PrefligthChecklistRequiresConfirmation) {
    SystemAction action = interpreter.interpretCommand("PREFLIGHT_CHECKLIST");
    
    EXPECT_EQ(action.type, SystemActionType::EXECUTE_CHECKLIST);
    EXPECT_TRUE(action.requires_confirmation);
}

TEST_F(VoiceInterpreterTest, EmergencyCommandHighPriority) {
    SystemAction action = interpreter.interpretCommand("EMERGENCY_LANDING");
    
    EXPECT_EQ(action.type, SystemActionType::EMERGENCY_LANDING);
    EXPECT_EQ(action.priority, 1.0);  // Critical priority
    EXPECT_TRUE(action.requires_confirmation);
}

TEST_F(VoiceInterpreterTest, GetLastInterpretedAction) {
    std::map<std::string, std::string> params = {{"altitude", "10000"}};
    interpreter.interpretCommandWithParameters("SET_ALTITUDE", params);
    
    SystemAction last = interpreter.getLastAction();
    EXPECT_EQ(last.type, SystemActionType::SET_ALTITUDE);
}

// ============================================================================
// Voice Output Tests
// ============================================================================

class VoiceOutputTest : public ::testing::Test {
protected:
    VoiceOutput voice_output;
    bool callback_called = false;
    SynthesizedSpeech last_speech;
    
    void SetUp() override {
        ASSERT_TRUE(voice_output.initialize());
        voice_output.registerAudioCallback(
            [this](const SynthesizedSpeech& speech) {
                callback_called = true;
                last_speech = speech;
            });
    }
};

TEST_F(VoiceOutputTest, PlayCommandConfirmation) {
    voice_output.playCommandConfirmation("Set altitude to 10000 feet");
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(last_speech.output_type, VoiceOutputType::COMMAND_CONFIRMATION);
}

TEST_F(VoiceOutputTest, PlayActionReadback) {
    voice_output.playActionReadback("Altitude set to 15000 feet");
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(last_speech.output_type, VoiceOutputType::ACTION_READBACK);
}

TEST_F(VoiceOutputTest, PlayWarningAlert) {
    callback_called = false;
    voice_output.setEnabled(false);
    
    voice_output.playWarningAlert("Terrain ahead!");
    
    // Warnings should play even when disabled
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(last_speech.output_type, VoiceOutputType::WARNING_ALERT);
}

TEST_F(VoiceOutputTest, RespectEnabledState) {
    callback_called = false;
    voice_output.setEnabled(false);
    
    voice_output.playInformation("This is information");
    
    // Non-critical messages should not play when disabled
    EXPECT_FALSE(callback_called);
}

TEST_F(VoiceOutputTest, SetActiveSpeaker) {
    voice_output.setActiveSpeaker(SpeakerProfile::CLEAR_ENUNCIATION);
    
    voice_output.playStatusAnnouncement("Current status");
    
    EXPECT_TRUE(callback_called);
}

TEST_F(VoiceOutputTest, GenerateAltitudeReadback) {
    ReadbackGenerator gen;
    
    std::string readback = gen.generateAltitudeReadback(25000);
    EXPECT_NE(readback.find("Flight level"), std::string::npos);
    
    readback = gen.generateAltitudeReadback(5000);
    EXPECT_NE(readback.find("5000"), std::string::npos);
}

TEST_F(VoiceOutputTest, GenerateHeadingReadback) {
    ReadbackGenerator gen;
    
    std::string readback = gen.generateHeadingReadback(270);
    EXPECT_NE(readback.find("270"), std::string::npos);
}

TEST_F(VoiceOutputTest, GenerateSpeedReadback) {
    ReadbackGenerator gen;
    
    std::string readback = gen.generateSpeedReadback(150);
    EXPECT_NE(readback.find("150"), std::string::npos);
    EXPECT_NE(readback.find("knots"), std::string::npos);
}

// ============================================================================
// Integration Tests
// ============================================================================

class VoiceInterfaceIntegrationTest : public ::testing::Test {
protected:
    VoiceInput voice_input;
    SpeechRecognizer recognizer;
    VoiceInterpreter interpreter;
    VoiceOutput voice_output;
    
    void SetUp() override {
        ASSERT_TRUE(voice_input.initialize());
        ASSERT_TRUE(recognizer.initialize());
        ASSERT_TRUE(interpreter.initialize());
        ASSERT_TRUE(voice_output.initialize());
    }
};

TEST_F(VoiceInterfaceIntegrationTest, EndToEndVoiceCommand) {
    voice_input.startListening();
    
    // Simulate recognition
    RecognitionResult result = recognizer.recognizeText("climb to twenty thousand feet");
    EXPECT_EQ(result.command, "SET_ALTITUDE");
    
    // Interpret command
    std::map<std::string, std::string> params = {{"altitude", "20000"}};
    SystemAction action = interpreter.interpretCommandWithParameters(result.command, params);
    EXPECT_EQ(action.type, SystemActionType::SET_ALTITUDE);
    
    // Generate readback
    ReadbackGenerator gen;
    std::string readback = gen.generateAltitudeReadback(20000);
    EXPECT_NE(readback.find("Flight level"), std::string::npos);
}

TEST_F(VoiceInterfaceIntegrationTest, MultipleCommandSequence) {
    std::vector<std::string> commands = {
        "set heading two seven zero",
        "reduce speed to one five zero",
        "direct to lima",
        "request runway information"
    };
    
    for (const auto& cmd : commands) {
        RecognitionResult result = recognizer.recognizeText(cmd);
        EXPECT_NE(result.command, "UNKNOWN");
        
        SystemAction action = interpreter.interpretCommand(result.command);
        EXPECT_NE(action.type, SystemActionType::UNKNOWN);
    }
}

TEST_F(VoiceInterfaceIntegrationTest, CommandRecognitionAccuracy) {
    std::vector<std::pair<std::string, std::string>> test_cases = {
        {"climb to ten thousand", "SET_ALTITUDE"},
        {"set heading one eight zero", "SET_HEADING"},
        {"direct to charlie", "DIRECT_TO_WAYPOINT"},
        {"find best runway", "FIND_BEST_RUNWAY"},
        {"current weather", "CURRENT_WEATHER"},
        {"system status", "SYSTEM_STATUS"},
        {"stabilized approach", "STABILIZED_APPROACH_CHECK"},
    };
    
    int successful = 0;
    for (const auto& [spoken, expected_cmd] : test_cases) {
        RecognitionResult result = recognizer.recognizeText(spoken);
        if (result.command == expected_cmd && result.confidence >= 0.85) {
            successful++;
        }
    }
    
    double accuracy = (double)successful / test_cases.size();
    EXPECT_GE(accuracy, 0.90);  // 90%+ accuracy
}

TEST_F(VoiceInterfaceIntegrationTest, ResponsePerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 50; ++i) {
        recognizer.recognizeText("climb to ten thousand feet");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    double avg_time = (double)duration_ms / 50;
    EXPECT_LT(avg_time, 200);  // < 200ms per command
}

} // namespace AICopilot

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
