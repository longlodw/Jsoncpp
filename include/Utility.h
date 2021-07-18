#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <string_view>
#include <functional>
#include <charconv>
#include <deque>
#include <cstring>
#include <cstdio>

namespace Jsoncpp {
    using char_ptr = char *;
    using const_char_ptr = const char *;

    enum class BraceType { curly,
                           bracket,
                           square_bracket,
                           single_quote,
                           double_quote
    };

    template <typename Ptr>
    constexpr bool is_char_pointer = std::is_same_v<Ptr, char_ptr> || std::is_same_v<Ptr, const_char_ptr>;

    template <typename Ptr>
    constexpr bool convertible_to_char_pointer = std::is_convertible_v<Ptr, char_ptr> || std::is_convertible_v<Ptr, const_char_ptr>;

    template <typename T>
    using removeConstReference = std::remove_reference_t<std::remove_const_t<T>>;

    template <typename Ptr>
    void movePtrElement(Ptr des, Ptr src, const size_t& num) {
        for (size_t k = 0; k < num; ++k) {
            des[k] = std::move(src[k]);
        }
    }

    template <typename Ptr>
    bool compare(Ptr ptr0, Ptr ptr1, const size_t& length) {
        for (size_t k = 0; k < length; ++k) {
            if (ptr0[k] != ptr1[k]) {
                return false;
            }
        }
        return true;
    }

    template <typename Ptr>
    void copyPtrElement(Ptr des, Ptr src, const size_t& num) {
        for (size_t k = 0; k < num; ++k) {
            des[k] = src[k];
        }
    }

    /*template <typename Ptr, typename = std::enable_if_t<std::is_same_v<char, decltype(*std::declval<Ptr>())>>>
    bool isLong(Ptr ptr, const size_t& length) {
        if (length == 0) {
            return false;
        }
        size_t k = 0;
        if (ptr[0] == '-' || ptr[0] == '+') {
            ++k;
        }
        for (; k < length; ++k) {
            if (ptr[k] < 48 || ptr[k] > 57) {
                return false;
            }
        }
        return true;
    }

    template <typename Ptr, typename = std::enable_if_t<std::is_same_v<char, decltype(*std::declval<Ptr>())>>>
    bool isDouble(Ptr ptr, const size_t& length) {
        if (length == 0) {
            return false;
        }
        size_t k = 0;
        if (ptr[0] == '-' || ptr[0] == '+') {
            ++k;
        }

    }*/

    template <typename Ptr, typename = std::enable_if_t<convertible_to_char_pointer<Ptr>>>
    std::string_view trimWhiteSpace(Ptr ptr, const size_t& size) {
        Ptr result = ptr;
        size_t len = size;
        for (size_t k = 0; k < size; ++k) {
            if (ptr[k] != ' ' & ptr[k] != '\t' & ptr[k] != '\n' & ptr[k] != '\v' & ptr[k] != '\f' & ptr[k] != '\r') {
                result += k;
                len -= k;
                break;
            }
        }
        for (size_t k = 1; k <= size; ++k) {
            auto &ch = ptr[size - k];
            if (ch != ' ' & ch != '\t' & ch != '\n' & ch != '\v' & ch != '\f' & ch != '\r') {
                len -= (k - 1);
                break;
            }
        }
        return std::string_view(result, len);
    }


    constexpr char getMatchingClosingBrace(const char& ch) {
        switch (ch) {
            case '{' : {
                return '}';
            }
            case '"' : {
                return '"';
            }
            case '[' : {
                return ']';
            }
            default : {
                return 0;
            }
        }
    }

    constexpr char getMatchingBrace(const char& ch) {
        switch (ch) {
            case '{' : {
                return '}';
            }
            case '}' : {
                return '{';
            }
            case '\"' : {
                return '\"';
            }
            case '[' : {
                return ']';
            }
            case ']' : {
                return '[';
            }
            default : {
                return 0;
            }
        }
    }

    template <typename Ptr>
    size_t findClosingBrace(Ptr ptr, const size_t& size, const size_t& po) {
        std::deque<char> close_braces;
        auto search_brace = getMatchingBrace(ptr[po]);
        if (search_brace)
        {
            close_braces.push_back(search_brace);
            for (size_t k = po + 1; k < size; ++k) {
                auto &cur_char = ptr[k];
                if (cur_char == '\\')
                {
                    ++k;
                    continue;
                }
                auto cur_close_brace = getMatchingClosingBrace(cur_char);
                if (cur_char == close_braces.back()) {
                    close_braces.pop_back();
                }
                else if (cur_close_brace) {
                    close_braces.push_back(cur_close_brace);
                }
                if (close_braces.size() == 0) {
                    return k;
                }
            }
        }
        return size;
    }

    template <char ch, typename Ptr, bool ignore_brace = false, typename = std::enable_if_t<convertible_to_char_pointer<Ptr>>>
    void splitToView(Ptr ptr, const size_t& size, const std::function<void(const std::string_view&)>& func) {
        size_t pre = 0;
        for (size_t k = 0; k < size;)
        {
            auto &cur_char = ptr[k];
            if (cur_char == '\\') {
                k += 2;
                continue;
            }
            if (cur_char == ch) {
                func(std::string_view(ptr + pre, k - pre));
                pre = k++;
                continue;
            }
            if (!ignore_brace) {
                auto match_brace = findClosingBrace(ptr, size, k);
                if (match_brace != size) {
                    k = match_brace + 1;
                    continue;
                }
            }
            ++k;
        }
        func(std::string_view(ptr + pre, size - pre));
    }
}

