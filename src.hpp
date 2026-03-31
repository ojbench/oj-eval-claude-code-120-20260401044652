#pragma once

#include "Task.hpp"
#include <vector>
// 你不可以添加任何头文件

class TaskNode {
    friend class TimingWheel;
    friend class Timer;
public:
    TaskNode(Task* t, int tm) : task(t), next(nullptr), prev(nullptr), time(tm) {}

private:
    Task* task; // 需要触发的任务，你无需关心其内存管理
    TaskNode* next, *prev; // 双向链表，你可以按你自己的想法修改
    int time; // 任务距离触发的时间，可以参考示例中的伪代码
};

class TimingWheel {
    friend class Timer;
public:
    TimingWheel(size_t size, size_t interval);
    ~TimingWheel();

    // 添加任务到指定槽位
    void addTaskToSlot(TaskNode* node, size_t slot);

    // 从槽位移除任务
    void removeTask(TaskNode* node, size_t slot);

    // 获取当前槽位的任务列表头
    TaskNode* getCurrentSlotTasks();

    // 清空当前槽位
    void clearCurrentSlot();

    // 前进一格
    void advance();

private:
    const size_t size, interval; // 时间轮的大小，以及每个槽位代表的时间间隔
    size_t current_slot = 0; // 当前指向的槽位
    TaskNode** slots; // 槽位数组，每个槽位存储链表头
};

class Timer {
public:
    // 不需要考虑拷贝构造和赋值运算符
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    Timer();
    ~Timer();

    // 添加一个任务，并且返回指向该任务对应的 TaskNode 的指针。
    TaskNode* addTask(Task* task);

    // 通过指向 TaskNode 的指针取消一个任务，将其从时间轮上移除。
    void cancelTask(TaskNode *p);

    // 最低时间轮前进一格，返回在此时刻需要执行的任务列表，无需关心列表中的任务顺序。需要注意时间轮的进位。
    std::vector<Task*> tick();

private:
    TimingWheel* second_wheel;  // 秒级时间轮
    TimingWheel* minute_wheel;  // 分钟级时间轮
    TimingWheel* hour_wheel;    // 小时级时间轮

    // 添加任务节点到合适的时间轮
    void addTaskNode(TaskNode* node);
};

// TimingWheel 实现
TimingWheel::TimingWheel(size_t size, size_t interval)
    : size(size), interval(interval), current_slot(0) {
    slots = new TaskNode*[size];
    for (size_t i = 0; i < size; i++) {
        slots[i] = nullptr;
    }
}

TimingWheel::~TimingWheel() {
    // 清理所有槽位中的任务节点
    for (size_t i = 0; i < size; i++) {
        TaskNode* current = slots[i];
        while (current) {
            TaskNode* next = current->next;
            delete current;
            current = next;
        }
    }
    delete[] slots;
}

void TimingWheel::addTaskToSlot(TaskNode* node, size_t slot) {
    // 插入到链表头部
    node->next = slots[slot];
    node->prev = nullptr;
    if (slots[slot]) {
        slots[slot]->prev = node;
    }
    slots[slot] = node;
}

void TimingWheel::removeTask(TaskNode* node, size_t slot) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        // 是链表头
        slots[slot] = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    }
}

TaskNode* TimingWheel::getCurrentSlotTasks() {
    return slots[current_slot];
}

void TimingWheel::clearCurrentSlot() {
    slots[current_slot] = nullptr;
}

void TimingWheel::advance() {
    current_slot = (current_slot + 1) % size;
}

// Timer 实现
Timer::Timer() {
    // 秒级：60个槽位，每个槽位1秒
    second_wheel = new TimingWheel(60, 1);
    // 分钟级：60个槽位，每个槽位60秒
    minute_wheel = new TimingWheel(60, 60);
    // 小时级：24个槽位，每个槽位3600秒
    hour_wheel = new TimingWheel(24, 3600);
}

Timer::~Timer() {
    delete second_wheel;
    delete minute_wheel;
    delete hour_wheel;
}

TaskNode* Timer::addTask(Task* task) {
    size_t first_interval = task->getFirstInterval();
    TaskNode* node = new TaskNode(task, first_interval);
    addTaskNode(node);
    return node;
}

void Timer::addTaskNode(TaskNode* node) {
    int time = node->time;

    // 根据时间选择合适的时间轮
    if (time <= 0) {
        // 立即执行的任务放到当前槽位
        second_wheel->addTaskToSlot(node, second_wheel->current_slot);
    } else if (time / (int)second_wheel->interval <= (int)second_wheel->size) {
        // 放在秒级时间轮
        size_t slot = (second_wheel->current_slot + time / second_wheel->interval) % second_wheel->size;
        second_wheel->addTaskToSlot(node, slot);
    } else if (time / (int)minute_wheel->interval <= (int)minute_wheel->size) {
        // 放在分钟级时间轮
        // time = time + current_slot * interval，这样下放时才能正确计算位置
        int adjusted_time = time + minute_wheel->current_slot * minute_wheel->interval;
        size_t slot = (adjusted_time / minute_wheel->interval) % minute_wheel->size;
        minute_wheel->addTaskToSlot(node, slot);
    } else if (time / (int)hour_wheel->interval <= (int)hour_wheel->size) {
        // 放在小时级时间轮
        int adjusted_time = time + hour_wheel->current_slot * hour_wheel->interval;
        size_t slot = (adjusted_time / hour_wheel->interval) % hour_wheel->size;
        hour_wheel->addTaskToSlot(node, slot);
    } else {
        // 超出范围，直接删除
        delete node;
    }
}

void Timer::cancelTask(TaskNode *p) {
    if (!p) return;

    // 需要找到节点所在的时间轮和槽位
    // 遍历所有时间轮的所有槽位
    for (size_t i = 0; i < second_wheel->size; i++) {
        TaskNode* current = second_wheel->slots[i];
        while (current) {
            if (current == p) {
                second_wheel->removeTask(p, i);
                delete p;
                return;
            }
            current = current->next;
        }
    }

    for (size_t i = 0; i < minute_wheel->size; i++) {
        TaskNode* current = minute_wheel->slots[i];
        while (current) {
            if (current == p) {
                minute_wheel->removeTask(p, i);
                delete p;
                return;
            }
            current = current->next;
        }
    }

    for (size_t i = 0; i < hour_wheel->size; i++) {
        TaskNode* current = hour_wheel->slots[i];
        while (current) {
            if (current == p) {
                hour_wheel->removeTask(p, i);
                delete p;
                return;
            }
            current = current->next;
        }
    }
}

std::vector<Task*> Timer::tick() {
    std::vector<Task*> result;

    // 秒级时间轮前进一格
    second_wheel->advance();

    // 检查是否需要分钟级进位
    if (second_wheel->current_slot == 0) {
        minute_wheel->advance();

        // 分钟级时间轮进位后，需要将当前槽位的任务下放到秒级
        TaskNode* minute_tasks = minute_wheel->getCurrentSlotTasks();
        minute_wheel->clearCurrentSlot();

        while (minute_tasks) {
            TaskNode* next = minute_tasks->next;
            minute_tasks->time %= minute_wheel->interval;
            minute_tasks->next = minute_tasks->prev = nullptr;
            addTaskNode(minute_tasks);
            minute_tasks = next;
        }

        // 检查是否需要小时级进位
        if (minute_wheel->current_slot == 0) {
            hour_wheel->advance();

            // 小时级时间轮进位后，需要将当前槽位的任务下放到分钟级
            TaskNode* hour_tasks = hour_wheel->getCurrentSlotTasks();
            hour_wheel->clearCurrentSlot();

            while (hour_tasks) {
                TaskNode* next = hour_tasks->next;
                hour_tasks->time %= hour_wheel->interval;
                hour_tasks->next = hour_tasks->prev = nullptr;
                addTaskNode(hour_tasks);
                hour_tasks = next;
            }
        }
    }

    // 获取当前秒级槽位需要执行的任务
    TaskNode* current = second_wheel->getCurrentSlotTasks();
    second_wheel->clearCurrentSlot();

    while (current) {
        TaskNode* next = current->next;
        result.push_back(current->task);

        // 检查是否需要重新调度（周期性任务）
        size_t period = current->task->getPeriod();
        if (period > 0) {
            current->time = period;
            current->next = current->prev = nullptr;
            addTaskNode(current);
        } else {
            delete current;
        }

        current = next;
    }

    return result;
}
