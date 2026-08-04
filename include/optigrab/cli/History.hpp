#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace optigrab {

// In-memory command history for REPL recall (up/down).
// Navigation cursor sits one past the end when editing a fresh line.
class History {
public:
    explicit History(std::size_t maxEntries = 1000);

    // Append a submitted line. Skips empty and consecutive duplicates.
    void add(std::string line);

    // Move to an older entry. Pass the current edit buffer so the first
    // "up" can restore it later via newer().
    // Returns nullopt if there is no older entry.
    [[nodiscard]] std::optional<std::string> older(const std::string& currentLine);

    // Move toward newer entries / the saved draft.
    // Returns nullopt only when already on the draft (or empty history).
    // When leaving history into the draft, returns the draft (possibly empty).
    [[nodiscard]] std::optional<std::string> newer();

    void clear();
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool isNavigating() const;

private:
    std::vector<std::string> entries_;
    std::size_t maxEntries_;
    std::size_t index_{0};  // entries_.size() => editing draft
    std::string draft_;
    bool navigating_{false};
};

}  // namespace optigrab
