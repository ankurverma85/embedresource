#pragma once
#include <cassert>
#include <cstddef>
#include <optional>
#include <string_view>
#include <type_traits>
#include <span.h>
#include <functional>
#define COLLECTION_METHOD_PREFIX GetCollectionInfo
#define STR(s) #s
#define COLLECTION_FUNC(COLLECTION) GetCollectionInfo##COLLECTION

namespace EmbeddedResource
{

/// Resource Name and byte-range for embedded resources in a binary
class Resource
{
    public:
    typedef Resource GetFunc() noexcept;

    const std::wstring_view&          name() const { return _name; }
    const std::span<const std::byte>& data() const { return _data; }
    template <typename T> auto        data() const
    {
        auto const   ptr  = reinterpret_cast<const T*>(_data.data());
        size_t const size = _data.size() / sizeof(T);
        assert(_data.size() % sizeof(T) == 0);
        return std::span<const T>{ptr, ptr + size};
    }

    std::string_view  string() const { return std::string_view(data<char>().data(), data<char>().size()); }
    std::wstring_view wstring() const { return std::wstring_view(data<wchar_t>().data(), data<wchar_t>().size()); }

    Resource(const std::wstring_view name, const std::span<const std::byte> data) : _name(name), _data(data) {}

    private:
    std::wstring_view          _name;
    std::span<const std::byte> _data;
};

/// Named collections of resources embedded into the binary
class Collection
{
    public:
    typedef Collection GetFunc();

    static std::string GetLoadCollectionMethodName(std::string_view name)
    {
        return std::string(STR(COLLECTION_METHOD_PREFIX)) + std::string(name);
    }

    class Iterator
    {
        public:
        Iterator() = default;

        Iterator(std::reference_wrapper<const Collection> collection) : _collection(collection) {}

        Iterator(const Iterator&) = delete;
        Iterator(Iterator&&)      = delete;
        Iterator& operator=(const Iterator&) = delete;
        Iterator& operator=(Iterator&&) = delete;

        Resource  operator*() { return _collection->get().GetAt(_index); }
        Iterator& operator++()
        {
            _index++;
            if (_index >= _collection->get().Size())
            {
                _index = 0;
                _collection.reset();
            }

            return *this;
        }

        bool operator==(const Iterator& r) const
        {
            return _index == r._index && (_collection.has_value() == r._collection.has_value())
                   && (!_collection.has_value() || (&_collection.value().get() == &r._collection.value().get()));
        }

        bool operator!=(const Iterator& r) const { return !(*this == r); }

        private:
        size_t _index{0};

        std::optional<std::reference_wrapper<const Collection>> _collection{};
    };

    Resource GetResourceByName(const std::wstring_view name)
    {
        for (auto const& res : (*this))
        {
            if (res.name() == name)
            {
                return res;
            }
        }
        throw std::logic_error("Resource Not found");
    }

    Collection(std::span<Resource::GetFunc* const> fnTable) : _fnTable(fnTable) {}

    size_t   Size() const { return _fnTable.size(); }
    Resource GetAt(const size_t index) const { return (*_fnTable[index])(); }

    Iterator begin() const { return Iterator(*this); }
    Iterator end() const { return Iterator(); }

    private:
    std::span<Resource::GetFunc* const> _fnTable;
};
}    // namespace EmbeddedResource

#define DECLARE_RESOURCE_COLLECTION(name) EmbeddedResource::Collection COLLECTION_FUNC(name)()

#define LOAD_RESOURCE_COLLECTION(name) COLLECTION_FUNC(name)()

#if defined(_MSC_VER)    // compiling with VisualStudio
#define EXPORTED_API __declspec(dllexport)
#elif defined(__GNUC__)    // compiling with GCC
#define EXPORTED_API __attribute__((visibility("default")))
#else
#error "Unknown Compiler. Dont know how to export symbol"
#endif

#define EXPORT_RESOURCE_COLLECTION(symname, name) \
    DECLARE_RESOURCE_COLLECTION(name);            \
    extern "C" EXPORTED_API auto symname() { return COLLECTION_FUNC(name); }
