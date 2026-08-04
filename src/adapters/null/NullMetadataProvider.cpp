#include "optigrab/adapters/null/NullMetadataProvider.hpp"

// Header-only behavior; translation unit keeps the linker happy if needed.
namespace optigrab {
namespace {
[[maybe_unused]] const NullMetadataProvider kNullMetadataProviderAnchor;
}
}  // namespace optigrab
