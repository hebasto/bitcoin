/*
The full include-list for /ci_container_base/src/core_read.cpp:
#include <primitives/block.h>  // for CBlockHeader (ptr only)
#include <streams.h>           // for DataStream
*/

#include <primitives/block.h> // IWYU pragma: keep

#include <ios>
#include <optional>
#include <span>
#include <string>
#include <vector>

class DataStream
{
protected:
    using vector_type = std::vector<std::byte>;
    vector_type vch;
    vector_type::size_type m_read_pos{0};

public:
    typedef vector_type::allocator_type   allocator_type;
    typedef vector_type::size_type        size_type;
    typedef vector_type::difference_type  difference_type;
    typedef vector_type::reference        reference;
    typedef vector_type::const_reference  const_reference;
    typedef vector_type::value_type       value_type;
    typedef vector_type::iterator         iterator;
    typedef vector_type::const_iterator   const_iterator;
    typedef vector_type::reverse_iterator reverse_iterator;

    explicit DataStream() = default;
    explicit DataStream(std::span<const uint8_t> sp) : DataStream{std::as_bytes(sp)} {}
    explicit DataStream(std::span<const value_type> sp) : vch(sp.data(), sp.data() + sp.size()) {}

    std::string str() const
    {
        return std::string{data(), data() + size()};
    }


    //
    // Vector subset
    //
    const_iterator begin() const                     { return vch.begin() + m_read_pos; }
    iterator begin()                                 { return vch.begin() + m_read_pos; }
    const_iterator end() const                       { return vch.end(); }
    iterator end()                                   { return vch.end(); }
    size_type size() const                           { return vch.size() - m_read_pos; }
    bool empty() const                               { return vch.size() == m_read_pos; }
    void resize(size_type n, value_type c = value_type{}) { vch.resize(n + m_read_pos, c); }
    void reserve(size_type n)                        { vch.reserve(n + m_read_pos); }
    const_reference operator[](size_type pos) const  { return vch[pos + m_read_pos]; }
    reference operator[](size_type pos)              { return vch[pos + m_read_pos]; }
    void clear()                                     { vch.clear(); m_read_pos = 0; }
    value_type* data()                               { return vch.data() + m_read_pos; }
    const value_type* data() const                   { return vch.data() + m_read_pos; }

    inline void Compact()
    {
        vch.erase(vch.begin(), vch.begin() + m_read_pos);
        m_read_pos = 0;
    }

    bool Rewind(std::optional<size_type> n = std::nullopt)
    {
        // Total rewind if no size is passed
        if (!n) {
            m_read_pos = 0;
            return true;
        }
        // Rewind by n characters if the buffer hasn't been compacted yet
        if (*n > m_read_pos)
            return false;
        m_read_pos -= *n;
        return true;
    }


    //
    // Stream subset
    //
    bool eof() const             { return size() == 0; }
    int in_avail() const         { return size(); }

    void read(std::span<value_type> dst)
    {
        if (dst.size() == 0) return;

        // Read from the beginning of the buffer
        auto next_read_pos{CheckedAdd(m_read_pos, dst.size())};
        if (!next_read_pos.has_value() || next_read_pos.value() > vch.size()) {
            throw std::ios_base::failure("DataStream::read(): end of data");
        }
        memcpy(dst.data(), &vch[m_read_pos], dst.size());
        if (next_read_pos.value() == vch.size()) {
            m_read_pos = 0;
            vch.clear();
            return;
        }
        m_read_pos = next_read_pos.value();
    }

    void ignore(size_t num_ignore)
    {
        // Ignore from the beginning of the buffer
        auto next_read_pos{CheckedAdd(m_read_pos, num_ignore)};
        if (!next_read_pos.has_value() || next_read_pos.value() > vch.size()) {
            throw std::ios_base::failure("DataStream::ignore(): end of data");
        }
        if (next_read_pos.value() == vch.size()) {
            m_read_pos = 0;
            vch.clear();
            return;
        }
        m_read_pos = next_read_pos.value();
    }

    void write(std::span<const value_type> src)
    {
        // Write to the end of the buffer
        vch.insert(vch.end(), src.begin(), src.end());
    }

    template<typename T>
    DataStream& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }

    template <typename T>
    DataStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }

    /** Compute total memory usage of this object (own memory + any dynamic memory). */
    size_t GetMemoryUsage() const noexcept;
};

void f(DataStream& ser_header, A& a)
{
    ser_header >> a;
}
