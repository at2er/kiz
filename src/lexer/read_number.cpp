#include "lexer.hpp"

namespace kiz {
void Lexer::read_num() {
    size_t start_char = char_pos_;
    size_t start_lno = lineno_;
    size_t start_col = col_;
    bool has_dot = false;
    bool has_sci = false;

    dep::UTF8String num_str;

    // 消费第一个数字或点
    num_str += src_[char_pos_];
    if (src_[char_pos_] == '.') {
        has_dot = true;
    }
    next();

    // 消费数字
    while (char_pos_ < src_.size()) {
        dep::UTF8Char c = src_[char_pos_];

        if (c.is_digit()) {
            num_str += c;
            next();
        }
        else if (c == '.' && !has_dot && !has_sci) {
            // 检查下一个字符是否为数字
            if (char_pos_ + 1 < src_.size() && src_[char_pos_ + 1].is_digit()) {
                has_dot = true;
                num_str += c;
                next();
            } else {
                break;
            }
        }
        else if ((c == 'e' || c == 'E') && !has_sci) {
            // 科学计数法
            has_sci = true;
            num_str += c;
            next();

            // 处理科学计数法的正负号
            if (char_pos_ < src_.size()) {
                dep::UTF8Char next_c = src_[char_pos_];
                if (next_c == '+' || next_c == '-') {
                    num_str += next_c;
                    next();
                }
            }

            // 科学计数法后必须跟数字
            if (char_pos_ >= src_.size() || !src_[char_pos_].is_digit()) {
                break;
            }
        }
        else {
            break;
        }
    }

    // 判定类型
    TokenType type = (has_sci || has_dot) ? TokenType::Decimal : TokenType::Number;
    emit_token(type, start_char, char_pos_, start_lno, start_col, lineno_, col_ - 1);
    curr_state_ = LexState::Start;
}
}