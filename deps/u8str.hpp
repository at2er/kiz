#pragma once
#include <string>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace dep {

// UTF-8 字符串类
class u8str {
    std::string data_;

    // 辅助函数：获取 UTF-8 字符的字节长度
    static size_t utf8_char_len(char c) noexcept {
        if ((c & 0x80) == 0) return 1;
        if ((c & 0xE0) == 0xC0) return 2;
        if ((c & 0xF0) == 0xE0) return 3;
        if ((c & 0xF8) == 0xF0) return 4;
        return 1; // 非法字符按单字节处理
    }

    // 辅助函数：从指定位置解析 Unicode 码点
    static char32_t utf8_to_codepoint(const char*& ptr) {
        size_t len = utf8_char_len(*ptr);
        char32_t cp = 0;

        switch (len) {
            case 1: cp = static_cast<uint8_t>(*ptr++); break;
            case 2:
                cp = (static_cast<uint8_t>(*ptr++) & 0x1F) << 6;
                cp |= static_cast<uint8_t>(*ptr++) & 0x3F;
                break;
            case 3:
                cp = (static_cast<uint8_t>(*ptr++) & 0x0F) << 12;
                cp |= (static_cast<uint8_t>(*ptr++) & 0x3F) << 6;
                cp |= static_cast<uint8_t>(*ptr++) & 0x3F;
                break;
            case 4:
                cp = (static_cast<uint8_t>(*ptr++) & 0x07) << 18;
                cp |= (static_cast<uint8_t>(*ptr++) & 0x3F) << 12;
                cp |= (static_cast<uint8_t>(*ptr++) & 0x3F) << 6;
                cp |= static_cast<uint8_t>(*ptr++) & 0x3F;
                break;
            default: ptr++; break;
        }
        return cp;
    }

    // 辅助函数：从 Unicode 码点计算 UTF-8 字节长度
    static size_t codepoint_utf8_len(char32_t cp) noexcept {
        if (cp <= 0x7F) return 1;
        if (cp <= 0x7FF) return 2;
        if (cp <= 0xFFFF) return 3;
        if (cp <= 0x10FFFF) return 4;
        return 1;
    }

public:
    // 嵌套迭代器类，遍历 Unicode 码点
    class iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = char32_t;
        using difference_type = std::ptrdiff_t;
        using pointer = const char32_t*;
        using reference = char32_t;

        iterator(const char* ptr, const u8str& str) noexcept : ptr_(ptr), str_(str) {}

        reference operator*() const {
            const char* p = ptr_;
            return utf8_to_codepoint(p);
        }

        iterator& operator++() {
            ptr_ += utf8_char_len(*ptr_);
            return *this;
        }

        iterator operator++(int) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        iterator& operator--() {
            do { --ptr_; } while ((ptr_ > str_.data_.data()) && ((*ptr_ & 0xC0) == 0x80));
            return *this;
        }

        iterator operator--(int) {
            iterator temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const iterator& other) const noexcept {
            return ptr_ == other.ptr_;
        }

        bool operator!=(const iterator& other) const noexcept {
            return !(*this == other);
        }

    private:
        const char* ptr_;
        const u8str& str_;
    };

    // 构造函数
    u8str() = default;
    u8str(const char* s) : data_(s ? s : "") {}
    u8str(const std::string& s) : data_(s) {}
    u8str(const u8str& other) = default;
    u8str(u8str&& other) noexcept = default;

    // 赋值运算符
    u8str& operator=(const u8str& other) = default;
    u8str& operator=(u8str&& other) noexcept = default;

    // 迭代器接口
    iterator begin() const noexcept { return iterator(data_.data(), *this); }
    iterator end() const noexcept { return iterator(data_.data() + data_.size(), *this); }

    // 成员函数：返回 Unicode 码点数量
    size_t len() const noexcept {
        size_t count = 0;
        const char* ptr = data_.data();
        const char* end = ptr + data_.size();
        while (ptr < end) {
            ptr += utf8_char_len(*ptr);
            count++;
        }
        return count;
    }

    // 成员函数：转为 C 风格字符串
    const char* to_cstr() const noexcept { return data_.c_str(); }

    // 成员函数：转为 std::string
    const std::string& to_cppstr() const noexcept { return data_; }

    // 成员函数：判断是否包含子串
    bool contains(const u8str& substr) const noexcept {
        return data_.find(substr.data_) != std::string::npos;
    }

    // 成员函数：判断是否以子串开头
    bool startswith(const u8str& prefix) const noexcept {
        if (prefix.data_.size() > data_.size()) return false;
        return data_.substr(0, prefix.data_.size()) == prefix.data_;
    }

    // 成员函数：判断是否以子串结尾
    bool endswith(const u8str& suffix) const noexcept {
        if (suffix.data_.size() > data_.size()) return false;
        return data_.substr(data_.size() - suffix.data_.size()) == suffix.data_;
    }

    // 运算符重载：[] 按 Unicode 码点索引
    char32_t operator[](size_t index) const {
        if (index >= len()) throw std::out_of_range("u8str index out of range");
        const char* ptr = data_.data();
        for (size_t i = 0; i < index; ++i) {
            ptr += utf8_char_len(*ptr);
        }
        return utf8_to_codepoint(ptr);
    }

    // 运算符重载：== 比较
    bool operator==(const u8str& other) const noexcept {
        return data_ == other.data_;
    }

    // 运算符重载：!= 比较
    bool operator!=(const u8str& other) const noexcept {
        return !(*this == other);
    }

    // 运算符重载：+ 拼接
    u8str operator+(const u8str& other) const {
        return u8str(data_ + other.data_);
    }

    // 运算符重载：* 重复
    u8str operator*(size_t n) const {
        std::string res;
        res.reserve(data_.size() * n);
        for (size_t i = 0; i < n; ++i) {
            res += data_;
        }
        return u8str(res);
    }
};

} // namespace dep

// 全局输出重载，支持打印 char32_t 码点（简单实现）
std::ostream& operator<<(std::ostream& os, char32_t cp) {
    // 转为 UTF-8 字节并输出
    char buf[4] = {0};
    size_t len = u8str::codepoint_utf8_len(cp);
    switch (len) {
        case 4: buf[3] = (cp & 0x3F) | 0x80; cp >>= 6;
        case 3: buf[2] = (cp & 0x3F) | 0x80; cp >>= 6;
        case 2: buf[1] = (cp & 0x3F) | 0x80; cp >>= 6;
        case 1: buf[0] = static_cast<char>(cp | (0xFF << (8 - len)));
    }
    os.write(buf, len);
    return os;
}