#include "includes/shared.h"

static constexpr uint64_t KILO_BYTE = 1000;
static constexpr uint64_t MEGA_BYTE = KILO_BYTE * 1000;
static constexpr uint64_t GIGA_BYTE = MEGA_BYTE * 1000;

uint64_t Helper::getTimeInMillis() {
    const std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    return now.count();
}

std::string Helper::formatMemoryUsage(const VkDeviceSize size) {
    if (size < KILO_BYTE) {
        return std::to_string(size) + " B";
    }

    if (size < MEGA_BYTE) {
        return std::to_string(size / KILO_BYTE) + " KB";
    }

    if (size < GIGA_BYTE) {
        return std::to_string(size / MEGA_BYTE) + " MB";
    }

     return std::to_string(size / GIGA_BYTE) + " GB";
}

float Helper::getRandomFloatBetween0and1() {
    return Helper::distribution(Helper::default_random_engine);
}

std::uniform_real_distribution<float> Helper::distribution = std::uniform_real_distribution<float>(0.0, 1.0);
std::default_random_engine Helper::default_random_engine = std::default_random_engine();

