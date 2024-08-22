#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#define VERSION_1

namespace
{
    template <typename T>
    struct __DefaultDeleter{
      void operator()(const T& o) const{
            delete o;
            o = nullptr;
      }  
    };

    template <typename T, typename R>
    struct __ReturnToCall
    {
        typedef R(*Deleter)(T) ;
        explicit __ReturnToCall(T& o, Deleter deleter = __DefaultDeleter<T>)
            :obj_(o), del_(deleter){}
        ~__ReturnToCall(){
            del_(obj_);
        }

        void* operator new(size_t) = delete;
        void operator delete(void*) = delete;
        void* operator new [] (size_t size) = delete;
    private:
        T& obj_;
        Deleter del_;
    };

#define ReturnToCall(o,c) __ReturnToCall _return_to_call_##__LINE__(o,c)


void controlCPUUsage(int targetUsage) {
    // targetUsage 是 CPU 目标使用率，范围是 0 到 100
    const int workTimeMs = 100;  // 工作时间 10 毫秒
    const int totalCycleMs = workTimeMs * 100 / targetUsage;  // 计算整个周期的时间

    while (true) {
        auto start = std::chrono::steady_clock::now();

        // 模拟计算工作
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < workTimeMs) {
            // 在这段时间内，CPU 全速工作
        }

        // 休眠，控制 CPU 使用率
        std::this_thread::sleep_for(std::chrono::milliseconds(totalCycleMs - workTimeMs));
    }
}
    
} // namespace


 
void handler()
{
    std::cout << "Memory allocation failed, terminating\n";
    exit(1);
}


int main()
{
    std::set_new_handler(handler);
#ifdef VERSION_1
    // 创建 Job 对象
    HANDLE hJob = CreateJobObject(nullptr, nullptr);
    if (hJob == nullptr) {
        std::cerr << "Failed to create Job object" << std::endl;
        return 1;
    }

    ReturnToCall(hJob,CloseHandle);
    // 设置 Job 对象限制
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {};
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    jobInfo.ProcessMemoryLimit = 100 * 1024 * 1024;  // 100 MB

    if (!SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo))) {
        std::cerr << "Failed to set Job object information" << std::endl;
        return 1;
    }

	JOBOBJECT_CPU_RATE_CONTROL_INFORMATION info;
	info.CpuRate = 10 * 100;
	info.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
	if (!::SetInformationJobObject(hJob, JobObjectCpuRateControlInformation, &info, sizeof(info))){
        std::cerr << "Failed to set Job object information" << std::endl;
		return 1;
        
    }

    // 将当前进程加入 Job 对象
    if (!AssignProcessToJobObject(hJob, GetCurrentProcess())) {
        std::cerr << "Failed to assign process to Job object" << std::endl;
        CloseHandle(hJob);
        return 1;
    }

#elif defined(VERSION_2)
    SIZE_T minSize = 10 * 1024 * 1024;  // 10 MB
    SIZE_T maxSize = 100 * 1024 * 1024; // 100 MB

    if (!SetProcessWorkingSetSizeEx(GetCurrentProcess(), minSize, maxSize, QUOTA_LIMITS_HARDWS_MAX_ENABLE)) {
        std::cerr << "Failed to set process working set size" << std::endl;
        return 1;
    }

#endif
    std::vector<std::thread> threads;
    for(size_t i = 0;i < 32; ++i){
        int targetCPUUsage = 50;  // 设置目标 CPU 使用率为 50%
        threads.push_back(std::thread(controlCPUUsage, targetCPUUsage));
    }
    
    for(auto& t: threads) t.join();
    
    // 你的应用程序代码
    std::cout << "Memory limit set to 100 MB" << std::endl;

    char* buffer = new char[200 * 1024 * 1024];  // 分配 200 MB
    memset(buffer, 0, 200 * 1024 * 1024);

    std::cout << "Allocated 200 MB" << std::endl;

    delete[] buffer;
    return 0;
}
