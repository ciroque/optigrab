#include "optigrab/domain/TrackRange.hpp"

#include "optigrab/domain/Errors.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

namespace optigrab {
namespace {

std::string trim(std::string s) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

void addTrack(std::set<int>& out, int n, int maxTrack) {
    if (n < 1 || n > maxTrack) {
        throw ParseError("Track number out of range: " + std::to_string(n) +
                         " (valid 1-" + std::to_string(maxTrack) + ")");
    }
    out.insert(n);
}

void parsePiece(const std::string& piece, int maxTrack, std::set<int>& out) {
    const auto p = trim(piece);
    if (p.empty()) {
        throw ParseError("Empty track specifier");
    }

    const auto dash = p.find('-');
    if (dash == std::string::npos) {
        addTrack(out, std::stoi(p), maxTrack);
        return;
    }

    if (dash == 0 || dash + 1 >= p.size()) {
        throw ParseError("Invalid range: " + p);
    }

    const int a = std::stoi(p.substr(0, dash));
    const int b = std::stoi(p.substr(dash + 1));
    if (a > b) {
        throw ParseError("Invalid range (start > end): " + p);
    }
    for (int i = a; i <= b; ++i) {
        addTrack(out, i, maxTrack);
    }
}

}  // namespace

std::vector<int> parseTrackRange(const std::string& spec, int maxTrack) {
    if (maxTrack < 1) {
        throw ParseError("No audio tracks available");
    }

    const auto s = trim(spec);
    if (s.empty()) {
        throw ParseError("Missing track specifier (e.g. all, 1, 1-3, 1,3,5)");
    }

    if (s == "all" || s == "*") {
        std::vector<int> all(static_cast<std::size_t>(maxTrack));
        for (int i = 0; i < maxTrack; ++i) {
            all[static_cast<std::size_t>(i)] = i + 1;
        }
        return all;
    }

    std::set<int> ordered;
    std::stringstream ss(s);
    std::string piece;
    while (std::getline(ss, piece, ',')) {
        try {
            parsePiece(piece, maxTrack, ordered);
        } catch (const std::invalid_argument&) {
            throw ParseError("Invalid track specifier: " + piece);
        } catch (const std::out_of_range&) {
            throw ParseError("Track number out of range in: " + piece);
        }
    }

    if (ordered.empty()) {
        throw ParseError("No tracks selected");
    }

    return std::vector<int>(ordered.begin(), ordered.end());
}

}  // namespace optigrab
