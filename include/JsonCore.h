#pragma once
#include "Utility.h"

struct BasicDynamicContainer {
    char_ptr pointer;
    size_t size;
    size_t length;
};

union BasicJSON {
    BasicDynamicContainer dynamic_container;
    long integer;
    double decimal;
    bool boolean;
};

enum class JsonType {
    Null, Boolean, Integer, Decimal, String, Array, Object
};

