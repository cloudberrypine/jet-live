
#pragma once

#include <string>
#include <vector>

namespace jet
{
    /**
     * Configuration parameters.
     */
    struct LiveConfig
    {
        /**
         * The maximum amount of possible worker threads used by the library.
         * Usually all these threads are busy compiling new code.
         */
        size_t workerThreadsCount = 4;

        /**
         * If `true`, also reload code when app receives `SIGUSR1`.
         */
        bool reloadOnSignal = true;

        /**
         * Optional absolute path to compile_commands.json.
         * If empty, default discovery near executable is used.
         */
        std::string compileCommandsPath;

        /**
         * If true, skip static/global relocation and only hook functions.
         */
        bool functionOnlyReload = false;

        /**
         * If true and functionOnlyReload is enabled, compiles reload units with
         * reduced debug info for faster turnaround (injects -g0).
         */
        bool fastCompileForFunctionReload = true;
    };
}
