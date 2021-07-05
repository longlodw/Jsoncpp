#pragma once
#include "JsonCore.h"
#include <iostream>

//Json class
template <typename Alloc = std::allocator<char>>
struct Json {
    using allocator_type = Alloc;
    using alloc_traits = std::allocator_traits<allocator_type>;
    using alloc_propagate_c = typename alloc_traits::propagate_on_container_copy_assignment;
    using alloc_propagate_m = typename alloc_traits::propagate_on_container_move_assignment;

    JsonType type;
    BasicJSON json;
    allocator_type allocator_object;


    Json() = default;
    Json(const JsonType &t);
    Json(const Json &other);
    Json(Json&& other) noexcept;
    Json& operator= (const Json& other);
    Json& operator= (Json&& other) noexcept;
    ~Json();

    virtual bool operator== (const Json& other) const;
};


//JsonString class
template <typename Alloc = std::allocator<char>>
struct JsonString : public Json<Alloc> {
    JsonString();
    template <typename Ptr>
    JsonString(Ptr ptr);
    template <typename Ptr>
    JsonString(Ptr ptr, const size_t& l);
    size_t length() const;
};

//JsonArray class
template <typename Alloc = std::allocator<char>>
struct JsonArray: public Json<Alloc> {
    JsonArray();
    JsonArray(const size_t& num);
    const Json<Alloc> &operator[](const size_t &index) const;
    Json<Alloc> &operator[](const size_t &index);
    size_t length() const;
    template <typename T>
    std::enable_if_t<std::is_convertible_v<T, Json<Alloc>>> pushBack(T &&element);
};

//hash
namespace std {
    template <typename Alloc>
    struct hash<JsonString<Alloc>> : public hash<string_view> {
        size_t operator()(const JsonString<Alloc>& s) const {
            return reinterpret_cast<const hash<string_view>*>(this)->operator()(string_view(s.json.dynamic_container.pointer, s.length()));
        }
    };
}

//JsonKeyValuePair class
template <typename Alloc>
struct JsonKeyValuePair {
    JsonString<Alloc> key;
    Json<Alloc> value;
    bool active = false;
};

//JsonObject class
template <typename Alloc = std::allocator<char>>
struct JsonObject : public  Json<Alloc> {
    static constexpr std::hash<JsonString<Alloc>> hasher{};
    
    JsonObject(const size_t &num = 32);
    const Json<Alloc>* at(const JsonString<Alloc> &key) const;
    Json<Alloc> &operator[](const JsonString<Alloc> &key);
    Json<Alloc> &operator[](JsonString<Alloc> &&key);
    template <typename K, typename V>
    std::enable_if_t<std::is_convertible_v<K, JsonString<Alloc>> && std::is_convertible_v<V, Json<Alloc>>> insert(K &&key, V &&value);
    bool rehash(const size_t& num);
};

//JsonDecimal class
template <typename Alloc = std::allocator<char>>
struct JsonDecimal : public Json<Alloc> {
    JsonDecimal(const double &num = 0);
};

//JsonInteger class
template <typename Alloc = std::allocator<char>>
struct JsonInteger : public Json<Alloc> {
    JsonInteger(const long &num = 0);
};

template <typename Alloc = std::allocator<char>>
struct JsonBoolean : public Json<Alloc> {
    JsonBoolean(const bool &b);
};

template <typename Alloc = std::allocator<char>>
struct JsonNull : public Json<Alloc> {
    JsonNull();
};

/*Functions--------------------------------------------------------------------------------------------------------------------------------*/

//Json class member function

template <typename Alloc>
inline Json<Alloc>::Json(const Json &other)
    : type(other.type), allocator_object(alloc_traits::select_on_container_copy_construction(other.allocator_object)), json(other.json)
{
    std::cout << "copy constructor" << std::endl;
    switch (type) {
    case JsonType::String: {
        auto &dc = json.dynamic_container;
        dc.pointer = alloc_traits::allocate(allocator_object, dc.size);
        copyPtrElement(dc.pointer, other.json.dynamic_container.pointer, dc.size);
        break;
        }
        case JsonType::Array: {
            auto &dc = json.dynamic_container;
            dc.pointer = alloc_traits::allocate(allocator_object, dc.size);
            copyPtrElement(reinterpret_cast<Json<Alloc> *>(dc.pointer), reinterpret_cast<Json<Alloc> *>(other.json.dynamic_container.pointer), dc.length);
            break;
        }
        case JsonType::Object: {
            auto &dc = json.dynamic_container;
            dc.pointer = alloc_traits::allocate(allocator_object, dc.size);
            copyPtrElement(reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer), reinterpret_cast<JsonKeyValuePair<Alloc> *>(other.json.dynamic_container.pointer), dc.length);
            break;
        }
    }
}

template <typename Alloc>
inline Json<Alloc>::Json(Json&& other) noexcept 
: type(other.type), allocator_object(std::move(other.allocator_object)), json(other.json) {
    other.json.dynamic_container.pointer = nullptr;
    std::cout << "move constructor" << std::endl;
}

template <typename Alloc>
inline Json<Alloc>::Json(const JsonType& t)
: type(t), allocator_object({}), json({}) {}

template <typename Alloc>
Json<Alloc>& Json<Alloc>::operator=(const Json& other) {
    std::cout << "assign" << std::endl;
    if (this == &other)
    {
        return *this;
    }
    auto &dc = json.dynamic_container;
    std::cout << "deallocating" << std::endl;
    alloc_traits::deallocate(allocator_object, dc.pointer, dc.size);
    std::cout << "done deallocating" << std::endl;
    if (alloc_propagate_c::value)
    {
        allocator_object = other.allocator_object;
    }
    std::cout << "allocator reassignment" << std::endl;
    type = other.type;
    auto &odc = other.json.dynamic_container;
    dc.length = odc.length;
    dc.size = odc.size;
    dc.pointer = alloc_traits::allocate(allocator_object, dc.size);
    std::cout << "done allocation" << std::endl;
    switch (type) {
        case JsonType::String: {
            copyPtrElement(dc.pointer, odc.pointer, dc.size);
            break;
        }
        case JsonType::Array: {
            copyPtrElement(reinterpret_cast<Json<Alloc> *>(dc.pointer), reinterpret_cast<Json<Alloc> *>(odc.pointer), dc.length);
            break;
        }
        case JsonType::Object: {
            copyPtrElement(reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer), reinterpret_cast<JsonKeyValuePair<Alloc> *>(odc.pointer), dc.length);
            break;
        }
    }
    std::cout << "done allocation" << std::endl;
    return *this;
}

template <typename Alloc>
Json<Alloc>& Json<Alloc>::operator=(Json&& other) noexcept {
    std::cout << "move assignment" << std::endl;
    auto &dc = json.dynamic_container;
    alloc_traits::deallocate(allocator_object, dc.pointer, dc.size);
    dc = other.json.dynamic_container;
    other.json.dynamic_container.pointer = nullptr;
    if (alloc_propagate_m::value) {
        allocator_object = std::move(other.allocator_object);
    }
    return *this;
}

template <typename Alloc>
inline Json<Alloc>::~Json() {
    if (type == JsonType::String | type == JsonType::Array | type == JsonType::Object) {
        alloc_traits::deallocate(allocator_object, json.dynamic_container.pointer, json.dynamic_container.size);
    }
}

template <typename Alloc>
bool Json<Alloc>::operator==(const Json& other) const {
    if (this == &other) {
        return true;
    }
    auto &dc = json.dynamic_container;
    auto &odc = other.json.dynamic_container;
    return (type == other.type) & (allocator_object == other.allocator_object) & (dc.length == odc.length) & (dc.pointer == odc.pointer) & (dc.size == odc.size);
}

//JsonString class member function

template <typename Alloc>
JsonString<Alloc>::JsonString() : Json<Alloc>(JsonType::String) {}

template <typename Alloc>
template <typename Ptr>
JsonString<Alloc>::JsonString(Ptr ptr) 
: Json<Alloc>(JsonType::String) {
    static_assert(is_char_pointer<Ptr>);
    auto &dc = Json<Alloc>::json.dynamic_container;
    dc.length = std::strlen(ptr);
    dc.size = dc.length + 1;
    dc.pointer = Json<Alloc>::alloc_traits::allocate(Json<Alloc>::allocator_object, dc.size);
    std::memcpy(dc.pointer, ptr, dc.size);
}

template <typename Alloc>
template <typename Ptr>
JsonString<Alloc>::JsonString(Ptr ptr, const size_t& l) 
: Json<Alloc>(JsonType::String) {
    static_assert(is_char_pointer<Ptr>);
    auto &dc = Json<Alloc>::json.dynamic_container;
    dc.length = l;
    dc.size = l + 1;
    dc.pointer = Json<Alloc>::alloc_traits::allocate(Json<Alloc>::allocator_object, dc.size);
    std::memcpy(dc.pointer, ptr, dc.size);
}

template <typename Alloc>
size_t JsonString<Alloc>::length() const {
    return Json<Alloc>::json.dynamic_container.length;
}

//JsonArray class memeber function

template <typename Alloc>
JsonArray<Alloc>::JsonArray() : Json<Alloc>(JsonType::Array) {}

template <typename Alloc>
JsonArray<Alloc>::JsonArray(const size_t& num)
: Json<Alloc>() {
    Json<Alloc>::type = JsonType::Array;
    auto &dc = Json<Alloc>::json.dynamic_container;
    dc.length = 0;
    dc.size = num * sizeof(Json<Alloc>);
    dc.pointer = Json<Alloc>::alloc_traits::allocate(Json<Alloc>::allocator_object, dc.size);
}

template <typename Alloc>
const Json<Alloc>& JsonArray<Alloc>::operator[](const size_t& index) const {
    return reinterpret_cast<Json<Alloc> *>(Json<Alloc>::json.dynamic_container.pointer)[index];
}

template <typename Alloc>
Json<Alloc>& JsonArray<Alloc>::operator[](const size_t& index) {
    return reinterpret_cast<Json<Alloc> *>(Json<Alloc>::json.dynamic_container.pointer)[index];
}

template <typename Alloc>
size_t JsonArray<Alloc>::length() const {
    return Json<Alloc>::json.dynamic_container.length;
}

template <typename Alloc>
template <typename T>
std::enable_if_t<std::is_convertible_v<T, Json<Alloc>>> JsonArray<Alloc>::pushBack(T &&element) {
    auto &dc = Json<Alloc>::json.dynamic_container;
    if (dc.length * sizeof(Json<Alloc>) == dc.size) {
        dc.size = ((dc.size == 0) ? 1 : (dc.size * 2));
        char_ptr temp = Json<Alloc>::alloc_traits::allocate(Json<Alloc>::allocator_object, dc.size);
        movePtrElement(reinterpret_cast<Json<Alloc> *>(temp), reinterpret_cast<Json<Alloc> *>(dc.pointer), dc.length);
        Json<Alloc>::alloc_traits::deallocate(Json<Alloc>::allocator_object, dc.pointer, dc.length * sizeof(Json<Alloc>));
        dc.pointer = temp;
    }
    std::cout << "before pushback" << std::endl;
    reinterpret_cast<Json<Alloc> *>(dc.pointer)[dc.length++] = std::forward<T>(element);
}

//JsonObject class member function
template <typename Alloc>
JsonObject<Alloc>::JsonObject(const size_t &num) : Json<Alloc>(JsonType::Object) {
    auto &dc = Json<Alloc>::json.dynamic_container;
    dc.length = num;
    dc.size = num * sizeof(JsonKeyValuePair<Alloc>);
    dc.pointer = Json<Alloc>::alloc_traits::allocate(Json<Alloc>::allocator_object, dc.size);
    for (size_t k = 0; k < num; ++k) {
        reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer)[k] = JsonKeyValuePair<Alloc>();
    }
}

template <typename Alloc>
const Json<Alloc>* JsonObject<Alloc>::at(const JsonString<Alloc>& key) const {
    size_t hash_value = hasher(key);
    auto &dc = Json<Alloc>::json.dynamic_container;
    size_t start = hash_value % dc.length;
    for (size_t k = start; k < dc.length; ++k) {
        auto &slot = reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer)[k];
        if (slot.active && slot.key == key) {
            return &slot.value;
        }
    }
    return nullptr;
}

template <typename Alloc>
Json<Alloc>& JsonObject<Alloc>::operator[](const JsonString<Alloc>& key) {
    size_t hash_value = hasher(key);
label:
    auto &dc = Json<Alloc>::json.dynamic_container;
    size_t start = hash_value % dc.length;
    for (size_t k = start; k < dc.length; ++k) {
        auto &slot = reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer)[k];
        if (slot.active) {
            if (slot.key == key) {
                return slot.value;
            }
        }
        else {
            slot.key = key;
            slot.active = true;
            return slot.value;
        }
    }
    rehash(2 * dc.length);
    goto label;
}

template <typename Alloc>
Json<Alloc>& JsonObject<Alloc>::operator[](JsonString<Alloc>&& key) {
    JsonString<Alloc> moved_key = std::forward<JsonString<Alloc>>(key);
    size_t hash_value = hasher(moved_key);
label:
    auto &dc = Json<Alloc>::json.dynamic_container;
    size_t start = hash_value % dc.length;
    for (size_t k = start; k < dc.length; ++k) {
        auto &slot = reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer)[k];
        if (slot.active) {
            if (slot.key == moved_key) {
                return slot.value;
            }
        }
        else {
            slot.key = std::move(moved_key);
            slot.active = true;
            return slot.value;
        }
    }
    rehash(2 * dc.length);
    goto label;
}

template <typename Alloc>
template <typename K, typename V>
inline std::enable_if_t<std::is_convertible_v<K, JsonString<Alloc>> && std::is_convertible_v<V, Json<Alloc>>> JsonObject<Alloc>::insert(K&& key, V&& value) {
    this->operator[](std::forward<K>(key)) = std::forward<V>(value);
}

template <typename Alloc>
bool JsonObject<Alloc>::rehash(const size_t& num) {
    char_ptr ptr = Json<Alloc>::alloc_traits::allocate(Json<Alloc>::allocator_object, num * sizeof(JsonKeyValuePair<Alloc>));
    for (size_t k = 0; k < num; ++k) {
        reinterpret_cast<JsonKeyValuePair<Alloc> *>(ptr)[k] = JsonKeyValuePair<Alloc>();
    }
    auto &dc = Json<Alloc>::json.dynamic_container;
    for (size_t k = 0; k < dc.length; ++k) {
        auto &po = reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer)[k];
        if (po.active) {
            size_t hash_loc = hasher(po.key) % num;
            for (size_t c = hash_loc; c < num; ++c) {
                auto& new_po = reinterpret_cast<JsonKeyValuePair<Alloc> *>(ptr)[c];
                if (new_po.active) {
                    continue;
                }
                new_po.active = true;
                new_po.key = std::move(po.key);
                new_po.value = std::move(po.value);
                po.active = false;
                goto label;
            }
            for (size_t c = 0; c < num; ++c) {
                auto& new_po = reinterpret_cast<JsonKeyValuePair<Alloc> *>(ptr)[c];
                if (new_po.active) {
                    this->insert(std::move(new_po.key), std::move(new_po.value));
                }
            }
            Json<Alloc>::alloc_traits::deallocate(Json<Alloc>::allocator_object, ptr, num * sizeof(JsonKeyValuePair<Alloc>));
            return false;
        label:;
        }
    }
    Json<Alloc>::alloc_traits::deallocate(Json<Alloc>::allocator_object, dc.pointer, dc.size);
    dc.pointer = ptr;
    dc.size = num * sizeof(JsonKeyValuePair<Alloc>);
    dc.length = num;
    return true;
}

//JsonDecimal class member function
template <typename Alloc>
JsonDecimal<Alloc>::JsonDecimal(const double& num) : Json<Alloc>::Json(JsonType::Decimal) {
    Json<Alloc>::json.decimal = num;
}

//JsonInteger class member function
template <typename Alloc>
JsonInteger<Alloc>::JsonInteger(const long& num) : Json<Alloc>::Json(JsonType::Integer) {
    Json<Alloc>::json.integer = num;
}

//JsonBoolean class member function
template <typename Alloc>
JsonBoolean<Alloc>::JsonBoolean(const bool& b) : Json<Alloc>::Json(JsonType::Boolean) {
    Json<Alloc>::json.boolean = b;
}

//JsonNull class member function
template <typename Alloc>
JsonNull<Alloc>::JsonNull() : Json<Alloc>::Json(JsonType::Null) {}