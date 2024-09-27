#ifndef __CORE_H_INCLUDED__
#define __CORE_H_INCLUDED__
#include <string>

namespace core
{

    // 结构体定义
    struct TodoItem {
        uint64_t id;
        std::wstring text;
        std::wstring remark;
        uint64_t lastmodified{0};
        uint64_t deadline{0};
        uint64_t remind{0};
        bool everyday{false};
        bool status{0};
    };

    time_t SystemTimeToTimestamp(const LPSYSTEMTIME ft);
    SYSTEMTIME TimestampToSystemTime(time_t tm);

    // 将ANSI字符串转换为UTF-8
    std::string AnsiToUtf8(const std::string& ansiString);
    std::string Utf8ToAnsi(const std::string& utf8String);
    std::string wstrToUTF8(const std::wstring& wstr);
    std::wstring utf8Towstr(const std::string& str);

    // 删除字符串两端的空白字符
    std::wstring& trim(std::wstring &str);

class TodoMgr
{
    TodoMgr();
public:
    struct Iterator{
        explicit Iterator(void* d);
        ~Iterator();
        void operator++();
        bool operator==(const Iterator& rhs) const;
        bool operator!=(const Iterator& rhs) const;
        const TodoItem& operator*() const;
        operator const TodoItem&() const;
        struct Impl;
        Impl* pimpl;
    };

    ~TodoMgr();
    static TodoMgr* instance();
    void dump();
    uint64_t add(TodoItem& item);
    void remove(const uint64_t& id);
    const TodoItem& at(const uint64_t& id);
    Iterator begin();
    Iterator end();
    void operator++() const;
    size_t size();

    void update_status(uint64_t idx, bool finished);
    void update_title(uint64_t idx, const std::wstring& str);
    void update_remark(uint64_t idx, const std::wstring& str);
    void update_deadline(uint64_t idx, time_t t);
    void update_remaind(uint64_t idx, time_t t);
    void update_everyday(uint64_t idx, bool everyday);
private:
    void load();

private:
    struct Impl;
    Impl * pimpl_;
};


} // namespace core


#endif // __CORE_H_INCLUDED__