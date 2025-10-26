# Ollama Integration for ATC Menu Selection

## Overview

The AI Copilot FS now supports **Ollama** integration for intelligent ATC menu selection. Ollama is a local LLM (Large Language Model) runtime that allows you to run powerful AI models like Llama 2, Llama 3, Mistral, and others locally on your machine.

## Why Ollama?

The rule-based ATC menu selection system works well for standard scenarios, but Ollama integration provides:

- **Better Context Understanding**: LLMs can understand complex ATC messages with nuance
- **Natural Language Processing**: Better interpretation of non-standard phraseology
- **Improved Accuracy**: More accurate menu selection in ambiguous situations
- **Learning from Examples**: Can handle edge cases better through training data
- **Fallback Support**: Automatically falls back to rule-based selection if Ollama is unavailable

## Prerequisites

### 1. Install Ollama

Download and install Ollama from: https://ollama.ai

**Linux:**
```bash
curl -fsSL https://ollama.ai/install.sh | sh
```

**macOS:**
```bash
brew install ollama
```

**Windows:**
Download installer from https://ollama.ai/download

### 2. Pull a Model

After installing Ollama, pull a model (e.g., Llama 2):

```bash
ollama pull llama2
```

Recommended models:
- `llama2` (7B) - Good balance of speed and accuracy
- `llama3` (8B) - Better accuracy, slightly slower
- `mistral` (7B) - Fast and accurate
- `phi` (2.7B) - Very fast, good for quick responses

### 3. Start Ollama Server

The Ollama server should start automatically after installation. Verify it's running:

```bash
curl http://localhost:11434/api/tags
```

If not running, start it with:
```bash
ollama serve
```

## Usage

### Command Line

Enable Ollama integration with the `--ollama` flag:

```bash
# Basic usage with Ollama
./aicopilot_advanced --ollama aircraft.cfg flightplan.pln

# Specify custom model
./aicopilot_advanced --ollama --ollama-model llama3 aircraft.cfg plan.pln

# Custom Ollama host (if running remotely)
./aicopilot_advanced --ollama --ollama-host http://192.168.1.100:11434 aircraft.cfg plan.pln
```

### Programmatic API

```cpp
#include "ai_pilot.h"

AICopilot::AIPilot pilot;

// Initialize
pilot.initialize(AICopilot::SimulatorType::MSFS2024);
pilot.startAutonomousFlight();

// Enable Ollama (after startAutonomousFlight)
pilot.enableOllamaATC(true, "http://localhost:11434");
pilot.setOllamaModel("llama2");

// Check if enabled successfully
if (pilot.isOllamaEnabled()) {
    std::cout << "Ollama AI enabled!" << std::endl;
}
```

## How It Works

### 1. Prompt Generation

When an ATC message arrives with menu options, the system generates a prompt like:

```
You are an AI assistant helping a pilot select the best ATC response.

Current Flight Phase: TAKEOFF

ATC Message: "N12345, you are cleared for takeoff runway 16R"

Available Response Options:
0. Roger, cleared for takeoff runway 16R
1. Unable
2. Request taxi instructions
3. Standby

Based on the flight phase, ATC message, and available options, select the 
MOST APPROPRIATE response by responding with ONLY the option number (0-3).
```

### 2. LLM Processing

The prompt is sent to Ollama, which analyzes:
- Current flight phase context
- ATC message content
- Available menu options
- Standard aviation procedures

### 3. Response Parsing

The LLM responds with a number (e.g., "0"), which is parsed and validated.

### 4. Fallback

If Ollama:
- Is not available
- Fails to connect
- Returns an invalid response
- Times out

The system automatically falls back to the rule-based selection algorithm.

## Configuration

### Model Selection

Different models have different characteristics:

| Model | Size | Speed | Accuracy | Use Case |
|-------|------|-------|----------|----------|
| phi | 2.7B | ★★★★★ | ★★★☆☆ | Quick responses, testing |
| llama2 | 7B | ★★★★☆ | ★★★★☆ | Balanced, recommended |
| mistral | 7B | ★★★★☆ | ★★★★★ | High accuracy |
| llama3 | 8B | ★★★☆☆ | ★★★★★ | Best accuracy |

### Temperature Setting

The Ollama client uses `temperature = 0.3` for deterministic responses. This can be adjusted in `ollama_client.cpp` if needed.

### Timeout

Default timeout is 30 seconds for LLM responses. This ensures the system doesn't hang waiting for a response.

## Performance Considerations

- **First Request**: May take 2-5 seconds as the model loads into memory
- **Subsequent Requests**: Typically 0.5-2 seconds depending on model and hardware
- **Memory**: Models require 4-16 GB RAM depending on size
- **CPU vs GPU**: GPU acceleration significantly improves response time

## Troubleshooting

### Ollama Connection Failed

**Symptom:** `[Ollama] Connection failed: Couldn't connect to server`

**Solutions:**
1. Check if Ollama is running: `curl http://localhost:11434/api/tags`
2. Start Ollama: `ollama serve`
3. Check firewall settings if using remote host

### Model Not Found

**Symptom:** `[Ollama] Could not parse valid option from: ...`

**Solutions:**
1. Pull the model: `ollama pull llama2`
2. List available models: `ollama list`
3. Verify model name matches

### Slow Responses

**Symptom:** Long delays between ATC messages and responses

**Solutions:**
1. Use smaller model (e.g., `phi` instead of `llama3`)
2. Ensure Ollama has GPU access
3. Close other applications to free up RAM
4. Consider increasing timeout in code

### Invalid Responses

**Symptom:** `[Ollama] Could not parse valid option from: ...`

**Solution:** The system will automatically fall back to rule-based selection. This is normal for edge cases.

## Comparison: Ollama vs Rule-Based

### Rule-Based Selection
✅ Fast (< 1ms)
✅ No external dependencies
✅ Predictable behavior
❌ Limited context understanding
❌ May struggle with unusual phrasing

### Ollama AI Selection
✅ Better context understanding
✅ Handles complex scenarios
✅ Natural language processing
❌ Slower (0.5-2 seconds)
❌ Requires local LLM installation
❌ Uses more resources

## Best Practices

1. **Start Without Ollama**: Test your setup with rule-based selection first
2. **Use Appropriate Model**: Start with `llama2`, upgrade to `llama3` if needed
3. **Monitor Performance**: Check response times in verbose mode
4. **Keep Ollama Updated**: `ollama pull <model>` to get latest versions
5. **Fallback is OK**: Don't worry if Ollama occasionally falls back to rules

## Example Session

```bash
$ ./aicopilot_advanced --ollama --ollama-model llama2 --verbose

╔════════════════════════════════════════════════════════════════╗
║   AI Copilot FS - Advanced Autonomous Flight System           ║
║   Microsoft Flight Simulator 2024 & Prepar3D V6               ║
╚════════════════════════════════════════════════════════════════╝

[INIT] Connecting to simulator...
[OK] Connected successfully

[START] Initiating autonomous flight operations...
[OLLAMA] Enabling Ollama AI for ATC menu selection...
[Ollama] Connected to http://localhost:11434
[Ollama] Model set to: llama2
[OLLAMA] ✓ Ollama AI enabled successfully

[ATC] ATC: "N12345, Seattle ground, taxi to runway 16R via alpha, bravo"
[ATC] Using Ollama AI for menu selection
[Ollama] Querying LLM for ATC menu selection...
[Ollama] Selected option 2 from LLM response
[ATC] Ollama selected option 2: Roger, taxi to runway 16R via alpha, bravo
```

## Integration Details

### Files Added
- `aicopilot/include/ollama_client.h` - Ollama client header
- `aicopilot/src/atc/ollama_client.cpp` - Ollama client implementation

### Files Modified
- `aicopilot/include/atc_controller.h` - Added Ollama methods
- `aicopilot/src/atc/atc_controller.cpp` - Integrated Ollama selection
- `aicopilot/include/ai_pilot.h` - Added Ollama API methods
- `aicopilot/src/ai/ai_pilot.cpp` - Implemented Ollama API
- `aicopilot/examples/advanced_example.cpp` - Added Ollama CLI options
- `CMakeLists.txt` - Added libcurl and jsoncpp dependencies

### Dependencies
- **libcurl**: HTTP communication with Ollama server
- **jsoncpp**: JSON parsing for Ollama API responses

## Future Enhancements

- **Fine-tuning**: Train models on aviation-specific ATC communications
- **Context Memory**: Remember previous ATC interactions for better context
- **Multi-turn**: Handle complex multi-message ATC sequences
- **Voice Integration**: Combine with speech recognition for voice ATC
- **Performance Metrics**: Track Ollama accuracy vs rule-based selection

## Support

For issues or questions:
1. Check Ollama documentation: https://ollama.ai/docs
2. Verify model compatibility
3. Review system logs for error messages
4. Test with `--verbose` flag for detailed output
