#include <windows.h>
#include <time.h>
#include "core.hpp"
#include <vector>
#include <fstream>
#include <codecvt>
#include <locale>
#include "common_header/json.hpp"

using json = nlohmann::json;

namespace core
{

time_t FileTimeToUnixTime(const FILETIME* ft) {
    // 1601年到1970年的时间差值 (100纳秒单位)
    const __int64 UNIX_EPOCH_IN_WIN_TICKS = 116444736000000000LL;

    // 将 FILETIME 转换为 64 位整数
    ULARGE_INTEGER ull;
    ull.LowPart = ft->dwLowDateTime;
    ull.HighPart = ft->dwHighDateTime;

    // 减去 1601 到 1970 年的时间差值，并将 100 纳秒单位转换为秒
    return (ull.QuadPart - UNIX_EPOCH_IN_WIN_TICKS) / 10000000LL;
}


void TimetToFileTime(time_t t, LPFILETIME pft)
{
    ULARGE_INTEGER time_value;
    time_value.QuadPart = (t * 10000000LL) + 116444736000000000LL;
    pft->dwLowDateTime = time_value.LowPart;
    pft->dwHighDateTime = time_value.HighPart;
}


time_t SystemTimeToTimestamp(const LPSYSTEMTIME st)
{
    FILETIME ft ={0};
    SystemTimeToFileTime(st, &ft);
    return FileTimeToUnixTime(&ft);
}


SYSTEMTIME TimestampToSystemTime(time_t tm)
{
    FILETIME ft;
    TimetToFileTime(tm,&ft);
    SYSTEMTIME st ={0};
    FileTimeToSystemTime(&ft,&st);
    return st;
}

// 将ANSI字符串转换为UTF-8
std::string AnsiToUtf8(const std::string& ansiString) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wideString;
    int len = MultiByteToWideChar(CP_ACP, 0, ansiString.c_str(), -1, NULL, 0);
    wideString.resize(len - 1); // 减去NULL终止符
    MultiByteToWideChar(CP_ACP, 0, ansiString.c_str(), -1, &wideString[0], len);
    return converter.to_bytes(wideString);
}

std::string Utf8ToAnsi(const std::string& utf8String) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wideString = converter.from_bytes(utf8String);
    int len = WideCharToMultiByte(CP_ACP, 0, wideString.c_str(), -1, NULL, 0, NULL, NULL);
    std::string ansiString(len - 1, '\0'); // 减去NULL终止符
    WideCharToMultiByte(CP_ACP, 0, wideString.c_str(), -1, &ansiString[0], len, NULL, NULL);
    return ansiString;
}

std::string wstrToUTF8(const std::wstring& wstr) {
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Length <= 0) {
        return "";
    }

    std::string utf8String(utf8Length - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8String[0], utf8Length - 1, nullptr, nullptr);
    return utf8String;
}

std::wstring utf8Towstr(const std::string& str) {
    int utf8Length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (utf8Length <= 0) {
        return L"";
    }

    std::wstring utf8String(utf8Length - 1, '\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &utf8String[0], utf8Length);
    return utf8String;
}



struct TodoMgr::Impl{
    const std::string filename{"todos.json"};
    std::vector<TodoItem> todos;

    auto at(const int& id){
        TodoItem val;
        val.id = id;
        auto findret = std::equal_range(todos.begin(), todos.end(),val, [](const TodoItem& lhs, const TodoItem& rhs){
            return lhs.id < rhs.id;
        });

        return findret.first;
    }
};


TodoMgr::TodoMgr()
    : pimpl_(new Impl)
{
    this->load();
}

TodoMgr::~TodoMgr()
{
    delete pimpl_;
}

TodoMgr* TodoMgr::instance()
{
    static TodoMgr inst;
    return &inst;
}


void TodoMgr::dump() {
    json j;
    for (const auto& item : pimpl_->todos) {
        j.push_back({
            {"text",wstrToUTF8(item.text)},
            {"remark", wstrToUTF8(item.remark)}, 
            {"status", item.completed}, 
            {"createTime", item.timestamp},
            {"deadline", item.deadline},
            {"remind", item.remind}
            });
    }
    std::ofstream o(pimpl_->filename);
    o << j << std::endl;
}

void TodoMgr::load() {
    std::ifstream i(pimpl_->filename);
    if (i.good()) {
        json j;
        i >> j;
        size_t id = 0;
        for (const auto& item : j) {
            core::TodoItem todo;
            todo.id = id++;
            todo.text = utf8Towstr(item["text"]);
            todo.remark = utf8Towstr(item["remark"]);
            todo.completed = item["status"];
            todo.timestamp = item["createTime"];
            todo.deadline = item["deadline"];
            todo.remind = item["remind"];
            pimpl_->todos.push_back(todo);
        }
    }
}

int TodoMgr::add(const TodoItem& item)
{
    auto newid = pimpl_->todos.empty()? 0 : pimpl_->todos.back().id + 1;
    pimpl_->todos.push_back(item);
    pimpl_->todos.back().id = newid;
    dump();
    return newid;
}

void TodoMgr::remove(const int& id)
{
    assert(id >=0 && id < pimpl_->todos.size());
    pimpl_->todos.erase(pimpl_->at(id));
    dump();
}

const TodoItem& TodoMgr::operator[](int idx) const
{
    return *pimpl_->at(idx);
}

size_t TodoMgr::size()
{
    return pimpl_->todos.size();
}

TodoItem& TodoMgr::at(int idx)
{
    assert(idx >=0 && idx < pimpl_->todos.size());
    return *pimpl_->at(idx);
}

void TodoMgr::update_status(int idx, bool finished)
{
    assert(idx >=0 && idx < pimpl_->todos.size());
    pimpl_->at(idx)->completed = finished;
    dump();
}

void TodoMgr::update_title(int idx, const std::wstring& str)
{
    assert(idx >=0 && idx < pimpl_->todos.size());
    pimpl_->at(idx)->text = str;
    dump();
}


void TodoMgr::update_remark(int idx, const std::wstring& str){
    assert(idx >=0 && idx < pimpl_->todos.size());
    pimpl_->at(idx)->remark = str;
    dump();
}


void TodoMgr::update_deadline(int idx, time_t t){
    assert(idx >=0 && idx < pimpl_->todos.size());
    pimpl_->at(idx)->deadline = t;
    dump();
}


void TodoMgr::update_remaind(int idx, time_t t){
    assert(idx >=0 && idx < pimpl_->todos.size());
    pimpl_->at(idx)->remind = t;
    dump();
}

void TodoMgr::update_everyday(int idx, bool everyday){
    assert(idx >=0 && idx < pimpl_->todos.size());
    pimpl_->at(idx)->everyday = everyday;
    dump();
}

} // namespace core

