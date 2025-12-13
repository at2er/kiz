# Kiz-lang v0.1.0
📌 **现状: 开发中...**

- 📚 文档完善
- 🪄 多范式兼容：支持OOP、FP等主流编程范式
- 🔅 语法极简：关键字集高度精简，仅包含：
```kiz
if else while break next
fn end object import
try catch throw 
nonlocal global 
is not or in and 
True Nil False
```
- ✅ 规范友好：中文注释+统一命名规范
- ✔️ 开发者友好：低门槛快速上手
- 📃 TODO: 
    ** 已完成的 **
    - ~~**fix** user function的调用问题~~ (感谢三文鱼)
    - ~~**fix** 修复Nil, False, True作为字面量出现的undefined var问题~~
    - ~~**feature** 完成list的IR生成~~
    - ~~**feature** 实现getattr~~
    - ~~**feature** 实现setattr~~
    - ~~**feature** 实现call method~~
    - ~~**feature** 完成 and not or in运算符(在vm中要支持判断model::Bool, 如果对象不是model::Bool, 需尝试调用Object.__bool__魔术方法)~~

    ** 近期的 **
    - **feature[急需的]** 添加对于运行时错误的报错器的TraceBack输出
    - **feature[急需的]** 实现完整oop语法(语句用法见examples/oop.kiz)
    - **feature[急需的]** 通过kiz::Position这个结构体来储存token, ast, instruction的位置信息
    - **fix[急需的]** if, while 语句的跳转问题
    - **fix[急需的]** 测试注释功能
    - **fix[急需的]** 测试nonlocal和global语句(语句用法见examples/onestop.kiz)

    - **fix** 完成所有builtin函数
    - **feature** 完成 >= <= (通过添加操作指令OP_GE, OP_LE)

    - **feature(maybe has big change)** 所有报错使用util::err_reporter函数代替现在临时的assert
    - **fix(maybe has big change)** 统一报错和DEBUG信息和输出信息为标准英文
    - **feature(maybe has big change)** Object->to_string改为Object的魔术方法(__str__和__repr__)

    ** 远期的 **
    - **feature** 添加import(语句形式:import "path"与import std_mod_name并存), 循环导入检查, std模块系统(在model::std_modules中注册)和用户模块系统
    - **feature** 完善builtins object的, __getitem__, __setitem__, __hash__ 这些魔术方法, 同时支持用户定义的魔术方法
    - **feature** 完成try-catch throw语句
