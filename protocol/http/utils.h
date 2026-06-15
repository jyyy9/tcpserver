#pragma once
/*
    实现一些HTTP请求或响应处理的中的小的零碎功能方法
        1. 字符串分割函数：
            // src-原始字符串， sep-间隔符；  dst: 目标数据，分割结果放在其中
            // 默认不获取空数据:  a,b,c,,,,d,e;  
            size_t split(const std::string &src, const std::string &sep, std::vector<std::string> &dst,);
        2. URL编码：
            //src-编码前的字符串；  dst：编码后的字符串； convert_space_to_plus-是否转换空格-用于查询字符串
            bool urlEncode(const std::string &src, std::string &dst, bool convert_space_to_plus);
        3. URL解码：
            //src-解码前的字符串；  dst：解码后的字符串； convert_space_to_plus-是否转换空格-用于查询字符串
            bool urlDecode(const std::string &src, std::string &dst, bool convert_space_to_plus);
*/
#include <iostream>
#include <string>
#include <vector>

namespace proto {
namespace http {
    // src-原始字符串， sep-间隔符；  dst: 目标数据，分割结果放在其中
    // 默认不获取空数据:  abc,bcd,cde,,,d,e;  
    static inline size_t split(const std::string &src, 
        const std::string &sep, 
        std::vector<std::string> &dst) {
        dst.clear();
        size_t pos = 0;
        size_t lastPos = 0;
        while ((pos = src.find(sep, lastPos)) != std::string::npos) {
            //有可能存在一种情况： pos == lastPos
            if (pos == lastPos) {
                lastPos = pos + sep.size();
                continue;
            }
            dst.push_back(src.substr(lastPos, pos - lastPos));
            lastPos = pos + sep.size();
        }
        if (lastPos < src.size()) {
            dst.push_back(src.substr(lastPos));
        }
        return dst.size();
    }

    //src-编码前的字符串；  dst：编码后的字符串； convert_space_to_plus-是否转换空格-用于查询字符串
    static inline bool urlEncode(const std::string &src, 
            std::string &dst, 
            bool convert_space_to_plus) {
            // 清空目标字符串
            dst.clear();
            // 预分配内存以提高性能（通常编码后长度会增加）
            dst.reserve(src.length() * 3);
            // 需要编码的字符集（RFC 3986 保留字符）
            const std::string reserved_chars = "!*'();:@&=+$,/?#[] ";
            for (unsigned char c : src) {
                // 处理空格
                if (c == ' ') {
                    if (convert_space_to_plus) {
                        dst += '+';
                    } else {
                        // 将空格编码为 %20
                        dst += "%20";
                    }
                    continue;
                }
                // 检查是否需要编码
                // 字母、数字、连字符、下划线、点、波浪线不需要编码
                if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                    dst += c;
                } 
                // 其他字符都需要编码
                else {
                    // 转换为十六进制表示
                    char hex[4];
                    snprintf(hex, sizeof(hex), "%%%02X", c);
                    dst += hex;
                }
            }
            return true;
        }

    //src-解码前的字符串；  dst：解码后的字符串； convert_space_to_plus-是否转换空格-用于查询字符串
    static inline bool urlDecode(const std::string &src, 
        std::string &dst, 
        bool convert_plus_to_space) {
        // 清空目标字符串
        dst.clear();
        // 预分配内存
        dst.reserve(src.length());
        for (size_t i = 0; i < src.length(); ++i) {
            unsigned char c = src[i];
            // 处理加号（如果配置为转换为空格）
            if (c == '+' && convert_plus_to_space) {
                dst += ' ';
                continue;
            }
            // 处理百分号编码
            if (c == '%') {
                // 检查是否有足够的字符进行解码（至少需要2个十六进制字符）
                if (i + 2 >= src.length()) {
                    // 无效的百分号编码
                    dst.clear();
                    return false;
                }
                // 获取两个十六进制字符
                char hex1 = src[i + 1];
                char hex2 = src[i + 2];
                // 检查字符是否为有效的十六进制字符
                if (!std::isxdigit(hex1) || !std::isxdigit(hex2)) {
                    // 无效的十六进制字符
                    dst.clear();
                    return false;
                }
                // 将十六进制转换为字节值
                unsigned char decoded = 0;
                // 处理第一个十六进制字符
                if (hex1 >= '0' && hex1 <= '9') {
                    decoded = (hex1 - '0') * 16;
                } else if (hex1 >= 'A' && hex1 <= 'F') {
                    decoded = (hex1 - 'A' + 10) * 16;
                } else if (hex1 >= 'a' && hex1 <= 'f') {
                    decoded = (hex1 - 'a' + 10) * 16;
                }
                
                // 处理第二个十六进制字符
                if (hex2 >= '0' && hex2 <= '9') {
                    decoded += (hex2 - '0');
                } else if (hex2 >= 'A' && hex2 <= 'F') {
                    decoded += (hex2 - 'A' + 10);
                } else if (hex2 >= 'a' && hex2 <= 'f') {
                    decoded += (hex2 - 'a' + 10);
                }
                // 添加解码后的字符
                dst += static_cast<char>(decoded);
                // 跳过已处理的两个十六进制字符
                i += 2;
                continue;
            }
            // 普通字符直接添加
            dst += c;
        }
        return true;
    }
    // URI中的path有效性检测： 是否符合RFC 3986规范 //bcd/abc/../index.html  /../a.txt
    //  /  == ./wwwroot/    /index.html  =>  ./wwwroot/../index.html
    static inline bool isPathValid(const std::string &path) {
        // 空路径视为有效（相当于根路径）
        if (path.empty()) {
            return true;
        }
        // 检查路径长度限制（通常URL路径不应超过8192字节，这里取一个合理值）
        if (path.length() > 8192) {
            return false;
        }
        // 检查路径是否以NULL字符开头
        if (path[0] == '\0') {
            return false;
        }
        bool last_was_slash = false;
        int consecutive_dots = 0;
        for (size_t i = 0; i < path.length(); ++i) {
            unsigned char c = path[i];
            // 检查非法字符（根据RFC 3986，路径中允许的字符）
            // 允许的字符：A-Z a-z 0-9 - _ . ~ ! $ & ' ( ) * + , ; = : @ /
            // 以及百分号编码的字符（%XX）
            bool is_allowed = false;
            // 字母数字
            if (std::isalnum(c)) {
                is_allowed = true;
            }
            // 特殊字符
            else if (c == '-' || c == '_' || c == '.' || c == '~' || 
                    c == '!' || c == '$' || c == '&' || c == '\'' || 
                    c == '(' || c == ')' || c == '*' || c == '+' || 
                    c == ',' || c == ';' || c == '=' || c == ':' || 
                    c == '@' || c == '/') {
                is_allowed = true;
            }
            // 百分号编码开始
            else if (c == '%') {
                // 检查百分号编码是否有效
                if (i + 2 >= path.length()) {
                    return false; // 不完整的百分号编码
                }
                char hex1 = path[i + 1];
                char hex2 = path[i + 2];
                if (!std::isxdigit(hex1) || !std::isxdigit(hex2)) {
                    return false; // 无效的十六进制字符
                }
                // 跳过已检查的两个字符
                i += 2;
                is_allowed = true;
            }
            if (!is_allowed) {
                return false;
            }
            // 检查路径遍历攻击（防止 ../ 或 ..\）
            if (c == '/') {
                // 检查连续的斜杠（某些服务器可能允许，但通常应避免）
                if (last_was_slash) {
                    // 多个连续斜杠通常被视为有效，但这里可以选择拒绝
                    // 为了更严格的验证，可以取消下面的注释
                    // return false;
                }
                last_was_slash = true;
                consecutive_dots = 0;
            } else if (c == '.') {
                if (last_was_slash) {
                    consecutive_dots++;
                    // 检查路径遍历模式：/.. 或 /.
                    if (consecutive_dots == 2) {
                        // 发现 /.. 路径遍历尝试
                        return false;
                    }
                } else if (consecutive_dots > 0) {
                    consecutive_dots++;
                    if (consecutive_dots > 2) {
                        // 超过两个点
                        return false;
                    }
                }
            } else {
                last_was_slash = false;
                consecutive_dots = 0;
            }
            // 检查NULL字符（字符串终止符）
            if (c == '\0') {
                return false;
            }
            // 检查控制字符（ASCII 0-31，除了制表符、换行符等通常不允许）
            if (c < 32 && c != '\t' && c != '\n' && c != '\r') {
                return false;
            }
            // 检查DEL字符
            if (c == 127) {
                return false;
            }
        }
        return true;
    }
}
}