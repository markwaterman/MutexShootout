// Written by Mark Waterman, and placed in the public domain.
// The author hereby disclaims copyright to this source code.

#pragma once

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap/list_of.hpp>
#include <optional>
#include <cassert>
#include <memory>
#include <mutex>
#include <type_traits>

namespace shootout {

// Thread-safe LRU cache implementation.
template<
    typename Key, 
    typename Val,
    typename Mutex,
    typename Hash = std::hash<Key>,
    typename Equals = std::equal_to<Key>
   >
class LruCache
{
static_assert(!std::is_pointer<Val>::value, "LruCache of raw pointers not allowed (ownership of values is shared between LruCache and callers). Use a shared_ptr or value semantics instead.");

public:

    using Container = boost::bimaps::bimap<
        boost::bimaps::unordered_set_of<Key, Hash, Equals>,
        boost::bimaps::list_of<Val>
    >;

    using Lock = std::lock_guard<Mutex>;

    LruCache(size_t capacity) :
        capacity_(capacity),
        container_(),
        mutex_()
    { 
        assert(capacity > 0);
    }

    LruCache(const LruCache&) = delete;
    LruCache& operator=(const LruCache&) = delete;

    // Creates/updates the value associated with the given key and
    // makes it the most recently accessed.
    void set(const Key& key, const Val& val)
    {
        Lock l(mutex_);

        auto left_iter = container_.left.find(key);

        if (left_iter == container_.left.end())
        {
            // Adding a new item to the front of the container.
            // If there isn't room, remove an item from the end.
            if (container_.size() == capacity_)
            {
                auto end_iter = container_.right.erase(--container_.right.end());

                // erase() returned an iterator pointing to the element immediately following 
                // the one that was deleted, or end() if no such element exists. Debugging sanity check:
                assert(end_iter == container_.right.end());
            }

            // Add the new item to front of the container.
            container_.right.push_front({ val, key });
        }
        else
        {
            // Item with this key exists. Update with new value and move to front of container.
            left_iter->second = val;
            container_.right.relocate(container_.right.begin(), container_.project_right(left_iter));
        }
    }

    // Gets a boost::optional<V> associated with the given key and
    // makes it the most recently accessed if it exists.
    std::optional<Val> get(const Key& key)
    {
        Lock l(mutex_);

        auto left_iter = container_.left.find(key);

        if (left_iter != container_.left.end())
        {
            // Item exists. Move to front of container and return the value.
            container_.right.relocate(container_.right.begin(), container_.project_right(left_iter));
            return left_iter->second;
        }
        else
        {
            // Not found.
	  return std::nullopt;
        }
    }

    // Removes the value associated with the provided key. Returns
    // true if removed successfully, false if the element didn't exist.
    bool erase(const Key& key)
    {
        Lock l(mutex_);

        size_t elements_erased = container_.left.erase(key);
        assert(elements_erased < 2);

        return (elements_erased != 0);
    }

    // Clears all items from the LruCache.
    void clear()
    {
        Lock l(mutex_);
        container_.clear();
    }

    
private:
    size_t capacity_;
    Container container_;
    Mutex mutex_;
};
}
