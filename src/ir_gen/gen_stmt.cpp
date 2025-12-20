#include "kiz.hpp"
#include "opcode.hpp"
#include "../../include/ir_gen.hpp"
#include "../../include/ast.hpp"
#include "../../include/models.hpp"

namespace kiz {

model::Module* IRGenerator::gen_mod(
    const std::string& module_name,
    const std::vector<std::string>& names,
    const std::vector<Instruction>& code_list,
    const std::vector<model::Object*>& consts,
    const std::vector<std::tuple<size_t, size_t>>& lineno_map
) {
    const auto code_obj = new model::CodeObject(
        code_list,
        consts,
        names,
        lineno_map
    );
    code_obj->make_ref();
    DEBUG_OUTPUT("code object created with code list len " + std::to_string(code_list.size()));
    assert(code_obj != nullptr);
    const auto module_obj = new model::Module(
        module_name,
        code_obj
    );
    module_obj->make_ref();
    return module_obj;
}


// 处理代码块（生成块内所有语句的IR）
void IRGenerator::gen_block(const BlockStmt* block) {
    if (!block) return;
    for (auto& stmt : block->statements) {
        switch (stmt->ast_type) {
            case AstType::AssignStmt: {
                // 变量声明：生成初始化表达式IR + 存储变量指令
                const auto* var_decl = dynamic_cast<AssignStmt*>(stmt.get());
                gen_expr(var_decl->expr.get()); // 生成初始化表达式IR
                const size_t name_idx = get_or_add_name(curr_names, var_decl->name);

                curr_code_list.emplace_back(
                    Opcode::SET_LOCAL,
                    std::vector<size_t>{name_idx}
                );
                break;
            }
            case AstType::NonlocalAssignStmt: {
                // 变量声明：生成初始化表达式IR + 存储变量指令
                const auto* var_decl = dynamic_cast<NonlocalAssignStmt*>(stmt.get());
                gen_expr(var_decl->expr.get()); // 生成初始化表达式IR
                const size_t name_idx = get_or_add_name(curr_names, var_decl->name);

                curr_code_list.emplace_back(
                    Opcode::SET_NONLOCAL,
                    std::vector<size_t>{name_idx}
                );
                break;
            }

            case AstType::GlobalAssignStmt: {
                // 变量声明：生成初始化表达式IR + 存储变量指令
                const auto* var_decl = dynamic_cast<GlobalAssignStmt*>(stmt.get());
                gen_expr(var_decl->expr.get()); // 生成初始化表达式IR
                const size_t name_idx = get_or_add_name(curr_names, var_decl->name);

                curr_code_list.emplace_back(
                    Opcode::SET_GLOBAL,
                    std::vector<size_t>{name_idx}
                );
                break;
            }
            case AstType::ExprStmt: {
                // 表达式语句：生成表达式IR + 弹出结果（避免栈泄漏）
                auto* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get());
                gen_expr(expr_stmt->expr.get());
                // curr_code_list.emplace_back(
                // Opcode::POP_TOP,
                // std::vector<size_t>{},
                //     stmt->start_ln,
                //     stmt->end_ln
                // );
                break;
            }
            case AstType::IfStmt:
                gen_if(dynamic_cast<IfStmt*>(stmt.get()));
                break;
            case AstType::WhileStmt:
                gen_while(dynamic_cast<WhileStmt*>(stmt.get()));
                break;
            case AstType::ReturnStmt: {
                // 返回语句：生成返回值表达式IR + RET指令
                auto* ret_stmt = dynamic_cast<ReturnStmt*>(stmt.get());
                if (ret_stmt->expr) {
                    gen_expr(ret_stmt->expr.get());
                } else {
                    // 无返回值时压入Nil常量
                    auto* nil = new model::Nil();
                    const size_t const_idx = get_or_add_const(curr_consts, nil);
                    curr_code_list.emplace_back(
                        Opcode::LOAD_CONST,
                        std::vector<size_t>{const_idx}
                    );
                }
                curr_code_list.emplace_back(
                    Opcode::RET,
                    std::vector<size_t>{}
                );
                break;
            }
            case AstType::BreakStmt: {
                // 修复：BreakStmt 跳转逻辑 - 从栈中获取JUMP_IF_FALSE指令索引，取其目标作为循环结束位置
                assert(block_stack.size() >= 2 && "BreakStmt: 无活跃循环块（需至少包含continue/break两个标记）");
                // 栈结构：top → break标记（JUMP_IF_FALSE指令索引），next → continue标记（循环入口）
                size_t jump_if_false_idx = block_stack.top(); // 获取JUMP_IF_FALSE指令的索引
                size_t loop_exit_idx = curr_code_list[jump_if_false_idx].opn_list[0]; // 循环结束位置
                curr_code_list.emplace_back(
                    Opcode::JUMP,
                    std::vector<size_t>{loop_exit_idx}
                );
                break;
            }
            case AstType::NextStmt: {
                // 修复：ContinueStmt 跳转逻辑 - 正确获取循环入口地址
                assert(block_stack.size() >= 2 && "NextStmt: 无活跃循环块（需至少包含continue/break两个标记）");
                // 临时弹出break标记，取continue标记，再恢复
                size_t break_mark = block_stack.top();
                block_stack.pop();
                size_t loop_entry_idx = block_stack.top(); // 循环入口（条件判断位置）
                block_stack.push(break_mark);

                curr_code_list.emplace_back(
                    Opcode::JUMP,
                    std::vector<size_t>{loop_entry_idx}
                );
                break;
            }
            case AstType::SetMemberStmt: {
                // 设置成员：生成对象表达式 -> 生成值表达式 -> 加载属性名 -> SET_ATTR指令
                const auto* set_mem = dynamic_cast<SetMemberStmt*>(stmt.get());
                const auto* get_mem = dynamic_cast<GetMemberExpr*>(set_mem->g_mem.get());
                assert(get_mem != nullptr);
                gen_expr(get_mem->father.get()); // 生成对象IR
                gen_expr(set_mem->val.get());   // 生成值IR

                size_t name_idx = get_or_add_name(curr_names, get_mem->child->name);
                curr_code_list.emplace_back(
                Opcode::SET_ATTR,
                    std::vector<size_t>{name_idx}
                );
                break;
            }
            default:
                assert(false && "gen_block: 未处理的语句类型");
        }
    }
}

void IRGenerator::gen_if(IfStmt* if_stmt) {
    assert(if_stmt && "gen_if: if节点为空");
    // 生成条件表达式IR
    gen_expr(if_stmt->condition.get());

    // 生成JUMP_IF_FALSE指令（目标先占位，后续填充）
    size_t jump_if_false_idx = curr_code_list.size();
    curr_code_list.emplace_back(
        Opcode::JUMP_IF_FALSE,
        std::vector<size_t>{0} // 占位目标索引
    );

    // 生成then块IR
    gen_block(if_stmt->thenBlock.get());

    // 生成JUMP指令（跳过else块，目标占位）
    size_t jump_else_idx = curr_code_list.size();
    curr_code_list.emplace_back(
        Opcode::JUMP,
        std::vector<size_t>{0} // 占位目标索引
    );

    // 填充JUMP_IF_FALSE的目标（else块开始位置）
    curr_code_list[jump_if_false_idx].opn_list[0] = curr_code_list.size();

    // 生成else块IR（存在则生成）
    if (if_stmt->elseBlock) {
        gen_block(if_stmt->elseBlock.get());
    }

    // 填充JUMP的目标（if-else结束位置）
    curr_code_list[jump_else_idx].opn_list[0] = curr_code_list.size();
}

void IRGenerator::gen_while(WhileStmt* while_stmt) {
    assert(while_stmt && "gen_while: while节点为空");
    // 记录循环入口（条件判断开始位置）→ continue跳这里
    size_t loop_entry_idx = curr_code_list.size();

    // 生成循环条件IR
    gen_expr(while_stmt->condition.get());

    // 生成JUMP_IF_FALSE指令（目标：循环结束位置，先占位）
    const size_t jump_if_false_idx = curr_code_list.size();
    curr_code_list.emplace_back(
        Opcode::JUMP_IF_FALSE,
        std::vector<size_t>{0} // 占位，后续填充为循环结束位置
    );

    // 修复：调整block_stack压入内容
    // 栈结构：先压continue标记（循环入口），再压break标记（JUMP_IF_FALSE指令索引）
    block_stack.push(loop_entry_idx);    // continue → 跳回循环入口
    block_stack.push(jump_if_false_idx); // break → 关联JUMP_IF_FALSE指令（后续取其目标）

    // 生成循环体IR
    gen_block(while_stmt->body.get());

    // 生成JUMP指令，跳回循环入口
    curr_code_list.emplace_back(
        Opcode::JUMP,
        std::vector<size_t>{loop_entry_idx}
    );

    // 填充JUMP_IF_FALSE的目标（循环结束位置 = 当前代码列表长度）
    size_t loop_exit_idx = curr_code_list.size();
    curr_code_list[jump_if_false_idx].opn_list[0] = loop_exit_idx;

    // 弹出循环栈帧（顺序与push相反）
    block_stack.pop(); // 弹出break标记
    block_stack.pop(); // 弹出continue标记
}

}
