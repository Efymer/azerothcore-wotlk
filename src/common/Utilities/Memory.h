/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ACORE_MEMORY_H
#define ACORE_MEMORY_H

// Ported from TrinityCore's Memory.h. Provides make_unique_ptr_with_deleter to
// build std::unique_ptr objects guarded by a custom (stateful or stateless)
// deleter - used by the bnetserver Main/SslContext for RAII over C resources.

#include "CompilerDefs.h"
#include <concepts>
#include <memory>

#if AC_COMPILER == AC_COMPILER_GNU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#endif

namespace Acore
{
namespace Impl
{
template<typename T, typename Del>
struct stateful_unique_ptr_deleter
{
    using pointer = T;
    explicit(false) stateful_unique_ptr_deleter(Del deleter) : _deleter(std::move(deleter)) { }
    void operator()(pointer ptr) const { (void)_deleter(ptr); }

private:
    Del _deleter;
};

template<typename T, auto Del>
struct stateless_unique_ptr_deleter
{
    using pointer = T;
    void operator()(pointer ptr) const
    {
        if constexpr (std::is_member_function_pointer_v<decltype(Del)>)
            (void)(ptr->*Del)();
        else
            (void)Del(ptr);
    }
};
}

/**
 * Convenience function to construct type aliases for std::unique_ptr stateful deleters (such as lambda with captures)
 */
template <typename Ptr, typename Del> requires std::invocable<Del, Ptr> && std::is_pointer_v<Ptr>
Impl::stateful_unique_ptr_deleter<Ptr, Del> unique_ptr_deleter(Del deleter)
{
    return Impl::stateful_unique_ptr_deleter<Ptr, Del>(std::move(deleter));
}

/**
 * Convenience function to construct type aliases for std::unique_ptr stateless deleters
 */
template <typename Ptr, auto Del> requires std::invocable<decltype(Del), Ptr> && std::is_pointer_v<Ptr>
Impl::stateless_unique_ptr_deleter<Ptr, Del> unique_ptr_deleter()
{
    return Impl::stateless_unique_ptr_deleter<Ptr, Del>();
}

/**
 * Utility function to construct a std::unique_ptr object with custom stateful deleter (such as lambda with captures)
 */
template<typename Ptr, typename T = std::remove_pointer_t<Ptr>, typename Del> requires std::invocable<Del, Ptr> && std::is_pointer_v<Ptr>
std::unique_ptr<T, Impl::stateful_unique_ptr_deleter<Ptr, Del>> make_unique_ptr_with_deleter(Ptr ptr, Del deleter)
{
    return std::unique_ptr<T, Impl::stateful_unique_ptr_deleter<Ptr, Del>>(ptr, std::move(deleter));
}

/**
 * Utility function to construct a std::unique_ptr object with custom stateless deleter (function pointer, captureless lambda)
 */
template<auto Del, typename Ptr, typename T = std::remove_pointer_t<Ptr>> requires std::invocable<decltype(Del), Ptr> && std::is_pointer_v<Ptr>
std::unique_ptr<T, Impl::stateless_unique_ptr_deleter<Ptr, Del>> make_unique_ptr_with_deleter(Ptr ptr)
{
    return std::unique_ptr<T, Impl::stateless_unique_ptr_deleter<Ptr, Del>>(ptr);
}
}

#if AC_COMPILER == AC_COMPILER_GNU
#pragma GCC diagnostic pop
#endif

#endif // ACORE_MEMORY_H
