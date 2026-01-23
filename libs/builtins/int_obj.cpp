#include "../../src/models/models.hpp"
#include "../../src/vm/vm.hpp"
#include "include/builtin_functions.hpp"

namespace model {

// Int.__call__
model::Object* int_call(model::Object* self, const model::List* args) {
    auto a = builtin::get_one_arg(args);
    dep::BigInt val(0);
    if (auto s = dynamic_cast<String*>(a)) val = dep::BigInt(s->val);
    else if (!kiz::Vm::is_true(a)) val = dep::BigInt(0);
    return new model::Int(val);
}

// Int.__bool__
model::Object* int_bool(model::Object* self, const model::List* args) {
    const auto self_int = dynamic_cast<Int*>(self);
    if (self_int->val == dep::BigInt(0)) return new model::Bool(false);
    return new model::Bool(true);
}

// Int.__add__ 整数加法：self + args[0]
model::Object* int_add(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments");
    assert(args->val.size() == 1 && "function Int.add need 1 arg");

    const auto self_int = dynamic_cast<Int*>(self);
    assert(self_int!=nullptr && "function Int.add need 1 arg typed Int");
    auto another_int = dynamic_cast<Int*>(args->val[0]);
    if (another_int) {
        return new Int(self_int->val + another_int->val);
    }
    auto another_rational = dynamic_cast<Rational*>(args->val[0]);
    if (another_rational) {
        const auto left_rational = dep::Rational(self_int->val,dep::BigInt(1));
        return new Rational(left_rational + another_rational->val);
    }
    assert(false && "function Int.add second arg need be Rational or Int");
};

// Int.__sub__ 整数减法：self - args[0]
model::Object* int_sub(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (int_sub)");
    assert(args->val.size() == 1 && "function Int.sub need 1 arg");
    
    auto self_int = dynamic_cast<Int*>(self);
    auto another_int = dynamic_cast<Int*>(args->val[0]);
    if (another_int) {
        return new Int(self_int->val - another_int->val);
    }
    auto another_rational = dynamic_cast<Rational*>(args->val[0]);
    if (another_rational) {
        const auto left_rational = dep::Rational(self_int->val,dep::BigInt(1));
        return new Rational(left_rational - another_rational->val);
    }
    assert(false && "function Int.sub second arg need be Rational or Int");
};

// Int.__mul__ 整数乘法：self * args[0]
model::Object* int_mul(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (int_mul)");
    assert(args->val.size() == 1 && "function Int.mul need 1 arg");
    
    auto self_int = dynamic_cast<Int*>(self);
    auto another_int = dynamic_cast<Int*>(args->val[0]);
    if (another_int) {
        return new Int(self_int->val * another_int->val);
    }
    auto another_rational = dynamic_cast<Rational*>(args->val[0]);
    if (another_rational) {
        const auto left_rational = dep::Rational(self_int->val,dep::BigInt(1));
        return new Rational(left_rational * another_rational->val);
    }
    assert(false && "function Int.mul second arg need be Rational or Int");
};

// Int.__div__ 整数除法 self / args[0]
model::Object* int_div(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (int_div)");
    assert(args->val.size() == 1 && "function Int.div need 1 arg");

    auto self_int = dynamic_cast<Int*>(self);
    auto another_int = dynamic_cast<Int*>(args->val[0]);
    if (another_int) {
        return new Rational(operator/(self_int->val , another_int->val));
    }
    auto another_rational = dynamic_cast<Rational*>(args->val[0]);
    if (another_rational) {
        const auto left_rational = dep::Rational(self_int->val,dep::BigInt(1));
        return new Rational(left_rational / another_rational->val);
    }
    assert(false && "function Int.div second arg need be Rational or Int");
};

// Int.__pow__ 整数幂运算：self ^ args[0]（self的args[0]次方）
model::Object* int_pow(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (int_pow)");
    assert(args->val.size() == 1 && "function Int.pow need 1 arg");
    
    auto self_int = dynamic_cast<Int*>(self);
    auto exp_int = dynamic_cast<Int*>(args->val[0]);
    return new Int(self_int->val.pow(exp_int->val));
};

// Int.__mod__ 整数取模：self % args[0]（余数与除数同号）
model::Object* int_mod(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (int_mod)");
    assert(args->val.size() == 1 && "function Int.mod need 1 arg");
    assert(dynamic_cast<Int*>(args->val[0])->val != dep::BigInt(0) && "mod by zero");
    
    auto self_int = dynamic_cast<Int*>(self);
    auto another_int = dynamic_cast<Int*>(args->val[0]);
    dep::BigInt remainder = self_int->val % another_int->val;
    // 修正余数符号（确保与除数同号）
    if (remainder != dep::BigInt(0)
        and self_int->val < dep::BigInt(0) != another_int->val < dep::BigInt(0)
    ) {
        remainder += another_int->val;
    }
    return new Int(dep::BigInt(remainder));
};

// Int.__eq__ 相等判断：self == args[0]（返回Bool对象）
model::Object* int_eq(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (int_eq)");
    assert(args->val.size() == 1 && "function Int.eq need 1 arg");
    
    auto self_int = dynamic_cast<Int*>(self);
    auto another_int = dynamic_cast<Int*>(args->val[0]);
    if (another_int) {
        return new Bool(self_int->val == another_int->val);
    }
    auto another_rational = dynamic_cast<Rational*>(args->val[0]);
    if (another_rational) {
        const auto left_rational = dep::Rational(self_int->val,dep::BigInt(1));
        return new Bool(left_rational == another_rational->val);
    }
    assert(false && "function Int.eq second arg need be Rational or Int");
};

// Int.__lt__ 小于判断：self < args[0]（返回Bool对象）
model::Object* int_lt(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (int_lt)");
    assert(args->val.size() == 1 && "function Int.lt need 1 arg");
    
    auto self_int = dynamic_cast<Int*>(self);
    auto another_int = dynamic_cast<Int*>(args->val[0]);
    if (another_int) {
        return new Bool(self_int->val < another_int->val);
    }
    auto another_rational = dynamic_cast<Rational*>(args->val[0]);
    if (another_rational) {
        const auto left_rational = dep::Rational(self_int->val,dep::BigInt(1));
        return new Bool(left_rational < another_rational->val);
    }
    assert(false && "function Int.lt second arg need be Rational or Int");
};

// Int.__gt__ 大于判断：self > args[0]（返回Bool对象）
model::Object* int_gt(model::Object* self, const model::List* args) {
    DEBUG_OUTPUT("You given " + std::to_string(args->val.size()) + " arguments (int_gt)");
    assert(args->val.size() == 1 && "function Int.gt need 1 arg");
    
    auto self_int = dynamic_cast<Int*>(self);
    auto another_int = dynamic_cast<Int*>(args->val[0]);
    if (another_int) {
        return new Bool(self_int->val > another_int->val);
    }
    auto another_rational = dynamic_cast<Rational*>(args->val[0]);
    if (another_rational) {
        const auto left_rational = dep::Rational(self_int->val,dep::BigInt(1));
        return new Bool(left_rational > another_rational->val);
    }
    assert(false && "function Int.gt second arg need be Rational or Int");
};

}  // namespace model