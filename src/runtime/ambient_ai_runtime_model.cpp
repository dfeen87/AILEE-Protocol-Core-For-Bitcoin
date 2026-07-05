#include "ambient_ai_runtime_model.hpp"
#include "ambient_ai_runtime_state_machine.hpp"
#include "ambient_ai_subsystem_integration.hpp"
#include "ambient_ai_epoch.hpp"

namespace ailee {
namespace runtime {

void AmbientRuntimeEventLoop::tick(uint64_t logicalTimestamp) {
    (void)logicalTimestamp;
}

bool AmbientRuntimeEventLoop::enqueueTask(const std::vector<uint8_t>& serializedTask) {
    (void)serializedTask;
    return false;
}

void AmbientRuntimeScheduler::executeScheduledTasks(uint64_t logicalTimestamp) {
    (void)logicalTimestamp;
}

bool AmbientRuntimeScheduler::validateTaskConstraints(const std::vector<uint8_t>& serializedTask) const {
    (void)serializedTask;
    return false;
}

AmbientRuntime::AmbientRuntime(const AmbientRuntimeConfig& config) : isRunning(false) {
    (void)config;
}

AmbientRuntime::~AmbientRuntime() {}

bool AmbientRuntime::initialize() {
    isRunning = true;
    return true;
}

void AmbientRuntime::step(uint64_t logicalTimestamp, uint64_t currentBitcoinHeight) {
    if (!isRunning) return;
    (void)logicalTimestamp;
    (void)currentBitcoinHeight;
}

void AmbientRuntime::shutdown() {
    isRunning = false;
}

}
}
