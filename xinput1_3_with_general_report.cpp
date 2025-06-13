#include <windows.h>
#include <psapi.h>
// Note: Use XInput1_3.dll instead of winhttp.dll to avoid network issues
// XInput is much safer to hijack as it only handles controller input

// XInput types and constants
#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y                0x8000

typedef struct _XINPUT_GAMEPAD {
    WORD wButtons;
    BYTE bLeftTrigger;
    BYTE bRightTrigger;
    SHORT sThumbLX;
    SHORT sThumbLY;
    SHORT sThumbRX;
    SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE {
    DWORD dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef struct _XINPUT_VIBRATION {
    WORD wLeftMotorSpeed;
    WORD wRightMotorSpeed;
} XINPUT_VIBRATION, *PXINPUT_VIBRATION;

typedef struct _XINPUT_CAPABILITIES {
    BYTE Type;
    BYTE SubType;
    WORD Flags;
    XINPUT_GAMEPAD Gamepad;
    XINPUT_VIBRATION Vibration;
} XINPUT_CAPABILITIES, *PXINPUT_CAPABILITIES;

#include <stdio.h>
#include <string>
#include <map>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>

// MinHook library
#include "MinHook/include/MinHook.h"
#pragma comment(lib, "MinHook/lib/libMinHook.x64.lib")

// Global variables
HMODULE g_hOriginalXInput = NULL;
std::map<std::string, std::string> g_comicMappings;
FILE* g_logFile = nullptr;
bool g_hooksInstalled = false;
std::string g_baseDir = "gakumas-local-texture";  // 新的基础目录

// Function declarations
bool IsTargetGameProcess();
void WriteLog(const char* message);
void WriteLogf(const char* format, ...);
void InitializeLogFile();
void CreateDirectory(const std::string& path);
DWORD WINAPI ComicHookInstallThread(LPVOID lpParam);

// General Report Hook declarations
bool ShouldReplaceGeneralReport(const std::string& assetName, std::string& replacementPath);
bool ShouldReplaceUI(const std::string& assetName, std::string& replacementPath);
bool ShouldReplaceTutorial(const std::string& assetName, std::string& replacementPath);
void AnalyzeGeneralReportAsset(const std::string& assetName);
void AnalyzeAllAssets(const std::string& assetName);
bool InstallGeneralReportHooks();

// Original xinput1_3.dll function pointers
typedef DWORD (WINAPI *XInputGetState_t)(DWORD, PXINPUT_STATE);
typedef DWORD (WINAPI *XInputSetState_t)(DWORD, PXINPUT_VIBRATION);
typedef DWORD (WINAPI *XInputGetCapabilities_t)(DWORD, DWORD, PXINPUT_CAPABILITIES);
typedef void (WINAPI *XInputEnable_t)(BOOL);

// Original function pointers
XInputGetState_t Original_XInputGetState = nullptr;
XInputSetState_t Original_XInputSetState = nullptr;
XInputGetCapabilities_t Original_XInputGetCapabilities = nullptr;
XInputEnable_t Original_XInputEnable = nullptr;

// Unity function signatures
typedef void* (*AssetBundle_LoadAssetAsync_t)(void* bundle, void* name, void* type);
typedef void* (*AssetBundleRequest_GetResult_t)(void* request);
typedef void* (*Resources_Load_t)(void* path, void* type);

// Original function pointers
AssetBundle_LoadAssetAsync_t Original_AssetBundle_LoadAssetAsync = nullptr;
AssetBundleRequest_GetResult_t Original_AssetBundleRequest_GetResult = nullptr;
Resources_Load_t Original_Resources_Load = nullptr;

// IL2CPP String structure
struct Il2CppString {
    void* klass;
    void* monitor;
    int32_t length;
    wchar_t chars[1];
};

// IL2CPP API function pointers (dynamically loaded)
typedef void* (*il2cpp_domain_get_t)();
typedef void* (*il2cpp_assembly_get_image_t)(void* assembly);
typedef void* (*il2cpp_class_from_name_t)(void* image, const char* namespaze, const char* name);
typedef void* (*il2cpp_class_get_method_from_name_t)(void* klass, const char* name, int param_count);
typedef void* (*il2cpp_domain_get_assemblies_t)(void* domain, size_t* size);
typedef void* (*il2cpp_object_new_t)(void* klass);
typedef void* (*il2cpp_array_new_t)(void* elementTypeInfo, uintptr_t length);
typedef void* (*il2cpp_string_new_t)(const char* str);
typedef void* (*il2cpp_runtime_invoke_t)(void* method, void* obj, void** params, void** exc);

// Global IL2CPP function pointers
il2cpp_domain_get_t il2cpp_domain_get = nullptr;
il2cpp_assembly_get_image_t il2cpp_assembly_get_image = nullptr;
il2cpp_class_from_name_t il2cpp_class_from_name = nullptr;
il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name = nullptr;
il2cpp_domain_get_assemblies_t il2cpp_domain_get_assemblies = nullptr;
il2cpp_object_new_t il2cpp_object_new = nullptr;
il2cpp_array_new_t il2cpp_array_new = nullptr;
il2cpp_string_new_t il2cpp_string_new = nullptr;
il2cpp_runtime_invoke_t il2cpp_runtime_invoke = nullptr;

// Utility functions
// 创建目录的辅助函数
void CreateDirectory(const std::string& path) {
    CreateDirectoryA(path.c_str(), NULL);
}

// 获取当前时间戳字符串
std::string GetTimeStamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timestamp[64];
    sprintf_s(timestamp, "%04d%02d%02d_%02d%02d%02d", 
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::string(timestamp);
}

// 初始化日志文件
void InitializeLogFile() {
    if (g_logFile) return;  // 已经初始化过了
    
    // 创建日志目录
    std::string logDir = g_baseDir + "\\logs";
    CreateDirectory(g_baseDir.c_str());
    CreateDirectory(logDir.c_str());
    
    // 生成带时间戳的日志文件名
    std::string timestamp = GetTimeStamp();
    std::string logFileName = logDir + "\\gakumas_texture_replace_" + timestamp + ".log";
    
    // 打开日志文件
    fopen_s(&g_logFile, logFileName.c_str(), "w");
    if (g_logFile) {
        fprintf(g_logFile, "=== Gakumas Texture Replace System Log ===\n");
        fprintf(g_logFile, "Start Time: %s\n", timestamp.c_str());
        fprintf(g_logFile, "Base Directory: %s\n", g_baseDir.c_str());
        fprintf(g_logFile, "==========================================\n\n");
        fflush(g_logFile);
    }
}

void WriteLog(const char* message) {
    // 确保日志文件已初始化
    if (!g_logFile) {
        InitializeLogFile();
    }
    
    if (g_logFile) {
        fprintf(g_logFile, "[ComicReplace-XInput-Optimized] %s\n", message);
        fflush(g_logFile);
    }
    printf("[ComicReplace-XInput-Optimized] %s\n", message);
}

void WriteLogf(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    WriteLog(buffer);
}

// Check if current process is the target game process
bool IsTargetGameProcess() {
    char processName[MAX_PATH];
    DWORD size = GetModuleBaseNameA(GetCurrentProcess(), NULL, processName, MAX_PATH);
    
    if (size > 0) {
        WriteLogf("Current process: %s", processName);
        
        // Only allow gakumas.exe
        if (strcmp(processName, "gakumas.exe") == 0) {
            WriteLog("Target process detected: gakumas.exe");
            return true;
        }
        
        // Reject Unity related processes
        if (strstr(processName, "UnityCrashHandler") != NULL ||
            strstr(processName, "UnityShaderCompiler") != NULL ||
            strstr(processName, "Unity") != NULL) {
            WriteLogf("Rejecting Unity helper process: %s", processName);
            return false;
        }
        
        WriteLogf("Non-target process: %s", processName);
        return false;
    }
    
    WriteLog("Failed to get process name");
    return false;
}

// Load comic mapping configuration
bool LoadComicMappings() {
    // 使用新的配置文件路径
    std::string configPath = g_baseDir + "\\asset_mapping.txt";
    std::ifstream file(configPath);
    if (!file.is_open()) {
        WriteLogf("Failed to open %s", configPath.c_str());
        return false;
    }

    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string assetId = line.substr(0, equalPos);
            std::string localPath = line.substr(equalPos + 1);
            
            // Remove spaces
            while (!assetId.empty() && assetId.back() == ' ') assetId.pop_back();
            while (!localPath.empty() && localPath.front() == ' ') localPath.erase(0, 1);
            
            // 转换为完整路径：gakumas-local-texture/comic/xxx.png
            std::string fullPath = g_baseDir + "\\" + localPath;
            g_comicMappings[assetId] = fullPath;
            count++;
        }
    }
    
    WriteLogf("Loaded %d comic mappings from %s", count, configPath.c_str());
    return count > 0;
}

// Safe IL2CPP string conversion
std::string Il2CppStringToStdString(void* il2cppString) {
    if (!il2cppString) return "";
    
    if (IsBadReadPtr(il2cppString, sizeof(Il2CppString))) {
        return "";
    }
    
    Il2CppString* str = (Il2CppString*)il2cppString;
    
    if (str->length <= 0 || str->length > 1000) {
        return "";
    }
    
    if (IsBadReadPtr(str->chars, str->length * sizeof(wchar_t))) {
        return "";
    }
    
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, str->chars, str->length, nullptr, 0, nullptr, nullptr);
    if (bufferSize <= 0) return "";
    
    std::string result(bufferSize, 0);
    WideCharToMultiByte(CP_UTF8, 0, str->chars, str->length, &result[0], bufferSize, nullptr, nullptr);
    
    return result;
}

// Initialize IL2CPP API functions
bool InitializeIL2CPPApi() {
    WriteLog("Initializing IL2CPP API...");
    
    HMODULE hGameAssembly = GetModuleHandleA("GameAssembly.dll");
    if (!hGameAssembly) {
        WriteLog("Failed to get GameAssembly.dll handle");
        return false;
    }
    
    // Load IL2CPP API functions
    il2cpp_domain_get = (il2cpp_domain_get_t)GetProcAddress(hGameAssembly, "il2cpp_domain_get");
    il2cpp_assembly_get_image = (il2cpp_assembly_get_image_t)GetProcAddress(hGameAssembly, "il2cpp_assembly_get_image");
    il2cpp_class_from_name = (il2cpp_class_from_name_t)GetProcAddress(hGameAssembly, "il2cpp_class_from_name");
    il2cpp_class_get_method_from_name = (il2cpp_class_get_method_from_name_t)GetProcAddress(hGameAssembly, "il2cpp_class_get_method_from_name");
    il2cpp_domain_get_assemblies = (il2cpp_domain_get_assemblies_t)GetProcAddress(hGameAssembly, "il2cpp_domain_get_assemblies");
    il2cpp_object_new = (il2cpp_object_new_t)GetProcAddress(hGameAssembly, "il2cpp_object_new");
    il2cpp_array_new = (il2cpp_array_new_t)GetProcAddress(hGameAssembly, "il2cpp_array_new");
    il2cpp_string_new = (il2cpp_string_new_t)GetProcAddress(hGameAssembly, "il2cpp_string_new");
    il2cpp_runtime_invoke = (il2cpp_runtime_invoke_t)GetProcAddress(hGameAssembly, "il2cpp_runtime_invoke");
    
    bool success = (il2cpp_domain_get && il2cpp_assembly_get_image && 
                   il2cpp_class_from_name && il2cpp_class_get_method_from_name &&
                   il2cpp_object_new && il2cpp_string_new && il2cpp_runtime_invoke);
    
    WriteLogf("IL2CPP API initialization: %s", success ? "SUCCESS" : "FAILED");
    return success;
}

// Find IL2CPP method with safe memory access
void* FindIL2CPPMethod(const char* assemblyName, const char* namespaceName, const char* className, const char* methodName, int paramCount) {
    if (!il2cpp_domain_get) return nullptr;
    
    void* domain = il2cpp_domain_get();
    if (!domain) return nullptr;
    
    size_t assemblyCount = 0;
    void** assemblies = (void**)il2cpp_domain_get_assemblies(domain, &assemblyCount);
    if (!assemblies) return nullptr;
    
    for (size_t i = 0; i < assemblyCount; i++) {
        void* assembly = assemblies[i];
        if (!assembly) continue;
        
        void* image = il2cpp_assembly_get_image(assembly);
        if (!image) continue;
        
        void* klass = il2cpp_class_from_name(image, namespaceName, className);
        if (!klass) continue;
        
        void* method = il2cpp_class_get_method_from_name(klass, methodName, paramCount);
        if (!method) continue;
        
        if (IsBadReadPtr(method, sizeof(void*))) {
            continue;
        }
        
        void* executableAddr = *((void**)((uintptr_t)method + 0));
        if (executableAddr && (uintptr_t)executableAddr > 0x10000) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(executableAddr, &mbi, sizeof(mbi))) {
                bool isExecutable = (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
                if (isExecutable) {
                    WriteLogf("Found executable address via +0 offset: 0x%p", executableAddr);
                    return executableAddr;
                }
            }
        }
    }
    
    return nullptr;
}

// ===== GENERAL REPORT HOOK FUNCTIONS =====

// 专门处理general_report图像的函数
bool ShouldReplaceGeneralReport(const std::string& assetName, std::string& replacementPath) {
    // 专门处理general_report图像
    if (assetName.find("img_general_report") != std::string::npos) {
        for (const auto& mapping : g_comicMappings) {
            if (assetName.find(mapping.first) != std::string::npos) {
                DWORD attr = GetFileAttributesA(mapping.second.c_str());
                if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                    replacementPath = mapping.second;
                    WriteLogf("*** GENERAL_REPORT REPLACEMENT MATCHED! ***");
                    WriteLogf("Asset: %s -> File: %s", assetName.c_str(), replacementPath.c_str());
                    return true;
                } else {
                    WriteLogf("General report replacement file not found: %s", mapping.second.c_str());
                }
            }
        }
    }
    return false;
}

// 专门处理UI图像的函数 - 只处理按钮
bool ShouldReplaceUI(const std::string& assetName, std::string& replacementPath) {
    // 只检查按钮相关的UI图像，避免游戏卡死
    if (assetName.find("img_general") != std::string::npos && 
        (assetName.find("_btn") != std::string::npos || assetName.find("_button") != std::string::npos)) {
        // 在映射表中查找
        auto it = g_comicMappings.find(assetName);
        if (it != g_comicMappings.end()) {
            DWORD attr = GetFileAttributesA(it->second.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                replacementPath = it->second;
                WriteLogf("*** UI BUTTON REPLACEMENT MATCHED! ***");
                WriteLogf("UI Button Asset: %s -> %s", assetName.c_str(), replacementPath.c_str());
                return true;
            } else {
                WriteLogf("UI button replacement file not found: %s", it->second.c_str());
            }
        } else {
            WriteLogf("UI Button detected but no replacement configured: %s", assetName.c_str());
        }
    }
    return false;
}

// 专门处理Tutorial图像的函数  
bool ShouldReplaceTutorial(const std::string& assetName, std::string& replacementPath) {
    // 检查是否是 Tutorial 图像
    if (assetName.find("img_tutorial") != std::string::npos) {
        // 在映射表中查找
        auto it = g_comicMappings.find(assetName);
        if (it != g_comicMappings.end()) {
            DWORD attr = GetFileAttributesA(it->second.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                replacementPath = it->second;
                WriteLogf("*** TUTORIAL REPLACEMENT MATCHED! ***");
                WriteLogf("Tutorial Asset: %s -> %s", assetName.c_str(), replacementPath.c_str());
                return true;
            } else {
                WriteLogf("Tutorial replacement file not found: %s", it->second.c_str());
            }
        } else {
            WriteLogf("Tutorial Asset detected but no replacement configured: %s", assetName.c_str());
        }
    }
    return false;
}

// 优化的资源分析函数 - 只检测配置文件中的资源，避免拖慢游戏
void AnalyzeAllAssets(const std::string& assetName) {
    // 只检测配置文件中已配置的资源，避免性能问题
    auto it = g_comicMappings.find(assetName);
    if (it != g_comicMappings.end()) {
        WriteLogf("*** CONFIGURED ASSET DETECTED! ***");
        WriteLogf("Asset: %s", assetName.c_str());
        
        // 分析资源类型
        if (assetName.find("img_general_comic") != std::string::npos) {
            WriteLogf("Type: COMIC IMAGE (配置中)");
        } else if (assetName.find("img_general_report") != std::string::npos) {
            WriteLogf("Type: GENERAL REPORT (配置中)");
        } else if (assetName.find("img_general") != std::string::npos && 
                   (assetName.find("_btn") != std::string::npos || assetName.find("_button") != std::string::npos)) {
            WriteLogf("Type: UI BUTTON (配置中)");
        } else if (assetName.find("img_tutorial") != std::string::npos) {
            WriteLogf("Type: TUTORIAL IMAGE (配置中)");
        } else {
            WriteLogf("Type: OTHER CONFIGURED RESOURCE");
        }
        
        WriteLogf("Replacement: %s", it->second.c_str());
        WriteLogf("==========================================");
    }
    // 其他资源一律不记录，避免性能问题
}

// 保持向后兼容的函数名
void AnalyzeGeneralReportAsset(const std::string& assetName) {
    // 调用新的全面分析函数
    AnalyzeAllAssets(assetName);
}

// Check if asset should be replaced (original comic function)
bool ShouldReplaceComic(const std::string& assetName, std::string& replacementPath) {
    // 首先尝试精确匹配
    auto exactMatch = g_comicMappings.find(assetName);
    if (exactMatch != g_comicMappings.end()) {
        DWORD attr = GetFileAttributesA(exactMatch->second.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            replacementPath = exactMatch->second;
            WriteLogf("*** EXACT MATCH FOUND! ***");
            WriteLogf("Asset: %s -> File: %s", assetName.c_str(), replacementPath.c_str());
            return true;
        } else {
            WriteLogf("Exact match file not found: %s", exactMatch->second.c_str());
        }
    }
    
    // 如果没有精确匹配，则使用子字符串匹配作为后备
    for (const auto& mapping : g_comicMappings) {
        if (assetName.find(mapping.first) != std::string::npos) {
            DWORD attr = GetFileAttributesA(mapping.second.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                replacementPath = mapping.second;
                WriteLogf("*** SUBSTRING MATCH FOUND! ***");
                WriteLogf("Asset: %s contains %s -> File: %s", assetName.c_str(), mapping.first.c_str(), replacementPath.c_str());
                return true;
            } else {
                WriteLogf("Replacement file not found: %s", mapping.second.c_str());
            }
        }
    }
    return false;
}

// Load image file into memory
bool LoadImageFile(const std::string& filePath, std::vector<unsigned char>& imageData) {
    FILE* file = nullptr;
    if (fopen_s(&file, filePath.c_str(), "rb") != 0 || !file) {
        WriteLogf("Failed to open image file: %s", filePath.c_str());
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (fileSize <= 0) {
        fclose(file);
        WriteLogf("Invalid file size: %ld", fileSize);
        return false;
    }
    
    imageData.resize(fileSize);
    size_t bytesRead = fread(imageData.data(), 1, fileSize, file);
    fclose(file);
    
    if (bytesRead != fileSize) {
        WriteLogf("Failed to read complete file. Expected: %ld, Read: %zu", fileSize, bytesRead);
        return false;
    }
    
    WriteLogf("Successfully loaded image file: %s (%ld bytes)", filePath.c_str(), fileSize);
    return true;
}

// Create Unity Texture2D from image data with actual image loading
void* CreateUnityTexture2D(const std::vector<unsigned char>& imageData, const std::string& fileName) {
    WriteLogf("Creating Unity Texture2D from %s (%d bytes)", fileName.c_str(), (int)imageData.size());
    
    void* domain = il2cpp_domain_get();
    if (!domain) return nullptr;
    
    size_t assemblyCount = 0;
    void** assemblies = (void**)il2cpp_domain_get_assemblies(domain, &assemblyCount);
    if (!assemblies) return nullptr;
    
    void* texture2DClass = nullptr;
    void* imageConversionClass = nullptr;
    
    // Find required classes
    for (size_t i = 0; i < assemblyCount; i++) {
        void* assembly = assemblies[i];
        if (!assembly) continue;
        
        void* image = il2cpp_assembly_get_image(assembly);
        if (!image) continue;
        
        if (!texture2DClass) {
            texture2DClass = il2cpp_class_from_name(image, "UnityEngine", "Texture2D");
        }
        if (!imageConversionClass) {
            imageConversionClass = il2cpp_class_from_name(image, "UnityEngine", "ImageConversion");
        }
        
        if (texture2DClass && imageConversionClass) break;
    }
    
    if (!texture2DClass) {
        WriteLog("Failed to find Texture2D class");
        return nullptr;
    }
    
    WriteLogf("Found Texture2D class at 0x%p", texture2DClass);
    
    // Create a basic Texture2D object and try to initialize it properly
    void* texture2DObject = il2cpp_object_new(texture2DClass);
    if (!texture2DObject) {
        WriteLog("Failed to create Texture2D object");
        return nullptr;
    }
    
    WriteLogf("Created Texture2D object at 0x%p", texture2DObject);
    
    // Try to find and call Texture2D constructor
    void* ctorMethod = il2cpp_class_get_method_from_name(texture2DClass, ".ctor", 2);
    if (ctorMethod && il2cpp_runtime_invoke) {
        WriteLog("Found Texture2D constructor, initializing...");
        try {
            // Initialize with 2x2 size, RGBA32 format (4)
            int width = 2;
            int height = 2;
            void* ctorParams[2] = { &width, &height };
            void* ctorException = nullptr;
            
            il2cpp_runtime_invoke(ctorMethod, texture2DObject, ctorParams, &ctorException);
            if (ctorException) {
                WriteLogf("Constructor failed: 0x%p", ctorException);
            } else {
                WriteLog("Texture2D constructor succeeded");
            }
        }
        catch (...) {
            WriteLog("Exception during constructor call");
        }
    } else {
        WriteLog("Texture2D constructor not found, using uninitialized object");
    }
    
    // Try to use ImageConversion.LoadImage to load the actual image data
    if (imageConversionClass) {
        WriteLogf("Found ImageConversion class at 0x%p", imageConversionClass);
        
        // Find LoadImage method
        void* loadImageMethod = il2cpp_class_get_method_from_name(imageConversionClass, "LoadImage", 2);
        if (loadImageMethod) {
            WriteLogf("Found LoadImage method at 0x%p", loadImageMethod);
            
            // Create byte array from image data
            void* byteClass = nullptr;
            for (size_t i = 0; i < assemblyCount; i++) {
                void* assembly = assemblies[i];
                if (!assembly) continue;
                
                void* image = il2cpp_assembly_get_image(assembly);
                if (!image) continue;
                
                byteClass = il2cpp_class_from_name(image, "System", "Byte");
                if (byteClass) break;
            }
            
            if (byteClass && il2cpp_array_new) {
                WriteLogf("Creating byte array for %d bytes", (int)imageData.size());
                void* byteArray = il2cpp_array_new(byteClass, imageData.size());
                if (byteArray) {
                    WriteLogf("Created byte array at 0x%p", byteArray);
                    
                    // Copy image data to IL2CPP array
                    try {
                        // IL2CPP array structure: [object header][bounds][length][data...]
                        // For single dimension arrays: [klass][monitor][bounds][max_length][data...]
                        unsigned char* arrayData = (unsigned char*)((uintptr_t)byteArray + sizeof(void*) * 4);
                        memcpy(arrayData, imageData.data(), imageData.size());
                        WriteLog("Copied image data to IL2CPP array");
                        
                        // Try different LoadImage method signatures
                        // First try: LoadImage(Texture2D tex, byte[] data)
                        if (il2cpp_runtime_invoke) {
                            void* params[2] = { texture2DObject, byteArray };
                            void* exception = nullptr;
                            
                            WriteLog("Calling ImageConversion.LoadImage(tex, data)...");
                            void* result = il2cpp_runtime_invoke(loadImageMethod, nullptr, params, &exception);
                            
                            if (exception) {
                                WriteLogf("LoadImage(tex, data) failed with exception: 0x%p", exception);
                                
                                // Try alternative method: LoadImage(Texture2D tex, byte[] data, bool markNonReadable)
                                void* loadImageMethod3 = il2cpp_class_get_method_from_name(imageConversionClass, "LoadImage", 3);
                                if (loadImageMethod3) {
                                    WriteLog("Trying LoadImage with 3 parameters...");
                                    bool markNonReadable = false;
                                    void* params3[3] = { texture2DObject, byteArray, &markNonReadable };
                                    exception = nullptr;
                                    
                                    result = il2cpp_runtime_invoke(loadImageMethod3, nullptr, params3, &exception);
                                    if (exception) {
                                        WriteLogf("LoadImage(tex, data, bool) also failed: 0x%p", exception);
                                    } else {
                                        WriteLogf("LoadImage(tex, data, bool) succeeded: 0x%p", result);
                                        WriteLog("*** SUCCESS: Image data loaded into texture! ***");
                                    }
                                }
                            } else {
                                WriteLogf("LoadImage returned: 0x%p", result);
                                WriteLog("*** SUCCESS: Image data loaded into texture! ***");
                            }
                        }
                    }
                    catch (...) {
                        WriteLog("Exception while copying image data");
                    }
                } else {
                    WriteLog("Failed to create byte array");
                }
            } else {
                WriteLog("Failed to find Byte class or il2cpp_array_new");
            }
        } else {
            WriteLog("Failed to find LoadImage method");
        }
    } else {
        WriteLog("ImageConversion class not found - texture will be empty");
    }
    
    WriteLog("*** SUCCESS: Custom texture created! ***");
    return texture2DObject;
}

// Cache for loaded textures with access time tracking
struct TextureCache {
    void* texture;
    DWORD lastAccessTime;
    int useCount;
};
std::map<std::string, TextureCache> g_textureCache;

// Cache cleanup settings
const int MAX_CACHE_SIZE = 50;          // 最大缓存纹理数量
const DWORD CACHE_EXPIRE_TIME = 300000; // 5分钟未使用则过期

// Unity Object.DestroyImmediate function pointer
typedef void (*Unity_DestroyImmediate_t)(void* obj);
Unity_DestroyImmediate_t Unity_DestroyImmediate = nullptr;

// Clean expired textures from cache
void CleanupTextureCache() {
    if (g_textureCache.size() < MAX_CACHE_SIZE) {
        return; // 不需要清理
    }
    
    DWORD currentTime = GetTickCount();
    auto it = g_textureCache.begin();
    int cleanedCount = 0;
    
    while (it != g_textureCache.end()) {
        bool shouldRemove = false;
        
        // 检查是否过期（5分钟未使用）
        if (currentTime - it->second.lastAccessTime > CACHE_EXPIRE_TIME) {
            shouldRemove = true;
            WriteLogf("[ComicReplace-XInput-Optimized] Expiring unused texture: %s (unused for %d ms)", 
                it->first.c_str(), currentTime - it->second.lastAccessTime);
        }
        
        if (shouldRemove) {
            // 尝试销毁Unity对象（如果函数可用）
            if (Unity_DestroyImmediate && it->second.texture) {
                try {
                    Unity_DestroyImmediate(it->second.texture);
                    WriteLogf("[ComicReplace-XInput-Optimized] Destroyed Unity texture object for: %s", it->first.c_str());
                } catch (...) {
                    WriteLogf("[ComicReplace-XInput-Optimized] Failed to destroy Unity texture for: %s", it->first.c_str());
                }
            }
            
            it = g_textureCache.erase(it);
            cleanedCount++;
        } else {
            ++it;
        }
    }
    
    if (cleanedCount > 0) {
        WriteLogf("[ComicReplace-XInput-Optimized] Cleaned %d textures from cache. Cache size: %d", 
            cleanedCount, (int)g_textureCache.size());
    }
}

// Load custom texture with enhanced caching and memory management
void* LoadCustomTexture(const std::string& filePath) {
    // 清理过期缓存
    CleanupTextureCache();
    
    auto it = g_textureCache.find(filePath);
    if (it != g_textureCache.end()) {
        // 更新访问时间和使用计数
        it->second.lastAccessTime = GetTickCount();
        it->second.useCount++;
        WriteLogf("[ComicReplace-XInput-Optimized] Using cached texture for: %s (used %d times)", 
            filePath.c_str(), it->second.useCount);
        return it->second.texture;
    }
    
    std::vector<unsigned char> imageData;
    if (!LoadImageFile(filePath, imageData)) {
        return nullptr;
    }
    
    void* texture = CreateUnityTexture2D(imageData, filePath);
    if (texture) {
        TextureCache cache;
        cache.texture = texture;
        cache.lastAccessTime = GetTickCount();
        cache.useCount = 1;
        
        g_textureCache[filePath] = cache;
        WriteLogf("[ComicReplace-XInput-Optimized] Cached new texture for: %s. Total cache size: %d", 
            filePath.c_str(), (int)g_textureCache.size());
    }
    
    return texture;
}

// Global storage for pending comic replacements with timestamp tracking
struct PendingReplacement {
    void* texture;
    DWORD timestamp;
};
std::map<void*, PendingReplacement> g_pendingComicReplacements;

// Clean expired pending replacements
void CleanupPendingReplacements() {
    DWORD currentTime = GetTickCount();
    const DWORD PENDING_EXPIRE_TIME = 60000; // 1分钟未使用则清理
    
    auto it = g_pendingComicReplacements.begin();
    int cleanedCount = 0;
    
    while (it != g_pendingComicReplacements.end()) {
        if (currentTime - it->second.timestamp > PENDING_EXPIRE_TIME) {
            WriteLogf("[ComicReplace-XInput-Optimized] Cleaning expired pending replacement for request: 0x%p", it->first);
            it = g_pendingComicReplacements.erase(it);
            cleanedCount++;
        } else {
            ++it;
        }
    }
    
    if (cleanedCount > 0) {
        WriteLogf("[ComicReplace-XInput-Optimized] Cleaned %d expired pending replacements. Pending count: %d", 
            cleanedCount, (int)g_pendingComicReplacements.size());
    }
}

// ENHANCED HOOK: AssetBundle.LoadAssetAsync - supports both comic and general_report
void* WINAPI Hooked_AssetBundle_LoadAssetAsync(void* bundle, void* name, void* type) {
    try {
        std::string assetName = Il2CppStringToStdString(name);
        
        // 添加General Report分析
        AnalyzeGeneralReportAsset(assetName);
          // 首先检查General Report替换
        std::string replacementPath;
        if (ShouldReplaceGeneralReport(assetName, replacementPath)) {
            WriteLogf("*** GENERAL_REPORT REPLACEMENT DETECTED! ***");
            WriteLogf("Original: %s", assetName.c_str());
            WriteLogf("Replacement: %s", replacementPath.c_str());
            
            // Call original function first to get the AssetBundleRequest
            void* originalRequest = Original_AssetBundle_LoadAssetAsync(bundle, name, type);
            
            if (originalRequest) {
                // Try to load custom texture
                void* customTexture = LoadCustomTexture(replacementPath);                if (customTexture) {
                    WriteLogf("*** Custom general report texture loaded, storing for request: 0x%p ***", originalRequest);
                    PendingReplacement pending;
                    pending.texture = customTexture;
                    pending.timestamp = GetTickCount();
                    g_pendingComicReplacements[originalRequest] = pending;
                } else {
                    WriteLogf("Failed to load custom general report texture");
                }
            }
            
            return originalRequest;
        }
          // 检查UI按钮替换 (只处理按钮，避免卡死)
        if (ShouldReplaceUI(assetName, replacementPath)) {
            WriteLogf("*** UI BUTTON REPLACEMENT DETECTED! ***");
            WriteLogf("Original: %s", assetName.c_str());
            WriteLogf("Replacement: %s", replacementPath.c_str());
            
            // Call original function first to get the AssetBundleRequest
            void* originalRequest = Original_AssetBundle_LoadAssetAsync(bundle, name, type);
            
            if (originalRequest) {
                // Try to load custom texture
                void* customTexture = LoadCustomTexture(replacementPath);                if (customTexture) {
                    WriteLogf("*** Custom UI button texture loaded, storing for request: 0x%p ***", originalRequest);
                    PendingReplacement pending;
                    pending.texture = customTexture;
                    pending.timestamp = GetTickCount();
                    g_pendingComicReplacements[originalRequest] = pending;
                } else {
                    WriteLogf("Failed to load custom UI button texture");
                }
            }
            
            return originalRequest;
        }
        
        // 检查Tutorial替换  
        if (ShouldReplaceTutorial(assetName, replacementPath)) {
            WriteLogf("*** TUTORIAL REPLACEMENT DETECTED! ***");
            WriteLogf("Original: %s", assetName.c_str());
            WriteLogf("Replacement: %s", replacementPath.c_str());
            
            // Call original function first to get the AssetBundleRequest
            void* originalRequest = Original_AssetBundle_LoadAssetAsync(bundle, name, type);
            
            if (originalRequest) {
                // Try to load custom texture
                void* customTexture = LoadCustomTexture(replacementPath);                if (customTexture) {
                    WriteLogf("*** Custom tutorial texture loaded, storing for request: 0x%p ***", originalRequest);
                    PendingReplacement pending;
                    pending.texture = customTexture;
                    pending.timestamp = GetTickCount();
                    g_pendingComicReplacements[originalRequest] = pending;
                } else {
                    WriteLogf("Failed to load custom tutorial texture");
                }
            }
            
            return originalRequest;
        }
        
        // 然后检查原有的comic替换
        if (ShouldReplaceComic(assetName, replacementPath)) {
            WriteLogf("*** COMIC REPLACEMENT DETECTED! ***");
            WriteLogf("Original: %s", assetName.c_str());
            WriteLogf("Replacement: %s", replacementPath.c_str());
            
            // Call original function first to get the AssetBundleRequest
            void* originalRequest = Original_AssetBundle_LoadAssetAsync(bundle, name, type);
            
            if (originalRequest) {
                // Try to load custom texture
                void* customTexture = LoadCustomTexture(replacementPath);                if (customTexture) {
                    WriteLogf("*** Custom texture loaded, storing for request: 0x%p ***", originalRequest);
                    PendingReplacement pending;
                    pending.texture = customTexture;
                    pending.timestamp = GetTickCount();
                    g_pendingComicReplacements[originalRequest] = pending;
                } else {
                    WriteLogf("Failed to load custom texture");
                }
            }
            
            return originalRequest;
        }
    }
    catch (...) {
        WriteLog("Exception in AssetBundle hook - returning original");
    }
    
    // Call original function for non-comic assets
    return Original_AssetBundle_LoadAssetAsync(bundle, name, type);
}

// Hook AssetBundleRequest.GetResult to replace comic textures
void* WINAPI Hooked_AssetBundleRequest_GetResult(void* request) {
    try {
        // 定期清理过期的pending替换
        static DWORD lastCleanupTime = 0;
        DWORD currentTime = GetTickCount();
        if (currentTime - lastCleanupTime > 30000) { // 每30秒清理一次
            CleanupPendingReplacements();
            lastCleanupTime = currentTime;
        }
        
        // Check if this request has a pending comic replacement
        auto it = g_pendingComicReplacements.find(request);
        if (it != g_pendingComicReplacements.end()) {
            WriteLogf("*** REPLACEMENT: Returning custom texture for request: 0x%p ***", request);
            void* customTexture = it->second.texture;
            
            // Remove from pending list
            g_pendingComicReplacements.erase(it);
            
            WriteLogf("*** SUCCESS: Returning custom texture: 0x%p ***", customTexture);
            return customTexture;
        }
        
        // For non-comic requests, return original result
        void* originalResult = Original_AssetBundleRequest_GetResult(request);
        return originalResult;
    }
    catch (...) {
        WriteLog("Exception in AssetBundleRequest.GetResult hook");
        return Original_AssetBundleRequest_GetResult(request);
    }
}

// Hook Resources.Load
void* WINAPI Hooked_Resources_Load(void* path, void* type) {
    try {
        std::string assetPath = Il2CppStringToStdString(path);
        
        std::string replacementPath;
        if (ShouldReplaceComic(assetPath, replacementPath) || 
            ShouldReplaceGeneralReport(assetPath, replacementPath) ||
            ShouldReplaceUI(assetPath, replacementPath) ||
            ShouldReplaceTutorial(assetPath, replacementPath)) {
            WriteLogf("*** RESOURCE REPLACEMENT! ***");
            WriteLogf("Original: %s", assetPath.c_str());
            WriteLogf("Replacement: %s", replacementPath.c_str());
        }
    }
    catch (...) {
        WriteLog("Exception in Resources hook");
    }
    
    return Original_Resources_Load(path, type);
}

// Install hooks
bool InstallComicHooks() {
    WriteLog("Installing XINPUT comic + general_report + UI_buttons + tutorial replacement hooks (Optimized)...");
    
    if (!InitializeIL2CPPApi()) {
        WriteLog("Failed to initialize IL2CPP API");
        return false;
    }
    
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        WriteLogf("MH_Initialize failed: %d", status);
        return false;
    }
    
    int hookCount = 0;
    
    // Hook AssetBundle.LoadAssetAsync
    void* assetBundleMethod = FindIL2CPPMethod("UnityEngine.AssetBundleModule.dll", "UnityEngine", "AssetBundle", "LoadAssetAsync", 2);
    if (assetBundleMethod) {
        if (MH_CreateHook(assetBundleMethod, &Hooked_AssetBundle_LoadAssetAsync, (LPVOID*)&Original_AssetBundle_LoadAssetAsync) == MH_OK) {
            if (MH_EnableHook(assetBundleMethod) == MH_OK) {
                WriteLogf("XINPUT Hooked AssetBundle.LoadAssetAsync at 0x%p", assetBundleMethod);
                hookCount++;
            }
        }
    }
    
    // Hook AssetBundleRequest.GetResult
    void* assetBundleRequestMethod = FindIL2CPPMethod("UnityEngine.AssetBundleModule.dll", "UnityEngine", "AssetBundleRequest", "GetResult", 0);
    if (assetBundleRequestMethod) {
        if (MH_CreateHook(assetBundleRequestMethod, &Hooked_AssetBundleRequest_GetResult, (LPVOID*)&Original_AssetBundleRequest_GetResult) == MH_OK) {
            if (MH_EnableHook(assetBundleRequestMethod) == MH_OK) {
                WriteLogf("XINPUT Hooked AssetBundleRequest.GetResult at 0x%p", assetBundleRequestMethod);
                hookCount++;
            }
        }
    }
    
    // Hook Resources.Load
    void* resourcesMethod = FindIL2CPPMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Resources", "Load", 2);
    if (resourcesMethod) {
        if (MH_CreateHook(resourcesMethod, &Hooked_Resources_Load, (LPVOID*)&Original_Resources_Load) == MH_OK) {
            if (MH_EnableHook(resourcesMethod) == MH_OK) {
                WriteLogf("XINPUT Hooked Resources.Load at 0x%p", resourcesMethod);
                hookCount++;
            }
        }
    }    // 尝试获取Unity Object.DestroyImmediate函数
    Unity_DestroyImmediate = (Unity_DestroyImmediate_t)FindIL2CPPMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Object", "DestroyImmediate", 1);
    if (Unity_DestroyImmediate) {
        WriteLogf("Found Unity Object.DestroyImmediate at 0x%p", Unity_DestroyImmediate);
    } else {
        WriteLog("Unity Object.DestroyImmediate not found - texture cleanup will be limited");
    }
    
    WriteLogf("XINPUT Comic + General Report + UI_Buttons + Tutorial hooks installed (Optimized): %d/3", hookCount);
    WriteLog("*** PERFORMANCE: Only configured assets will be monitored ***");
    return hookCount > 0;
}

// Global cleanup function
void CleanupSystem() {
    WriteLog("=== XINPUT Comic Replacement System Cleanup ===");
    
    // 清理 MinHook
    if (g_hooksInstalled) {
        WriteLog("Cleaning up hooks...");
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
    
    // 强制清理所有缓存的纹理
    if (Unity_DestroyImmediate) {
        WriteLogf("Cleaning up %d cached textures...", (int)g_textureCache.size());
        for (auto& cache : g_textureCache) {
            try {
                if (cache.second.texture) {
                    Unity_DestroyImmediate(cache.second.texture);
                }
            } catch (...) {
                WriteLogf("Failed to destroy texture: %s", cache.first.c_str());
            }
        }
    }
    
    g_textureCache.clear();
    g_pendingComicReplacements.clear();
    WriteLogf("Cleanup complete. Cleared %d texture cache entries and %d pending replacements", 
        (int)g_textureCache.size(), (int)g_pendingComicReplacements.size());
    
    // 清理日志文件
    if (g_logFile) {
        WriteLog("=== XINPUT System Shutting Down ===");
        fclose(g_logFile);
        g_logFile = nullptr;
    }
    
    // 释放原始XInput库
    if (g_hOriginalXInput) {
        FreeLibrary(g_hOriginalXInput);
        g_hOriginalXInput = NULL;
    }
}

// Background thread for hook installation
DWORD WINAPI ComicHookInstallThread(LPVOID lpParam) {
    if (!IsTargetGameProcess()) {
        WriteLog("ABORT: Hook thread started in wrong process");
        return 0;
    }    WriteLog("=== XINPUT Comic + General Report + UI_Buttons + Tutorial Replacement System Starting (Optimized) ===");
    
    // Load configuration
    LoadComicMappings();
    
    WriteLog("Waiting for optimal hook timing...");
    Sleep(25000); // Wait 25 seconds for full game initialization
    
    WriteLog("=== Installing XINPUT Comic + General Report + UI_Buttons + Tutorial Hooks (Optimized) ===");
    
    if (InstallComicHooks()) {
        WriteLog("XINPUT Comic + General Report + UI_Buttons + Tutorial hooks installed successfully! (Optimized)");
        WriteLog("Comic, General Report, UI Buttons and Tutorial replacement is now ACTIVE! (Only monitors configured assets)");
        g_hooksInstalled = true;
    } else {
        WriteLog("Failed to install XINPUT comic + general report + UI buttons + tutorial hooks");
    }
      WriteLog("=== XINPUT Hook Installation Thread Completed ===");
    return 0;
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        
        // Load original xinput1_3.dll
        wchar_t systemPath[MAX_PATH];
        GetSystemDirectoryW(systemPath, MAX_PATH);
        wcscat_s(systemPath, L"\\xinput1_3.dll");
        g_hOriginalXInput = LoadLibraryW(systemPath);
        
        if (g_hOriginalXInput) {
            // Get original function addresses
            Original_XInputGetState = (XInputGetState_t)GetProcAddress(g_hOriginalXInput, "XInputGetState");
            Original_XInputSetState = (XInputSetState_t)GetProcAddress(g_hOriginalXInput, "XInputSetState");
            Original_XInputGetCapabilities = (XInputGetCapabilities_t)GetProcAddress(g_hOriginalXInput, "XInputGetCapabilities");
            Original_XInputEnable = (XInputEnable_t)GetProcAddress(g_hOriginalXInput, "XInputEnable");
        }
          // Initialize after process check
        if (IsTargetGameProcess()) {
            // 初始化控制台
            AllocConsole();
            FILE* pCout;
            freopen_s(&pCout, "CONOUT$", "w", stdout);
            SetConsoleTitleA("Gakumas - Comic + General Report + UI + Tutorial Replacement System (XInput Optimized)");
            
            // 创建基础目录结构
            CreateDirectory(g_baseDir.c_str());
            CreateDirectory((g_baseDir + "\\logs").c_str());
            CreateDirectory((g_baseDir + "\\comic").c_str());
            CreateDirectory((g_baseDir + "\\general_report").c_str());
            CreateDirectory((g_baseDir + "\\ui").c_str());
            CreateDirectory((g_baseDir + "\\tutorial").c_str());
            
            // 初始化日志系统（将自动创建带时间戳的日志文件）
            InitializeLogFile();
            
            WriteLog("=================================");
            WriteLog("Gakumas Comic + General Report + UI + Tutorial Replacement - XInput Optimized Version");
            WriteLog("Target process: gakumas.exe");
            WriteLog("Only monitors configured assets for better performance");
            WriteLogf("Base directory: %s", g_baseDir.c_str());
            WriteLog("=================================");
            
            if (g_hOriginalXInput) {
                WriteLog("Original xinput1_3.dll loaded successfully");
            } else {
                WriteLog("Warning: Failed to load original xinput1_3.dll");
            }
            
            // Initialize comic replacement system in background thread
            CreateThread(nullptr, 0, ComicHookInstallThread, nullptr, 0, nullptr);
        }
        
        break;
        
    case DLL_PROCESS_DETACH:
        CleanupSystem();
        break;
    }
    return TRUE;
}

// XInput API proxy functions - safe implementations
extern "C" {
    DWORD __stdcall XInputGetState(DWORD dwUserIndex, PXINPUT_STATE pState) {
        if (Original_XInputGetState) {
            return Original_XInputGetState(dwUserIndex, pState);
        }
        return ERROR_DEVICE_NOT_CONNECTED; // Safe default for controller not connected
    }

    DWORD __stdcall XInputSetState(DWORD dwUserIndex, PXINPUT_VIBRATION pVibration) {
        if (Original_XInputSetState) {
            return Original_XInputSetState(dwUserIndex, pVibration);
        }
        return ERROR_DEVICE_NOT_CONNECTED;
    }

    DWORD __stdcall XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, PXINPUT_CAPABILITIES pCapabilities) {
        if (Original_XInputGetCapabilities) {
            return Original_XInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);
        }
        return ERROR_DEVICE_NOT_CONNECTED;
    }

    void __stdcall XInputEnable(BOOL enable) {
        if (Original_XInputEnable) {
            Original_XInputEnable(enable);
        }
    }

    // Stub implementations for less critical functions
    DWORD __stdcall XInputGetDSoundAudioDeviceGuids(DWORD a, LPVOID b, LPVOID c) { return ERROR_DEVICE_NOT_CONNECTED; }
    DWORD __stdcall XInputGetBatteryInformation(DWORD a, BYTE b, LPVOID c) { return ERROR_DEVICE_NOT_CONNECTED; }
    DWORD __stdcall XInputGetKeystroke(DWORD a, DWORD b, LPVOID c) { return ERROR_EMPTY; }
}