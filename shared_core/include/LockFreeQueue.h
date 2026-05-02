#pragma once

#include <atomic>
#include <vector>
#include <cstdint>

namespace audiostream {

// A highly efficient Single-Producer, Single-Consumer (SPSC) Lock-Free Ring Buffer
// Designed for zero-allocation audio data passing between network and audio threads.
// Safe to use inside High-Priority Real-Time threads.
template <typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t capacity) 
        : m_capacity(capacity + 1) // +1 to distinguish empty from full
        , m_buffer(capacity + 1)
        , m_head(0)
        , m_tail(0) 
    {}

    // Push data (Producer Thread). Returns false if full.
    bool push(const T& item) {
        size_t current_tail = m_tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % m_capacity;

        if (next_tail == m_head.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }

        m_buffer[current_tail] = item;
        m_tail.store(next_tail, std::memory_order_release);
        return true;
    }

    // Pop data (Consumer Thread). Returns false if empty.
    bool pop(T& item) {
        size_t current_head = m_head.load(std::memory_order_relaxed);

        if (current_head == m_tail.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }

        item = m_buffer[current_head];
        size_t next_head = (current_head + 1) % m_capacity;
        m_head.store(next_head, std::memory_order_release);
        return true;
    }

    // Current readable size
    size_t size() const {
        size_t head = m_head.load(std::memory_order_acquire);
        size_t tail = m_tail.load(std::memory_order_acquire);
        if (tail >= head) {
            return tail - head;
        }
        return m_capacity - head + tail;
    }

    // Current free space for writing
    size_t free_space() const {
        return m_capacity - 1 - size();
    }

private:
    size_t m_capacity;
    std::vector<T> m_buffer; // Pre-allocated vector
    // Head and Tail indices are cache-aligned automatically by std::atomic on modern compilers
    // but separating them into different cache lines can further optimize false sharing.
    alignas(64) std::atomic<size_t> m_head; // Consumer reads from head
    alignas(64) std::atomic<size_t> m_tail; // Producer writes to tail
};

} // namespace audiostream
