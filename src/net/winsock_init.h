#pragma once

class WinsockInit {
public:
    WinsockInit();
    ~WinsockInit();

    WinsockInit(const WinsockInit&) = delete;
    WinsockInit& operator=(const WinsockInit&) = delete;

    bool ok() const { return ok_; }

private:
    bool ok_{false};
};