#include "transfer/receiver.h"
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

int svanipp::run_receiver(uint16_t port, const string& outDir, bool overwrite) {
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

    sockaddr_in client{};
    int clientLen = sizeof(client);
    socket_t clientSock = ::accept(listenSock, reinterpret_cast<sockaddr*>(&client), &clientLen);
    if (clientSock == INVALID_SOCKET) {
        cerr << "accept() failed\n";
        closesocket(listenSock);
        return 1;
    }

    // Read fixed header
    svanipp::proto::HeaderFixed hf{};
    if (!recvExact(clientSock, &hf, sizeof(hf))) {
        cerr << "Failed to read header\n";
        closesocket(clientSock);
        closesocket(listenSock);
        return 1;
    }

    // Validate magic
    if (memcmp(hf.magic, svanipp::proto::MAGIC, 8) != 0) {
        cerr << "Bad MAGIC (not a svanipp stream)\n";
        closesocket(clientSock);
        closesocket(listenSock);
        return 1;
    }

    const uint16_t version = ntoh_u16(hf.version);
    const uint16_t nameLen = ntoh_u16(hf.filename_len);
    const uint64_t fileSize = ntoh_u64(hf.file_size);

    if (version != svanipp::proto::VERSION) {
        cerr << "Unsupported version: " << version << "\n";
        closesocket(clientSock);
        closesocket(listenSock);
        return 1;
    }
    if (nameLen == 0 || nameLen > 4096) {
        cerr << "Invalid filename length\n";
        closesocket(clientSock);
        closesocket(listenSock);
        return 1;
    }

    // Read filename
    vector<char> nameBuf(nameLen);
    if (!recvExact(clientSock, nameBuf.data(), nameBuf.size())) {
        cerr << "Failed to read filename\n";
        closesocket(clientSock);
        closesocket(listenSock);
        return 1;
    }
    string filename(nameBuf.begin(), nameBuf.end());
    filename = sanitizeFilename(filename);

    fs::path saveAs = outPath / filename;
    if (!overwrite) {
        saveAs = uniquePath(saveAs);
    }

    ofstream out(saveAs, ios::binary);
    if (!out) {
        cerr << "Cannot open output file: " << saveAs.string() << "\n";
        closesocket(clientSock);
        closesocket(listenSock);
        return 1;
    }

    // Stream file bytes
    const size_t BUF = 64 * 1024;
    vector<char> buf(BUF);

    svanipp::crypto::Sha256 hasher;
    uint64_t remaining = fileSize;
    uint64_t received = 0;

    while (remaining > 0) {
        const size_t want = (remaining > BUF) ? BUF : static_cast<size_t>(remaining);
        int r = ::recv(clientSock, buf.data(), static_cast<int>(want), 0);
        if (r <= 0) {
            cerr << "\nConnection lost while receiving\n";
            break;
        }
        hasher.update(buf.data(), static_cast<size_t>(r));
        out.write(buf.data(), r);
        remaining -= static_cast<uint64_t>(r);
        received += static_cast<uint64_t>(r);

        // progress
        if (fileSize > 0) {
            int pct = static_cast<int>((received * 100ULL) / fileSize);
            cout << "\rReceiving " << filename << " ... " << pct << "%";
            cout.flush();
        }
    }

    uint8_t gotDigest[32];
    if (received == fileSize) {
        if (!recvExact(clientSock, gotDigest, sizeof(gotDigest))) {
            cerr << "Failed to read SHA-256 digest\n";
            closesocket(clientSock);
            closesocket(listenSock);
            return 2;
        }

        uint8_t calcDigest[32];
        hasher.final(calcDigest);

        if (memcmp(gotDigest, calcDigest, 32) != 0) {
            cerr << "SHA-256 MISMATCH (file integrity failed)\n";
            closesocket(clientSock);
            closesocket(listenSock);
            return 3;
        } else {
            cout << "SHA-256 OK\n";
        }
    }

    cout << "\nSaved: " << saveAs.string()
              << " (" << received << " bytes)\n";

    closesocket(clientSock);
    closesocket(listenSock);
    return (received == fileSize) ? 0 : 2;
}