#ifndef __FIBER__H_
#define __FIBER__H_

#include <coroutine>
#include <memory>
#include <thread>

// TODO: implement Fiber on Coroutine

class Fiber : std::enable_shared_from_this<Fiber>
{
public:
    typedef Fiber self;
    typedef std::shared_ptr<self> ptr;

    typedef unsigned long int id_t;
    static auto GetId() -> id_t { return id_t(-1); }

private:
};

//     // 获取协程 id
//     uint64_t getID() const { return m_id; }

//     // 获取协程状态
//     State getState() const { return m_state; }

//     // 判断协程是否执行结束
//     bool finish() const noexcept;

// private:
//     // 用于创建 master fiber
//     Fiber();

// public:
//     // 获取当前正在执行的 fiber 的智能指针，如果不存在，则在当前线程上创建 master fiber
//     static Fiber::ptr GetThis();
//     // 设置当前 fiber
//     static void SetThis(Fiber* fiber);
//     // 挂起当前协程，转换为 READY 状态，等待下一次调度
//     static void Yield();
//     // 挂起当前协程，转换为 HOLD 状态，等待下一次调度
//     static void YieldToHold();
//     // 获取存在的协程数量
//     static uint64_t TotalFiber();
//     // 获取当前协程 id
//     static uint64_t GetFiberID();
//     // 协程入口函数
//     static void MainFunc();

// private:
//     // 协程 id
//     uint64_t m_id;
//     // 协程栈大小
//     uint64_t m_stack_size;
//     // 协程状态
//     State m_state;
//     // 协程上下文
//     ucontext_t m_ctx;
//     // 协程栈空间指针
//     void* m_stack;
//     // 协程执行函数
//     FiberFunc m_callback;
// };

// namespace FiberInfo
// {

// // 最后一个协程的 id
// static std::atomic_uint64_t s_fiber_id { 0 };
// // 存在的协程数量
// static std::atomic_uint64_t s_fiber_count { 0 };

// // 当前线程正在执行的协程
// static thread_local Fiber* t_fiber = nullptr;
// // 当前线程的主协程
// static thread_local Fiber::ptr t_master_fiber {};

// // 协程栈大小配置项
// static ConfigVar<uint64_t>::ptr g_fiber_stack_size = Config::Lookup<uint64_t>("fiber.stack_size", 1024 * 1024);
// } // namespace FiberInfo
#endif // __FIBER__H_