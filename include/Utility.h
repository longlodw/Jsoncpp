#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <cstring>
#include <string_view>
#include <functional>

using char_ptr = char *;
using const_char_ptr = const char *;

template <typename Ptr>
constexpr bool is_char_pointer = std::is_same_v<Ptr, char_ptr> || std::is_same_v<Ptr, const_char_ptr>;

template <typename T>
using removeConstReference = std::remove_reference_t<std::remove_const_t<T>>;

template <typename Ptr>
void movePtrElement(Ptr des, Ptr src, const size_t& num) {
    for (size_t k = 0; k < num; ++k) {
        des[k] = std::move(src[k]);
    }
}

template <typename Ptr>
void copyPtrElement(Ptr des, Ptr src, const size_t& num) {
    for (size_t k = 0; k < num; ++k) {
        des[k] = src[k];
    }
}