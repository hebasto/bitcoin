/*
The full include-list for /ci_container_base/src/core_read.cpp:
#include <primitives/block.h>  // for CBlockHeader (ptr only)
#include <streams.h>           // for DataStream
*/

#include <primitives/block.h> // IWYU pragma: keep
#include <streams.h>

void f(DataStream& ser_header, A& a)
{
    ser_header >> a;
}
