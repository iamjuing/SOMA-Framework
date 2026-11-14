// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_TIME_H_
#define SOMA_TIME_H_

#include <cstdint>

namespace SOMA
{
    namespace Time
    {
        /// @brief Returns elapsed milliseconds since the scheduler started.
        uint32_t uptime_ms( void );

        ///
        /// @brief Timestamp for measuring millisecond intervals.
        ///
        /// Comparisons and differences are valid for intervals no longer than
        /// half of the uint32_t millisecond range.
        class Stamp
        {
        public:
            Stamp( void );
            Stamp( const Stamp& init );
            Stamp( uint32_t time_ms );

            uint32_t get_time_ms( void ) const;
            uint32_t get_remaining_ms( void ) const;
            bool passed( void ) const;

            Stamp& operator=( const Stamp& rhs );
            Stamp& operator=( uint32_t rhs_time_ms );

            int64_t operator-( const Stamp& rhs ) const;
            Stamp& operator+( uint32_t rhs_time_ms );
            void operator+=( uint32_t time_ms );

            bool operator<( const Stamp& rhs ) const;
            bool operator>( const Stamp& rhs ) const;
            bool operator==( const Stamp& rhs ) const;
            bool operator!=( const Stamp& rhs ) const;

            static Stamp now( void );

            static constexpr uint32_t max_interval_ms( void )
            {
                return UINT32_MAX >> 1;
            }

        private:
            uint32_t m_time_ms;
        };

        inline Stamp::Stamp( void )
        {
            // intentionally empty
        }

        inline Stamp::Stamp( const Stamp& init )
            : m_time_ms( init.m_time_ms )
        {
            // intentionally empty
        }

        inline Stamp::Stamp( uint32_t time_ms )
            : m_time_ms( time_ms )
        {
            // intentionally empty
        }

        inline uint32_t Stamp::get_time_ms( void ) const
        {
            return m_time_ms;
        }

        inline Stamp& Stamp::operator=( const Stamp& rhs )
        {
            m_time_ms = rhs.m_time_ms;

            return *this;
        }

        inline Stamp& Stamp::operator=( uint32_t rhs_time_ms )
        {
            m_time_ms = rhs_time_ms;

            return *this;
        }

        inline bool Stamp::operator==( const Stamp& rhs ) const
        {
            return m_time_ms == rhs.m_time_ms;
        }

        inline bool Stamp::operator!=( const Stamp& rhs ) const
        {
            return m_time_ms != rhs.m_time_ms;
        }
    } // namespace Time
} // namespace SOMA

#endif // SOMA_TIME_H_
