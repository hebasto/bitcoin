#include <array>
#include <cstddef>
#include <memory>

template <typename T>
void Set(const T pbegin, const T pend, bool fCompressedIn)
{
    std::unique_ptr<std::array<unsigned char, 32>> keydata;

    if (size_t(pend - pbegin) != std::tuple_size_v<KeyType>) {
        ClearKeyData();
    } else if (Check(UCharCast(&pbegin[0]))) {
        MakeKeyData();
        memcpy(keydata->data(), (unsigned char*)&pbegin[0], keydata->size());
        fCompressed = fCompressedIn;
    } else {
        ClearKeyData();
    }
}

[[maybe_unused]] static void foo()
{


    std::array<std::byte, 32> key_data{std::byte{1}};
    // CKey privkey;
    // privkey.Set(key_data.begin(), key_data.end(), true);
    Set(key_data.begin(), key_data.end(), true);
}
