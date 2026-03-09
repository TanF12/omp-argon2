#pragma once

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <atomic>
#include <unordered_map>
#include "argon2-utils.hpp"

struct CallbackArg {
    enum class Type { Int, Float, String } type = Type::Int;
    cell cellValue = 0;
    std::string stringValue;
};

struct ArgonTask {
    AMX* amx_ = nullptr;
    uint64_t amx_generation = 0;
    bool isHash = false;
    int playerid = -1;
    std::string callback;
    std::string input;
    std::string hash;
    uint32_t t_cost = 0;
    uint32_t m_cost = 0;
    uint32_t parallelism = 0;
    std::vector<CallbackArg> args;
};

struct ArgonResult {
    AMX* amx_ = nullptr;
    uint64_t amx_generation = 0;
    bool isHash = false;
    int playerid = -1;
    std::string callback;
    bool success = false;
    std::string hash;
    std::vector<CallbackArg> args;
};

class Argon2Component final 
    : public IComponent
    , public PawnEventHandler
    , public CoreEventHandler
{
private:
    ICore* core_ = nullptr;
    IPawnComponent* pawn_ = nullptr;

    std::vector<std::thread> workers_;
    std::queue<ArgonTask> tasks_;
    std::queue<ArgonResult> results_;
    
    std::mutex taskMutex_;
    SpinLock resultSpinLock_;
    std::condition_variable cv_;
    
    bool stopWorkers_ = false;
    size_t threadLimit_ = 0; 
    
    std::atomic<int> activeWorkers_{0};

    inline static Argon2Component* instance_ = nullptr;

    void workerLoop();

private:
    std::unordered_map<AMX*, uint64_t> amxGenerations_;
    uint64_t currentGeneration_ = 0;

public:
    uint64_t getAmxGeneration(AMX* amx) {
        auto it = amxGenerations_.find(amx);
        return (it != amxGenerations_.end()) ? it->second : 0;
    }

public:
    PROVIDE_UID(0x34D89EC50658252C);

    Argon2Component();
    ~Argon2Component();

    static Argon2Component* getInstance();

    void enqueueTask(ArgonTask task);
    void setThreadLimit(size_t limit);

    StringView componentName() const override { return "open.mp Argon2 Component"; }
    SemanticVersion componentVersion() const override { return SemanticVersion(0, 1, 1, 3); }

    void onLoad(ICore* c) override;
    void onInit(IComponentList* components) override;
    void onReady() override {}
    void onFree(IComponent* component) override;
    void free() override { delete this; }
    void reset() override {}

    void onAmxLoad(IPawnScript& script) override;
    void onAmxUnload(IPawnScript& script) override;

    void onTick(Microseconds elapsed, TimePoint now) override;
};