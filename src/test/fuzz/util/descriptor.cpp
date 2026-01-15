#include <key.h>

#include <array>
#include <cstddef>

[[maybe_unused]] static void foo()
{
    std::array<std::byte, 32> key_data{std::byte{1}};
    CKey privkey;
    privkey.Set(key_data.begin(), key_data.end(), true);
}
