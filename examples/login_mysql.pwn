#include <open.mp>
#include <a_mysql>
#include <omp_argon2>

#define DIALOG_REGISTER 1
#define DIALOG_LOGIN    2

new MySQL:g_SQL;

main() {
    print("Server starting...");
}

public OnGameModeInit() {
    new MySQLOpt:option_id = mysql_init_options();
    mysql_set_option(option_id, AUTO_RECONNECT, true);
    g_SQL = mysql_connect("localhost", "samp_user", "your_password", "server_db", option_id);
    
    if(mysql_errno(g_SQL) != 0) {
        print("Connection Failed! Shutting down...");
        SendRconCommand("exit");
        return 1;
    }
    print("Connection Successful!");

    argon2_set_thread_limit(3);
    return 1;
}

public OnGameModeExit()
{
    mysql_close(g_SQL);
    return 1;
}

public OnPlayerConnect(playerid) {
    new name[MAX_PLAYER_NAME + 1], query[128];
    GetPlayerName(playerid, name, sizeof(name));
    
    mysql_format(g_SQL, query, sizeof(query), "SELECT `id` FROM `users` WHERE `username` = '%e' LIMIT 1", name);
    mysql_tquery(g_SQL, query, "OnAccountCheck", "i", playerid);
    return 1;
}

forward OnAccountCheck(playerid);
public OnAccountCheck(playerid) {
    if(!IsPlayerConnected(playerid)) return 1;

    if(cache_num_rows() > 0) {
        ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Login", "Welcome back! Please enter your password to login:", "Login", "Quit");
    } else {
        ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_INPUT, "Register", "Welcome! Please enter a secure password to register:", "Register", "Quit");
    }
    return 1;
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[]) {
    if(dialogid == DIALOG_REGISTER) {
        if(!response) return Kick(playerid);
        
        if(strlen(inputtext) < 4) {
            ShowPlayerDialog(playerid, DIALOG_REGISTER, DIALOG_STYLE_INPUT, "Register", "Password too short!\nPlease enter a secure password to register:", "Register", "Quit");
            return 1;
        }

        // time_cost = 2, memory_cost = 65536 (64 MB), parallelism = 1
        // These are the OWASP recommended settings for Argon2id
        argon2_hash(playerid, "OnPlayerRegisterHash", inputtext, 2, 65536, 1);
        SendClientMessage(playerid, -1, "Generating secure hash... please wait.");
        return 1;
    }

    if(dialogid == DIALOG_LOGIN) {
        if(!response) return Kick(playerid);

        new name[MAX_PLAYER_NAME + 1], query[128];
        GetPlayerName(playerid, name, sizeof(name));

        // Fetch the stored hash from the DB. We pass their plain "inputtext" through to the callback so we can verify it against the DB hash
        mysql_format(g_SQL, query, sizeof(query), "SELECT `password` FROM `users` WHERE `username` = '%e' LIMIT 1", name);
        mysql_tquery(g_SQL, query, "OnAccountFetchHash", "is", playerid, inputtext);
        
        SendClientMessage(playerid, -1, "Verifying password... please wait.");
        return 1;
    }
    return 0;
}

forward OnPlayerRegisterHash(playerid, bool:success, const hash[]);
public OnPlayerRegisterHash(playerid, bool:success, const hash[]) {
    if(!IsPlayerConnected(playerid)) return 1;

    if(!success) {
        SendClientMessage(playerid, -1, "An error occurred while securing your password. Please reconnect.");
        Kick(playerid);
        return 1;
    }

    new name[MAX_PLAYER_NAME + 1], query[512];
    GetPlayerName(playerid, name, sizeof(name));

    mysql_format(g_SQL, query, sizeof(query), "INSERT INTO `users` (`username`, `password`) VALUES ('%e', '%e')", name, hash);
    mysql_tquery(g_SQL, query, "OnPlayerRegistered", "i", playerid);
    return 1;
}

forward OnPlayerRegistered(playerid);
public OnPlayerRegistered(playerid) {
    if(!IsPlayerConnected(playerid)) return 1;

    SendClientMessage(playerid, 0x00FF00FF, "Registration successful! You are now logged in.");
    SpawnPlayer(playerid);
    return 1;
}

forward OnAccountFetchHash(playerid, const inputtext[]);
public OnAccountFetchHash(playerid, const inputtext[]) {
    if(!IsPlayerConnected(playerid)) return 1;

    if(cache_num_rows() == 0) {
        SendClientMessage(playerid, -1, "Account not found in database");
        Kick(playerid);
        return 1;
    }

    // Retrieve the Argon2id string from the database
    new stored_hash[ARGON2_HASH_LENGTH];
    cache_get_value_name(0, "password", stored_hash, sizeof(stored_hash));

    // Send the plaintext inputted password + stored hash to the background worker
    argon2_verify(playerid, "OnPlayerLoginVerify", inputtext, stored_hash);
    return 1;
}

forward OnPlayerLoginVerify(playerid, bool:success);
public OnPlayerLoginVerify(playerid, bool:success) {
    if(!IsPlayerConnected(playerid)) return 1;

    if(success) { // The plaintext password correctly matches the Argon2 hash
        SendClientMessage(playerid, 0x00FF00FF, "Login successful! Welcome back.");
        SpawnPlayer(playerid);
    } else {
        SendClientMessage(playerid, 0xFF0000FF, "Incorrect password!");
        ShowPlayerDialog(playerid, DIALOG_LOGIN, DIALOG_STYLE_PASSWORD, "Login", "Incorrect password!\nPlease enter your password to login:", "Login", "Quit");
    }
    return 1;
}