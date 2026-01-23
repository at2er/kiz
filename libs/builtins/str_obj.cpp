#include "../../src/models/models.hpp"
#include "include/builtin_functions.hpp"

namespace model {

// String.__call__
model::Object* str_call(model::Object* self, const model::List* args) {
    std::string val = 
        args->val.empty()
        ? ""
        : builtin::get_one_arg(args)->to_string();

    return new model::String(val);
}

// String.__bool__
model::Object* str_bool(model::Object* self, const model::List* args) {
    const auto self_int = dynamic_cast<String*>(self);
    if (self_int->val.empty()) return new model::Bool(false);
    return new model::Bool(true);
}

// String.__add__：字符串拼接（self + 传入String，返回新String，不修改原对象）
model::Object* str_add(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (str_add)");
    assert(args->val.size() == 1 && "function String.add need 1 arg");
    
    auto self_str = dynamic_cast<String*>(self);
    assert(self_str != nullptr && "str_add must be called by String object");
    
    auto another_str = dynamic_cast<String*>(args->val[0]);
    assert(another_str != nullptr && "String.add only supports String type argument");
    
    // 拼接并返回新String
    return new String(self_str->val + another_str->val);
};

// String.__mul__：字符串重复n次（self * n，返回新String，n为非负整数）
model::Object* str_mul(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (str_mul)");
    assert(args->val.size() == 1 && "function String.mul need 1 arg");
    
    auto self_str = dynamic_cast<String*>(self);
    assert(self_str != nullptr && "str_mul must be called by String object");
    
    auto times_int = dynamic_cast<Int*>(args->val[0]);
    assert(times_int != nullptr && "String.mul only supports Int type argument");
    assert(times_int->val >= dep::BigInt(0) && "String.mul requires non-negative integer argument");
    
    std::string result;
    dep::BigInt times = times_int->val;
    for (dep::BigInt i = dep::BigInt(0); i < times; i+=dep::BigInt(1)) {
        result += self_str->val;
    }
    
    return new String(std::move(result));
};

// String.__eq__：判断两个字符串是否相等 self == x
model::Object* str_eq(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (str_eq)");
    assert(args->val.size() == 1 && "function String.eq need 1 arg");
    
    auto self_str = dynamic_cast<String*>(self);
    assert(self_str != nullptr && "str_eq must be called by String object");
    
    auto another_str = dynamic_cast<String*>(args->val[0]);
    assert(another_str != nullptr && "String.eq only supports String type argument");
    
    return new Bool(self_str->val == another_str->val);
};

// String.__contains__：判断是否包含子字符串 x in self
model::Object* str_contains(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (str_contains)");
    assert(args->val.size() == 1 && "function String.contains need 1 arg");
    
    auto self_str = dynamic_cast<String*>(self);
    assert(self_str != nullptr && "str_contains must be called by String object");
    
    auto sub_str = dynamic_cast<String*>(args->val[0]);
    assert(sub_str != nullptr && "String.contains only supports String type argument");
    
    bool exists = self_str->val.find(sub_str->val) != std::string::npos;
    return new Bool(exists);
};

}  // namespace model