# Design and purpose - ABBA

## Purpose

General anti-tampering / anti-cheating program to run alongside any arbitrary binary.

## Prevents

Based on templates provided by the application.

- Memory tampering
- Abnormal input
- Invalid configuration
- Invalid network data
- Hash based file tampering protection

## TODOs
- 1 Injected-module detection, instead of snapshots, we could pull an external hash to confirm, we could have something like a whitelist of hashes hosted somewhere externally
because the file could be modified before running and we only make a snapshot at the beginning, alternatively, there is something called
signature verification on windows, WinVerifyTrust (pain), that we could use, on linux i have no idea
- 2 Inline/IAT hook detection, a lot of it done, just need to check correct regions for that mostly windows work, look at patchIat for inspiration
- 3 Checking for hidden threads inside the process itself, the threads we don't recognize/didn't make ourselves
- 4 External handle detection, checking if someone is trying to access the process with PROCESS_VM_WRITE or similar
- 5 Checking if someone is debugging the process (if we do external handle detection this one can basically be skipped, although this is probably easier)