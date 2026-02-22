#include "transfer/sender.h"
#include "transfer/protocol.h"
#include "net/socket_utils.h"
#include "crypto/sha256.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>

using namespace std;

int svanipp::run_sender(const string& ip, uint16_t port, const string& filePath) {
    namespace fs = filesystem;

    fs::path p(filePath);
    if (!fs::exists(p)) {
        cerr << "File not found: " << filePath << "\n";
        return 1;
    }

    const string filename = p.filename().string();
    const uint64_t fileSize = static_cast<uint64_t>(fs::file_size(p));

    ifstream in(filePath, ios::binary);
    if (!in) {
        cerr << "Cannot open file: " << filePath << "\n";
        return 1;
    }

    socket_t sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cerr << "socket() failed\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        cerr << "Invalid IP: " << ip << "\n";
        closesocket(sock);
        return 1;
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        cerr << "connect() failed\n";
        closesocket(sock);
        return 1;
    }

    svanipp::proto::HeaderFixed hf{};
    memcpy(hf.magic, svanipp::proto::MAGIC, 8);
    hf.version = hton_u16(svanipp::proto::VERSION);
    hf.filename_len = hton_u16(static_cast<uint16_t>(filename.size()));
    hf.file_size = hton_u64(fileSize);

    if (!sendAll(sock, &hf, sizeof(hf))) {
        cerr << "Failed to send header\n";
        closesocket(sock);
        return 1;
    }
    if (!sendAll(sock, filename.data(), filename.size())) {
        cerr << "Failed to send filename\n";
        closesocket(sock);
        return 1;
    }

    svanipp::crypto::Sha256 hasher;

    const size_t BUF = 64 * 1024;
    vector<char> buf(BUF);

    uint64_t sent = 0;
    while (in) {
        in.read(buf.data(), static_cast<streamsize>(BUF));
        streamsize n = in.gcount(); 
        if (n <= 0) break;
        hasher.update(buf.data(), static_cast<size_t>(n));

        if (!sendAll(sock, buf.data(), static_cast<size_t>(n))) {
            cerr << "\nConnection lost while sending\n";
            closesocket(sock);
            return 1;
        }

        sent += static_cast<uint64_t>(n);
        if (fileSize > 0) {
            int pct = static_cast<int>((sent * 100ULL) / fileSize);
            cout << "\rSending " << filename << " ... " << pct << "%";
            cout.flush();
        }
    }

    uint8_t digest[32];
    hasher.final(digest);

    if (!sendAll(sock, digest, sizeof(digest))) {
        cerr << "\nFailed to send SHA-256 digest\n";
        closesocket(sock);
        return 1;
    }

    cout << "\nSent: " << filename << " (" << sent << " bytes)\n";
    closesocket(sock);
    return (sent == fileSize) ? 0 : 2;
}