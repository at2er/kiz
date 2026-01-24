#include "../models/models.hpp"
#include "../libs/math/include/math_lib.hpp"

namespace kiz {

void Vm::entry_std_modules() {
    std_modules.insert("math", new model::NativeFunction(
        math_lib::_init_module_
    ));
}

} // namespace model