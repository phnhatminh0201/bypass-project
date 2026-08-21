// MADE BY FINEX BOYZZ.
// DC LINK: https://discord.gg/AHwg2YA6sE 

#include "BypassManager.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <thread>
#include <tlhelp32.h>
#include <UrlMon.h> 
#include <winver.h>
#include <cctype>
#include <psapi.h>
#include <nlohmann/json.hpp>
#include <imgui.h>
extern void AddNotification(const std::string& message, const std::string& type = "error", float displayTime = 3.0f);
struct NotificationSystemCompat {
    void AddNotification(const std::string& title, const std::string& message, ImColor color) {
        std::string type = "error";
        if (title == "Success" || title == "Logs" || title == "ADB" || title == "Info") {
            type = "success";
        }
        ::AddNotification(title + ": " + message, type, 3.0f);
    }
};
static NotificationSystemCompat notificationSystem;
#define white ImColor(255, 255, 255, 255)
#pragma comment(lib, "version.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Ws2_32.lib")

//constexpr const char* PROXY_ADDRESS = "144.172.116.157:65535";
constexpr const char* PROXY_ADDRESS = "217.216.35.82:54232";
constexpr const char* NO_PROXY = ":0";
constexpr const char* DEFAULT_PROXY_PORT = "54233";

using nlohmann::json;
namespace fs = std::filesystem;


typedef NTSTATUS(NTAPI* _NtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
    );
typedef NTSTATUS(NTAPI* _NtDuplicateObject)(
    HANDLE SourceProcessHandle,
    HANDLE SourceHandle,
    HANDLE TargetProcessHandle,
    PHANDLE TargetHandle,
    ACCESS_MASK DesiredAccess,
    ULONG HandleAttributes,
    ULONG Options
    );
typedef NTSTATUS(NTAPI* _NtQueryObject)(
    HANDLE Handle,
    ULONG ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength
    );
typedef struct _SYSTEM_HANDLE_INFORMATION {
    ULONG NumberOfHandles;
    struct {
        ULONG ProcessId;
        UCHAR ObjectType;
        UCHAR HandleFlags;
        USHORT HandleValue;
        PVOID Object;
        ULONG GrantedAccess;
    } Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;
typedef struct _OBJECT_NAME_INFORMATION {
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, * POBJECT_NAME_INFORMATION;
#define SystemHandleInformation 16
#define ObjectNameInformation 1
#define STATUS_SUCCESS 0x00000000
#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004


struct CertManager::Impl 
{
    GameType selectedGame = GameType::None;

    std::string adbPort;
    std::string proxyPort = DEFAULT_PROXY_PORT;  // local proxy port (54233)
    EmulatorType selectedEmulator = EmulatorType::None;
    LogCallback logCallback;
    std::string lastError;
    bool adbConnected = false;

    std::string emulatorVersion;

    std::string GetADBPath() const;
    std::string GetEngineRootPath() const;
    std::string GetMemuAdbPath() const;


    DWORD FindProcessId(const std::wstring& processName);
    std::string ConvertNtPathToDosPath(const std::string& ntPath);
    std::string FindBstkCoreLogPath(DWORD targetPid);
    std::string GetCurrentEngineAdbPort();

    bool KillProcessByName(const std::string& processName);
    std::vector<DWORD> FindProcessesByName(const std::string& processName);

    bool ExecuteADB(const std::string& command, std::string& output);
    bool ExecuteADBNoDevice(const std::string& command, std::string& output); // for root/remount
    bool PushFile(const std::string& localPath, const std::string& remotePath);
    bool FileExistsOnDevice(const std::string& path);

    bool PatchVHDFile(const std::string& filePath);
    void CleanLogs(const std::string& engineRoot);

    std::string ExtractCertificateToTemp();

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    bool DownloadJSON(const std::string& url, std::string& output);

    void Log(const std::string& message, LogLevel level);

    bool GetGlobalProxy(std::string& proxyValue);
};

CertManager::CertManager() : pImpl(std::make_unique<Impl>()) {}
CertManager::~CertManager() = default;

bool CertManager::Initialize(const std::string& adbPort) {
    pImpl->adbPort = adbPort;
    pImpl->Log("Certificate Manager initialized with ADB port: " + adbPort, LogLevel::Info);
    return true;
}

bool CertManager::InstallCertificate() {
    if (pImpl->selectedEmulator == EmulatorType::None) {
        pImpl->lastError = "No emulator selected";
        return false;
    }

    if (!ConnectADB()) {
        return false;
    }


    std::string tempCertPath = pImpl->ExtractCertificateToTemp();
    if (tempCertPath.empty()) {
        pImpl->lastError = "Failed to extract certificate";
        notificationSystem.AddNotification("Error", "Failed to extract certificate", ImColor(1.f, 0.f, 0.f, 1.f));
        return false;
    }


    std::string remotePath = std::string("/sdcard/") + CertData::CERT_HASH + ".0";
    if (!pImpl->PushFile(tempCertPath, remotePath)) {
        return false;
    }

    std::string output;

    if (pImpl->selectedEmulator == EmulatorType::Memu) {
        // MEmu uses adb root + remount instead of su
        pImpl->ExecuteADBNoDevice("root", output);
        pImpl->ExecuteADBNoDevice("remount", output);

        std::string cmd =
            "mount -o rw,remount /system && "
            "mount -o rw,remount / && "
            "mkdir -p /system/etc/security/cacerts/ && "
            "cp " + remotePath + " /system/etc/security/cacerts/" + CertData::CERT_HASH + ".0 && "
            "chmod 644 /system/etc/security/cacerts/" + CertData::CERT_HASH + ".0 && "
            "chown root:root /system/etc/security/cacerts/" + CertData::CERT_HASH + ".0 && "
            "mount -o ro,remount /system && "
            "rm " + remotePath;

        pImpl->ExecuteADB("shell \"" + cmd + "\"", output);
    }
    else {
        // BlueStacks / MSI -- use su path
        std::string suPath = "/boot/android/android/system/xbin/bstk/su";
        std::string command = suPath + " -c 'mount -o rw,remount /dev/sda1 /system && "
            "cp " + remotePath + " /system/etc/security/cacerts/" + CertData::CERT_HASH + ".0 && "
            "chmod 644 /system/etc/security/cacerts/" + CertData::CERT_HASH + ".0 && "
            "chcon u:object_r:system_file:s0 /system/etc/security/cacerts/" + CertData::CERT_HASH + ".0 && "
            "mount -o ro,remount /dev/sda1 /system && "
            "rm " + remotePath + " && "
            "setprop ctl.restart zygote'";

        pImpl->ExecuteADB("shell \"" + command + "\"", output);
    }

    fs::remove(tempCertPath);

    if (IsCertificateInstalled()) {
        pImpl->Log("Certificate installed successfully", LogLevel::Success);
        return true;
    }

    pImpl->lastError = "Certificate installation failed";
    return false;
}

bool CertManager::UninstallCertificate() {
    if (pImpl->selectedEmulator == EmulatorType::None) {
        pImpl->lastError = "No emulator selected";
        return false;
    }

    if (!ConnectADB()) {
        return false;
    }

    std::string output;

    if (pImpl->selectedEmulator == EmulatorType::Memu) {
        pImpl->ExecuteADBNoDevice("root", output);
        pImpl->ExecuteADBNoDevice("remount", output);

        std::string cmd =
            "mount -o rw,remount /system && "
            "rm /system/etc/security/cacerts/" + std::string(CertData::CERT_HASH) + ".0 && "
            "mount -o ro,remount /system";
        pImpl->ExecuteADB("shell \"" + cmd + "\"", output);
    }
    else {
        std::string suPath = "/boot/android/android/system/xbin/bstk/su";
        std::string command = suPath + " -c 'mount -o rw,remount /dev/sda1 /system && "
            "rm /system/etc/security/cacerts/" + std::string(CertData::CERT_HASH) + ".0 && "
            "mount -o ro,remount /dev/sda1 /system &'";

        pImpl->ExecuteADB("shell \"" + command + "\"", output);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!IsCertificateInstalled()) {
        pImpl->Log("Certificate removed successfully", LogLevel::Success);
        return true;
    }

    pImpl->lastError = "Certificate removal failed";
    return false;
}

bool CertManager::IsCertificateInstalled() {
    if (!pImpl->adbConnected && !ConnectADB()) {
        return false;
    }

    std::string certPath = std::string("/system/etc/security/cacerts/") + CertData::CERT_HASH + ".0";
    return pImpl->FileExistsOnDevice(certPath);
}

bool CertManager::PatchVHDConfigs() {
    std::string engineRoot = pImpl->GetEngineRootPath();
    if (engineRoot.empty()) {
        pImpl->lastError = "Engine root path not found";
        notificationSystem.AddNotification("Error", "Engine root path not found", ImColor(1.f, 0.f, 0.f, 1.f));
        return false;
    }

    for (const auto& entry : fs::recursive_directory_iterator(engineRoot)) {
        if (!entry.is_regular_file()) continue;

        std::string filePath = entry.path().string();
        std::string filename = entry.path().filename().string();


        auto ends_with_fn = [](const std::string& str, const std::string& suffix) {
            return str.size() >= suffix.size() && 
                   str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
        };

        if (filename == "Android.bstk.in" ||
            ends_with_fn(filename, ".bstk") ||
            ends_with_fn(filename, ".bstk-prev"))
        {
            pImpl->PatchVHDFile(filePath);
        }
    }

    pImpl->Log("VHD configurations patched", LogLevel::Success);
    notificationSystem.AddNotification("Logs", "Emulator Patched", ImColor(0.f, 1.f, 0.f, 1.f));
    return true;
}

bool CertManager::RequestAccess() {

    if (!KillEmulatorProcesses()) {
        pImpl->Log("Warning: Some processes may not have been killed", LogLevel::Warning);
    }


    if (!PatchVHDConfigs()) {
        return false;
    }


    std::string engineRoot = pImpl->GetEngineRootPath();
    if (!engineRoot.empty()) {
        pImpl->CleanLogs(engineRoot);
    }

    pImpl->Log("Access granted", LogLevel::Success);
    notificationSystem.AddNotification("Success", "Access granted", ImColor(0.f, 1.f, 0.f, 1.f));
    return true;
}

std::vector<EmulatorType> CertManager::DetectAvailableEmulators() {
    std::vector<EmulatorType> available;

    if (fs::exists("C:\\Program Files\\Bluestacks_msi5\\HD-Adb.exe")) {
        available.push_back(EmulatorType::MSI5);
    }

    if (fs::exists("C:\\Program Files\\BlueStacks_nxt\\HD-Adb.exe")) {
        available.push_back(EmulatorType::BlueStacks5);
    }

    if (!pImpl->GetMemuAdbPath().empty()) {
        available.push_back(EmulatorType::Memu);
    }

    return available;
}

bool CertManager::SelectEmulator(EmulatorType type) {
    pImpl->selectedEmulator = type;
    pImpl->Log("Selected emulator: " + std::to_string(static_cast<int>(type)), LogLevel::Info);
    return true;
}

EmulatorType CertManager::GetSelectedEmulator() const {
    return pImpl->selectedEmulator;
}

bool CertManager::KillEmulatorProcesses() {
    std::vector<std::string> processes;

    if (pImpl->selectedEmulator == EmulatorType::Memu) {
        processes = { "MEmu", "MEmuHeadless", "MEmuSVC", "MEmuManage" };
    }
    else {
        processes = { "HD-Player", "HD-Adb", "HD-MultiInstanceManager", "BstkSVC" };
    }

    bool allKilled = true;
    for (const auto& procName : processes) {
        if (!pImpl->KillProcessByName(procName)) {
            allKilled = false;
        }
    }

    if (allKilled) {
        pImpl->Log("All emulator processes terminated", LogLevel::Success);
        notificationSystem.AddNotification("Logs", "Emulator Closed", white);
    }
    return allKilled;
}

bool CertManager::ConnectADB() {
    std::string adbPath = pImpl->GetADBPath();
    if (adbPath.empty()) {
        pImpl->lastError = "ADB executable not found";
        notificationSystem.AddNotification("Error", "ADB Executable Not found", ImColor(1.f,0.f,0.f,1.f));
        return false;
    }

    std::string output;
    std::string command = "connect 127.0.0.1:" + pImpl->adbPort;

    if (pImpl->ExecuteADB(command, output)) {
        if (output.find("connected") != std::string::npos) {
            pImpl->adbConnected = true;
            pImpl->Log("ADB connected", LogLevel::Success);

            return true;
        }
    }

    pImpl->lastError = "ADB connection failed";
    notificationSystem.AddNotification("Error", "ADB connection failed", ImColor(1.f, 0.f, 0.f, 1.f));
    pImpl->adbConnected = false;
    return false;
}

bool CertManager::DisconnectADB() {
    std::string output;
    std::string command = "disconnect 127.0.0.1:" + pImpl->adbPort;
    pImpl->ExecuteADB(command, output);
    pImpl->adbConnected = false;
    pImpl->Log("ADB disconnected", LogLevel::Info);

    return true;
}

bool CertManager::IsADBConnected() const {
    return pImpl->adbConnected;
}

void CertManager::SetLogCallback(LogCallback callback) {
    pImpl->logCallback = callback;
}

std::string CertManager::GetLastError() const {
    return pImpl->lastError;
}

// ---------------------------------------------------------------------------
// Proxy port configuration
// ---------------------------------------------------------------------------
void CertManager::SetProxyPort(const std::string& port) {
    if (port.empty()) {
        pImpl->proxyPort = DEFAULT_PROXY_PORT; // Fallback to hardcoded port (54233)
    } else {
        pImpl->proxyPort = port;
    }
}

std::string CertManager::GetProxyPort() const {
    return pImpl->proxyPort;
}

// ---------------------------------------------------------------------------
// Get local machine IPv4 (e.g. 192.168.x.x)
// ---------------------------------------------------------------------------
std::string CertManager::GetLocalIPv4() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return "127.0.0.1";

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return "127.0.0.1";
    }

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    inet_pton(AF_INET, "8.8.8.8", &serv.sin_addr);
    serv.sin_port = htons(53);

    if (connect(sock, (const struct sockaddr*)&serv, sizeof(serv)) != 0) {
        closesocket(sock);
        WSACleanup();
        return "127.0.0.1";
    }

    struct sockaddr_in name;
    int namelen = sizeof(name);
    if (getsockname(sock, (struct sockaddr*)&name, &namelen) != 0) {
        closesocket(sock);
        WSACleanup();
        return "127.0.0.1";
    }

    closesocket(sock);
    WSACleanup();

    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &name.sin_addr, ipStr, sizeof(ipStr));
    return std::string(ipStr);
}

// ---------------------------------------------------------------------------
// Launch emulator executable
// ---------------------------------------------------------------------------
bool CertManager::StartEmulator(EmulatorType type) {
    std::string exePath;
    EmulatorType targetType = (type != EmulatorType::None) ? type : pImpl->selectedEmulator;
    
    switch (targetType) {
    case EmulatorType::BlueStacks5:
        exePath = "C:\\Program Files\\BlueStacks_nxt\\HD-Player.exe";
        break;
    case EmulatorType::MSI5:
        exePath = "C:\\Program Files\\Bluestacks_msi5\\HD-Player.exe";
        break;
    case EmulatorType::Memu: {
        std::string memuAdb = pImpl->GetMemuAdbPath();
        if (memuAdb.empty()) return false;
        std::string memuDir = fs::path(memuAdb).parent_path().string();
        exePath = memuDir + "\\MEmu.exe";
        break;
    }
    default:
        return false;
    }

    if (!fs::exists(exePath)) return false;

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(exePath.c_str(), nullptr, nullptr, nullptr,
                             FALSE, 0, nullptr, nullptr, &si, &pi);
    if (ok) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        notificationSystem.AddNotification("Logs", "Emulator Launched", white);
    }
    return ok != FALSE;
}

std::string CertManager::Impl::GetADBPath() const {
    switch (selectedEmulator) {
    case EmulatorType::MSI5:
        return "C:\\Program Files\\Bluestacks_msi5\\HD-Adb.exe";
    case EmulatorType::BlueStacks5:
        return "C:\\Program Files\\BlueStacks_nxt\\HD-Adb.exe";
    case EmulatorType::Memu:
        return GetMemuAdbPath();
    default:
        return "";
    }
}

std::string CertManager::Impl::GetMemuAdbPath() const {
    const char* paths[] = {
        "C:\\Program Files\\Microvirt\\MEmu\\adb.exe",
        "C:\\Program Files (x86)\\Microvirt\\MEmu\\adb.exe",
        "D:\\Program Files\\Microvirt\\MEmu\\adb.exe",
        "D:\\Program Files (x86)\\Microvirt\\MEmu\\adb.exe"
    };
    for (const char* p : paths) {
        if (fs::exists(p)) return p;
    }
    return "";
}

std::string CertManager::Impl::GetEngineRootPath() const {
    switch (selectedEmulator) {
    case EmulatorType::MSI5:
        return "C:\\ProgramData\\Bluestacks_msi5\\Engine";
    case EmulatorType::BlueStacks5:
        return "C:\\ProgramData\\BlueStacks_nxt\\Engine";
    case EmulatorType::Memu:
        return ""; // MEmu does not use .bstk VHD configs
    default:
        return "";
    }
}

bool CertManager::Impl::KillProcessByName(const std::string& processName) {

    auto pids = FindProcessesByName(processName);
    bool allKilled = true;

    for (DWORD pid : pids) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess != NULL) {
            if (!TerminateProcess(hProcess, 0)) {
                allKilled = false;
            }
            CloseHandle(hProcess);
        }
        else {
            allKilled = false;
        }
    }

    return allKilled;
}

std::vector<DWORD> CertManager::Impl::FindProcessesByName(const std::string& processName) {
    std::vector<DWORD> pids;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return pids;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
#ifdef UNICODE
            std::wstring wExeName = pe32.szExeFile;
            std::string exeName(wExeName.begin(), wExeName.end());
#else
            std::string exeName = pe32.szExeFile;
#endif
            if (exeName.find(processName) != std::string::npos) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return pids;
}

bool CertManager::Impl::ExecuteADB(const std::string& command, std::string& output) {
    std::string adbPath = GetADBPath();
    if (adbPath.empty()) return false;

    std::string fullCommand = "\"" + adbPath + "\" -s 127.0.0.1:" + adbPort + " " + command;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return false;
    }

    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;

    if (!CreateProcessA(NULL, const_cast<char*>(fullCommand.c_str()), NULL, NULL,
        TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hWritePipe);

    char buffer[4096];
    DWORD bytesRead;
    output.clear();

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    CloseHandle(hReadPipe);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

// Run ADB command WITHOUT -s device flag (for 'root', 'remount', 'connect')
bool CertManager::Impl::ExecuteADBNoDevice(const std::string& command, std::string& output) {
    std::string adbPath = GetADBPath();
    if (adbPath.empty()) return false;

    std::string fullCommand = "\"" + adbPath + "\" " + command;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return false;

    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError  = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    if (!CreateProcessA(NULL, const_cast<char*>(fullCommand.c_str()), NULL, NULL,
        TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hWritePipe);

    char buf[4096]; DWORD read;
    output.clear();
    while (ReadFile(hReadPipe, buf, sizeof(buf)-1, &read, NULL) && read > 0) {
        buf[read] = '\0'; output += buf;
    }
    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool CertManager::Impl::PushFile(const std::string& localPath, const std::string& remotePath) {
    std::string output;
    return ExecuteADB("push \"" + localPath + "\" " + remotePath, output);
}

bool CertManager::Impl::FileExistsOnDevice(const std::string& path) {
    std::string output;
    std::string command = "shell \"[ -f " + path + " ] && echo yes || echo no\"";

    if (ExecuteADB(command, output)) {
        return output.find("yes") != std::string::npos;
    }
    return false;
}

bool CertManager::IsEmulatorPatched() {
    std::string engineRoot = pImpl->GetEngineRootPath();
    if (engineRoot.empty()) return false;

    for (const auto& entry : fs::recursive_directory_iterator(engineRoot)) {
        if (!entry.is_regular_file()) continue;

        std::string filePath = entry.path().string();
        std::ifstream file(filePath);
        if (!file.is_open()) continue;

        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        file.close();

        if (content.find("type=\"Readonly\"") != std::string::npos) {
            return false;
        }
    }
    return true;
}

bool CertManager::Impl::PatchVHDFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    std::regex rootVhdRegex(
        R"((<HardDisk\b[^>]*location\s*=\s*"Root\.vhd"[^>]*type\s*=\s*")Readonly("\s*/?>))",
        std::regex::icase
    );
    content = std::regex_replace(content, rootVhdRegex, "$1Normal$2");

    std::regex dataVhdxRegex(
        R"((<HardDisk\b[^>]*location\s*=\s*"Data\.vhdx"[^>]*type\s*=\s*")Readonly("\s*/?>))",
        std::regex::icase
    );
    content = std::regex_replace(content, dataVhdxRegex, "$1Normal$2");

    std::ofstream outFile(filePath);
    if (!outFile.is_open()) return false;

    outFile << content;
    outFile.close();

    return true;
}

void CertManager::Impl::CleanLogs(const std::string& engineRoot) {

    std::string managerDir = engineRoot + "\\Manager";
    if (fs::exists(managerDir)) {
        for (const auto& entry : fs::directory_iterator(managerDir)) {
            std::string filename = entry.path().filename().string();
            if (filename.find("BstkServer.log") == 0) {
                try {
                    fs::remove(entry.path());
                }
                catch (...) {

                    std::ofstream file(entry.path().string(), std::ios::trunc);
                    file.close();
                }
            }
        }
    }


    for (const auto& instanceDir : fs::directory_iterator(engineRoot)) {
        if (instanceDir.is_directory()) {
            std::string logsDir = instanceDir.path().string() + "\\Logs";
            if (fs::exists(logsDir)) {
                for (const auto& logFile : fs::directory_iterator(logsDir)) {
                    std::string filename = logFile.path().filename().string();
                    if (filename.find("BstkCore.log") == 0) {
                        try {
                            fs::remove(logFile.path());
                        }
                        catch (...) {}
                    }
                }
            }
        }
    }
}

std::string CertManager::Impl::ExtractCertificateToTemp() {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);

    std::string certFile = std::string(tempPath) + CertData::CERT_HASH + ".0";

    std::ofstream file(certFile, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    file.write(reinterpret_cast<const char*>(CertData::CERTIFICATE_BYTES),
        CertData::CERTIFICATE_SIZE);
    file.close();

    return certFile;
}

size_t CertManager::Impl::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool CertManager::Impl::DownloadJSON(const std::string& url, std::string& output)
{
    char tmpPath[MAX_PATH]{};
    char tmpFile[MAX_PATH]{};

    if (!GetTempPathA(MAX_PATH, tmpPath)) return false;
    if (!GetTempFileNameA(tmpPath, "cmj", 0, tmpFile)) return false;

    HRESULT hr = URLDownloadToFileA(nullptr, url.c_str(), tmpFile, 0, nullptr);
    if (FAILED(hr))
    {
        DeleteFileA(tmpFile);
        return false;
    }

    std::ifstream ifs(tmpFile, std::ios::binary);
    if (!ifs.is_open())
    {
        DeleteFileA(tmpFile);
        return false;
    }

    output.assign(std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>());
    ifs.close();

    DeleteFileA(tmpFile);
    return true;
}

void CertManager::Impl::Log(const std::string& message, LogLevel level) {
    if (logCallback) {
        logCallback(message, level);
    }
}

bool CertManager::CheckADBConnection() {
    std::string output;
    if (!pImpl->ExecuteADB("devices", output)) {
        pImpl->lastError = "Failed to execute ADB devices command";
        pImpl->Log(pImpl->lastError, LogLevel::Error);
        pImpl->adbConnected = false;
        return false;
    }
    std::string deviceStr = "127.0.0.1:" + pImpl->adbPort + "\tdevice";
    if (output.find(deviceStr) != std::string::npos) {
        pImpl->adbConnected = true;
        pImpl->Log("ADB is connected to port " + pImpl->adbPort, LogLevel::Success);
        return true;
    }
    pImpl->adbConnected = false;
    pImpl->lastError = "ADB is not connected to port " + pImpl->adbPort;
    pImpl->Log(pImpl->lastError, LogLevel::Warning);
    return false;
}


// Game selection methods
void CertManager::SetGame(GameType game) {
    pImpl->selectedGame = game;
    std::string gameName = (game == GameType::FreeFire) ? "Free Fire" :
        (game == GameType::FreeFireMAX) ? "Free Fire MAX" : "None";
    pImpl->Log("Game selected: " + gameName, LogLevel::Info);
}

GameType CertManager::GetGame() const {
    return pImpl->selectedGame;
}

std::string CertManager::GetGamePackageName() const {
    switch (pImpl->selectedGame) {
    case GameType::FreeFire:
        return "com.dts.freefireth";
    case GameType::FreeFireMAX:
        return "com.dts.freefiremax";
    case GameType::None:
    default:
        return "";
    }
}

GameType CertManager::DetectInstalledGame() {
    std::string output;

    // Check for Free Fire
    if (pImpl->ExecuteADB("shell pm list packages com.dts.freefireth", output)) {
        if (output.find("com.dts.freefireth") != std::string::npos) {
            pImpl->Log("Detected Free Fire installed", LogLevel::Success);
            pImpl->selectedGame = GameType::FreeFire;
            return GameType::FreeFire;
        }
    }

    // Check for Free Fire MAX
    if (pImpl->ExecuteADB("shell pm list packages com.dts.freefiremax", output)) {
        if (output.find("com.dts.freefiremax") != std::string::npos) {
            pImpl->Log("Detected Free Fire MAX installed", LogLevel::Success);
            pImpl->selectedGame = GameType::FreeFireMAX;
            return GameType::FreeFireMAX;
        }
    }

    pImpl->Log("No supported game detected", LogLevel::Warning);
    pImpl->selectedGame = GameType::None;
    return GameType::None;
}

bool CertManager::InstallProxy() {
    if (pImpl->selectedEmulator == EmulatorType::None) {
        pImpl->lastError = "No emulator selected";
        pImpl->Log(pImpl->lastError, LogLevel::Error);
        return false;
    }

    if (!pImpl->adbConnected) {
        if (!CheckADBConnection()) {
            if (!ConnectADB()) {
                return false;
            }
        }
    }

    // Auto-detect game if none selected
    if (pImpl->selectedGame == GameType::None) {
        pImpl->Log("No game selected, attempting auto-detection...", LogLevel::Info);
        if (DetectInstalledGame() == GameType::None) {
            pImpl->lastError = "No supported game found (Free Fire or Free Fire MAX)";
            pImpl->Log(pImpl->lastError, LogLevel::Error);
            return false;
        }
    }

    // Get package name based on selected game
    std::string packageName = GetGamePackageName();
    if (packageName.empty()) {
        pImpl->lastError = "Invalid game package";
        pImpl->Log(pImpl->lastError, LogLevel::Error);
        return false;
    }

    std::string gameName = (pImpl->selectedGame == GameType::FreeFire) ? "Free Fire" : "Free Fire MAX";
    pImpl->Log("Installing proxy for: " + gameName + " (" + packageName + ")", LogLevel::Info);

    std::string proxyAddr = GetLocalIPv4() + ":" + pImpl->proxyPort;
    std::string command = "shell settings put global http_proxy " + proxyAddr;
    std::string output;

    // 1. Force stop the game
    pImpl->ExecuteADB("shell am force-stop " + packageName, output);
    pImpl->Log("Force stopped: " + packageName, LogLevel::Info);

    // 2. Set proxy
    if (!pImpl->ExecuteADB(command, output)) {
        pImpl->lastError = "Failed to install proxy";
        pImpl->Log(pImpl->lastError, LogLevel::Error);
        return false;
    }

    // 3. Launch the game
    pImpl->ExecuteADB("shell monkey -p " + packageName + " -c android.intent.category.LAUNCHER 1", output);
    pImpl->Log("Launched: " + packageName, LogLevel::Info);

    // 4. Verify
    std::string proxyValue;
    if (!pImpl->GetGlobalProxy(proxyValue)) {
        pImpl->lastError = "Failed to check proxy installation";
        pImpl->Log(pImpl->lastError, LogLevel::Error);
        return false;
    }

    if (proxyValue.find(proxyAddr) != std::string::npos) {
        pImpl->Log("✓ Proxy installed successfully for " + gameName, LogLevel::Success);
        pImpl->Log("  Proxy: " + proxyValue, LogLevel::Info);
        return true;
    }

    pImpl->lastError = "Proxy installation verification failed: " + proxyValue;
    pImpl->Log(pImpl->lastError, LogLevel::Error);
    return false;
}


//bool CertManager::InstallProxy() {
//    if (pImpl->selectedEmulator == EmulatorType::None) {
//        pImpl->lastError = "No emulator selected";
//        pImpl->Log(pImpl->lastError, LogLevel::Error);
//        return false;
//    }
//    if (!pImpl->adbConnected) {
//        if (!CheckADBConnection()) {
//            if (!ConnectADB()) {
//                return false;
//            }
//        }
//    }
//
//    std::string proxyAddr = GetLocalIPv4() + ":" + pImpl->proxyPort;
//    std::string command = "shell settings put global http_proxy " + proxyAddr;
//
//    std::string output;
//    
//    // 1. Force stop the game
//    pImpl->ExecuteADB("shell am force-stop com.dts.freefireth", output);
//
//    if (!pImpl->ExecuteADB(command, output)) {
//        pImpl->lastError = "Failed to install proxy";
//        pImpl->Log(pImpl->lastError, LogLevel::Error);
//        return false;
//    }
//
//    // 3. Launch the game
//    pImpl->ExecuteADB("shell monkey -p com.dts.freefireth -c android.intent.category.LAUNCHER 1", output);
//    std::string proxyValue;
//    if (!pImpl->GetGlobalProxy(proxyValue)) {
//        pImpl->lastError = "Failed to check proxy installation";
//        pImpl->Log(pImpl->lastError, LogLevel::Error);
//        return false;
//    }
//    if (proxyValue.find(proxyAddr) != std::string::npos) {
//        pImpl->Log("Proxy installed successfully: " + proxyValue, LogLevel::Success);
//        return true;
//    }
//    pImpl->lastError = "Proxy installation verification failed: " + proxyValue;
//    pImpl->Log(pImpl->lastError, LogLevel::Error);
//    return false;
//}

bool CertManager::UninstallProxy() {
    if (pImpl->selectedEmulator == EmulatorType::None) {
        pImpl->lastError = "No emulator selected";
        pImpl->Log(pImpl->lastError, LogLevel::Error);
        return false;
    }
    if (!pImpl->adbConnected) {
        if (!CheckADBConnection()) {
            if (!ConnectADB()) {
                return false;
            }
        }
    }

    std::string command = "shell settings put global http_proxy :0";

    std::string output;
    if (!pImpl->ExecuteADB(command, output)) {
        pImpl->lastError = "Failed to uninstall proxy";
        pImpl->Log(pImpl->lastError, LogLevel::Error);
        return false;
    }
    std::string proxyValue;
    if (!pImpl->GetGlobalProxy(proxyValue)) {
        pImpl->lastError = "Failed to check proxy uninstallation";
        pImpl->Log(pImpl->lastError, LogLevel::Error);
        return false;
    }
    if (proxyValue.find(NO_PROXY) != std::string::npos) {
        pImpl->Log("Proxy uninstalled successfully: " + proxyValue, LogLevel::Success);
        return true;
    }
    pImpl->lastError = "Proxy uninstallation verification failed: " + proxyValue;
    pImpl->Log(pImpl->lastError, LogLevel::Error);
    return false;
}

bool CertManager::Impl::GetGlobalProxy(std::string& proxyValue) {
    std::string getCommand = "shell \"settings get global http_proxy\"";
    if (!ExecuteADB(getCommand, proxyValue)) {
        return false;
    }
    proxyValue.erase(std::remove(proxyValue.begin(), proxyValue.end(), '\n'), proxyValue.end());
    proxyValue.erase(std::remove(proxyValue.begin(), proxyValue.end(), '\r'), proxyValue.end());
    return true;
}


void CertManager::StartEmulatorMonitoring() {
    std::thread([impl = pImpl.get()]() {
        while (true) {
            auto pids = impl->FindProcessesByName("HD-Player.exe");
            auto memuPids = impl->FindProcessesByName("MEmu.exe");

            if (!memuPids.empty()) {
                if (impl->selectedEmulator != EmulatorType::Memu) {
                    impl->selectedEmulator = EmulatorType::Memu;
                    impl->emulatorVersion = "";
                    impl->Log("MEmu detected", LogLevel::Info);
                }
                // Get MEmu version
                std::string memuAdb = impl->GetMemuAdbPath();
                if (!memuAdb.empty()) {
                    std::string memuDir = fs::path(memuAdb).parent_path().string();
                    std::string memuExe = memuDir + "\\MEmu.exe";
                    DWORD verHandle = 0;
                    DWORD verSize = GetFileVersionInfoSizeA(memuExe.c_str(), &verHandle);
                    if (verSize) {
                        std::vector<char> verData(verSize);
                        if (GetFileVersionInfoA(memuExe.c_str(), verHandle, verSize, verData.data())) {
                            VS_FIXEDFILEINFO* verInfo = nullptr;
                            UINT verInfoSize = 0;
                            if (VerQueryValueA(verData.data(), "\\", reinterpret_cast<void**>(&verInfo), &verInfoSize) && verInfoSize) {
                                std::ostringstream oss;
                                oss << HIWORD(verInfo->dwFileVersionMS) << "."
                                    << LOWORD(verInfo->dwFileVersionMS) << "."
                                    << HIWORD(verInfo->dwFileVersionLS) << "."
                                    << LOWORD(verInfo->dwFileVersionLS);
                                impl->emulatorVersion = oss.str();
                            }
                        }
                    }
                }
            }
            else if (!pids.empty()) {
                DWORD pid = pids[0];
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                bool pathFound = false;
                if (hProcess) {
                    char path[MAX_PATH];
                    DWORD size = MAX_PATH;
                    if (QueryFullProcessImageNameA(hProcess, 0, path, &size)) {
                        pathFound = true;
                        std::string spath = path;
                        std::transform(spath.begin(), spath.end(), spath.begin(), ::tolower);
                        EmulatorType type = EmulatorType::None;
                        if (spath.find("nxt") != std::string::npos) {
                            type = EmulatorType::BlueStacks5;
                        }
                        else if (spath.find("msi5") != std::string::npos) {
                            type = EmulatorType::MSI5;
                        }
                        if (type != EmulatorType::None && type != impl->selectedEmulator) {
                            impl->selectedEmulator = type;
                            impl->Log("Detected emulator: " + std::to_string(static_cast<int>(type)), LogLevel::Info);
                        }

                        DWORD verHandle = 0;
                        DWORD verSize = GetFileVersionInfoSizeA(path, &verHandle);
                        if (verSize) {
                            std::vector<char> verData(verSize);
                            if (GetFileVersionInfoA(path, verHandle, verSize, verData.data())) {
                                VS_FIXEDFILEINFO* verInfo = nullptr;
                                UINT verInfoSize = 0;
                                if (VerQueryValueA(verData.data(), "\\", reinterpret_cast<void**>(&verInfo), &verInfoSize)) {
                                    if (verInfoSize) {
                                        std::ostringstream oss;
                                        oss << HIWORD(verInfo->dwFileVersionMS) << "."
                                            << LOWORD(verInfo->dwFileVersionMS) << "."
                                            << HIWORD(verInfo->dwFileVersionLS) << "."
                                            << LOWORD(verInfo->dwFileVersionLS);
                                        impl->emulatorVersion = oss.str();
                                    }
                                }
                            }
                        }
                    }
                    CloseHandle(hProcess);
                }
                
                if (!pathFound && impl->selectedEmulator == EmulatorType::None) {
                    impl->selectedEmulator = EmulatorType::BlueStacks5;
                    impl->emulatorVersion = "Unknown";
                    impl->Log("Detected emulator (Fallback): BlueStacks 5", LogLevel::Info);
                }
            }
            else {
                if (impl->selectedEmulator != EmulatorType::None) {
                    impl->selectedEmulator = EmulatorType::None;
                    impl->emulatorVersion = "";
                    impl->Log("Emulator no longer detected", LogLevel::Info);
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        }).detach();
}

std::string CertManager::GetEmulatorName() const {
    switch (pImpl->selectedEmulator) {
    case EmulatorType::BlueStacks5: return "BlueStacks App Player 5";
    case EmulatorType::MSI5:        return "MSI App Player 5";
    case EmulatorType::Memu:        return "MEmu Player";
    default:                        return "None";
    }
}

std::string CertManager::GetEmulatorVersion() const {

    if (GetEmulatorName() == "None")
    {
		return "None";
    }
    
    return pImpl->emulatorVersion;
}

DWORD CertManager::Impl::FindProcessId(const std::wstring& processName) {
    DWORD processId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (processName == pe.szExeFile) {
                processId = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return processId;
}

std::string CertManager::Impl::ConvertNtPathToDosPath(const std::string& ntPath) {
    if (ntPath.find("\\Device\\") != 0) {
        return ntPath;
    }

    DWORD drives = GetLogicalDrives();
    for (char letter = 'A'; letter <= 'Z'; letter++) {
        if (drives & (1 << (letter - 'A'))) {
            std::string driveLetter = std::string(1, letter) + ":";
            char deviceName[MAX_PATH];

            if (QueryDosDeviceA(driveLetter.c_str(), deviceName, MAX_PATH)) {
                std::string deviceStr(deviceName);

                if (ntPath.find(deviceStr) == 0) {

                    std::string dosPath = driveLetter + ntPath.substr(deviceStr.length());
                    return dosPath;
                }
            }
        }
    }
    return ntPath;
}

std::string CertManager::Impl::FindBstkCoreLogPath(DWORD targetPid) {
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return "";
    _NtQuerySystemInformation NtQuerySystemInformation =
        (_NtQuerySystemInformation)GetProcAddress(ntdll, "NtQuerySystemInformation");
    _NtDuplicateObject NtDuplicateObject =
        (_NtDuplicateObject)GetProcAddress(ntdll, "NtDuplicateObject");
    _NtQueryObject NtQueryObject =
        (_NtQueryObject)GetProcAddress(ntdll, "NtQueryObject");
    if (!NtQuerySystemInformation || !NtDuplicateObject || !NtQueryObject) return "";
    ULONG bufferSize = 0x100000;
    PVOID buffer = malloc(bufferSize);
    if (!buffer) return "";
    NTSTATUS status = NtQuerySystemInformation(SystemHandleInformation, buffer, bufferSize, &bufferSize);
    if (status == STATUS_INFO_LENGTH_MISMATCH) {
        free(buffer);
        buffer = malloc(bufferSize);
        if (!buffer) return "";
        status = NtQuerySystemInformation(SystemHandleInformation, buffer, bufferSize, &bufferSize);
    }
    if (status != STATUS_SUCCESS) {
        free(buffer);
        return "";
    }
    PSYSTEM_HANDLE_INFORMATION handleInfo = (PSYSTEM_HANDLE_INFORMATION)buffer;
    HANDLE sourceProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, targetPid);
    if (!sourceProcess) {
        free(buffer);
        return "";
    }
    std::string result;
    for (ULONG i = 0; i < handleInfo->NumberOfHandles; i++) {
        if (handleInfo->Handles[i].ProcessId != targetPid) continue;
        HANDLE duplicatedHandle;
        if (NtDuplicateObject(sourceProcess, (HANDLE)handleInfo->Handles[i].HandleValue,
            GetCurrentProcess(), &duplicatedHandle, 0, 0, 0) != STATUS_SUCCESS) {
            continue;
        }
        ULONG nameSize = 0x1000;
        POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)malloc(nameSize);
        if (nameInfo && NtQueryObject(duplicatedHandle, ObjectNameInformation, nameInfo, nameSize, &nameSize) == STATUS_SUCCESS) {
            if (nameInfo->Name.Buffer) {
                std::wstring filename(nameInfo->Name.Buffer, nameInfo->Name.Length / sizeof(WCHAR));
                std::string filenameA(filename.begin(), filename.end());

                if (filenameA.find("BstkCore.log") != std::string::npos) {

                    result = ConvertNtPathToDosPath(filenameA);
                    free(nameInfo);
                    CloseHandle(duplicatedHandle);
                    break;
                }
            }
        }
        if (nameInfo) free(nameInfo);
        CloseHandle(duplicatedHandle);
    }
    CloseHandle(sourceProcess);
    free(buffer);
    return result;
}

std::string CertManager::Impl::GetCurrentEngineAdbPort() {
    DWORD pid = FindProcessId(L"HD-Player.exe");
    if (!pid) {
        Log("[ADB PORT] ERROR: HD-Player.exe process not found.", LogLevel::Error);
        return "";
    }
    std::string logPath;
    int retryCount = 0;
    const int maxRetries = 10;
    const int retryDelayMs = 100;
    do {
        logPath = FindBstkCoreLogPath(pid);
        if (!logPath.empty()) break;
        Sleep(retryDelayMs);
        retryCount++;
    } while (retryCount < maxRetries);
    if (logPath.empty()) {
        Log("[ADB PORT] ERROR: BstkCore.log handle not found in HD-Player.exe.", LogLevel::Error);
        return "";
    }
    std::filesystem::path logFsPath(logPath);
    std::filesystem::path logDir = logFsPath.parent_path();
    if (logDir.empty()) {
        Log("[ADB PORT] ERROR: Could not determine log directory.", LogLevel::Error);
        return "";
    }
    std::filesystem::path engineDir = logDir.parent_path();
    if (engineDir.empty()) {
        Log("[ADB PORT] ERROR: Could not determine Engine directory.", LogLevel::Error);
        return "";
    }
    std::string engineName = engineDir.filename().string();
    std::filesystem::path blueStacksRoot = engineDir.parent_path().parent_path();
    std::filesystem::path confPath = blueStacksRoot / "bluestacks.conf";
    if (!std::filesystem::exists(confPath)) {
        Log("[ADB PORT] ERROR: bluestacks.conf not found: " + confPath.string(), LogLevel::Error);
        return "";
    }

    std::string searchKey = "bst.instance." + engineName + ".status.adb_port";
    std::ifstream confFile(confPath);
    std::string line;
    std::regex portRegex("adb_port\\s*=\\s*\"?(\\d+)\"?");
    while (std::getline(confFile, line)) {
        std::string lineNoSpaces = line;
        lineNoSpaces.erase(std::remove(lineNoSpaces.begin(), lineNoSpaces.end(), ' '), lineNoSpaces.end());
        if (lineNoSpaces.find(searchKey + "=") != std::string::npos) {
            std::smatch match;
            if (std::regex_search(line, match, portRegex)) {
                Log("Detected ADB port: " + match[1].str(), LogLevel::Info);
                return match[1].str();
            }
        }
    }
    Log("[ADB PORT] ERROR: ADB port not found for " + engineName + " in bluestacks.conf.", LogLevel::Error);
    return "";
}