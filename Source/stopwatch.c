#include <stopwatch.h>

#include <glfw3.h>
void Stopwatch(update)(stopwatch_t* stopwatch) {
    if (!stopwatch) return;

    const f64 now = glfwGetTime();
    if (stopwatch->last == 0.0) {
        stopwatch->last = now;
        return;
    }
    stopwatch->delta = now - stopwatch->last;
    stopwatch->last = now;
}
