#include <key.h>
#include <util/strencodings.h>

#include <ranges>
#include <stack>

std::array<std::byte, 32> key_data{std::byte{1}};

static constexpr uint8_t KEY_TYPES_COUNT{6};

static bool IdIsUnCompPubKey(uint8_t idx) { return idx % KEY_TYPES_COUNT == 1; }

static void foo()
{
    CKey privkey;
    privkey.Set(key_data.begin(), key_data.end(), !IdIsUnCompPubKey(i));
}
