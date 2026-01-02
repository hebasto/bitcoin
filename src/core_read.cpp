/*
The full include-list for /ci_container_base/src/core_read.cpp:
#include <primitives/block.h>  // for CBlockHeader (ptr only)
#include <streams.h>           // for DataStream
*/

#include <primitives/block.h> // IWYU pragma: keep

#include <cstring>
#include <ios>
#include <optional>
#include <span>
#include <string>
#include <vector>


template <typename Stream>
void Unserialize(Stream& s, uint32_t& a)
{
    s.read(std::as_writable_bytes(std::span{&a, 1}));
}


class DataStream
{
public:
    void read(std::span<std::uint32_t> dst)
    {
        const std::size_t bytes = dst.size_bytes();
        std::memcpy(dst.data(), m_data.data() + m_pos, bytes);
        m_pos += bytes;
    }

    template <typename T>
    DataStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }

private:
    std::vector<std::byte> m_data;
    std::size_t m_pos;

};


void f(DataStream& ds, A& a)
{
    ds >> a;
}
