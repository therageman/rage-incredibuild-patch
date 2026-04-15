#pragma once
#include <iostream>
#include <vector>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include "defs.h"

enum PacketType : DWORD {
    PKT_TEXT = 1,
    PKT_SOUND = 2,
    PKT_CIN = 3
};

struct PacketHeader {
    DWORD magic;
    PacketType type;
    DWORD dataSize;
};

struct SoundData {
    char soundName[64];
    DWORD flags;
};


struct SoundPacket {
    DWORD magic = 0x42495347;
    char soundName[64];
    DWORD flags;
};

class PipeStreamBuf : public std::streambuf {
    HANDLE hPipe;
public:
    PipeStreamBuf(HANDLE pipe) : hPipe(pipe) {}

protected:
    virtual int sync() override { // just to make std::cout less awful [requires the line to be ended.]
        std::string s = str();
        if (!s.empty()) {
            PacketHeader header = { PACKET_MAGIC, PKT_TEXT, (DWORD)s.length() };
            DWORD written;
            WriteFile(hPipe, &header, sizeof(header), &written, NULL);
            WriteFile(hPipe, s.c_str(), (DWORD)s.length(), &written, NULL);

            str().clear();
        }
        return 0;
    }
    ;

    virtual int overflow(int c) override {
        if (c != EOF) {
            str().push_back(static_cast<char>(c));
            if (c == '\n') sync(); // Auto-flush on newline only
        }
        return c;
    }


private:
    std::string& str() { static std::string s; return s; }
};

std::string PipeCin(HANDLE hPipe) { // good enough
    PacketHeader header = { PACKET_MAGIC, PKT_CIN, 0 };
    DWORD written;
    WriteFile(hPipe, &header, sizeof(header), &written, NULL);

    char buffer[256] = { 0 };
    DWORD read;
    if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &read, NULL)) {
        return std::string(buffer, read);
    }
    return "";
}