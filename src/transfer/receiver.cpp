#include "transfer/receiver.h"
#include "transfer/protocol.h"
#include "net/socket_utils.h"
#include "crypto/sha256.h"
#include "console/console_ui.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <cctype>
#include <sstream>

using namespace std;

static string sanitizeFilename(string name) {
    // minimal Windows-safe sanitization
    for (char& c : name) {
        if (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' ||
            c == '\\' || c == '|' || c == '?' || c == '*') {
            c = '_';
        }
    }
    if (name.empty()) name = "file.bin";
    return name;
}

static filesystem::path uniquePath(filesystem::path p) {
    namespace fs = filesystem;

    if (!fs::exists(p)) return p;

    fs::path dir = p.parent_path();
    string stem = p.stem().string();
    string ext  = p.extension().string();

    for (int i = 1; i < 10000; ++i) {
        fs::path candidate = dir / (stem + " (" + to_string(i) + ")" + ext);
        if (!fs::exists(candidate)) return candidate;
    }

    // fallback if somehow everything exists
    return dir / (stem + " (copy)" + ext);
}

static string truncate_middle(const string& s, size_t maxLen) {
    if (s.size() <= maxLen) return s;
    if (maxLen <= 3) return s.substr(0, maxLen);
    size_t keepFront = (maxLen - 3) / 2;
    size_t keepBack  = maxLen - 3 - keepFront;
    return s.substr(0, keepFront) + "..." + s.substr(s.size() - keepBack);
}

static void print_status_line(const string& line, size_t& lastLen) {
    cout << '\r' << line;
    if (line.size() < lastLen) {
        cout << string(lastLen - line.size(), ' ');
    }
    lastLen = line.size();
    cout.flush();
}

static bool is_subpath(const filesystem::path& base, const filesystem::path& target) {
    string baseStr = base.generic_u8string();
    string targetStr = target.generic_u8string();
    if (baseStr.empty()) return false;
    if (baseStr.back() != '/') baseStr.push_back('/');
#ifdef _WIN32
    transform(baseStr.begin(), baseStr.end(), baseStr.begin(), ::tolower);
    transform(targetStr.begin(), targetStr.end(), targetStr.begin(), ::tolower);
#endif
    return targetStr.rfind(baseStr, 0) == 0;
}




int svanipp::run_receiver(uint16_t port, const string& outDir, bool overwrite, bool summary) {
    namespace fs = filesystem;

    fs::path outPath = fs::path(outDir);
    error_code ec;
    fs::create_directories(outPath, ec);

    socket_t listenSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        cerr << "socket() failed\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    int yes = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&yes), sizeof(yes));

    if (::bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        cerr << "bind() failed\n";
        closesocket(listenSock);
        return 1;
    }

    if (::listen(listenSock, 1) == SOCKET_ERROR) {
        cerr << "listen() failed\n";
        closesocket(listenSock);
        return 1;
    }

    cout << "Listening on 0.0.0.0:" << port << ", saving to " << outPath.string() << "\n";
    auto& ui = svanipp::console::ConsoleUI::instance();

    uint64_t okFiles = 0;
    uint64_t okBytes = 0;
    uint64_t failedFiles = 0;

    for (;;) {
        sockaddr_in client{};
        int clientLen = sizeof(client);
        socket_t clientSock = ::accept(listenSock, reinterpret_cast<sockaddr*>(&client), &clientLen);
        if (clientSock == INVALID_SOCKET) {
            ui.log(svanipp::console::Style::Fail, "accept() failed");
            continue;
        }

        bool attempted = false;
        // Read fixed header
        svanipp::proto::HeaderFixed hf{};
        if (!recvExact(clientSock, &hf, sizeof(hf))) {
            ui.log(svanipp::console::Style::Fail, "Failed to read header");
            closesocket(clientSock);
            continue;
        }

        // Validate magic
        if (memcmp(hf.magic, svanipp::proto::MAGIC, 8) != 0) {
            ui.log(svanipp::console::Style::Fail, "Bad MAGIC (not a svanipp stream)");
            closesocket(clientSock);
            continue;
        }

        const uint16_t version = ntoh_u16(hf.version);
        const uint16_t nameLen = ntoh_u16(hf.filename_len);
        const uint64_t fileSize = ntoh_u64(hf.file_size);

        if (version != svanipp::proto::VERSION) {
            ui.log(svanipp::console::Style::Fail, "Unsupported version: " + to_string(version));
            closesocket(clientSock);
            continue;
        }
        if (nameLen == 0 || nameLen > 4096) {
            ui.log(svanipp::console::Style::Fail, "Invalid filename length");
            closesocket(clientSock);
            continue;
        }

        // Read filename
        vector<char> nameBuf(nameLen);
        if (!recvExact(clientSock, nameBuf.data(), nameBuf.size())) {
            ui.log(svanipp::console::Style::Fail, "Failed to read filename");
            closesocket(clientSock);
            continue;
        }
        string relPath(nameBuf.begin(), nameBuf.end());
        attempted = true;
        if (relPath.empty()) {
            ui.log(svanipp::console::Style::Fail, "Rejected unsafe path: " + relPath);
            closesocket(clientSock);
            failedFiles++;
            continue;
        }
        // Reject absolute paths, drive letters, and UNC paths
        if (!relPath.empty() && (relPath.front() == '\\' || relPath.front() == '/')) {
            ui.log(svanipp::console::Style::Fail, "Rejected unsafe path: " + relPath);
            closesocket(clientSock);
            failedFiles++;
            continue;
        }
        if (relPath.size() >= 2 && ((relPath[1] == ':' && isalpha(static_cast<unsigned char>(relPath[0]))) ||
                                    (relPath[0] == '\\' && relPath[1] == '\\') ||
                                    (relPath[0] == '/' && relPath[1] == '/'))) {
            ui.log(svanipp::console::Style::Fail, "Rejected unsafe path: " + relPath);
            closesocket(clientSock);
            failedFiles++;
            continue;
        }

        // Parse UTF-8 path and sanitize: normalize and reject any ".." segment
        fs::path rel = fs::u8path(relPath).lexically_normal();
        bool bad = rel.is_absolute() || rel.has_root_name() || rel.has_root_directory();
        if (rel.empty() || rel == ".") {
            bad = true;
        }
        for (const auto& part : rel) {
            if (part == "..") {
                bad = true;
                break;
            }
        }
        if (bad) {
            ui.log(svanipp::console::Style::Fail, "Rejected unsafe path: " + relPath);
            closesocket(clientSock);
            failedFiles++;
            continue;
        }

        // Compose final path and verify it stays inside output directory
        fs::path baseCanonical = fs::weakly_canonical(outPath, ec);
        fs::path saveAs = outPath / rel;
        fs::path saveCanonical = fs::weakly_canonical(saveAs, ec);
        if (ec || !is_subpath(baseCanonical, saveCanonical)) {
            ui.log(svanipp::console::Style::Fail, "Rejected unsafe path: " + relPath);
            closesocket(clientSock);
            failedFiles++;
            continue;
        }

        // Create parent directories
        fs::create_directories(saveAs.parent_path(), ec);
        
        // Check for overwrite: prompt if file exists and overwrite not flagged
        if (fs::exists(saveAs)) {
            if (!overwrite) {
                ui.progress_end(true);
                cout << "File exists: " << saveAs.filename().u8string() << ". Overwrite? (y/n): ";
                cout.flush();
                string answer;
                getline(cin, answer);
                if (answer.empty() || (answer[0] != 'y' && answer[0] != 'Y')) {
                    // User said no, auto-rename
                    saveAs = uniquePath(saveAs);
                }
                // If user said yes, proceed with overwrite
            }
            // If overwrite flag is set, just proceed with overwrite without prompt
        }
        ofstream out(saveAs, ios::binary);
        if (!out) {
            ui.log(svanipp::console::Style::Fail, "Cannot open output file: " + saveAs.string());
            closesocket(clientSock);
            failedFiles++;
            continue;
        }

        // Stream file bytes
        const size_t BUF = 64 * 1024;
        vector<char> buf(BUF);

        svanipp::crypto::Sha256 hasher;
        uint64_t remaining = fileSize;
        uint64_t received = 0;

        using clock = chrono::steady_clock;
        auto start_time = clock::now();

        while (remaining > 0) {
            const size_t want = (remaining > BUF) ? BUF : static_cast<size_t>(remaining);
            int r = ::recv(clientSock, buf.data(), static_cast<int>(want), 0);
            if (r <= 0) {
                ui.progress_end(true);
                ui.log(svanipp::console::Style::Fail, "Connection lost while receiving: " + relPath);
                break;
            }
            hasher.update(buf.data(), static_cast<size_t>(r));
            out.write(buf.data(), r);
            remaining -= static_cast<uint64_t>(r);
            received += static_cast<uint64_t>(r);

            // Progress: truncate relPath, erase using previous length, update every 150ms or percent change
            if (fileSize > 0) {
                int pct = static_cast<int>((received * 100ULL) / fileSize);
                auto now = clock::now();
                double elapsed = chrono::duration<double>(now - start_time).count();
                double mbps = (elapsed > 0.0) ? (received / (1024.0 * 1024.0)) / elapsed : 0.0;
                double bps = (elapsed > 0.0) ? (static_cast<double>(received) / elapsed) : 0.0;
                int eta = (bps > 0.0) ? static_cast<int>((fileSize - received) / bps) : -1;
                string line = ui.make_status_line("Recv", relPath, pct, mbps, eta);
                ui.progress_update(line, pct);
            }
        }
        ui.progress_end(true);

        uint8_t gotDigest[32];
        bool success = false;
        if (received == fileSize) {
            if (!recvExact(clientSock, gotDigest, sizeof(gotDigest))) {
                ui.log(svanipp::console::Style::Fail, "Failed to read SHA-256 digest: " + relPath);
                closesocket(clientSock);
                failedFiles++;
                continue;
            }

            uint8_t calcDigest[32];
            hasher.final(calcDigest);

            if (memcmp(gotDigest, calcDigest, 32) != 0) {
                ui.log(svanipp::console::Style::Fail, "SHA-256 mismatch: " + relPath);
                closesocket(clientSock);
                failedFiles++;
                continue;
            }
            success = true;
        }

        if (success) {
            double mb = received / (1024.0 * 1024.0);
            double elapsed = chrono::duration<double>(clock::now() - start_time).count();
            ostringstream okMsg;
            okMsg << "Saved " << saveAs.u8string() << " (" << fixed << setprecision(2) << mb
                  << " MB, " << fixed << setprecision(2) << elapsed << " s)";
            ui.log(svanipp::console::Style::Ok, okMsg.str());
            okFiles++;
            okBytes += received;
        } else if (attempted) {
            ui.log(svanipp::console::Style::Fail, "Transfer failed: " + relPath);
            failedFiles++;
        }

        if (summary) {
            double okMb = okBytes / (1024.0 * 1024.0);
            ostringstream summaryMsg;
            summaryMsg << "Session: OK " << okFiles << " files (" << fixed << setprecision(2)
                       << okMb << " MB), failed " << failedFiles;
            ui.log(svanipp::console::Style::Info, summaryMsg.str());
        }

        closesocket(clientSock);
        // loop back and accept next client; listenSock remains open
    }

    return 0;
}