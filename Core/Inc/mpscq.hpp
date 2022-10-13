#ifndef __MPSCQ_HPP
#define __MPSCQ_HPP

#include <memory>
#include <mutex>
#include <condition_variable>

#include "mpscq.h"

namespace MPSC {
template <typename T, class Allocator = std::allocator<T>>
class Queue {
public:
    using ElemType = T;

    Queue(size_t capacity = 0x100)
        : m_queue(mpscq_create(nullptr, capacity))
    {}
    template <typename ... Args>
    bool Enque(Args&& ... args)
    {
        bool result = false;
        ElemType *e = TraitsT::allocate(m_allocator, 1);

        if (!e) {
            return false;
        }

        TraitsT::construct(m_allocator, e, std::forward<Args>(args)...);

        result = mpscq_enqueue(m_queue, e);

        if (!result) {
            Deallocate(e);
        }
#if defined(_GLIBCXX_HAS_GTHREADS)
        else {
            m_cv.notify_one();
        }
#endif // Threads

        return result;
    }

    ElemType PopWait()
    {
        ElemType *e = nullptr;
#if defined(_GLIBCXX_HAS_GTHREADS)
        std::unique_lock lock (m_mx);
#endif // Threads
        do {
#if defined(_GLIBCXX_HAS_GTHREADS)
            m_cv.wait(lock, [this]() { return mpscq_count(m_queue) > 0; });
#else // Threads
            while(mpscq_count(m_queue) == 0);
#endif // Threads
            e = reinterpret_cast<ElemType *>(mpscq_dequeue(m_queue));
        } while (e == nullptr);

        ElemType result = std::move(*e);
        Deallocate(e);
        return result;
    }

private:
    using TraitsT = std::allocator_traits<Allocator>;

private:
    void Deallocate(ElemType *e)
    {
        TraitsT::destroy(m_allocator, e);
        TraitsT::deallocate(m_allocator, e, 1);
    }

private:
    struct mpscq *m_queue = nullptr;
    Allocator m_allocator = {};
#if defined(_GLIBCXX_HAS_GTHREADS)
    std::condition_variable m_cv = {};
    std::mutex m_mx = {};
#endif // Threads
};
}

#endif /* __MPSCQ_HPP */
