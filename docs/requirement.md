# codelint 功能表

## 支持的子命令
codelint 支持如下的子命令:
1. check_init: 主要检查初始化相关的问题
2. find_global: 查找全局变量
3. find_singleton: 查找单例模式

## 支持的选项

codelint 支持的选项如下:
* `-p`:指定 `compile_commands.json` 的目录
* `--version`: 输出版本信息,采取通用做法. 输出的内容里面包含LLVM的版本信息
* `--help`: 通用的帮助信息, 文档要适合AI和人类阅读
* `--output_json`: 输出json格式的内容
* `--scope`: 指定检查范围, 默认是文件级别, 但是可以选择改动范围,或者git commit, 因为这个工具会被用于CI集成, 用来检查PR中是否有违反规则

## 子功能概述

`codelint check_init` 命令支持如下独立选项:
* `--fix`:输出修复后的内容, 默认输出到命令行(整个文件级别的输出)
* `--inplace`:原地修改, 也就是修改源文件

该命令的功能主要是:
1. 检查未初始化的变量, 警告级别为严重
2. 按照部门要求, 将 `=` 号赋值改为 `{}`赋值, 注意里面的特殊场景
3. 检查变量是否可以加const/constexpr修饰, 该功能可以用 `--supress-constant`禁用(主要是为了回归测试可以通过)
