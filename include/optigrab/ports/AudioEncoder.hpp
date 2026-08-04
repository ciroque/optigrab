#pragma once

#include "optigrab/domain/Types.hpp"

#include <filesystem>
#include <functional>
#include <string>

namespace optigrab {

class AudioEncoder {
public:
    using ProgressFn = std::function<void(const std::string& message)>;

    virtual ~AudioEncoder() = default;

    virtual void encode(const std::filesystem::path& inputWav,
                        const std::filesystem::path& outputMp3,
                        const Tags& tags,
                        QualityPreset quality,
                        ProgressFn progress = {}) = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace optigrab
