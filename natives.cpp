#include "argon2-component.hpp"
#include <Server/Components/Pawn/Impl/pawn_natives.hpp>

// native argon2_hash(playerid, const callback[], const input[], time_cost, memory_cost, parallelism, const format[] = "", {Float, _}:...);
SCRIPT_API(argon2_hash, bool(int playerid, const std::string& callback, const std::string& input, int t_cost, int m_cost, int parallelism, const std::string& format))
{
    AMX* amx = GetAMX();

    ArgonTask task;
    task.amx_ = amx;
    task.isHash = true;
    task.playerid = playerid;
    task.callback = callback;
    task.input = input;
    task.t_cost = t_cost;
    task.m_cost = m_cost;
    task.parallelism = parallelism;

    cell* params = GetParams();
    int param_idx = 8; 
    int num_args = params[0] / sizeof(cell);

    for (char c : format) {
        if (param_idx > num_args) break; 

        CallbackArg arg;
        
        cell param_val = params[param_idx++]; 
        cell* phys_addr;        
        amx_GetAddr(amx, param_val, &phys_addr); 

        if (c == 'd' || c == 'i' || c == 'f') {
            arg.type = (c == 'f') ? CallbackArg::Type::Float : CallbackArg::Type::Int;
            arg.cellValue = *phys_addr; 
        } else if (c == 's') {
            arg.type = CallbackArg::Type::String;
            int len;
            amx_StrLen(phys_addr, &len);
            
            arg.stringValue.assign(len, '\0');
            amx_GetString(arg.stringValue.data(), phys_addr, 0, len + 1);
        } else {
            continue;
        }
        task.args.push_back(std::move(arg));
    }

    if (auto comp = Argon2Component::getInstance()) {
        comp->enqueueTask(std::move(task));
        return true;
    }
    return false;
}

// native argon2_verify(playerid, const callback[], const input[], const hash[], const format[] = "", {Float, _}:...);
SCRIPT_API(argon2_verify, bool(int playerid, const std::string& callback, const std::string& input, const std::string& hash, const std::string& format))
{
    AMX* amx = GetAMX();

    ArgonTask task;
    task.amx_ = amx;
    task.isHash = false;
    task.playerid = playerid;
    task.callback = callback;
    task.input = input;
    task.hash = hash;

    cell* params = GetParams();
    int param_idx = 6; 
    
    int num_args = params[0] / sizeof(cell);

    for (char c : format) {
        if (param_idx > num_args) break;

        CallbackArg arg;
        
        cell param_val = params[param_idx++]; 
        cell* phys_addr;        
        amx_GetAddr(amx, param_val, &phys_addr);

        if (c == 'd' || c == 'i' || c == 'f') {
            arg.type = (c == 'f') ? CallbackArg::Type::Float : CallbackArg::Type::Int;
            arg.cellValue = *phys_addr; 
        } else if (c == 's') {
            arg.type = CallbackArg::Type::String;
            int len;
            amx_StrLen(phys_addr, &len);
            
            arg.stringValue.assign(len, '\0');
            amx_GetString(arg.stringValue.data(), phys_addr, 0, len + 1);
        } else {
            continue;
        }
        task.args.push_back(std::move(arg));
    }

    if (auto comp = Argon2Component::getInstance()) {
        comp->enqueueTask(std::move(task));
        return true;
    }
    return false;
}

// native argon2_set_thread_limit(value);
SCRIPT_API(argon2_set_thread_limit, bool(int value))
{
    if (value < 1) return false;

    if (auto comp = Argon2Component::getInstance()) {
        comp->setThreadLimit(value);
        return true;
    }
    return false;
}