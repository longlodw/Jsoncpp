#pragma once
#include "JsonClass.h"
namespace Jsoncpp {
    template <typename Alloc, typename Ptr, typename = std::enable_if_t<convertible_to_char_pointer<Ptr>>>
    bool objectify(Json<Alloc>& json_ref, const Ptr& ptr, const size_t& size) {
        struct Position {
            std::string_view view;
            Json<Alloc> *json_ptr;
            Position() = default;
            Position(const std::string_view& v, Json<Alloc>* const& p) : view(v), json_ptr(p) {}
        };
        Json<Alloc> result;
        std::deque<Position> que;
        que.emplace_back(std::string_view(ptr, size), &result);
        bool correct = true;
        while (correct && !que.empty())
        {
            auto& top = que.front();
            auto &view = top.view;
            view = trimWhiteSpace(view.data(), view.size());
            auto s = view.size();
            auto &front = view.front();
            auto &back = view.back();
            auto &cur_json = *(top.json_ptr);
            if (front == '{' && back == '}')
            {
                cur_json = JsonObject<Alloc>();
                auto funct = [&que, &cur_json, &correct](const std::string_view &v)
                {
                    auto trimmed = trimWhiteSpace(v.data(), v.size());
                    if (trimmed.front() != '"')
                    {
                        correct = false;
                        return;
                    }
                    auto len = trimmed.size();
                    auto char_data = trimmed.data();
                    auto close_quote_position = findClosingBrace(char_data, len, 0);
                    if (close_quote_position == len)
                    {
                        correct = false;
                        return;
                    }
                    for (size_t k = close_quote_position + 1; k < len; ++k)
                    {
                        if (trimmed[k] == ':')
                        {
                            auto key = JsonString<Alloc>(char_data + 1, close_quote_position - 1);
                            reinterpret_cast<JsonObject<Alloc>*>(&cur_json)->insert(key, Json());
                            que.emplace_back(std::string_view(char_data + k + 1, len - k - 1), &((*reinterpret_cast<JsonObject<Alloc>*>(&cur_json))[key]));
                            return;
                        }
                        if (trimmed[k] != ' ')
                        {
                            correct = false;
                            return;
                        }
                    }
                    correct = false;
                };
                splitToView<','>(view.data() + 1, s - 2, funct);
            }
            else if (front == '[' && back == ']') {
                cur_json = JsonArray<Alloc>();
                auto funct = [&que, &cur_json, &correct](const std::string_view &v)
                {
                    auto *cur_json_ptr = reinterpret_cast<JsonArray<Alloc> *>(&cur_json);
                    cur_json_ptr->pushBack(Json<Alloc>());
                    que.emplace_back(v, &((*cur_json_ptr)[cur_json_ptr->length() - 1]));
                };
            }
            else if (front == '\"' && back == '\"') {
                cur_json = JsonString<Alloc>(view.data() + 1, s - 2);
            }
            else if (s == 4 && compare(view.data(), "null", 4)) {
                cur_json = JsonNull<Alloc>();
            }
            else {
                long temp_l;
                auto err = std::from_chars(view.begin(), view.end(), temp_l);
                if (err.ptr == view.end())
                {
                    cur_json = JsonInteger<Alloc>(temp_l);
                    goto label0;
                }
                double temp_d;
                err = std::from_chars(view.begin(), view.end(), temp_d);
                if (err.ptr == view.end()) {
                    cur_json = JsonDecimal<Alloc>(temp_d);
                    goto label0;
                }
                goto label1;
            }
label0:
            que.pop_front();
            continue;
label1:
            return false;
        }
        if (!correct) {
            return false;
        }
        json_ref = std::move(result);
        return true;
    }

    template <typename Alloc, typename Ptr>
    size_t toString(const Json<Alloc>& json, Ptr ptr, const size_t& s) {
        size_t result = 0;
        switch (json.type)
        {
            case JsonType::String: {
                result = 2 + json.json.dynamic_container.length;
                if ((result = 2 + json.json.dynamic_container.length)> s) {
                    return 0;
                }
                ptr[0] = '\"';
                strncpy(ptr + 1, json.json.dynamic_container.pointer, json.json.dynamic_container.length);
                ptr[json.json.dynamic_container.length + 1] = '\"';
                break;
            }
            case JsonType::Object: {
                if ((result = 1) > s) {
                    return 0;
                }
                ptr[0] = '{';
                auto &dc = json.json.dynamic_container;
                auto data_ptr = reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer);
                bool not_first = false;
                for (size_t k = 0; k < dc.length; ++k)
                {
                    auto &cur = data_ptr[k];
                    if (cur.active) {
                        if (not_first) {
                            if ((result += 1) > s) {
                                return 0;
                            }
                            ptr[result - 1] = ',';
                        }
                        size_t change = toString(cur.key, ptr + result, s - result);
                        if (change) {
                            result += (change + 1);
                            if (result > s) {
                                return 0;
                            }
                            ptr[result - 1] = ':';
                            change = toString(cur.value, ptr + result, s - result);
                            if (change) {
                                result += change;
                            }
                            else {
                                return 0;
                            }
                        }
                        else {
                            return 0;
                        }
                        not_first = true;
                    }
                }
                if ((result += 1) > s) {
                    return 0;
                }
                ptr[result - 1] = '}';
                break;
            }
            case JsonType::Array: {
                if ((result = 1) > s) {
                    return 0;
                }
                ptr[0] = '[';
                auto &dc = json.json.dynamic_container;
                auto data_ptr = reinterpret_cast<Json<Alloc> *>(dc.pointer);
                if (dc.length >= 1) {
                    size_t change = toString(data_ptr[0], ptr + result, s - result);
                    if (change) {
                        result += change;
                    }
                    else {
                        return 0;
                    }
                }
                for (size_t k = 1; k < dc.length; ++k) {
                    if ((result += 1) > s) {
                        return 0;
                    }
                    ptr[result - 1] = ',';
                    size_t change = toString(data_ptr[k], ptr + result, s - result);
                    if (change) {
                        result += change;
                    }
                    else {
                        return 0;
                    }
                }
                if ((result += 1) > s) {
                    return 0;
                }
                ptr[result - 1] = ']';
                break;
            }
            case JsonType::Boolean: {
                if (json.json.boolean) {
                    if ((result = 4) > s) {
                        return 0;
                    }
                    ptr[0] = 't';
                    ptr[1] = 'r';
                    ptr[2] = 'u';
                    ptr[3] = 'e';
                }
                else {
                    if ((result = 5) > s) {
                        return 0;
                    }
                    ptr[0] = 'f';
                    ptr[1] = 'a';
                    ptr[2] = 'l';
                    ptr[3] = 's';
                    ptr[4] = 'e';
                }
                break;
            }
            case JsonType::Decimal: {
                auto change = std::sprintf(ptr, "%lf", json.json.decimal);
                if (change < 0) {
                    return 0;
                }
                result += change;
                break;
            }
            case JsonType::Integer: {
                auto change = std::sprintf(ptr, "%ld", json.json.integer);
                if (change < 0) {
                    return 0;
                }
                result += change;
                break;
            }
            case JsonType::Null: {
                if ((result = 4) > s) {
                    return 0;
                }
                ptr[0] = 'n';
                ptr[1] = 'u';
                ptr[2] = 'l';
                ptr[3] = 'l';
                break;
            }
            default:
                break;
        }
        return result;
    }
}

