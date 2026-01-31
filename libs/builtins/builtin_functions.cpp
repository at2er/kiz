#include "include/builtin_functions.hpp"

#include <chrono>
#include <cstdint>

#include "../../src/models/models.hpp"
//#include "../deps/u8str.hpp"

namespace builtin {

model::Object* print(model::Object* self, const model::List* args) {
    //dep::u8str text;
    std::string text;
    for (const auto* arg : args->val) {
        text += arg->debug_string() + " ";
    }
    std::cout << text << std::endl;
    return new model::Nil();
}

model::Object* input(model::Object* self, const model::List* args) {
    if (! args->val.empty()) {
        const auto prompt_obj = get_one_arg(args);
        std::cout << prompt_obj->debug_string();
    }
    std::string result;
    std::getline(std::cin, result);
    return new model::String(result);
}

model::Object* ischild(model::Object* self, const model::List* args) {
    if (args->val.size() != 2) {
        assert(false && "函数参数不足两个");
    }

    const auto a = args->val[0];
    const auto b = args->val[1];
    return check_based_object(a, b);
    
}

model::Object* help(model::Object* self, const model::List* args) {
    const std::string text = R"(
The kiz help

Built-in Functions:
===========================
    print(...)
    input(prompt="")
    ischild(obj, for_check_obj)
    create(parent_obj=Object)
    breakpoint()
    help()
    range(start, step, end)
    cmd(command)
    now()
    type_of(obj)
    setattr(obj, attr_name, value)
    getattr(current_only=False, obj, attr_name, default_value)
    hasattr(current_only=False, obj, attr_name)
    delattr(obj, attr_name)
    get_refc(obj)

Built-in Objects:
===========================
    Object
    Int
    Dec
    Str
    List
    Dict
    Bool
    Func
    NFunc
    Error
    Module
    __CodeObject
    __Rational
    __Nil
)";
    std::cout << text;
    return new model::Nil();
}

model::Object* breakpoint(model::Object* self, const model::List* args) {
    size_t i = 0;
    for (auto& frame: kiz::Vm::call_stack) {
        std::cout << "Frame [" << i << "] " << frame->name << "\n";
        std::cout << "=================================" << "\n";
        std::cout << "Owner: " << frame->owner->debug_string() << "\n";
        std::cout << "Pc: " << frame->pc << "\n";

        std::cout << "Locals: " << "\n";
        size_t j = 1;
        auto locals_vector = frame->locals.to_vector();
        for (const auto& [n, l] : locals_vector) {
            std::cout << n << " = " << l->debug_string();
            if (j<locals_vector.size()) std::cout << ", ";
            ++j;
        }
        
        std::cout << "\n";
        std::cout << "Names: ";
        j = 1;
        for (const auto& n: frame->code_object->names) {
            std::cout << n;
            if (j<frame->code_object->names.size()) std::cout << ", ";
            ++j;
        }

        std::cout << "\n";
        std::cout << "Consts: ";
        j = 1;
        for (const auto c: frame->code_object->consts) {
            std::cout << c;
            if (j<frame->code_object->consts.size()) std::cout << ", ";
            ++j;
        }
        std::cout << "\n\n";
        ++i;
    }
    std::cout << "continue to run? (Y/[N])";
    std::string input;
    std::getline(std::cin, input);
    if (input == "Y") {
        return new model::Nil();
    }
    throw KizStopRunningSignal();
}

model::Object* cmd(model::Object* self, const model::List* args) {
    auto arg_vector = args->val;
    if (arg_vector.empty()) {
        return new model::Nil();
    }
    system(args[0].debug_string().c_str());
    return new model::Nil();
}

model::Object* now(model::Object* self, const model::List* args) {
    auto now =
        std::chrono::high_resolution_clock::now()
        .time_since_epoch();
    int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    return new model::Int( dep::BigInt(std::to_string(time)) );
}

model::Object* range(model::Object* self, const model::List* args) {
    auto arg_vector = args->val;
    std::vector<model::Object*> range_vector;
    dep::BigInt start_int = 0;
    dep::BigInt step_int = 1;
    dep::BigInt end_int = 1;

    if (arg_vector.size() == 1) {
        auto end_obj = arg_vector[0];
        auto end_int_obj = dynamic_cast<model::Int*>(end_obj);
        assert(end_int_obj != nullptr);
        end_int = end_int_obj->val;
    }
    else if (arg_vector.size() == 2) {
        auto start_obj = arg_vector[0];
        auto start_int_obj = dynamic_cast<model::Int*>(start_obj);
        assert(start_int_obj != nullptr);
        start_int = start_int_obj->val;

        auto end_obj = arg_vector[1];
        auto end_int_obj = dynamic_cast<model::Int*>(end_obj);
        assert(end_int_obj != nullptr);
        end_int = end_int_obj->val;
    }
    else if (arg_vector.size() == 3) {
        auto start_obj = arg_vector[0];
        auto start_int_obj = dynamic_cast<model::Int*>(start_obj);
        assert(start_int_obj != nullptr);
        start_int = start_int_obj->val;

        auto step_obj = arg_vector[1];
        auto step_int_obj = dynamic_cast<model::Int*>(step_obj);
        assert(step_int_obj != nullptr);
        step_int = step_int_obj->val;

        auto end_obj = arg_vector[2];
        auto end_int_obj = dynamic_cast<model::Int*>(end_obj);
        assert(end_int_obj != nullptr);
        end_int = end_int_obj->val;
    } else return new model::Nil();

    for (dep::BigInt i = start_int; i < end_int; i+=step_int) {
        auto i_obj = new model::Int(i);
        range_vector.emplace_back(i_obj);
    }
    return new model::List(range_vector);
}

model::Object* setattr(model::Object* self, const model::List* args) {
    auto arg_vector = args->val;
    if (arg_vector.size() != 3) {
        return new model::Nil();
    }
    auto for_set = arg_vector[0];
    auto attr_name = arg_vector[1];
    auto value = arg_vector[2];
    for_set->attrs.insert(attr_name->debug_string(), value);
    return new model::Nil();
}

model::Object* getattr(model::Object* self, const model::List* args) {
    auto arg_vector = args->val;
    if (arg_vector.size() == 1) {
        return new model::Nil();
    }
    model::Object* obj;
    model::Object* attr_name;
    model::Object* default_value = new model::Nil();
    if (arg_vector.size() == 2 or arg_vector.size() == 3) {
        obj = arg_vector[0];
        attr_name = arg_vector[1];
        if (arg_vector.size() == 3) {
            default_value = arg_vector[2];
        }
        try {
            return kiz::Vm::get_attr(obj, attr_name->debug_string());
        } catch (...) {
            return default_value;
        }
    }
    if (arg_vector.size() == 4) {
        model::Object* current_only = arg_vector[0];
        obj = arg_vector[1];
        attr_name = arg_vector[2];
        default_value = arg_vector[3];
        if (kiz::Vm::is_true(current_only)) {
            if (const auto value =
                obj->attrs.find(attr_name->debug_string())
            ) return value->value;
            return default_value;
        }

        try {
            return kiz::Vm::get_attr(obj, attr_name->debug_string());
        } catch (...) {
            return default_value;
        }

    }
    return new model::Nil();
}

model::Object* delattr(model::Object* self, const model::List* args) {
    auto arg_vector = args->val;
    if (arg_vector.size() == 1) {
        return new model::Nil();
    }
    model::Object* obj = arg_vector[0];
    model::Object* attr_name = arg_vector[1];
    obj->attrs.del(attr_name->debug_string());
    return new model::Nil();
}

model::Object* hasattr(model::Object* self, const model::List* args) {
    auto arg_vector = args->val;
    if (arg_vector.size() == 1) {
        return new model::Nil();
    }
    model::Object* obj;
    model::Object* attr_name;
    if (arg_vector.size() == 2) {
        obj = arg_vector[0];
        attr_name = arg_vector[1];

        try {
            kiz::Vm::get_attr(obj, attr_name->debug_string());
            return new model::Bool(true);
        } catch (...) {
            return new model::Bool(false);
        }
    }
    if (arg_vector.size() == 3) {
        model::Object* current_only = arg_vector[0];
        obj = arg_vector[1];
        attr_name = arg_vector[2];
        if (kiz::Vm::is_true(current_only)) {
            if (const auto value =
                obj->attrs.find(attr_name->debug_string())
            ) return new model::Bool(true);
            return new model::Bool(false);
        }
        try {
            kiz::Vm::get_attr(obj, attr_name->debug_string());
            return new model::Bool(true);
        } catch (...) {
            return new model::Bool(false);
        }
    }
    return new model::Nil();
}

model::Object* get_refc(model::Object* self, const model::List* args) {
    const auto obj = get_one_arg(args);
    return new model::Int( dep::BigInt(obj->get_refc_()) );
}

model::Object* create(model::Object* self, const model::List* args) {
    if (args->val.empty()) {
        auto o = new model::Object();
        o->attrs.insert("__parent__", model::based_obj);
        return o;
    }
    const auto obj = get_one_arg(args);
    const auto new_obj = new model::Object();
    new_obj->attrs.insert("__parent__", obj);
    return new_obj;

}

model::Object* type_of_obj(model::Object* self, const model::List* args) {
    auto for_check = get_one_arg(args);
    std::string type_str;
    switch (for_check->get_type()) {
        case model::Object::ObjectType::OT_Bool: type_str = "Bool"; break;
        case model::Object::ObjectType::OT_Int: type_str = "Int"; break;
        case model::Object::ObjectType::OT_Rational: type_str = "__Rational"; break;
        case model::Object::ObjectType::OT_String: type_str = "Str"; break;
        case model::Object::ObjectType::OT_Object: type_str = "Object"; break;
        case model::Object::ObjectType::OT_Nil: type_str = "__Nil"; break;
        case model::Object::ObjectType::OT_Error: type_str = "Error"; break;
        case model::Object::ObjectType::OT_Function: type_str = "Func"; break;
        case model::Object::ObjectType::OT_List: type_str = "List"; break;
        case model::Object::ObjectType::OT_Dictionary: type_str = "Dict"; break;
        case model::Object::ObjectType::OT_Decimal: type_str = "Decimal"; break;
        case model::Object::ObjectType::OT_CodeObject: type_str = "__CodeObject"; break;
        case model::Object::ObjectType::OT_CppFunction: type_str = "NFunc"; break;
        case model::Object::ObjectType::OT_Module: type_str = "Module"; break;
        default: type_str = "<Unknown>"; break;
    }
    return new model::String(type_str);
}

}
