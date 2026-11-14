// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#include <cstddef>
#include <cstdint>

#include "soma/config.h"

#ifdef SOMA_CONFIG_OVERRIDE_GLOBAL_NEW_DELETE
#include <cstdlib>
#include <new>
#endif

#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/portable.h"

#include "soma/assert.h"
#include "soma/runtime/heap.h"

namespace SOMA
{
    namespace
    {
        bool g_is_initialized = false;
        bool g_is_enabled = false;
        size_t g_capacity_B = 0u;
    }

    void Heap::initialize( Array<const Array<uint8_t>> regions,
                           Array<HeapRegion_t> descriptors )
    {
        ASSERT( g_is_initialized == false );
        ASSERT( descriptors.size() == regions.size() + 1u );

        if( regions.size() > 0u )
        {
            constexpr size_t c_block_link_size_B =
                ( ( sizeof( void* ) + sizeof( size_t ) + portBYTE_ALIGNMENT - 1u ) / portBYTE_ALIGNMENT ) * portBYTE_ALIGNMENT;
            constexpr size_t c_minimum_region_size_B = c_block_link_size_B + ( 2u * portBYTE_ALIGNMENT );

            for( size_t i = 0u; i < regions.size(); ++i )
            {
                const Array<uint8_t>& region = regions[i];
                const uintptr_t start = reinterpret_cast<uintptr_t>( region.storage() );

                ASSERT( region.storage() != nullptr );
                ASSERT( region.size_B() > c_minimum_region_size_B );
                ASSERT( region.size_B() <= ( UINTPTR_MAX - start ) );

                descriptors[i].pucStartAddress = region.storage();
                descriptors[i].xSizeInBytes = region.size_B();
            }

            // regions must already be in address order and must not overlap.
            for( size_t i = 1u; i < regions.size(); ++i )
            {
                const uintptr_t previous_start = reinterpret_cast<uintptr_t>( descriptors[i - 1u].pucStartAddress );
                const uintptr_t previous_end = previous_start + descriptors[i - 1u].xSizeInBytes;
                const uintptr_t current_start = reinterpret_cast<uintptr_t>( descriptors[i].pucStartAddress );

                ASSERT( current_start >= previous_end );
            }

            // The zero-initialized final descriptor terminates the heap_5 list.
            vPortDefineHeapRegions( descriptors.storage() );

            g_capacity_B = xPortGetFreeHeapSize();
            ASSERT( g_capacity_B > 0u );
            g_is_enabled = true;
        }

        g_is_initialized = true;
    }

    Heap::Statistics_t Heap::statistics( void )
    {
        ASSERT( g_is_initialized );

        Statistics_t statistics { g_is_enabled,
                                  g_capacity_B,
                                  0u,
                                  0u };

        if( g_is_enabled )
        {
            statistics.free_B = xPortGetFreeHeapSize();
            statistics.minimum_ever_free_B = xPortGetMinimumEverFreeHeapSize();
        }

        return statistics;
    }
} // namespace SOMA

#ifdef SOMA_CONFIG_OVERRIDE_GLOBAL_NEW_DELETE
namespace
{
    void* allocate_from_soma_heap( std::size_t size,
                                   std::size_t alignment ) noexcept
    {
        void* result = nullptr;

        ASSERT( SOMA::g_is_initialized && SOMA::g_is_enabled );

        if( size == 0u )
        {
            size = 1u;
        }

        if( alignment < alignof( std::max_align_t ) )
        {
            alignment = alignof( std::max_align_t );
        }

        // Keep the original FreeRTOS pointer just before the aligned C++ pointer.
        const std::size_t overhead = sizeof( void* ) + alignment - 1u;

        if( size <= SIZE_MAX - overhead )
        {
            void* const allocation = pvPortMalloc( size + overhead );

            if( allocation != nullptr )
            {
                const uintptr_t start = reinterpret_cast<uintptr_t>( allocation ) + sizeof( void* );
                const uintptr_t aligned = ( start + alignment - 1u ) & ~( static_cast<uintptr_t>( alignment ) - 1u );

                result = reinterpret_cast<void*>( aligned );
                reinterpret_cast<void**>( result )[-1] = allocation;
            }
        }

        ASSERT( result != nullptr );
        if( result == nullptr )
        {
            std::abort();
        }

        return result;
    }

    void deallocate_from_soma_heap( void* allocation ) noexcept
    {
        if( allocation != nullptr )
        {
            vPortFree( reinterpret_cast<void**>( allocation )[-1] );
        }
    }
}

void* operator new( std::size_t size )
{
    return allocate_from_soma_heap( size, alignof( std::max_align_t ) );
}

void* operator new[]( std::size_t size )
{
    return ::operator new( size );
}

void operator delete( void* allocation ) noexcept
{
    deallocate_from_soma_heap( allocation );
}

void operator delete[]( void* allocation ) noexcept
{
    deallocate_from_soma_heap( allocation );
}

void operator delete( void* allocation,
                      std::size_t ) noexcept
{
    deallocate_from_soma_heap( allocation );
}

void operator delete[]( void* allocation,
                        std::size_t ) noexcept
{
    deallocate_from_soma_heap( allocation );
}

void* operator new( std::size_t size,
                    std::align_val_t alignment )
{
    return allocate_from_soma_heap( size, static_cast<std::size_t>( alignment ) );
}

void* operator new[]( std::size_t size,
                      std::align_val_t alignment )
{
    return allocate_from_soma_heap( size, static_cast<std::size_t>( alignment ) );
}

void operator delete( void* allocation,
                      std::align_val_t ) noexcept
{
    deallocate_from_soma_heap( allocation );
}

void operator delete[]( void* allocation,
                        std::align_val_t ) noexcept
{
    deallocate_from_soma_heap( allocation );
}

void operator delete( void* allocation,
                      std::size_t,
                      std::align_val_t ) noexcept
{
    deallocate_from_soma_heap( allocation );
}

void operator delete[]( void* allocation,
                        std::size_t,
                        std::align_val_t ) noexcept
{
    deallocate_from_soma_heap( allocation );
}

#endif // SOMA_CONFIG_OVERRIDE_GLOBAL_NEW_DELETE
