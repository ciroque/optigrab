#include "optigrab/cli/History.hpp"

namespace optigrab {

History::History(std::size_t maxEntries) : maxEntries_(maxEntries == 0 ? 1 : maxEntries) {}

void History::add(std::string line) {
    if (line.empty()) {
        navigating_ = false;
        draft_.clear();
        index_ = entries_.size();
        return;
    }
    if (!entries_.empty() && entries_.back() == line) {
        navigating_ = false;
        draft_.clear();
        index_ = entries_.size();
        return;
    }
    entries_.push_back(std::move(line));
    if (entries_.size() > maxEntries_) {
        entries_.erase(entries_.begin(),
                       entries_.begin() + static_cast<std::ptrdiff_t>(entries_.size() - maxEntries_));
    }
    navigating_ = false;
    draft_.clear();
    index_ = entries_.size();
}

std::optional<std::string> History::older(const std::string& currentLine) {
    if (entries_.empty()) {
        return std::nullopt;
    }
    if (!navigating_) {
        draft_ = currentLine;
        navigating_ = true;
        index_ = entries_.size();
    }
    if (index_ == 0) {
        return entries_[0];
    }
    --index_;
    return entries_[index_];
}

std::optional<std::string> History::newer() {
    if (!navigating_ || entries_.empty()) {
        return std::nullopt;
    }
    if (index_ + 1 >= entries_.size()) {
        index_ = entries_.size();
        navigating_ = false;
        return draft_;
    }
    ++index_;
    return entries_[index_];
}

void History::clear() {
    entries_.clear();
    draft_.clear();
    index_ = 0;
    navigating_ = false;
}

std::size_t History::size() const { return entries_.size(); }

bool History::empty() const { return entries_.empty(); }

bool History::isNavigating() const { return navigating_; }

}  // namespace optigrab
