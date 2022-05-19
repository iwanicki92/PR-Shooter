#include <chrono>

template <class DurType = std::chrono::microseconds, class ClockType = std::chrono::steady_clock>
class Timer {
public:
    using DurationType = DurType;
    Timer() : start_time(ClockType::now()) {}

    void start() {
        start_time = ClockType::now();
    }
    auto duration() {
        return std::chrono::duration_cast<DurationType>(ClockType::now() - start_time);
    }
    auto restart() {
        auto dur = duration();
        start();
        return dur;
    }
    private:
        typename ClockType::time_point start_time;
};