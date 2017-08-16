// Written by Mark Waterman, and placed in the public domain.
// The author hereby disclaims copyright to this source code.

#pragma once

#include <cstdint>
#include <boost/uuid/uuid.hpp>
#include "MurmurHash3.h"

// Functor for hashing a boost::uuid. Type parameter is
// the seed for MurmurHash3_x86_32.
template<uint32_t Seed>
struct hash_uuid
{
    size_t operator()(const boost::uuids::uuid& uuid) const
    {
        uint32_t out;
        MurmurHash3_x86_32(&uuid, 16, Seed, &out);
        return out;
    }
};