// =============================================================================
// motion/queue.cpp — Fila circular thread-safe
// =============================================================================

#include "queue.h"

// Instância global
MotionQueue motionQueue;

// -----------------------------------------------------------------------------
void MotionQueue::init() {
    _mutex = xSemaphoreCreateMutex();
    _head  = 0;
    _tail  = 0;
    _count = 0;
    _totalBlocks    = 0;
    _consumedBlocks = 0;

    // Marca todos os slots como inválidos
    for (int i = 0; i < SIZE; i++) {
        _buffer[i].valid = false;
    }
}

// -----------------------------------------------------------------------------
bool MotionQueue::push(const MotionBlock& block) {
    if (!_mutex) return false;

    bool success = false;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (!_isFull()) {
            _buffer[_tail] = block;
            _buffer[_tail].valid = true;
            _tail = (_tail + 1) % SIZE;
            _count++;
            success = true;
        }
        xSemaphoreGive(_mutex);
    }
    return success;
}

// -----------------------------------------------------------------------------
bool MotionQueue::pop(MotionBlock& block) {
    if (!_mutex) return false;

    bool success = false;
    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (!_isEmpty()) {
            block = _buffer[_head];
            _buffer[_head].valid = false;
            _head = (_head + 1) % SIZE;
            _count--;
            _consumedBlocks++;
            success = true;
        }
        xSemaphoreGive(_mutex);
    }
    return success;
}

// -----------------------------------------------------------------------------
void MotionQueue::clear() {
    if (!_mutex) return;

    if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        _head  = 0;
        _tail  = 0;
        _count = 0;
        for (int i = 0; i < SIZE; i++) {
            _buffer[i].valid = false;
        }
        xSemaphoreGive(_mutex);
    }
}

// -----------------------------------------------------------------------------
bool MotionQueue::isEmpty() const {
    return _count == 0;
}

bool MotionQueue::isFull() const {
    return _count >= SIZE;
}

int MotionQueue::count() const {
    return _count;
}

// -----------------------------------------------------------------------------
float MotionQueue::progressPercent() const {
    if (_totalBlocks <= 0) return 0.0f;
    float p = (float)_consumedBlocks / (float)_totalBlocks * 100.0f;
    return p > 100.0f ? 100.0f : p;
}

void MotionQueue::setTotalBlocks(int total) {
    _totalBlocks    = total;
    _consumedBlocks = 0;
}
