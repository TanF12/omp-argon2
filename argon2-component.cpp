#include "argon2-component.hpp"
#include "argon2-utils.hpp"
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <Server/Components/Pawn/Impl/pawn_impl.hpp>
#include <argon2.h>
#include <algorithm>

#if defined(_WIN32)
    #include <windows.h>
    #include <bcrypt.h>
#else
    #include <sys/random.h>
    #include <string.h>     
#endif

static inline void SecureWipeString(std::string& str) {
    if (str.empty()) return;
#if defined(_WIN32)
    SecureZeroMemory(str.data(), str.size());
#else
    explicit_bzero(str.data(), str.size());
#endif
    str.clear();
}

static inline bool GenerateSecureSalt(uint8_t* buffer, size_t length) {
#if defined(_WIN32)
    return BCryptGenRandom(NULL, buffer, static_cast<ULONG>(length), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
    ssize_t res = getrandom(buffer, length, GRND_NONBLOCK);
    if (res == static_cast<ssize_t>(length)) return true;

    FILE* f = fopen("/dev/urandom", "re");
    if (f) {
        bool success = fread(buffer, 1, length, f) == length;
        fclose(f);
        return success;
    }
    return false;
#endif
}

Argon2Component::Argon2Component() {
    instance_ = this;
    setThreadLimit(3);
}

Argon2Component::~Argon2Component() {
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        stopWorkers_ = true;
        
        std::queue<ArgonTask> empty;
        std::swap(tasks_, empty); 
    }
    cv_.notify_all();
    
    for (std::thread& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
    
    if (pawn_) pawn_->getEventDispatcher().removeEventHandler(this);
    if (core_) core_->getEventDispatcher().removeEventHandler(this);
}

Argon2Component* Argon2Component::getInstance() {
    if (!instance_) instance_ = new Argon2Component();
    return instance_;
}

void Argon2Component::workerLoop() {
    while (true) {
        ArgonTask task;
        {
            std::unique_lock<std::mutex> lock(taskMutex_);
            cv_.wait(lock, [this] { return stopWorkers_ || !tasks_.empty(); });
            if (stopWorkers_ && tasks_.empty()) return;

            task = std::move(tasks_.front());
            tasks_.pop();
            activeWorkers_.fetch_add(1, std::memory_order_relaxed);
        }

        ArgonResult res;
        res.amx_ = task.amx_;
        res.amx_generation = task.amx_generation;
        res.isHash = task.isHash;
        res.playerid = task.playerid;
        res.callback = std::move(task.callback);
        res.args = std::move(task.args);
        res.success = false;

        if (task.isHash) {
            uint8_t salt[16];
            if (GenerateSecureSalt(salt, sizeof(salt))) {
                
                size_t encoded_len = argon2_encodedlen(task.t_cost, task.m_cost, task.parallelism, sizeof(salt), 32, Argon2_id);
                std::string encoded(encoded_len, '\0');

                int ret = argon2id_hash_encoded(
                    task.t_cost, task.m_cost, task.parallelism,
                    task.input.data(), task.input.size(),
                    salt, sizeof(salt), 32,
                    encoded.data(), encoded.size()
                );

                if (ret == ARGON2_OK) {
                    res.success = true;
                    encoded.erase(std::find(encoded.begin(), encoded.end(), '\0'), encoded.end());
                    res.hash = std::move(encoded);
                }
#if defined(_WIN32)
                SecureZeroMemory(salt, sizeof(salt));
#else
                explicit_bzero(salt, sizeof(salt));
#endif
            }
        } else {
            int ret = argon2id_verify(task.hash.c_str(), task.input.data(), task.input.size());
            res.success = (ret == ARGON2_OK);
        }

        SecureWipeString(task.input);

        {
            std::lock_guard<SpinLock> lock(resultSpinLock_); 
            results_.push(std::move(res));
        }
        
        activeWorkers_.fetch_sub(1, std::memory_order_relaxed);
    }
}

void Argon2Component::enqueueTask(ArgonTask task) {
    std::lock_guard<std::mutex> lock(taskMutex_);
    tasks_.push(std::move(task));
    cv_.notify_one();
}

void Argon2Component::setThreadLimit(size_t limit) {
    if (limit < 1 || limit == threadLimit_) return;

    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        
        if (!tasks_.empty() || activeWorkers_ > 0) {
            if (core_) core_->printLn("[Argon2] Cannot change thread limit while tasks are running. Call this in OnGameModeInit.");
            return;
        }

        stopWorkers_ = true;
    }

    cv_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
    
    {
        std::lock_guard<std::mutex> lock(taskMutex_);
        workers_.clear();
        stopWorkers_ = false;
        threadLimit_ = limit;
        for (size_t i = 0; i < threadLimit_; ++i) {
            workers_.emplace_back(&Argon2Component::workerLoop, this);
        }
    }
}

void Argon2Component::onLoad(ICore* c) {
    core_ = c;
    core_->printLn("open.mp Argon2 component loaded.");
    core_->getEventDispatcher().addEventHandler(this);
    setAmxLookups(core_);
}

void Argon2Component::onInit(IComponentList* components) {
    pawn_ = components->queryComponent<IPawnComponent>();
    if (pawn_) {
        setAmxFunctions(pawn_->getAmxFunctions());
        setAmxLookups(components);
        pawn_->getEventDispatcher().addEventHandler(this);
    }
}

void Argon2Component::onFree(IComponent* component) {
    if (component == pawn_) {
        pawn_ = nullptr;
        setAmxFunctions();
        setAmxLookups();
    }
}

void Argon2Component::onAmxLoad(IPawnScript& script) {
    pawn_natives::AmxLoad(script.GetAMX());
    amxGenerations_[script.GetAMX()] = ++currentGeneration_;
}

void Argon2Component::onAmxUnload(IPawnScript& script) {
    AMX* unloadedAmx = script.GetAMX();
    amxGenerations_.erase(unloadedAmx);

    std::lock_guard<std::mutex> lock(taskMutex_);
    size_t count = tasks_.size();
    for (size_t i = 0; i < count; ++i) {
        ArgonTask t = std::move(tasks_.front());
        tasks_.pop();
        if (t.amx_ != unloadedAmx) {
            tasks_.push(std::move(t));
        }
    }
}

void Argon2Component::onTick(Microseconds elapsed, TimePoint now) {
    if (!pawn_) return;

    std::queue<ArgonResult> batch;
    {
        std::lock_guard<SpinLock> lock(resultSpinLock_);
        if (results_.empty()) return; 
        batch.swap(results_);
    }

    while (!batch.empty()) {
        ArgonResult res = std::move(batch.front());
        batch.pop();

        auto it = amxGenerations_.find(res.amx_);
        if (it == amxGenerations_.end() || it->second != res.amx_generation) {
            continue;
        }

        AMX* amx = res.amx_;
        int index;

        if (amx_FindPublic(amx, res.callback.c_str(), &index) == AMX_ERR_NONE) {
            cell old_hea = amx->hea;

            for (auto arg_it = res.args.rbegin(); arg_it != res.args.rend(); ++arg_it) {
                if (arg_it->type == CallbackArg::Type::String) {
                    cell amx_addr, *phys_addr;
                    amx_PushString(amx, &amx_addr, &phys_addr, arg_it->stringValue.c_str(), 0, 0);
                } else {
                    amx_Push(amx, arg_it->cellValue);
                }
            }

            if (res.isHash) {
                cell amx_addr, *phys_addr;
                amx_PushString(amx, &amx_addr, &phys_addr, res.success ? res.hash.c_str() : "", 0, 0);
            }

            amx_Push(amx, res.success ? 1 : 0);
            amx_Push(amx, res.playerid);

            cell retval;
            amx_Exec(amx, &retval, index);

            amx->hea = old_hea;
        }
    }
}