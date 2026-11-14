// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_HEAP_H_
#define SOMA_HEAP_H_

#include <cstdint>

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/portable.h"

#include "soma/array.h"

namespace SOMA
{
    class Heap
    {
    public:
        struct Statistics_t
        {
            bool enabled;
            size_t capacity_B;
            size_t free_B;
            size_t minimum_ever_free_B;
        };

        template<size_t N>
        static void initialize( Array<const Array<uint8_t>> regions )
        {
            ASSERT( regions.size() == N );

            HeapRegion_t descriptors[N + 1u] {};

            initialize( regions, Array<HeapRegion_t> { descriptors } );
        }

        template<typename T, size_t N,
                 typename = std::enable_if_t<( N != dynamic_extent ) && std::is_same_v<std::remove_const_t<T>, Array<uint8_t>>>>
        static void initialize( const Array<T, N>& regions )
        {
            initialize<N>( Array<const Array<uint8_t>> { regions } );
        }

        template<size_t N>
        static void initialize( const Array<uint8_t> ( &regions )[N] )
        {
            initialize<N>( Array<const Array<uint8_t>> { regions } );
        }

        static Statistics_t statistics( void );

    private:
        static void initialize( Array<const Array<uint8_t>> regions,
                                Array<HeapRegion_t> descriptors );
    };
} // namespace SOMA

#endif // SOMA_HEAP_H_
