// Copyright 2018 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COMMON_RESULT_H_
#define COMMON_RESULT_H_

#include "common/Assert.h"
#include "common/Compiler.h"

#include <cstddef>
#include <cstdint>
#include <utility>

// Result<T, E> is the following sum type (Haskell notation):
//
//      data Result T E = Success T | Error E | Empty
//
// It is meant to be used as the return type of functions that might fail. The reason for the Empty
// case is that a Result should never be discarded, only destructured (its error or success moved
// out) or moved into a different Result. The Empty case tags Results that have been moved out and
// Result's destructor should ASSERT on it being Empty.
//
// Since C++ doesn't have efficient sum types for the special cases we care about, we provide
// template specializations for them.

template <typename T, typename E>
class Result;

// The interface of Result<T, E> shoud look like the following.
//  public:
//    Result(T&& success);
//    Result(E&& error);
//
//    Result(Result<T, E>&& other);
//    Result<T, E>& operator=(Result<T, E>&& other);
//
//    ~Result();
//
//    bool IsError() const;
//    bool IsSuccess() const;
//
//    T&& AcquireSuccess();
//    E&& AcquireError();

// Specialization of Result for returning errors only via pointers. It is basically a pointer
// where nullptr is both Success and Empty.
template <typename E>
class DAWN_NO_DISCARD Result<void, E*> {
  public:
    Result();
    Result(E* error);

    Result(Result<void, E*>&& other);
    Result<void, E*>& operator=(Result<void, E>&& other);

    ~Result();

    bool IsError() const;
    bool IsSuccess() const;

    void AcquireSuccess();
    E* AcquireError();

  private:
    E* mError = nullptr;
};

// Uses SFINAE to try to get alignof(T) but fallback to Default if T isn't defined.
template <typename T, size_t Default, typename = size_t>
constexpr size_t alignof_if_defined_else_default = Default;

template <typename T, size_t Default>
constexpr size_t alignof_if_defined_else_default<T, Default, decltype(alignof(T))> = alignof(T);

// Specialization of Result when both the error an success are pointers. It is implemented as a
// tagged pointer. The tag for Success is 0 so that returning the value is fastest.

namespace detail {
    // Utility functions to manipulate the tagged pointer. Some of them don't need to be templated
    // but we really want them inlined so we keep them in the headers
    enum PayloadType {
        Success = 0,
        Error = 1,
        Empty = 2,
    };

    intptr_t MakePayload(const void* pointer, PayloadType type);
    PayloadType GetPayloadType(intptr_t payload);

    template <typename T>
    static T* GetSuccessFromPayload(intptr_t payload);
    template <typename E>
    static E* GetErrorFromPayload(intptr_t payload);

    constexpr static intptr_t kEmptyPayload = Empty;
}  // namespace detail

template <typename T, typename E>
class DAWN_NO_DISCARD Result<T*, E*> {
  public:
    static_assert(alignof_if_defined_else_default<T, 4> >= 4,
                  "Result<T*, E*> reserves two bits for tagging pointers");
    static_assert(alignof_if_defined_else_default<E, 4> >= 4,
                  "Result<T*, E*> reserves two bits for tagging pointers");

    Result(T* success);
    Result(E* error);

    Result(Result<T*, E*>&& other);
    Result<T*, E*>& operator=(Result<T*, E>&& other);

    ~Result();

    bool IsError() const;
    bool IsSuccess() const;

    T* AcquireSuccess();
    E* AcquireError();

  private:
    intptr_t mPayload = detail::kEmptyPayload;
};

template <typename T, typename E>
class DAWN_NO_DISCARD Result<const T*, E*> {
  public:
    static_assert(alignof_if_defined_else_default<T, 4> >= 4,
                  "Result<T*, E*> reserves two bits for tagging pointers");
    static_assert(alignof_if_defined_else_default<E, 4> >= 4,
                  "Result<T*, E*> reserves two bits for tagging pointers");

    Result(const T* success);
    Result(E* error);

    Result(Result<const T*, E*>&& other);
    Result<const T*, E*>& operator=(Result<const T*, E>&& other);

    ~Result();

    bool IsError() const;
    bool IsSuccess() const;

    const T* AcquireSuccess();
    E* AcquireError();

  private:
    intptr_t mPayload = detail::kEmptyPayload;
};

// Catchall definition of Result<T, E> implemented as a tagged struct. It could be improved to use
// a tagged union instead if it turns out to be a hotspot. T and E must be movable and default
// constructible.
template <typename T, typename E>
class DAWN_NO_DISCARD Result {
  public:
    Result(T&& success);
    Result(E&& error);

    Result(Result<T, E>&& other);
    Result<T, E>& operator=(Result<T, E>&& other);

    ~Result();

    bool IsError() const;
    bool IsSuccess() const;

    T&& AcquireSuccess();
    E&& AcquireError();

  private:
    enum PayloadType {
        Success = 0,
        Error = 1,
        Acquired = 2,
    };
    PayloadType mType;

    E mError;
    T mSuccess;
};

// Implementation of Result<void, E*>
template <typename E>
Result<void, E*>::Result() {
}

template <typename E>
Result<void, E*>::Result(E* error) : mError(error) {
}

template <typename E>
Result<void, E*>::Result(Result<void, E*>&& other) : mError(other.mError) {
    other.mError = nullptr;
}

template <typename E>
Result<void, E*>& Result<void, E*>::operator=(Result<void, E>&& other) {
    ASSERT(mError == nullptr);
    mError = other.mError;
    other.mError = nullptr;
    return *this;
}

template <typename E>
Result<void, E*>::~Result() {
    ASSERT(mError == nullptr);
}

template <typename E>
bool Result<void, E*>::IsError() const {
    return mError != nullptr;
}

template <typename E>
bool Result<void, E*>::IsSuccess() const {
    return mError == nullptr;
}

template <typename E>
void Result<void, E*>::AcquireSuccess() {
}

template <typename E>
E* Result<void, E*>::AcquireError() {
    E* error = mError;
    mError = nullptr;
    return error;
}

// Implementation details of the tagged pointer Results
namespace detail {

    template <typename T>
    T* GetSuccessFromPayload(intptr_t payload) {
        ASSERT(GetPayloadType(payload) == Success);
        return reinterpret_cast<T*>(payload);
    }

    template <typename E>
    E* GetErrorFromPayload(intptr_t payload) {
        ASSERT(GetPayloadType(payload) == Error);
        return reinterpret_cast<E*>(payload ^ 1);
    }

}  // namespace detail

// Implementation of Result<T*, E*>
template <typename T, typename E>
Result<T*, E*>::Result(T* success) : mPayload(detail::MakePayload(success, detail::Success)) {
}

template <typename T, typename E>
Result<T*, E*>::Result(E* error) : mPayload(detail::MakePayload(error, detail::Error)) {
}

template <typename T, typename E>
Result<T*, E*>::Result(Result<T*, E*>&& other) : mPayload(other.mPayload) {
    other.mPayload = detail::kEmptyPayload;
}

template <typename T, typename E>
Result<T*, E*>& Result<T*, E*>::operator=(Result<T*, E>&& other) {
    ASSERT(mPayload == detail::kEmptyPayload);
    mPayload = other.mPayload;
    other.mPayload = detail::kEmptyPayload;
    return *this;
}

template <typename T, typename E>
Result<T*, E*>::~Result() {
    ASSERT(mPayload == detail::kEmptyPayload);
}

template <typename T, typename E>
bool Result<T*, E*>::IsError() const {
    return detail::GetPayloadType(mPayload) == detail::Error;
}

template <typename T, typename E>
bool Result<T*, E*>::IsSuccess() const {
    return detail::GetPayloadType(mPayload) == detail::Success;
}

template <typename T, typename E>
T* Result<T*, E*>::AcquireSuccess() {
    T* success = detail::GetSuccessFromPayload<T>(mPayload);
    mPayload = detail::kEmptyPayload;
    return success;
}

template <typename T, typename E>
E* Result<T*, E*>::AcquireError() {
    E* error = detail::GetErrorFromPayload<E>(mPayload);
    mPayload = detail::kEmptyPayload;
    return error;
}

// Implementation of Result<const T*, E*>
template <typename T, typename E>
Result<const T*, E*>::Result(const T* success)
    : mPayload(detail::MakePayload(success, detail::Success)) {
}

template <typename T, typename E>
Result<const T*, E*>::Result(E* error) : mPayload(detail::MakePayload(error, detail::Error)) {
}

template <typename T, typename E>
Result<const T*, E*>::Result(Result<const T*, E*>&& other) : mPayload(other.mPayload) {
    other.mPayload = detail::kEmptyPayload;
}

template <typename T, typename E>
Result<const T*, E*>& Result<const T*, E*>::operator=(Result<const T*, E>&& other) {
    ASSERT(mPayload == detail::kEmptyPayload);
    mPayload = other.mPayload;
    other.mPayload = detail::kEmptyPayload;
    return *this;
}

template <typename T, typename E>
Result<const T*, E*>::~Result() {
    ASSERT(mPayload == detail::kEmptyPayload);
}

template <typename T, typename E>
bool Result<const T*, E*>::IsError() const {
    return detail::GetPayloadType(mPayload) == detail::Error;
}

template <typename T, typename E>
bool Result<const T*, E*>::IsSuccess() const {
    return detail::GetPayloadType(mPayload) == detail::Success;
}

template <typename T, typename E>
const T* Result<const T*, E*>::AcquireSuccess() {
    T* success = detail::GetSuccessFromPayload<T>(mPayload);
    mPayload = detail::kEmptyPayload;
    return success;
}

template <typename T, typename E>
E* Result<const T*, E*>::AcquireError() {
    E* error = detail::GetErrorFromPayload<E>(mPayload);
    mPayload = detail::kEmptyPayload;
    return error;
}

// Implementation of Result<T, E>
template <typename T, typename E>
Result<T, E>::Result(T&& success) : mType(Success), mSuccess(std::move(success)) {
}

template <typename T, typename E>
Result<T, E>::Result(E&& error) : mType(Error), mError(std::move(error)) {
}

template <typename T, typename E>
Result<T, E>::~Result() {
    ASSERT(mType == Acquired);
}

template <typename T, typename E>
Result<T, E>::Result(Result<T, E>&& other)
    : mType(other.mType), mError(std::move(other.mError)), mSuccess(std::move(other.mSuccess)) {
    other.mType = Acquired;
}
template <typename T, typename E>
Result<T, E>& Result<T, E>::operator=(Result<T, E>&& other) {
    mType = other.mType;
    mError = std::move(other.mError);
    mSuccess = std::move(other.mSuccess);
    other.mType = Acquired;
    return *this;
}

template <typename T, typename E>
bool Result<T, E>::IsError() const {
    return mType == Error;
}

template <typename T, typename E>
bool Result<T, E>::IsSuccess() const {
    return mType == Success;
}

template <typename T, typename E>
T&& Result<T, E>::AcquireSuccess() {
    ASSERT(mType == Success);
    mType = Acquired;
    return std::move(mSuccess);
}

template <typename T, typename E>
E&& Result<T, E>::AcquireError() {
    ASSERT(mType == Error);
    mType = Acquired;
    return std::move(mError);
}

#endif  // COMMON_RESULT_H_
