#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <cstdint>
#include <span>

struct A
{
    uint32_t n;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s.read(std::as_writable_bytes(std::span{&n, 1}));
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
