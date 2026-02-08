#include "lexer.hpp"

namespace kiz {

void Lexer::read_string() {
    dep::UTF8Char quote_char = src_[char_pos_];

    size_t start_char = char_pos_;
    size_t start_lno = lineno_;
    size_t start_col = col_;

    next(); // 跳过引号

    //bool closed = false;
    dep::UTF8String raw_str;

    // 消费字符串内容
    while (char_pos_ < src_.size()) {
        dep::UTF8Char c = src_[char_pos_];

        if (c == '\n') {
            break; // 普通字符串不允许跨行
        }

        if (c == quote_char) {
            //closed = true;
            next(); // 跳过闭合引号
            break;
        }

        if (c == '\\' && char_pos_ + 1 < src_.size()) {
            raw_str += c;
            next(); // 跳过转义符
            c = src_[char_pos_];
            raw_str += c;
            next(); // 消费转义后的字符
            continue;
        }

        raw_str += c;
        next();
    }

    std::string raw = raw_str.to_string();
    std::string content = handle_escape(raw);

    // if (!closed) {
    //     err::error_reporter(file_path_, {start_lno, lineno_, start_col, col_},
    //                       "SyntaxError", "Unclosed string literal");
    // }

    tokens_.emplace_back(TokenType::String, content, start_lno, lineno_, start_col, col_ - 1);
    curr_state_ = LexState::Start;
}


void Lexer::read_mstring() {
    size_t start_char = char_pos_;
    size_t start_lno = lineno_;
    size_t start_col = col_;

    next(); // 跳过M/m
    next(); // 跳过开头的双引号"

    bool unclosed = true;
    dep::UTF8String raw_str;

    // 消费字符串内容
    while (char_pos_ < src_.size()) {
        dep::UTF8Char c = src_[char_pos_];

        // 处理转义符
        if (c == '\\' && char_pos_ + 1 < src_.size()) {
            raw_str += c;
            next(); // 跳过转义符\
            c = src_[char_pos_];
            raw_str += c;
            next(); // 消费转义后的字符
            continue;
        }

        // 匹配非转义的闭合引号
        if (c == '"') {
            unclosed = false;
            next(); // 跳过闭合引号
            break;
        }

        raw_str += c;
        next(); // 消费字符
    }

    std::string raw = raw_str.to_string();
    std::string content = handle_escape(raw);

    if (unclosed) {
        err::error_reporter(file_path_, {start_lno, lineno_, start_col, col_},
                          "SyntaxError", R"(Unclosed multiline string literal (m"): missing closing '"')");
    }

    tokens_.emplace_back(TokenType::String, content, start_lno, lineno_, start_col, col_ - 1);
    curr_state_ = LexState::Start;
}

void Lexer::read_fstring() {

}

}
