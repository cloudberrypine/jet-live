
#include "CodeReloadPipeline.hpp"
#include <cassert>
#include <chrono>
#include "jet/live/LiveContext.hpp"

namespace jet
{
    namespace
    {
        long long elapsedMillis(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end)
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        }
    }

    void CodeReloadPipeline::addStep(std::unique_ptr<ICodeReloadPipelineStep>&& step)
    {
        m_steps.emplace_back(std::move(step));
    }

    void CodeReloadPipeline::reload(LiveContext* context, Program* newProgram)
    {
        assert(newProgram);

        const auto totalStart = std::chrono::steady_clock::now();
        int stepIndex = 0;
        for (const auto& step : m_steps) {
            const auto stepStart = std::chrono::steady_clock::now();
            step->reload(context, newProgram);
            const auto stepMs = elapsedMillis(stepStart, std::chrono::steady_clock::now());
            context->events->addLog(
                LogSeverity::kInfo,
                "Reload step #" + std::to_string(++stepIndex) + ": " + std::to_string(stepMs) + " ms");
        }
        const auto totalMs = elapsedMillis(totalStart, std::chrono::steady_clock::now());
        context->events->addLog(LogSeverity::kInfo, "Reload pipeline total: " + std::to_string(totalMs) + " ms");
    }
}
