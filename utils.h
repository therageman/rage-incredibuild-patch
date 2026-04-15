#pragma once
#include "pipes.h"
#include "defs.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <shellapi.h>
#pragma comment(lib, "winmm.lib")

bool AreWeAnAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

// This is on the client side, when the application gets administrator previllages, it serves as a backend.
bool isSilentMode = false;
void EnsureAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    if (!isAdmin) {
        HANDLE hPipe = CreateNamedPipeA(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT, 1, 1024, 1024, 0, NULL);
        char szPath[MAX_PATH];
        GetModuleFileNameA(NULL, szPath, MAX_PATH);
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = szPath;
        std::string args;
        for (int i = 1; i < __argc; ++i) {
            args += __argv[i];
            if (i < __argc - 1) {
                args += " ";
            }
        }
        sei.lpParameters = args.c_str();
        sei.hwnd = NULL;
        sei.nShow = SW_HIDE;

        if (ShellExecuteExA(&sei)) {
            // We print feedback from the previllaged version which is hidden from the user. Just to keep it clean!
            ConnectNamedPipe(hPipe, NULL);
            PacketHeader header{};
            DWORD bytesRead;

            while (ReadFile(hPipe, &header, sizeof(header), &bytesRead, NULL) && bytesRead != 0) {
                if (header.magic != PACKET_MAGIC) {
                    std::cerr << "Invalid Magic!" << std::endl;
                    break;
                }

                if (header.type == PKT_TEXT) {
                    std::string msg(header.dataSize, '\0');
                    if (ReadFile(hPipe, &msg[0], header.dataSize, &bytesRead, NULL)) {
                        std::cout << msg;
                    }
                }
                else if (header.type == PKT_SOUND) {
                    SoundData sData{};
                    if (ReadFile(hPipe, &sData, sizeof(sData), &bytesRead, NULL)) {
                        PlaySoundA(sData.soundName[0] ? sData.soundName : NULL, NULL, sData.flags);
                    }
                } // good enough alternative for CIN
                else if (header.type == PKT_CIN && !isSilentMode) {
                    std::string userInput;
                    std::getline(std::cin, userInput);
                    DWORD written;
                    WriteFile(hPipe, userInput.c_str(), (DWORD)userInput.length(), &written, NULL);
                }
            }
            CloseHandle(hPipe);
            if(!isSilentMode) system("pause");
            exit(0);
        }
        else {
            PlaySoundA("SystemHand", NULL, SND_ALIAS | SND_ASYNC);
            std::cerr << "I'll need to overwrite the registry and that requires admin previllages!" << std::endl;
            if(!isSilentMode) system("pause");
            exit(1);
        }
    }
}

// This is how IncrediBuild formats their trial dates in the registry. Atleast in 4.0 and earlier
std::string GenerateCustomGUID(double win32Date) {
    unsigned char b[8];
    memcpy(b, &win32Date, 8);
    // M1 = b0 * b1 * b2 * b3
    unsigned int m1 = (unsigned int)b[0] * b[1] * b[2] * b[3];
    // M2 = b4 * b5
    unsigned int m2 = (unsigned int)b[4] * b[5];
    // M3 = b6 * b7
    unsigned int m3 = (unsigned int)b[6] * b[7];

    std::stringstream ss;
    ss << "{" << std::hex << std::uppercase << std::setfill('0')
        << std::setw(8) << m1 << "-"
        << std::setw(4) << m2 << "-"
        << std::setw(4) << m3 << "-"
        << std::setw(2) << (int)b[0] << std::setw(2) << (int)b[1] << "-"
        << std::setw(2) << (int)b[2] << std::setw(2) << (int)b[3]
        << std::setw(2) << (int)b[4] << std::setw(2) << (int)b[5]
        << std::setw(2) << (int)b[6] << std::setw(2) << (int)b[7]
        << "}";
    return ss.str();
}


HKEY hSettingsKey;
const char* settingsPath = "Software\\RAGEMAN\\IncrediBuildReset";
DWORD dwType = REG_DWORD;
DWORD dwSize = sizeof(DWORD);

void CreateSettingsRegistry() {
    RegCreateKeyExA(HKEY_CURRENT_USER, settingsPath, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hSettingsKey, NULL);
}

void SetRemindStartupValue(DWORD value) {
    if (hSettingsKey) RegSetValueExA(hSettingsKey, "RemindStartup", 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
}

void SetStartupActive(DWORD value) {
    if (hSettingsKey) RegSetValueExA(hSettingsKey, "StartupActive", 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
}

DWORD GetRemindStartupValue() {
    DWORD remindStartup = 0;
    LSTATUS status = RegGetValueA(
        HKEY_CURRENT_USER,
        settingsPath,
        "RemindStartup",
        RRF_RT_REG_DWORD,
        NULL,
        &remindStartup,
        &dwSize
    );

    if (status == ERROR_SUCCESS) {
        return remindStartup;
    }
    else { // default
        return 1;
    }
}

DWORD GetStartupActive() {
    DWORD startupActive = 0;
    LSTATUS status = RegGetValueA(
        HKEY_CURRENT_USER,
        settingsPath,
        "StartupActive",
        RRF_RT_REG_DWORD,
        NULL,
        &startupActive,
        &dwSize
    );

    if (status == ERROR_SUCCESS) {
        return startupActive;
    }
    else { // default
        return 0;
    }
}

void SettingsFinalize() {
    if (hSettingsKey) RegCloseKey(hSettingsKey);
}

const char* runKeyPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
void setupAsAStartup() {
    HKEY hRunKey;
    LSTATUS startupStatus = RegOpenKeyExA(HKEY_CURRENT_USER, runKeyPath, 0, KEY_SET_VALUE, &hRunKey);

    if (startupStatus == ERROR_SUCCESS) {
        char szPath[MAX_PATH];
        GetModuleFileNameA(NULL, szPath, MAX_PATH);

        // The user will be notified by UAC but it will be silent.
        std::string pathWithArg = "\"" + std::string(szPath) + "\" -silent";

        startupStatus = RegSetValueExA(hRunKey, "RageIncredibuildHackStartup", 0, REG_SZ,
            (const BYTE*)pathWithArg.c_str(), (DWORD)pathWithArg.length() + 1);

        if (startupStatus == ERROR_SUCCESS) {
            std::cout << "Added!" << std::endl;
            SetStartupActive(1);
        }
        else {
            std::cerr << "Failed [" << startupStatus << "]" << std::endl;
        }
        RegCloseKey(hRunKey);
    }
    else {
        std::cerr << "Failed to open Run key [" << startupStatus << "]" << std::endl;
    }
}

void removeFromStartup() {
    HKEY hRunKey;
    LSTATUS status = RegOpenKeyExA(HKEY_CURRENT_USER, runKeyPath, 0, KEY_SET_VALUE, &hRunKey);

    if (status == ERROR_SUCCESS) {
        status = RegDeleteValueA(hRunKey, "RageIncredibuildHackStartup");

        if (status == ERROR_SUCCESS) {
            std::cout << "Gone!" << std::endl;
            SetStartupActive(0);
        }
        else if (status != ERROR_FILE_NOT_FOUND) {
            std::cerr << "Failed to delete startup value [" << status << "]" << std::endl;
        }
        RegCloseKey(hRunKey);
    }
    else {
        std::cerr << "Failed to open Run key for deletion [" << status << "]" << std::endl;
    }
}
