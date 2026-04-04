// utf8.cpp - Edge case fixture for UTF-8 character handling
// Contains valid C++ with Chinese characters, emoji, and special Unicode
// Expected: codelint should handle UTF-8 without crashing

#include <iostream>
#include <string>
#include <vector>

// 中文注释：这是测试UTF-8处理的注释
// 🎯 测试emoji在代码中的处理

class 用户管理器 {
public:
    // 中文字符串作为变量名
    std::string 名称{"默认用户"};
    int 年龄{25};

    // 带中文的函数名测试
    void 获取用户信息() {
        std::cout << "用户: " << 名称 << std::endl;
    }
};

// 测试特殊字符在字符串中的处理
const char* emoji_test = "🎉🎊✨";
const char* chinese_test = "中文测试";

// 测试韩文
class 사용자관리자 {
public:
    std::string 이름{"사용자"};
};

// 测试日文
class ユーザー管理 {
public:
    std::string 名前{"テスト"};
};

int main() {
    用户管理器 mgr;
    mgr.获取用户信息();

    std::cout << emoji_test << std::endl;
    std::cout << chinese_test << std::endl;

    return 0;
}
