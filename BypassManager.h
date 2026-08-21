// MADE BY FINEX BOYZZ.
// DC LINK: https://discord.gg/AHwg2YA6sE 

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <algorithm>
#include <map>

enum class EmulatorType {
    None,
    MSI5,
    BlueStacks5,
    Memu
};

enum class LogLevel {
    Info,
    Success,
    Error,
    Warning
};

enum class GameType {
    None,
    FreeFire,
    FreeFireMAX
};

class CertManager {

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;

    std::string GetGamePackageName() const;

public:
    CertManager();
    ~CertManager();

    bool Initialize(const std::string& adbPort);

    bool CheckWhitelist(const std::string& uid);
    bool LoadWhitelistFromGithub(const std::string& url);

    bool InstallCertificate();
    bool UninstallCertificate();
    bool IsCertificateInstalled();

    bool IsEmulatorPatched();
    bool PatchVHDConfigs();
    bool RequestAccess();

    std::vector<EmulatorType> DetectAvailableEmulators();
    bool SelectEmulator(EmulatorType type);
    EmulatorType GetSelectedEmulator() const;
    bool KillEmulatorProcesses();

    bool ConnectADB();
    bool DisconnectADB();
    bool IsADBConnected() const;

    bool CheckADBConnection();

    // Proxy port config (default 54233, used with local machine IP)
    void SetProxyPort(const std::string& port);
    std::string GetProxyPort() const;

    bool InstallProxy();
    bool UninstallProxy();

    // Launch emulator executable
    bool StartEmulator(EmulatorType type = EmulatorType::None);

    using LogCallback = std::function<void(const std::string&, LogLevel)>;
    void SetLogCallback(LogCallback callback);

    std::string GetLastError() const;

    void StartEmulatorMonitoring();
    std::string GetEmulatorName() const;
    std::string GetEmulatorVersion() const;

    // Returns the machine's local IPv4 address (e.g. "192.168.x.x")
    static std::string GetLocalIPv4();

    // Game selection methods
    void SetGame(GameType game);
    GameType GetGame() const;
    GameType DetectInstalledGame();

};

namespace CertData {
    constexpr const char* CERT_HASH = "c8750f0d";

    constexpr const char CERTIFICATE_BYTES[] =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIDNTCCAh2gAwIBAgIUI/N0+3LMITnCqNmFVtWf/UfanIEwDQYJKoZIhvcNAQEL\n"
        "BQAwKDESMBAGA1UEAwwJbWl0bXByb3h5MRIwEAYDVQQKDAltaXRtcHJveHkwHhcN\n"
        "MjUxMDEyMDAxNjIxWhcNMzUxMDEyMDAxNjIxWjAoMRIwEAYDVQQDDAltaXRtcHJv\n"
        "eHkxEjAQBgNVBAoMCW1pdG1wcm94eTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\n"
        "AQoCggEBAMrW1Q4RZ2nEycaEUjaVSN4npI1WA5I8lKqMb/DBJcUi+j0EhTACXKPY\n"
        "BQQOzcilqFS90oQ/PfyU3YgxXd1Sy8xR6FzRPHRqmqbQlmEw5yhLcjSfvDBYv4TE\n"
        "6VRAaALmjSNkkPsOOgegmCPPSFfkuYw3U7GGf9r3PW08iR4FAZgOkWtt5fxXsqm8\n"
        "zdPSrWl2pVvxVk0lPPiwJU3x4d9dG74deJ0EP2g9oR03lMChP+GP8CjjwQHLS5y7\n"
        "JLtAWsPFNnRTrGATSXBYnv+uhN1WzQzynlSa8dugf0lIdSPm1GgGjENyajeZZFUD\n"
        "Pye5hlUXlKwqxjyCYPrzsxFk6kDjOYsCAwEAAaNXMFUwDwYDVR0TAQH/BAUwAwEB\n"
        "/zATBgNVHSUEDDAKBggrBgEFBQcDATAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE\n"
        "FBeoFi4QYl5evVvdWBGOUFWTjR9aMA0GCSqGSIb3DQEBCwUAA4IBAQCiDSLsZ2Oi\n"
        "qlxsptOvLkvWIoGetSBHlpg+FfPCL022WwAelHzV3LQPASl8yVo1B4mp4qK8Pt4N\n"
        "KaMwzeoxTHv+XKBsrW/X75HEHITg7zjOrKDjot7YEs5ZXfxm3Ni2b254Ywxh8e/q\n"
        "htMRKapPP6uV2ACnCR9QyCFtmq8s1oGGsISoUzIum5lTT2z8yDWoG30fQvXc+d53\n"
        "Fvkvp84tPzPDMnjdvmgl/1khrcar4hwlJasAZ1u0VNQ/WwIkdj7HmuwRmaZZuP/y\n"
        "+q7EjQvttIygDpW0cH7aCKGhFNIhQfPQhUxC/tqZGJa98LbURbd47LlR3o0s5msm\n"
        "8v4BnA9cmeTy\n"
        "-----END CERTIFICATE-----\n";

    constexpr size_t CERTIFICATE_SIZE = sizeof(CERTIFICATE_BYTES) - 1;
}