# open.mp Argon2 Component

An implementation of the Argon2id password hashing algorithm for the open.mp server. 


## Pawn API Reference

```pawn
// Generates an Argon2id hash asynchronously.
// Dispatches OnPasswordHashed(playerid, bool:success, const hash[], args...) upon completion.
native argon2_hash(playerid, const callback[], const input[], time_cost, memory_cost, parallelism, const args[] = "", {Float, _}:...);

// Verifies a plaintext input against an existing Argon2id hash asynchronously.
// Dispatches OnPasswordVerified(playerid, bool:success, args...) upon completion.
native argon2_verify(playerid, const callback[], const input[], const hash[], const args[] = "", {Float, _}:...);

// Adjusts the maximum number of background worker threads. Must be called before tasks are enqueued.
native argon2_set_thread_limit(value);
```
