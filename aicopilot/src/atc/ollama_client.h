#pragma once

#include <string>
#include <optional>
#include <chrono>

namespace AICopilot {

class OllamaClient {
public:
    struct Response {
        bool ok;
        std::string text;
        int httpStatus = 0;
        std::string error;
    };

    OllamaClient(const std::string &baseUrl = "http://localhost:11434", int connectTimeoutSec = 3, int timeoutSec = 10);
    ~OllamaClient();

    // Send a prompt and receive the model response (synchronous)
    Response generate(const std::string &model, const std::string &prompt);

    // Configure timeouts
    void setTimeouts(int connectSec, int totalSec);

private:
    std::string baseUrl_;
    int connectTimeoutSec_;
    int timeoutSec_;
};

} // namespace AICopilot
