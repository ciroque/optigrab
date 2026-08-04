#include "optigrab/cli/Tokenizer.hpp"

#include "optigrab/domain/Errors.hpp"

#include <cctype>

namespace optigrab {

std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;

    for (std::size_t i = 0; i < input.size(); ++i) {
        const char c = input[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            continue;
        }
        if (!inQuotes && std::isspace(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(c);
    }

    if (inQuotes) {
        throw ParseError("Unbalanced double quotes");
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

}  // namespace optigrab
