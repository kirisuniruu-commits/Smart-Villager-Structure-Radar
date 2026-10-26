# Filling in game signatures

Smart Villager & Structure Radar - created by ChrisNiruu

`libminecraftpe.so` is stripped, so hooks are found by byte pattern. Patterns and struct offsets are valid for
**one** Minecraft build only (the game version you run in LeviLauncher).

1. Open the matching `libminecraftpe.so` (arm64) in a disassembler (IDA / Ghidra).
2. Find the functions listed in `src/Signatures.hpp` (`ClientInstance::update`, `Level::getRuntimeActorList`).
   String anchors that exist in the binary: `minecraft:villager_v2`, `minecraft:zombie_villager_v2`,
   `minecraft:trade_table`, `minecraft:economy_trade_table`.
3. Make a unique pattern (wildcard relocated bytes with `??`) and paste it into `kEntries`.
4. Fill `kOffsets` (local player, position, rotation, villager profession/tier/restock).
5. Implement `NativeSource::poll()` in `src/GameBridge.cpp` to fill a `radar::Snapshot`:
   camera (eye pos, yaw, pitch, FOV), villagers, workstations, structures.
6. Check LeviLauncher logcat for `native bridge: signatures resolved`.

Because structure positions are server-side data in multiplayer, structure features only work where the client
has the data (single-player / local worlds).
