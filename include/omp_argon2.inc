#if defined _inc_omp_argon2
	#undef _inc_omp_argon2
#endif

#if defined _omp_argon2_included
	#endinput
#endif
#define _omp_argon2_included

#if !defined ARGON2_HASH_LENGTH
	#define ARGON2_HASH_LENGTH (256)
#endif

/*
argon2_hash(playerid, const callback[], const input[], time_cost, memory_cost, parallelism, const format[] = "", {Float, _}:...)
	Params:
		`playerid` - ID of the player
		`callback[]` - Callback to be called after the hashing process
		`input[]` - text/password to be hashed
		`time_cost` - time iterations (e.g.: 2)
		`memory_cost` - memory use in kilobytes (ex: 65536 for 64MB)
		`parallelism` - number of Argon2 threads (ex: 1)
		`format[]` - format string for custom arguments (e.g., "ids")
		`...` - custom arguments to be passed
		
	Expected Callback:
		forward OnPasswordHashed(playerid, bool:success, const hash[], args...);
*/
native argon2_hash(playerid, const callback[], const input[], time_cost, memory_cost, parallelism, const format[] = "", {Float, _}:...);

/*
argon2_verify(playerid, const callback[], const input[], const hash[], const format[] = "", {Float, _}:...)
	Params:
		`playerid` - ID of the player
		`callback[]` - Callback to be called after verifying
		`input[]` - Plaintext password to be compared
		`hash[]` - Stored hash to be compared
		`format[]` - Format string for custom arguments
		`...` - Custom arguments
		
	Expected Callback:
		forward OnPasswordVerified(playerid, bool:success, args...);
*/
native argon2_verify(playerid, const callback[], const input[], const hash[], const format[] = "", {Float, _}:...);

/*
argon2_set_thread_limit(value)
	Defines the maximum number of worker threads (default: 3).
*/
native argon2_set_thread_limit(value);