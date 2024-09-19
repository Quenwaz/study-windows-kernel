#ifndef __CORE_H_INCLUDED__
#define __CORE_H_INCLUDED__
#include <string>

namespace core
{

    // 结构体定义
    struct TodoItem {
        std::wstring text;
        std::wstring remark;
        intptr_t timestamp{0};
        intptr_t deadline{0};
        intptr_t remind{0};
        bool everyday{false};
        bool completed{false};
    };

    time_t SystemTimeToTimestamp(const LPSYSTEMTIME ft);
    SYSTEMTIME TimestampToSystemTime(time_t tm);

    // 将ANSI字符串转换为UTF-8
    std::string AnsiToUtf8(const std::string& ansiString);
    std::string Utf8ToAnsi(const std::string& utf8String);
    std::string wstrToUTF8(const std::wstring& wstr);
    std::wstring utf8Towstr(const std::string& str);

class TodoMgr
{
    TodoMgr();
public:
    ~TodoMgr();
    static TodoMgr* instance();
    void dump();
    int add(const TodoItem& item);
    TodoItem& at(int idx);
    const TodoItem& operator[](int idx) const;
    size_t size();

    void update_status(int idx, bool finished);
    void update_title(int idx, const std::wstring& str);
    void update_remark(int idx, const std::wstring& str);
    void update_deadline(int idx, time_t t);
    void update_remaind(int idx, time_t t);
    void update_everyday(int idx, bool everyday);
private:
    void load();

private:
    struct Impl;
    Impl * pimpl_;
};


} // namespace core


#endif // __CORE_H_INCLUDED__