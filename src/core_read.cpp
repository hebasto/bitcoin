/*
The full include-list for /ci_container_base/src/core_read.cpp:
#include <primitives/block.h>  // for CBlockHeader (ptr only)
#include <streams.h>           // for DataStream
*/

#include <primitives/block.h>

template <class T, class Stream>
concept Unserializable = requires(T a, Stream s) { a.Unserialize(s); };

template <typename Stream, typename T>
    requires Unserializable<T, Stream>
void Unserialize(Stream& is, T&& a)
{
    a.Unserialize(is);
}

#include <streams.h>

void f(DataStream& ds, A& a)
{
    ds >> a;
}
