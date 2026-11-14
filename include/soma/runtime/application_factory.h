// Copyright (c) 2026 Juraj Buno
// SPDX-License-Identifier: MIT

#ifndef SOMA_APPLICATION_FACTORY_H_
#define SOMA_APPLICATION_FACTORY_H_

#include <type_traits>
#include <utility>
#include <variant>

#include "soma/runtime/task.h"

namespace SOMA
{
    ///
    /// @brief Creates application objects at run-time.
    ///
    ///        - application memory is statically allocated (see std::variant)
    ///          NOTE: Application members are still free to use the dynamic allocation.
    ///
    ///        - supports multiple, independent applications to co-exist in a single firmware
    ///              - client startup code decides which application to instantiate using the create<T> method
    ///                NOTE: The decission process itself (e.g. by reading board identification analog/discrete inputs)
    ///                      can be a dedicated application.
    ///
    ///        - allows to switch between multiple applications at run-time
    ///          Switching applications must be implemented in the client startup code. Application is destroyed when its object
    ///          is destroyed (including OS tasks). It's up to the user to ensure the application ends at the right time.
    ///          Constructor arguments supplied to create<T>() are perfectly forwarded to T.
    ///
    ///        - object of this class must outlive every application it contains
    ///          NOTE: The factory is normally defined in static storage or owned by a long-lived client lifecycle task.
    template<typename ... A>
    class Application_factory
    {
    public:
        template<typename T, typename ... Args>
        T& create( Args&& ... args )
        {
            static_assert( ( std::is_same_v<T, A> || ... ), "Type T must be one of the factory's supported types" );
            static_assert( std::is_constructible_v<T, Args&& ...>, "Type T must be constructible from the supplied arguments" );

            T& application = m_application.template emplace<T>( std::forward<Args>( args )... );

            application.m_size_B = sizeof( T );

            return application;
        }

    private:
        std::variant<std::monostate, A...> m_application;
    };
} // namespace SOMA

#endif // APPLICATION_FACTORY_H_
