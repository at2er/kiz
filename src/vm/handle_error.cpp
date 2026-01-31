#include <ranges>

#include "vm.hpp"
#include "builtins/include/builtin_functions.hpp"

namespace kiz {

// -------------------------- 异常处理 --------------------------
auto Vm::gen_pos_info() -> std::vector<std::pair<std::string, err::PositionInfo>> {
    size_t frame_index = 0;
    std::vector<std::pair<std::string, err::PositionInfo>> positions;
    std::string path;
    for (const auto& frame: call_stack) {
        if (const auto m = dynamic_cast<model::Module*>(frame->owner)) {
            path = m->path;
        }
        err::PositionInfo pos{};
        bool cond = frame_index == call_stack.size() - 1;
        DEBUG_OUTPUT("frame_index: " << frame_index << ", call_stack.size(): " << call_stack.size());
        if (cond) {
            pos = frame->code_object->code.at(frame->pc).pos;
        } else {
            pos = frame->code_object->code.at(frame->pc - 1).pos;
        }
        DEBUG_OUTPUT(
            "Vm::gen_pos_info, pos = col "
            << pos.col_start << ", " << pos.col_end << " | line "
            << pos.lno_start << ", " << pos.lno_end
        );
        positions.emplace_back(path, pos);
        ++frame_index;
    }
    return positions;
}

void Vm::instruction_throw(const std::string& name, const std::string& content) {
    const auto err_name = new model::String(name);
    const auto err_msg = new model::String(content);

    const auto err_obj = new model::Error();
    err_obj->positions = gen_pos_info();
    err_obj->attrs.insert("__name__", err_name);
    err_obj->attrs.insert("__msg__", err_msg);
    DEBUG_OUTPUT("err_obj pos size = "+std::to_string(err_obj->positions.size()));
    curr_error = err_obj;
    handle_throw();
}


// 辅助函数
std::pair<std::string, std::string> get_err_name_and_msg(const model::Object* err_obj) {
    assert(err_obj != nullptr);
    auto err_name_it = err_obj->attrs.find("__name__");
    auto err_msg_it = err_obj->attrs.find("__msg__");
    assert(err_name_it != nullptr);
    assert(err_msg_it != nullptr);
    auto err_name = err_name_it->value->debug_string();
    auto err_msg = err_msg_it->value->debug_string();
    return {err_name, err_msg};
}

void Vm::exec_ENTER_TRY(const Instruction& instruction) {
    assert(!call_stack.empty() && "exec_ENTER_TRY: 调用栈为空，无法执行ENTER_TRY指令");
    size_t catch_start = instruction.opn_list[0];
    size_t finally_start = instruction.opn_list[1];
    call_stack.back()->try_blocks.emplace_back(false, catch_start, finally_start);
}


void Vm::exec_LOAD_ERROR(const Instruction& instruction) {
    std::cout << "loading curr error " + curr_error->debug_string() << std::endl;
    std::cout << call_stack.back()->pc << std::endl;
    assert(curr_error != nullptr);
    op_stack.push(curr_error);
}

void Vm::exec_THROW(const Instruction& instruction) {
    DEBUG_OUTPUT("exec throw...");
    std::cout << "top " << op_stack.top()->debug_string() << std::endl;
    const auto top = op_stack.top();
    curr_error = top;
    op_stack.pop();

    handle_throw();
}

void Vm::exec_JUMP_IF_FINISH_HANDLE_ERROR(const Instruction& instruction) {
    assert(!call_stack.empty());
    assert(!call_stack.back()->try_blocks.empty());
    bool finish_handle_error = call_stack.back()->try_blocks.back().handle_error;
    // 弹出TryFrame
    call_stack.back()->try_blocks.pop_back();
    std::cout << "Jump if finish handle error: finish_handle_error = " << finish_handle_error << std::endl;
    if (finish_handle_error) {
        call_stack.back()->pc = instruction.opn_list[0];
    } else {
        call_stack.back()->pc ++;
    }
}

void Vm::exec_MARK_HANDLE_ERROR(const Instruction& instruction) {
    assert(!call_stack.empty());
    assert(!call_stack.back()->try_blocks.empty());
    call_stack.back()->try_blocks.back().handle_error = true;
}


void Vm::exec_IS_CHILD(const Instruction& instruction) {
    auto b = fetch_one_from_stack_top();
    auto a = fetch_one_from_stack_top();
    std::cout << "a is " << a->debug_string() << std::endl;
    std::cout << "b is " << b->debug_string() << std::endl;
    std::cout << "ischild(a,b)" << std::endl;
    op_stack.emplace(builtin::check_based_object(a, b));
}

void Vm::handle_throw() {
    assert(curr_error != nullptr);

    size_t frames_to_pop = 0;
    CallFrame* target_frame = nullptr;
    size_t target_pc = 0;

    // 逆序遍历调用栈，寻找最近的 try 块
    for (auto frame_it = call_stack.rbegin(); frame_it != call_stack.rend(); ++frame_it) {
        CallFrame* frame = (*frame_it).get();
        if (!frame->try_blocks.empty()) {
            target_frame = frame;
            auto try_frame = frame->try_blocks.back();
            std::cout << "try_frame.handle_error: " << try_frame.handle_error << std::endl;
            if (try_frame.handle_error) {
                // 该情况下, error从catch中抛出
                // 改为从 finally 开始执行
                target_pc = try_frame.finally_start;
                std::cout << "find finally pc!" << target_pc << std::endl;
                // 重置错误处理状态, 使其在finally后可以rethrow
                frame->try_blocks.back().handle_error = false;
                std::cout << "change try_frame.handle_error: " << frame->try_blocks.back().handle_error << std::endl;
            } else {
                target_pc = try_frame.catch_start;
                assert(target_pc != 0);
                std::cout << "find catch pc!" << target_pc << std::endl;
                break;
            }
        }
        frames_to_pop++;
    }

    // 如果找到有效的 try 块
    if (target_frame) {
        // 弹出多余的栈帧
        for (size_t i = 0; i < frames_to_pop; ++i) {
            call_stack.pop_back();
        }
        // 设置 pc
        target_frame->pc = target_pc;
        return;
    }

    auto [error_name, error_msg] = get_err_name_and_msg(curr_error);

    // 报错
    if (auto err_obj = dynamic_cast<model::Error*>(curr_error)) {
        std::cout << Color::BRIGHT_RED << "\nTrace Back: " << Color::RESET << std::endl;
        for (auto& [_path, _pos]: err_obj->positions ) {
            DEBUG_OUTPUT(_path + " " + std::to_string(_pos.lno_start) + " "
                + std::to_string(_pos.lno_end) + " " + std::to_string(_pos.col_start) + " " + std::to_string(_pos.col_end));
            err::context_printer(_path, _pos);
        }
    }

    // 错误信息（类型加粗红 + 内容白）
    std::cout << Color::BOLD << Color::BRIGHT_RED << error_name
              << Color::RESET << Color::WHITE << " : " << error_msg
              << Color::RESET << std::endl;
    std::cout << std::endl;

    throw KizStopRunningSignal();
}

}