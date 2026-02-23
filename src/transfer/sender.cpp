#include "transfer/sender.h"
#include "transfer/protocol.h"
#include "net/socket_utils.h"
#include "crypto/sha256.h"
#include "console/console_ui.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstring>
#include <chrono>

using namespace std;

int svanipp::run_sender(const string& ip,
                        uint16_t port,
                        const string& filePath,
                        const string& relPath,
                        uint64_t& bytesSent,
                        string& error) {
    namespace fs = filesystem;
    bytesSent = 0;
    error.clear();
    auto& ui = svanipp::console::ConsoleUI::instance();

    fs::path p = fs::u8path(filePath);
    if (!fs::exists(p)) {
        ui.log(svanipp::console::Style::Fail, "File not found: " + filePath);
        error = "file not found";
        return 1;
    }

    // Use relPath for transfer
    const string filename = relPath;
    const uint64_t fileSize = static_cast<uint64_t>(fs::file_size(p));

    ifstream in(p, ios::binary);
    if (!in) {
        ui.log(svanipp::console::Style::Fail, "Cannot open file: " + filePath);
        error = "open failed";
        return 1;
    }

    socket_t sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        ui.log(svanipp::console::Style::Fail, "socket() failed");
        error = "socket failed";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        ui.log(svanipp::console::Style::Fail, "Invalid IP: " + ip);
        closesocket(sock);
        error = "invalid ip";
        return 1;
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        ui.log(svanipp::console::Style::Fail, "connect() failed");
        closesocket(sock);
        error = "connect failed";
        return 1;
    }

    svanipp::proto::HeaderFixed hf{};
    memcpy(hf.magic, svanipp::proto::MAGIC, 8);
    hf.version = hton_u16(svanipp::proto::VERSION);
    hf.filename_len = hton_u16(static_cast<uint16_t>(filename.size()));
    hf.file_size = hton_u64(fileSize);

    if (!sendAll(sock, &hf, sizeof(hf))) {
        ui.log(svanipp::console::Style::Fail, "Failed to send header");
        closesocket(sock);
        error = "send header failed";
        return 1;
    }
    if (!sendAll(sock, filename.data(), filename.size())) {
        ui.log(svanipp::console::Style::Fail, "Failed to send filename");
        closesocket(sock);
        error = "send filename failed";
        return 1;
    }

    svanipp::crypto::Sha256 hasher;

    const size_t BUF = 64 * 1024;
    vector<char> buf(BUF);

    using clock = chrono::steady_clock;
    auto start_time = clock::now();
    uint64_t sent = 0;
    while (in) {
        in.read(buf.data(), static_cast<streamsize>(BUF));
        streamsize n = in.gcount(); 
        if (n <= 0) break;
        hasher.update(buf.data(), static_cast<size_t>(n));

        if (!sendAll(sock, buf.data(), static_cast<size_t>(n))) {
            ui.progress_end(true);
            ui.log(svanipp::console::Style::Fail, "Connection lost while sending: " + filename);
            closesocket(sock);
            error = "connection lost";
            bytesSent = sent;
            return 1;
        }

        sent += static_cast<uint64_t>(n);
        if (fileSize > 0) {
            int pct = static_cast<int>((sent * 100ULL) / fileSize);
            auto now = clock::now();
            double elapsed = chrono::duration<double>(now - start_time).count();
            double mbps = (elapsed > 0.0) ? (sent / (1024.0 * 1024.0)) / elapsed : 0.0;
            double bps = (elapsed > 0.0) ? (static_cast<double>(sent) / elapsed) : 0.0;
            int eta = (bps > 0.0) ? static_cast<int>((fileSize - sent) / bps) : -1;
            string line = ui.make_status_line("Send", filename, pct, mbps, eta);
            ui.progress_update(line, pct);
        }
    }

    uint8_t digest[32];
    hasher.final(digest);

    if (!sendAll(sock, digest, sizeof(digest))) {
        ui.progress_end(true);
        ui.log(svanipp::console::Style::Fail, "Failed to send SHA-256 digest: " + filename);
        closesocket(sock);
        error = "send digest failed";
        bytesSent = sent;
        return 1;
    }

    ui.progress_end(true);
    double elapsed = chrono::duration<double>(clock::now() - start_time).count();
    double mb = sent / (1024.0 * 1024.0);
    ostringstream okMsg;
    okMsg << "Sent " << filename << " (" << fixed << setprecision(2) << mb << " MB, "
          << fixed << setprecision(2) << elapsed << " s)";
    ui.log(svanipp::console::Style::Ok, okMsg.str());
    closesocket(sock);
    bytesSent = sent;
    if (sent != fileSize) {
        error = "incomplete send";
        return 2;
    }
    return 0;
}