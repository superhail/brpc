// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef BRPC_SCOPED_GUARD_H
#define BRPC_SCOPED_GUARD_H

#include <type_traits>
#include "butil/macros.h"

namespace butil {

// Whether a no-argument callable returns void.
template<typename T>
struct returns_void_t
    : public std::is_same<void, decltype(std::declval<T&&>()())>
{};

template<typename Callback,
    typename = typename std::enable_if<
        returns_void_t<Callback>::value>::type>
class ScopeGuard;

template<typename Callback>
ScopeGuard<Callback> MakeScopeGuard(Callback&& callback) noexcept;

// ScopeGuard is a simple implementation to guarantee that
// a function is executed upon leaving the current scope.
template<typename Callback>
class ScopeGuard<Callback> {
public:
    ScopeGuard(ScopeGuard&& other) noexcept
        :_callback(std::move(other._callback))
        , _dismiss(other._dismiss) {
        other.dismiss();
    }

    ~ScopeGuard() noexcept {
        if(!_dismiss) {
            _callback();
        }
    }

    void dismiss() noexcept {
        _dismiss = true;
    }

    ScopeGuard() = delete;
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

private:
// Only MakeScopeGuard and move constructor can create ScopeGuard.
friend ScopeGuard<Callback> MakeScopeGuard<Callback>(Callback&& callback) noexcept;

    explicit ScopeGuard(Callback&& callback) noexcept
        :_callback(std::forward<Callback>(callback))
        , _dismiss(false) {}

private:
    Callback _callback;
    bool _dismiss;
};

// The MakeScopeGuard() function is used to create a new ScopeGuard object.
// It can be instantiated with a lambda function, a std::function<void()>,
// a functor, or a void(*)() function pointer.
template<typename Callback>
ScopeGuard<Callback> MakeScopeGuard(Callback&& callback) noexcept {
    return ScopeGuard<Callback>{ std::forward<Callback>(callback)};
}

namespace internal {
// for BAIDU_SCOPE_EXIT.
enum class ScopeExitHelper {};

template<typename Callback>
ScopeGuard<Callback>
operator+(ScopeExitHelper, Callback&& callback) {
    return MakeScopeGuard(std::forward<Callback>(callback));
}
} // namespace internal
} // namespace butil

#define BRPC_ANONYMOUS_VARIABLE(prefix) BAIDU_CONCAT(prefix, __COUNTER__)

// The code in the braces of BAIDU_SCOPE_EXIT always executes at the end of the scope.
// Variables used within BAIDU_SCOPE_EXIT are captured by reference.
// Example:
// int fd = open(...);
// BAIDU_SCOPE_EXIT {
//     close(fd);
// };
// use fd ...
//
#define BRPC_SCOPE_EXIT                                     \
  auto BRPC_ANONYMOUS_VARIABLE(SCOPE_EXIT) =                \
      ::butil::internal::ScopeExitHelper() + [&]() noexcept

#endif // BRPC_SCOPED_GUARD_H
