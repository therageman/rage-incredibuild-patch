// This is probably way more complicated then it needs to be.
#include "utils.h"


int main(int argc, char* argv[]) {
    HWND hWnd = GetConsoleWindow();
    if (!AreWeAnAdmin() && hWnd != NULL) {
        ShowWindow(hWnd, SW_HIDE);
    }
    for (int i = 1; i < argc; i++) {
        auto argument = std::string(argv[i]);
        if (argument == "-silent") {
            isSilentMode = true;
        }
    }
    if(!isSilentMode && hWnd) ShowWindow(hWnd, SW_SHOW);

    // Admin previllages will be required so i'll just ask.
    if(!AreWeAnAdmin()) EnsureAdmin(); // guranteed exit
    CreateSettingsRegistry();
    HANDLE hPipe = CreateFileA(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) return 1;
    PipeStreamBuf relayBuf(hPipe);
    std::streambuf* oldCout = std::cout.rdbuf(&relayBuf);
    std::streambuf* oldCerr = std::cerr.rdbuf(&relayBuf);

    for (int i = 1; i < argc; i++) {
        auto argument = std::string(argv[i]);
        if (argument == "-addasstartup") {
            setupAsAStartup();
            SettingsFinalize();
            CloseHandle(hPipe);
            return 0;
        }
        else if (argument == "-removestartup") {
            removeFromStartup();
            SettingsFinalize();
            CloseHandle(hPipe);
            return 0;
        }
    }

    auto PipePlaySound = [&](LPCSTR sound, DWORD flags) {
        PacketHeader header = { PACKET_MAGIC, PKT_SOUND, sizeof(SoundData) };
        SoundData sData = { 0 };
        if (sound) strncpy_s(sData.soundName, sound, _TRUNCATE);
        sData.flags = flags;

        DWORD written;
        WriteFile(hPipe, &header, sizeof(header), &written, NULL);
        WriteFile(hPipe, &sData, sizeof(sData), &written, NULL);
    };

    // Calculate time: 30 days from now at 23:59:59
    time_t now = time(0);
    struct tm ltm;
    localtime_s(&ltm, &now);
    ltm.tm_mday += 30;
    ltm.tm_hour = 23;
    ltm.tm_min = 59;
    ltm.tm_sec = 59;
    mktime(&ltm);

    // Convert to the proper format
    double unixDays = (double)mktime(&ltm) / 86400.0;
    double win32Date = unixDays + 25569.0;
    std::string customGuid = GenerateCustomGUID(win32Date);

    // Overwrite the register.
    HKEY hKey;
    const char* subKey = "Interface\\{23DE9F4B-25F9-4163-BF69-01639BB2B8BA}\\ProxyStubClsid32"; // This is hard coded version ID, If I were to fix this, I would oversee how xgConsole gets the GUID and then get the ID but thats too much work.

    LSTATUS status = RegOpenKeyExA(HKEY_CLASSES_ROOT, subKey, 0, KEY_SET_VALUE | KEY_WOW64_32KEY, &hKey);
    if (status == ERROR_SUCCESS) {
        status = RegSetValueExA(hKey, NULL, 0, REG_SZ, (const BYTE*)customGuid.c_str(), (DWORD)customGuid.length() + 1);

        if (status == ERROR_SUCCESS) {
            PipePlaySound("Asterisk", SND_ALIAS | SND_ASYNC);
            std::cout << "You'll now have another 30 days of Incredibuild 4.0 for building Grand Theft Auto V!" << std::endl;
            if (GetRemindStartupValue() == 1 && GetStartupActive() != 1) {
                std::cout << "Do you want to execute this application every time your computer starts? This will be the only time I'll ask (Y/n)" << std::endl;
                std::string response = PipeCin(hPipe);
                if (response == "Y" || response == "y") {
                    setupAsAStartup();
                }
                else {
                    std::cout << "You won't be asked again! However you can do it by yourself by putting -addasstartup as a argument in the future!" << std::endl;
                    SetRemindStartupValue(0);
                }
            }
        }
        else {
            PipePlaySound("SystemHand", SND_ALIAS | SND_ASYNC);
            std::cerr << "Something happened. [" << status << "]" << std::endl;
        }
        RegCloseKey(hKey);
    }
    else {
        std::cerr << "You most likely don't have IncrediBuild 4.0 installed! Install it at https://xoreax-incredibuild.software.informer.com/4.0/" << status << std::endl;
    }
    SettingsFinalize();
    CloseHandle(hPipe);
    return 0;
}