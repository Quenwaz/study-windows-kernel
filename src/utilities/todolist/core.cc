#include <windows.h>
#include <time.h>
#include "core.hpp"
#include <vector>
#include <set>
#include <fstream>
#include <codecvt>
#include <locale>
#include <execution>
#include "common_header/json.hpp"

using json = nlohmann::json;

namespace
{
    auto timestamp(){
        return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }
} // namespace

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


time_t SystemTimeToTimestamp(const LPSYSTEMTIME local_system_time)
{
    SYSTEMTIME utc_time = {0};
    TzSpecificLocalTimeToSystemTime(nullptr, local_system_time,&utc_time);
    FILETIME ft ={0};
    SystemTimeToFileTime(&utc_time, &ft);
    return FileTimeToUnixTime(&ft);
}


SYSTEMTIME TimestampToSystemTime(time_t tm)
{
    FILETIME utc_ft = {0};
    TimetToFileTime(tm,&utc_ft);

    FILETIME local_ft = {0};
    FileTimeToLocalFileTime(&utc_ft,&local_ft);

    SYSTEMTIME st ={0};
    FileTimeToSystemTime(&local_ft,&st);
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

std::wstring& trim(std::wstring &str) {
    // 从字符串开头查找第一个非空白字符
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));

    // 从字符串末尾查找第一个非空白字符
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), str.end());
    return str;
}

struct TodoItemKey{
    uint64_t id{0};
    short status{0};

    TodoItemKey(uint64_t i, short s): id(i), status(s){}

    bool operator<(const TodoItemKey& rhs) const{
        if (this->status == -1 || rhs.status == -1){
            return this->id < rhs.id;
        }

        return this->status == rhs.status? this->id > rhs.id:this->status==0?true:false;
    }
};


struct TodoMgr::Impl{
    typedef std::vector<TodoItem> container_type;
    const std::string filename{"todos.json"};

    size_t size() const{
        return todos.size();
    }

    auto at(const uint64_t &id){
        TodoItem temp;
        temp.id =id;
        auto iterfind = std::equal_range(this->todos.begin(), this->todos.end(), temp, [](const TodoItem& lhs,const TodoItem & rhs){
            return lhs.id < rhs.id;
        });
        assert(iterfind.first != iterfind.second);
        return iterfind.first;
    }

    void sort(){
        std::sort(std::execution::par,this->todos.begin(), this->todos.end(),[](const TodoItem& lhs,const TodoItem& rhs){
            return lhs.status == rhs.status? lhs.lastmodified > rhs.lastmodified:lhs.status ? true:false;
        });
    }

    int push(TodoItem& item)
    {
        item.id = tick++;
        this->todos.push_back(item);
        return item.id;
    }

    void remove(const uint64_t &id){
        this->todos.erase(at(id));
    }

    auto begin(){
        return this->todos.begin();
    }

    auto end(){
        return this->todos.end();
    }


private:
    container_type todos;
    uint64_t tick{0};
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
    for (auto& item : *pimpl_){
        j.push_back({
            {"text",wstrToUTF8(item.text)},
            {"remark", wstrToUTF8(item.remark)}, 
            {"status", item.status}, 
            {"lastmodified", item.lastmodified},
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
            todo.text = utf8Towstr(item["text"]);
            todo.remark = utf8Towstr(item["remark"]);
            todo.status = item["status"];
            todo.lastmodified = item["lastmodified"];
            todo.deadline = item["deadline"];
            todo.remind = item["remind"];
            pimpl_->push(todo);
        }
    }
}

const TodoItem& TodoMgr::at(const uint64_t& id)
{
    return *pimpl_->at(id);
}

uint64_t TodoMgr::add(TodoItem& item)
{
    item.lastmodified = timestamp();
    const auto id = pimpl_->push(item);
    dump();
    return id;
}

void TodoMgr::remove(const uint64_t& id)
{
    pimpl_->remove(id);
    dump();
}

struct TodoMgr::Iterator::Impl{
    Impl(const TodoMgr::Impl::container_type::iterator& i):iterator(i) {}
    TodoMgr::Impl::container_type::iterator iterator;
};

TodoMgr::Iterator::Iterator(void* d)
    : pimpl(new Impl(*static_cast<TodoMgr::Impl::container_type::iterator*>(d)))
{
}

bool TodoMgr::Iterator::operator==(const Iterator& rhs) const
{
    return pimpl->iterator == rhs.pimpl->iterator;
}

bool TodoMgr::Iterator::operator!=(const Iterator& rhs) const
{
    return !(pimpl->iterator == rhs.pimpl->iterator);
}

void TodoMgr::Iterator::operator++()
{
    ++pimpl->iterator;
}

const TodoItem& TodoMgr::Iterator::operator*() const
{
    return *pimpl->iterator;
}

TodoMgr::Iterator::operator const TodoItem&() const
{
    return *pimpl->iterator;
}

TodoMgr::Iterator::~Iterator(){
    delete pimpl;
    pimpl = nullptr;
}

TodoMgr::Iterator TodoMgr::begin()
{
    return Iterator(&pimpl_->begin());
}

TodoMgr::Iterator TodoMgr::end()
{
    return Iterator(&pimpl_->end());
}

size_t TodoMgr::size()
{
    return pimpl_->size();
}

void TodoMgr::update_status(uint64_t idx, bool finished)
{
    auto item = pimpl_->at(idx);
    item->status = finished;
    item->lastmodified = timestamp();
    dump();
}

void TodoMgr::update_title(uint64_t idx, const std::wstring& str)
{
    auto item = pimpl_->at(idx);
    item->text = str;
    dump();
}


void TodoMgr::update_remark(uint64_t idx, const std::wstring& str){
    auto item = pimpl_->at(idx);
    item->remark = str;
    dump();
}


void TodoMgr::update_deadline(uint64_t idx, time_t t){
    auto item = pimpl_->at(idx);
    item->deadline = t;
    dump();
}


void TodoMgr::update_remaind(uint64_t idx, time_t t){
    auto item = pimpl_->at(idx);
    item->remind = t;
    dump();
}

void TodoMgr::update_everyday(uint64_t idx, bool everyday){
    auto item = pimpl_->at(idx);
    item->everyday = everyday;
    dump();
}

} // namespace core

