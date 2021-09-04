#pragma once
#include "JsonCore.h"
#include "Utility.h"

namespace Jsoncpp {
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
        bool operator== (const Json& other) const;
        bool operator!=(const Json &other) const;
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
        bool operator==(const JsonString<Alloc> &other) const;
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
        bool operator==(const JsonArray<Alloc> &other) const;
    };

    //hash
    template <typename Alloc>
    struct JsonHash : public std::hash<std::string_view> {
        size_t operator()(const Json<Alloc>& j) const {
            switch (j.type) {
                case JsonType::Array:
                case JsonType::Object:
                case JsonType::String: {
                    return reinterpret_cast<const hash<std::string_view>*>(this)->operator()(std::string_view(j.json.dynamic_container.pointer, j.json.dynamic_container.length));
                }
                default: {
                    return j.json.integer;
                }
            }
        }
    };

    //JsonKeyValuePair class
    template <typename Alloc>
    struct JsonKeyValuePair {
        JsonString<Alloc> key;
        Json<Alloc> value;
        bool active = false;

        bool operator==(const JsonKeyValuePair<Alloc>& other) const {
            return key == other.key & value == other.value & active == other.active;
        }
    };

    //JsonObject class
    template <typename Alloc = std::allocator<char>>
    struct JsonObject : public  Json<Alloc> {
        static constexpr JsonHash<Alloc> hasher{};
        
        JsonObject(const size_t &num = 32);
        Json<Alloc>* at(const JsonString<Alloc> &key);
        Json<Alloc> &operator[](const JsonString<Alloc> &key);
        Json<Alloc> &operator[](JsonString<Alloc> &&key);
        template <typename K, typename V>
        std::enable_if_t<std::is_convertible_v<K, JsonString<Alloc>> && std::is_convertible_v<V, Json<Alloc>>> insert(K &&key, V &&value);
        bool rehash(const size_t& num);
        bool operator==(const JsonObject<Alloc> &other) const;
    };

    //JsonDecimal class
    template <typename Alloc = std::allocator<char>>
    struct JsonDecimal : public Json<Alloc> {
        JsonDecimal(const double &num = 0);
        bool operator==(const JsonDecimal<Alloc> &other) const;
    };

    //JsonInteger class
    template <typename Alloc = std::allocator<char>>
    struct JsonInteger : public Json<Alloc> {
        JsonInteger(const long &num = 0);
        bool operator==(const JsonInteger<Alloc> &other) const;
    };

    template <typename Alloc = std::allocator<char>>
    struct JsonBoolean : public Json<Alloc> {
        JsonBoolean(const bool &b);
        bool operator==(const JsonBoolean<Alloc> &other) const;
    };

    template <typename Alloc = std::allocator<char>>
    struct JsonNull : public Json<Alloc> {
        JsonNull();
        bool operator==(const JsonNull<Alloc> &other) const;
    };

    /*Functions--------------------------------------------------------------------------------------------------------------------------------*/

    //Json class member function

    template <typename Alloc>
    inline Json<Alloc>::Json(const Json &other)
        : type(other.type), allocator_object(alloc_traits::select_on_container_copy_construction(other.allocator_object)), json(other.json) {
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
    }

    template <typename Alloc>
    inline Json<Alloc>::Json(const JsonType& t)
    : type(t), allocator_object({}), json({}) {}

    template <typename Alloc>
    Json<Alloc>& Json<Alloc>::operator=(const Json& other) {
        if (this == &other)
        {
            return *this;
        }
        auto &dc = json.dynamic_container;
        alloc_traits::deallocate(allocator_object, dc.pointer, dc.size);
        if (alloc_propagate_c::value)
        {
            allocator_object = other.allocator_object;
        }
        type = other.type;
        auto &odc = other.json.dynamic_container;
        dc.length = odc.length;
        dc.size = odc.size;
        dc.pointer = alloc_traits::allocate(allocator_object, dc.size);
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
        return *this;
    }

    template <typename Alloc>
    Json<Alloc>& Json<Alloc>::operator=(Json&& other) noexcept {
        auto &dc = json.dynamic_container;
        if (type == JsonType::Array | type == JsonType::Object | type == JsonType::String){
            alloc_traits::deallocate(allocator_object, dc.pointer, dc.size);
        }
        type = other.type;
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
        if ((type == other.type) && (allocator_object == other.allocator_object)) {
            switch (type) {
                case JsonType::Array: {
                    return reinterpret_cast<const JsonArray<Alloc> *>(this)->operator==(*reinterpret_cast<const JsonArray<Alloc> *>(&other));
                }
                case JsonType::String: {
                    return reinterpret_cast<const JsonString<Alloc> *>(this)->operator==(*reinterpret_cast<const JsonString<Alloc> *>(&other));
                }
                case JsonType::Object: {
                    return reinterpret_cast<const JsonObject<Alloc> *>(this)->operator==(*reinterpret_cast<const JsonObject<Alloc> *>(&other));
                }
                case JsonType::Integer: {
                    return reinterpret_cast<const JsonInteger<Alloc> *>(this)->operator==(*reinterpret_cast<const JsonInteger<Alloc> *>(&other));
                }
                case JsonType::Decimal: {
                    return reinterpret_cast<const JsonDecimal<Alloc> *>(this)->operator==(*reinterpret_cast<const JsonDecimal<Alloc> *>(&other));
                }
                case JsonType::Boolean: {
                    return reinterpret_cast<const JsonBoolean<Alloc> *>(this)->operator==(*reinterpret_cast<const JsonBoolean<Alloc> *>(&other));
                }
                case JsonType::Null: {
                    return reinterpret_cast<const JsonNull<Alloc> *>(this)->operator==(*reinterpret_cast<const JsonNull<Alloc> *>(&other));
                }
            }
        }
        return false;
    }

    template <typename Alloc>
    inline bool Json<Alloc>::operator!=(const Json<Alloc>& other) const {
        return !(*this == other);
    }

    //JsonString class member function

    template <typename Alloc>
    inline JsonString<Alloc>::JsonString() : Json<Alloc>(JsonType::String) {}

    template <typename Alloc>
    template <typename Ptr>
    JsonString<Alloc>::JsonString(Ptr ptr) 
    : Json<Alloc>(JsonType::String) {
        static_assert(convertible_to_char_pointer<Ptr>);
        auto &dc = Json<Alloc>::json.dynamic_container;
        dc.length = std::strlen(ptr);
        dc.size = dc.length;
        dc.pointer = Json<Alloc>::alloc_traits::allocate(Json<Alloc>::allocator_object, dc.size);
        std::memcpy(dc.pointer, ptr, dc.size);
    }

    template <typename Alloc>
    template <typename Ptr>
    JsonString<Alloc>::JsonString(Ptr ptr, const size_t& l) 
    : Json<Alloc>(JsonType::String) {
        static_assert(convertible_to_char_pointer<Ptr>);
        auto &dc = Json<Alloc>::json.dynamic_container;
        dc.length = l;
        dc.size = l;
        dc.pointer = Json<Alloc>::alloc_traits::allocate(Json<Alloc>::allocator_object, dc.size);
        std::memcpy(dc.pointer, ptr, dc.size);
    }

    template <typename Alloc>
    inline size_t JsonString<Alloc>::length() const {
        return Json<Alloc>::json.dynamic_container.length;
    }

    template <typename Alloc>
    inline bool JsonString<Alloc>::operator==(const JsonString<Alloc>& other) const {
        auto &dc = Json<Alloc>::json.dynamic_container;
        auto &odc = other.json.dynamic_container;
        if (dc.length == odc.length && dc.size == odc.size) {
            return compare(dc.pointer, odc.pointer, dc.length);
        }
        return false;
    }

    //JsonArray class memeber function

    template <typename Alloc>
    inline JsonArray<Alloc>::JsonArray() : Json<Alloc>(JsonType::Array) {}

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
    inline const Json<Alloc>& JsonArray<Alloc>::operator[](const size_t& index) const {
        return reinterpret_cast<Json<Alloc> *>(Json<Alloc>::json.dynamic_container.pointer)[index];
    }

    template <typename Alloc>
    inline Json<Alloc>& JsonArray<Alloc>::operator[](const size_t& index) {
        return reinterpret_cast<Json<Alloc> *>(Json<Alloc>::json.dynamic_container.pointer)[index];
    }

    template <typename Alloc>
    inline size_t JsonArray<Alloc>::length() const {
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
        reinterpret_cast<Json<Alloc> *>(dc.pointer)[dc.length++] = std::forward<T>(element);
        for (size_t k = 0; k < dc.length; ++k) {
            auto &temp = reinterpret_cast<Json<Alloc> *>(dc.pointer)[k];
        }
    }

    template <typename Alloc>
    inline bool JsonArray<Alloc>::operator==(const JsonArray<Alloc>& other) const {
        auto &dc = Json<Alloc>::json.dynamic_container;
        auto &odc = other.json.dynamic_container;
        if (dc.length == odc.length && dc.size == odc.size) {
            return compare(reinterpret_cast<Json<Alloc>*>(dc.pointer), reinterpret_cast<Json<Alloc>*>(odc.pointer), dc.length);
        }
        return false;
    }

    //JsonObject class member function
    template <typename Alloc>
    JsonObject<Alloc>::JsonObject(const size_t &num) : Json<Alloc>(JsonType::Object) {
        auto &dc = Json<Alloc>::json.dynamic_container;
        dc.length = num;
        dc.size = num * sizeof(JsonKeyValuePair<Alloc>);
        dc.pointer = Json<Alloc>::alloc_traits::allocate(Json<Alloc>::allocator_object, dc.size);
        for (size_t k = 0; k < num; ++k) {
            reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer)[k].active = false;
        }
    }

    template <typename Alloc>
    Json<Alloc>* JsonObject<Alloc>::at(const JsonString<Alloc>& key) {
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

    template <typename Alloc>
    bool JsonObject<Alloc>::operator==(const JsonObject<Alloc>& other) const {
        auto &dc = Json<Alloc>::json.dynamic_container;
        auto &odc = other.json.dynamic_container;
        if (dc.length == odc.length) {
            auto data_ptr = reinterpret_cast<JsonKeyValuePair<Alloc> *>(dc.pointer);
            for (size_t k = 0; k < dc.length; ++k) {
                auto cur = data_ptr[k];
                if (cur.active)
                {
                    auto other_value = other.at(cur.key);
                    if (!other_value || (*other_value != cur.value)) {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }

    //JsonDecimal class member function
    template <typename Alloc>
    inline JsonDecimal<Alloc>::JsonDecimal(const double& num) : Json<Alloc>::Json(JsonType::Decimal) {
        Json<Alloc>::json.decimal = num;
    }

    template <typename Alloc>
    inline bool JsonDecimal<Alloc>::operator==(const JsonDecimal<Alloc>& other) const {
        return Json<Alloc>::json.decimal == other.json.decimal;
    }

    //JsonInteger class member function
    template <typename Alloc>
    inline JsonInteger<Alloc>::JsonInteger(const long& num) : Json<Alloc>::Json(JsonType::Integer) {
        Json<Alloc>::json.integer = num;
    }

    template <typename Alloc>
    inline bool JsonInteger<Alloc>::operator==(const JsonInteger<Alloc>& other) const {
        return Json<Alloc>::json.integer == other.json.integer;
    }

    //JsonBoolean class member function
    template <typename Alloc>
    inline JsonBoolean<Alloc>::JsonBoolean(const bool& b) : Json<Alloc>::Json(JsonType::Boolean) {
        Json<Alloc>::json.boolean = b;
    }

    template <typename Alloc>
    inline bool JsonBoolean<Alloc>::operator==(const JsonBoolean<Alloc>& other) const {
        return Json<Alloc>::json.boolean == other.json.boolean;
    }

    //JsonNull class member function
    template <typename Alloc>
    inline JsonNull<Alloc>::JsonNull() : Json<Alloc>::Json(JsonType::Null) {}

    template <typename Alloc>
    inline bool JsonNull<Alloc>::operator==(const JsonNull<Alloc>& other) const {
        return true;
    }
}
