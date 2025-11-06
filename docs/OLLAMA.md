# Ollama Integration (AI Copilot FS)

This document explains how to enable the full Ollama client in the AI Copilot FS project.

Summary
- By default the repository builds with a stub implementation of the Ollama client to allow building without external dependencies.
- To enable the full implementation you must provide `libcurl` and `jsoncpp` at CMake configure time.

Recommended (vcpkg) installation (Windows x64)
1. Install vcpkg (if you don't have it):
```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

2. Install required packages:
```powershell
C:\vcpkg\vcpkg.exe install curl[openssl]:x64-windows jsoncpp:x64-windows
```

3. Configure the project using the vcpkg toolchain file and enable Ollama:
```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DENABLE_OLLAMA=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 4
```

Notes and troubleshooting
- If `-DENABLE_OLLAMA=ON` is used but `curl`/`jsoncpp` are not found, CMake will fall back to the stub implementation and print a warning.
- Make sure your vcpkg path is correct and you used the x64 triplet if building 64-bit.
- If you prefer not to use vcpkg, install libcurl and jsoncpp using your system/package manager and ensure CMake can find the headers/libraries.

Runtime
- By default the Ollama client will try to connect to `http://localhost:11434`.
- If you run a local Ollama server, call `OllamaClient::connect()` with the correct URL.

Security
- The client supports passing an API key using `OllamaClient::setApiKey()` for enterprise or cloud deployments that require a bearer token.

Example usage (C++):
```cpp
AICopilot::OllamaClient client;
if (client.connect("http://localhost:11434")) {
    int idx = client.selectATCMenuOption(atcText, options, "APPROACH", context);
}
```

Contact
- If you run into issues, open an issue in the repository with the exact CMake output and your platform details.
