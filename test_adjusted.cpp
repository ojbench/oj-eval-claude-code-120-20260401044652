#include "Timer.hpp"
#include <iostream>

int main() {
    Timer timer;

    // Advance to tick 30 (second=30, minute=0)
    for (int i = 1; i <= 30; ++i) {
        timer.tick();
    }

    std::cout << "At tick 30, adding task with first_interval=90" << std::endl;

    // Now add a task with first_interval=90 (should fire at tick 120)
    Task task90("task90", 90, 0);
    timer.addTask(&task90);

    for (int i = 31; i <= 200; ++i) {
        auto tasks = timer.tick();
        if (tasks.size()) {
            std::cout << "tick " << i << std::endl;
            for (auto task : tasks) {
                task->execute();
            }
        }
    }

    return 0;
}
