#include <array>
#include <cstddef>
#include <memory>

template <typename T>
void Set(const T pbegin, const T pend, bool fCompressedIn)
{
    std::unique_ptr<std::array<unsigned char, 32>> keydata;
    memcpy(keydata->data(), (unsigned char*)&pbegin[0], keydata->size());
}

[[maybe_unused]] static void foo()
{
    std::array<std::byte, 32> key_data{std::byte{1}};
    Set(key_data.begin(), key_data.end(), true);
}
