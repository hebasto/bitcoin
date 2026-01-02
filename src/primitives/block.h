#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <serialize.h>

struct CBlockHeader
{
    int32_t n;

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        auto ser_action = ActionUnserialize{};
        ser_action.SerReadWriteMany(s, n);
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
