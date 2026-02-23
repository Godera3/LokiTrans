#pragma once

#include <string>
#include <vector>
#include <chrono>

using namespace std;

namespace svanipp::console {

enum class Style {
    Info,
    Ok,
    Warn,
    Fail
};

class ConsoleUI {
public:
    static ConsoleUI& instance();

    void init(bool noColor);
    void set_throttle_ms(int ms);

    void log(Style style, const string& message);
    void print_block(const vector<string>& lines);

    void progress_update(const string& line, int pct);
    void progress_end(bool newline = true);

    string make_status_line(const string& dir,
                                 const string& path,
                                 int pct,
                                 double mbps,
                                 int etaSec) const;

private:
    ConsoleUI() = default;

    string truncate_middle(const string& s, size_t maxLen) const;
    string style_prefix(Style style) const;
    string colorize(Style style, const string& text) const;

    bool should_update(int pct);
    void clear_progress(bool newline);

    bool useColor_ = false;
    bool useIcons_ = false;
    bool progressActive_ = false;
    size_t lastLen_ = 0;
    int lastPct_ = -1;
    int throttleMs_ = 250;
    chrono::steady_clock::time_point lastUpdate_{};
};

} // namespace svanipp::console
