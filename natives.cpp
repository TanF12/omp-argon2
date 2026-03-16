#include "argon2-component.hpp"
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>
#include <algorithm>

// native argon2_hash(playerid, const callback[], const input[], time_cost, memory_cost, parallelism, const format[] = "", {Float, _}:...);
SCRIPT_API(argon2_hash, bool(int playerid, const std::string& callback, cell input_addr, int t_cost, int m_cost, int parallelism, const std::string& format))
{
    AMX* amx = GetAMX();

    if (parallelism < 1) parallelism = 1;
    if (parallelism > 4) parallelism = 4;

    ArgonTask task;
    task.amx_ = amx;
    task.isHash = true;
    task.playerid = playerid;
    task.callback = callback;
    
    cell* phys_input = nullptr;
    amx_GetAddr(amx, input_addr, &phys_input);
    if (phys_input != nullptr) {
        int len;
        amx_StrLen(phys_input, &len);
        task.input.resize(len + 1);
        amx_GetString(task.input.data(), phys_input, 0, len + 1);
        task.input.pop_back();
    }

    task.t_cost = t_cost;
    task.m_cost = m_cost;
    task.parallelism = parallelism;

    cell* params = GetParams();
    int param_idx = 8; 
    int num_args = params[0] / sizeof(cell);

    task.args.reserve(format.size());
    for (char c : format) {
        if (param_idx > num_args) break;
        if (c != 'd' && c != 'i' && c != 'f' && c != 's') continue;

        CallbackArg arg;
        cell param_val = params[param_idx++]; 
        cell* phys_addr = nullptr;
        amx_GetAddr(amx, param_val, &phys_addr);

        if (phys_addr == nullptr) continue;

        if (c == 'd' || c == 'i' || c == 'f') {
            arg.type = (c == 'f') ? CallbackArg::Type::Float : CallbackArg::Type::Int;
            arg.cellValue = *phys_addr;
        } else if (c == 's') {
            arg.type = CallbackArg::Type::String;
            int len;
            amx_StrLen(phys_addr, &len);
            
            arg.stringValue.resize(len + 1);
            amx_GetString(arg.stringValue.data(), phys_addr, 0, len + 1);
            arg.stringValue.pop_back();
        }
        task.args.push_back(std::move(arg));
    }

    if (auto comp = Argon2Component::getInstance()) {
        task.amx_generation = comp->getAmxGeneration(amx);
        comp->enqueueTask(std::move(task));
        return true;
    }
    return false;
}

// native argon2_verify(playerid, const callback[], const input[], const hash[], const format[] = "", {Float, _}:...);
SCRIPT_API(argon2_verify, bool(int playerid, const std::string& callback, cell input_addr, const std::string& hash, const std::string& format))
{
    AMX* amx = GetAMX();

    ArgonTask task;
    task.amx_ = amx;
    task.isHash = false;
    task.playerid = playerid;
    task.callback = callback;
    
    cell* phys_input = nullptr;
    amx_GetAddr(amx, input_addr, &phys_input);
    if (phys_input != nullptr) {
        int len;
        amx_StrLen(phys_input, &len);
        task.input.resize(len + 1);
        amx_GetString(task.input.data(), phys_input, 0, len + 1);
        task.input.pop_back();
    }
    
    task.hash = hash;

    cell* params = GetParams();
    int param_idx = 6; 
    
    int num_args = params[0] / sizeof(cell);
    task.args.reserve(format.size());

    for (char c : format) {
        if (param_idx > num_args) break;

        CallbackArg arg;
        cell param_val = params[param_idx++];
        cell* phys_addr = nullptr;

        amx_GetAddr(amx, param_val, &phys_addr);

        if (phys_addr == nullptr) continue;

        if (c == 'd' || c == 'i' || c == 'f') {
            arg.type = (c == 'f') ? CallbackArg::Type::Float : CallbackArg::Type::Int;
            arg.cellValue = *phys_addr;
        } else if (c == 's') {
            arg.type = CallbackArg::Type::String;
            int len;
            amx_StrLen(phys_addr, &len);
            
            arg.stringValue.resize(len + 1);
            amx_GetString(arg.stringValue.data(), phys_addr, 0, len + 1);
            arg.stringValue.pop_back();
        }
        task.args.push_back(std::move(arg));
    }

    if (auto comp = Argon2Component::getInstance()) {
        task.amx_generation = comp->getAmxGeneration(amx);
        comp->enqueueTask(std::move(task));
        return true;
    }
    return false;
}

// native argon2_set_thread_limit(value);
SCRIPT_API(argon2_set_thread_limit, bool(int value))
{
    if (value < 1) value = 1;
    if (value > 8) value = 8; // Maximum for SA-MP server

    if (auto comp = Argon2Component::getInstance()) {
        return comp->setThreadLimit(value);
    }
    return false;
}