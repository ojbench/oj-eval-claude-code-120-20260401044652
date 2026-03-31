#include "Timer.hpp"
#include <iostream>

int main() {
    Timer timer;
    Task task60("task60", 60, 60);  // Fire at 60, 120, 180, ...

    timer.addTask(&task60);

    for (int i = 1; i <= 130; ++i) {
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
