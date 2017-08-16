// Written by Mark Waterman, and placed in the public domain.
// The author hereby disclaims copyright to this source code.

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <thread>
#include "stopwatch.hpp"
#include <random>
#include <cctype>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "hashing.hpp"
#include "lru_cache.hpp"

#include <mutex>
#include <boost/thread/mutex.hpp>
#include <boost/thread/null_mutex.hpp>
#include "alt_mutex.hpp"

const size_t OBJ_COUNT = 10000;
const size_t LRU_SIZE = 20000;
const size_t OP_COUNT = 50000000;
const uint32_t HASH_SEED = 1146518783;

// Aliases
using Guid = boost::uuids::uuid;
using HashGuid = hash_uuid<HASH_SEED>;
using Payload = std::array<char, 2048>;
template<typename Mutex> using Lru = shootout::LruCache<Guid, std::shared_ptr<const Payload>, Mutex, HashGuid>;

// Forward declarations
template<typename Mutex> double run_benchmark(const std::vector<Guid>& keys, size_t thread_count, size_t total_op_count, size_t lru_size);
template<typename Mutex> void populate_cache(Lru<Mutex>& lru_cache, const std::vector<Guid> keys);
template<typename Mutex> void do_gets(const std::vector<Guid>& keys, Lru<Mutex>& lru_cache, size_t op_count);
std::vector<Guid> create_keys(size_t count);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
#if _WIN32
        std::cerr << "Usage: " << argv[0] << " threadCount nolocks|std|boost|cs0|cs4k|srw\n";
#else
        std::cerr << "Usage: " << argv[0] << " threadCount nolocks|std|boost\n";
#endif
        return 1;
    }
    char *end;
    size_t thread_count = std::strtoul(argv[1], &end, 10);
    if (thread_count == 0 || thread_count > 1000)
    {
        std::cerr << "Invalid thread count.\n";
        return 1;
    }

    std::string mutex_arg{argv[2]};
    // make 2nd arg lowercase:
    std::transform(mutex_arg.begin(), mutex_arg.end(), mutex_arg.begin(), ::tolower);
    
    // create population of guid keys:
    auto keys = ::create_keys(OBJ_COUNT);
    double elapsed_secs;

    
    if (mutex_arg == "std")
    {
        elapsed_secs = run_benchmark<std::mutex>(keys, thread_count, OP_COUNT, LRU_SIZE);
        std::cout << "std::mutex:       " << elapsed_secs << std::endl;
    }
    else if (mutex_arg == "boost")
    {
        elapsed_secs = run_benchmark<boost::mutex>(keys, thread_count, OP_COUNT, LRU_SIZE);
        std::cout << "boost::mutex:     " << elapsed_secs << std::endl;
    }
#if _WIN32
    else if (mutex_arg == "cs0")
    {
        elapsed_secs = run_benchmark<shootout::cs_mutex<0>>(keys, thread_count, OP_COUNT, LRU_SIZE);
        std::cout << "cs_mutex_nospin:  " << elapsed_secs << std::endl;
    }
    else if (mutex_arg == "cs4k")
    {
        elapsed_secs = run_benchmark<shootout::cs_mutex<4000>>(keys, thread_count, OP_COUNT, LRU_SIZE);
        std::cout << "cs_mutex_4K:      " << elapsed_secs << std::endl;
    }
    else if (mutex_arg == "srw")
    {
        elapsed_secs = run_benchmark<shootout::srw_mutex>(keys, thread_count, OP_COUNT, LRU_SIZE);
        std::cout << "srw_mutex:        " << elapsed_secs << std::endl;
    }
#endif
    else if (mutex_arg == "nolocks")
    {
        if (thread_count == 1)
        {
            elapsed_secs = run_benchmark<boost::null_mutex>(keys, thread_count, OP_COUNT, LRU_SIZE);
            std::cout << "No locking:  " << elapsed_secs << std::endl;
        }
        else
        {
            std::cerr << "Only one thread allowed for non-locking.\n";
            return 1;
        }
    }
    else
    {
        std::cerr << "Unknown mutex type.\n";
        return 1;
    }

    return 0;

}


template <typename Mutex>
double run_benchmark(const std::vector<Guid>& keys, size_t thread_count, size_t total_op_count, size_t lru_size)
{
    Lru<Mutex> lru_cache(lru_size);
    populate_cache(lru_cache, keys);
    
    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    auto sw = mw::StopWatch::start_new();

    for (size_t i = 0; i < thread_count; i++)
        threads.emplace_back(do_gets<Mutex>, std::cref(keys), std::ref(lru_cache), (total_op_count / thread_count));

    // Wait for all threads to complete
    for (auto& thread : threads)
        thread.join();

    return sw.elapsed_seconds();
}


template <typename Mutex>
void do_gets(const std::vector<Guid>& keys, Lru<Mutex>& lru_cache, size_t op_count)
{
    for (size_t i = 0; i < op_count; i++)
    {
        auto result = lru_cache.get(keys[i % keys.size()]);
        assert(result);
    }
}


std::vector<Guid> create_keys(size_t count)
{
    // VS code analysis doesn't like the boost::mt19937 
    // implementation that random_generator uses by default.
    // Using std implementation to generate GUIDs instead...
    //using GuidGen = boost::uuids::random_generator;
    using GuidGen = boost::uuids::basic_random_generator<std::mt19937>;

    std::vector<Guid> keys;
    keys.reserve(count);

    std::random_device seed;
    std::mt19937 rand(seed());
    GuidGen guid_gen(&rand);

    for (size_t i = 0; i < count; i++)
    {
        keys.push_back(guid_gen());
    }

    return keys;
}


template <typename Mutex>
void populate_cache(Lru<Mutex>& lru_cache, const std::vector<Guid> keys)
{
    for (const auto key : keys)
    {
        auto payload_ptr = std::make_shared<Payload>();
        payload_ptr->fill('x');
        lru_cache.set(key, payload_ptr);
    }
}
