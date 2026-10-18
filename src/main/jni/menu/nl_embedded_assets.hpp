#pragma once

#include <cstddef>

struct nl_embedded_asset
{
    const unsigned char* data;
    std::size_t size;
    const char* name;
};

// Embedded asset lookup. Only these names are known, in all cases byte-identical
// copies of oonalang/IMGUI5 app/src/main/assets/:
//   "SSTMedium.TTF", "SSTBold.TTF", "SpaceGrotesk-SemiBold.ttf",
//   "fa-solid-900.ttf", "avatar.rgba", "logo.rgba"
namespace nl_embedded
{
const nl_embedded_asset* Find(const char* name);
}

// nl_ui::SetAssetReader-compatible reader. Returns a pointer to static data that
// must never be freed by the caller; nl_ui is built with FontDataOwnedByAtlas=false.
void* nl_embedded_asset_reader(const char* name, std::size_t* out_size);
