// Exercise the real filesystem backend: each macOS directory watch has its
// own dispatch queue, so saves in different directories enter jet concurrently.
#include "jet/live/FileWatcher.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
    namespace fs = std::filesystem;
    using namespace std::chrono;
    constexpr int directoryCount = 8;
    constexpr int filesPerDirectory = 12;
    char pattern[] = "/tmp/jet-live-watcher-XXXXXX";
    const char* temporary = mkdtemp(pattern);
    if (!temporary) return 1;
    const fs::path root(temporary);
    std::unordered_set<std::string> directories;
    for (int dir = 0; dir < directoryCount; ++dir) {
        const auto path = root / std::to_string(dir);
        fs::create_directory(path);
        directories.insert(path.string());
    }
    const auto owner = std::this_thread::get_id();
    std::unordered_set<std::string> observed;
    bool wrongThread = false;
    size_t delivered = 0;
    {
        jet::FileWatcher watcher(directories, [&](const jet::FileWatcher::Event& event) {
            wrongThread |= std::this_thread::get_id() != owner;
            observed.insert(event.directory + event.filename);
            ++delivered;
        }, [](const std::string&, const std::string& filename) {
            return filename.size() > 4 && filename.substr(filename.size() - 4) == ".cpp";
        });
        auto save = [&](int dir, int file, int version) {
            const auto path = root / std::to_string(dir) / (std::to_string(file) + ".cpp");
            const auto staging = path.string() + ".tmp";
            { std::ofstream out(staging); out << "// version " << version << '\n'; }
            fs::rename(staging, path);
            chmod(path.c_str(), version % 2 ? 0600 : 0644);
        };
        std::atomic<int> finished{0};
        std::vector<std::thread> writers;
        for (int dir = 0; dir < directoryCount; ++dir) {
            writers.emplace_back([&, dir] {
                for (int version = 0; version < 160; ++version) {
                    for (int file = 0; file < filesPerDirectory; ++file) save(dir, file, version);
                    std::this_thread::sleep_for(milliseconds(5));
                }
                ++finished;
            });
        }
        while (finished != directoryCount) {
            watcher.update();
            std::this_thread::sleep_for(milliseconds(1));
        }
        for (auto& writer : writers) writer.join();
        auto drain = [&](milliseconds duration) {
            const auto until = steady_clock::now() + duration;
            do {
                watcher.update();
                std::this_thread::sleep_for(milliseconds(1));
            } while (steady_clock::now() < until);
        };
        drain(milliseconds(500));
        observed.clear();
        // A save after the debounce window must still arrive for every path.
        for (int dir = 0; dir < directoryCount; ++dir)
            for (int file = 0; file < filesPerDirectory; ++file) save(dir, file, 1001);
        drain(milliseconds(1000));
    }
    fs::remove_all(root);
    std::cout << "delivered=" << delivered << " final_paths=" << observed.size()
              << " expected_paths=" << directoryCount * filesPerDirectory
              << " wrong_thread=" << wrongThread << std::endl;
    return wrongThread || observed.size() != directoryCount * filesPerDirectory ? 1 : 0;
}
