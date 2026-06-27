# 3.4.3 Protocol Parity Verification & Phase 2 Handover

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to work this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Tasks 1–4 are **investigation/verification** (they produce findings + decisions, not TDD code); Task 5 onward is the Phase 2 implementation roadmap.

**Goal:** Verify that the GCM nonce forward-gap fix matches how the reference 3.4.3 cores (Xian55 server + HermesProxy client-side) handle the client→server counter, scan AzerothCore for the remaining *fundamental* protocol divergences from a real 3.4.3.54261 core, and hand the native port off to a fresh conversation positioned to start Phase 2 (real character enumeration → character create → world entry).

**Architecture:** AzerothCore (WoW 3.3.5a / build 12340 emulator) is being natively forked to speak the modern **WotLK Classic 3.4.3.54261** world protocol (fork & replace, not a proxy). The realm/world handshake, AES-128-GCM framing, flat uint16 opcodes, and an (empty) character-select are working end-to-end against the **real patched client**. The world handshake reaches character-select; Phase 2 fills in characters and world entry.

**Tech Stack:** C++20, CMake + vcpkg (Visual Studio multi-config, `RelWithDebInfo`), MySQL (acore_auth/characters/world/hotfixes), OpenSSL 3.2+ (AES-GCM, Ed25519ctx), Asio (`Acore::Net`).

---

## 0. Handover Context (read this first)

### 0.1 Current milestone — WORKING

A real, patched 3.4.3.54261 client completes the **entire native world handshake and reaches a stable character-select screen** (empty roster — the test account has no characters). Proven live this session:

```
V2 banner → SMSG_AUTH_CHALLENGE (16-byte) → CMSG_AUTH_SESSION (RealmJoinTicket "4#1")
→ SHA-256 digest w/ win64AuthSeed → SessionKeyGenerator<SHA256> 40-byte key
→ SMSG_ENTER_ENCRYPTED_MODE (Ed25519ctx-signed) → CMSG_ENTER_ENCRYPTED_MODE_ACK
→ AddSession → SMSG_AUTH_RESPONSE (modern, ERROR_OK=0) → SMSG_CACHE_VERSION → SMSG_TUTORIAL_FLAGS
→ CMSG_ENUM_CHARACTERS → SMSG_ENUM_CHARACTERS_RESULT (empty, valid 21-byte packet)
→ stable character-select (no disconnect)
```

### 0.2 What was fixed THIS session (all UNCOMMITTED — commit first, see Task 1)

1. **Character enumeration brick** — bound `CMSG_ENUM_CHARACTERS` (13801) → `HandleCharEnumOpcode`, rewrote `HandleCharEnum` to emit the modern `SMSG_ENUM_CHARACTERS_RESULT` (9603 / 0x2583), added the `EnumCharactersResult` + `CharacterInfo` packet structs (byte-verified against Xian55 `CharacterPackets.cpp`), bound `CMSG_SERVER_TIME_OFFSET_REQUEST` (13980) → `SMSG_SERVER_TIME_OFFSET` (10004). Reviewed (spec byte-layout + code quality) — both passed. Real-character population is deferred (see 0.5).
2. **GCM nonce forward-gap tolerance** — `WorldPacketCrypt::DecryptRecv` now probes a small forward window of nonce counters on a tag-verify miss. **This is the fix that stopped the post-login disconnect.** Root cause (byte-accounting proven): the 3.4.3 client advances its outgoing GCM nonce *without transmitting a packet* (it encrypts a queued Battle.net store query, then cancels the send after a server reply), leaving a forward gap in the client→server counter sequence. AC's strictly-sequential receive counter then mismatched every following packet. GCM tag still authenticates and the counter only moves forward, so no replay window opens. **NOTE: Task 2 must verify this is the right fix vs the alternative (completing SMSG_AUTH_RESPONSE so the client never cancels).**
3. **Removed stray `SendAddonsInfo()`** from `WorldSession::InitializeSessionCallback` — the reference 3.4.3 core does not send the legacy `SMSG_ADDON_INFO` at char-select (its opcode isn't even mapped in the 54261 table; it was emitting a garbage/truncated opcode). Matches Xian55 + Hermes (Hermes literally has it commented out).

### 0.3 Files changed this session (uncommitted)

```
src/common/Cryptography/Authentication/WorldPacketCrypt.h     # MaxRecvCounterSkip + DecryptRecv decl
src/common/Cryptography/Authentication/WorldPacketCrypt.cpp    # forward-gap-tolerant DecryptRecv
src/server/game/Server/Packets/CharacterPackets.h             # EnumCharactersResult + CharacterInfo + sub-structs
src/server/game/Server/Packets/CharacterPackets.cpp           # their Write()/operator<<
src/server/game/Server/Packets/MiscPackets.h                  # ServerTimeOffset
src/server/game/Server/Packets/MiscPackets.cpp                # ServerTimeOffset::Write
src/server/game/Handlers/CharacterHandler.cpp                 # HandleCharEnum -> modern packet
src/server/game/Handlers/MiscHandler.cpp                      # HandleServerTimeOffsetRequest
src/server/game/Server/Protocol/Opcodes.cpp                   # 2 handler bindings
src/server/game/Server/WorldSession.h                         # HandleServerTimeOffsetRequest decl
src/server/game/Server/WorldSession.cpp                       # removed SendAddonsInfo()
```
`WorldSocket.cpp` was used for diagnostics this session but all of it was reverted — it is net-zero (matches commit `75410cbbb`). Do **not** expect changes there.

### 0.4 How to build & run (verified working this session)

```powershell
# Build only the target you need (NEVER build unit_tests, NEVER a full build). VS multi-config + vcpkg:
cmake --build D:/Games/azerothcore-wotlk/build --target game --config RelWithDebInfo          # compiles game.lib
cmake --build D:/Games/azerothcore-wotlk/build --target worldserver --config RelWithDebInfo   # links the exe
# bnetserver only needs rebuilding if you touch auth/bnet code.

# Run (worldserver listens on 8085, bnetserver on 1119+8081). Stop the old worldserver first (exe lock):
Get-Process worldserver -ErrorAction SilentlyContinue | Stop-Process -Force
$dir = "D:\Games\azerothcore-wotlk\build\bin\RelWithDebInfo"
Start-Process -FilePath "$dir\worldserver.exe" -WorkingDirectory $dir -WindowStyle Minimized
# Ready when Get-NetTCPConnection -LocalPort 8085 -State Listen returns. Log: $dir\Server.log
```
- MySQL: 127.0.0.1:3306 root/1234. DBs: `acore_auth`, `acore_characters`, `acore_world`, `acore_hotfixes`. Test account world ticket `gameaccount` = `account.username` = `"4#1"`, account id 5.
- **Client-side caveat:** the patched client needs its Ed25519 public key swapped to TC's (wow-patcher wrongly skips Ed25519 for "Classic"; 54261 DOES use it for EnterEncryptedMode). The user runs the client.

### 0.5 Known deferred work (do NOT treat as bugs)

- **Empty character list:** `HandleCharEnum` builds `CharacterInfo` only from the legacy `CHAR_SEL_ENUM` fields it can map (guid/name/race/class/sex/level/zone/map/pos/firstLogin). **Customizations, VisualItems (equipment), SpecID, guild GUID, pet are left default** — they need the modern `character_customizations` schema + DB2 `ChrCustomization` mapping (AC's `characters` table is legacy 3.3.5a; guilds are uint32, there is no `HighGuid::Guild`). TODOs are in `CharacterHandler.cpp::HandleCharEnum`.
- **`SMSG_AUTH_RESPONSE` is minimal:** `WorldSession::SendAuthResponse` (AuthHandler.cpp) sets only ActiveExpansionLevel/AccountExpansionLevel/VirtualRealmAddress/Time/VirtualRealms. TC's `AuthResponse::SuccessInfo` carries far more (currencies, game-time, race/class availability, char templates, etc.). See Task 2 — a complete AuthResponse may make the client stop cancelling its store query, which would eliminate the nonce gap at the source.
- **~1100 opcodes are `STATUS_UNHANDLED` stubs** from the brick-B re-value. They get bound per-feature.
- **`session_key_bnet binary(64)` padding:** LENGTH() always reads 64, so a continued-session LENGTH=40 check would never match (E2b concern, not yet hit).

### 0.6 Primary reference sources (use these, in order)

- **Xian55/3.4.3_Source** (TC-based 3.4.3 server) cloned at `D:/Games/xian55-3.4.3_Source`. Server-side ground truth. **Caveat:** it targets a build *near* 54261; treat WDC4/WDC5 (3.4.3.54261 vs 3.4.4.61581) format deltas as suspect and re-verify against the live client.
- **HermesProxy (54261)** extracted C# files in `D:/Games/azerothcore-wotlk/extract-test/hermes_*.cs` — proven byte-exact vs the live client. `hermes_PacketCrypt.cs`, `hermes_worldsocket.cs`, `hermes_opcode_54261.cs`, `hermes_AuthSession.cs`, `hermes_EnterEncryptedMode.cs` are the authoritative client-behaviour reference.
- **TC extract** in `extract-test/tc_*.cpp` (WorldSocket, CharacterHandler, AuthenticationPackets, ByteBuffer, PacketUtilities, opcodes).
- Project knowledge base: `docs/wotlk-classic-3.4.3/KNOWLEDGE.md`. Design spec: `docs/superpowers/specs/2026-06-26-azerothcore-wotlk-classic-3.4.3-design.md`. Prior plans in `docs/superpowers/plans/`.

---

## Task 1: Commit the working stack (do this first)

**Files:** (the 11 uncommitted files in 0.3)

- [ ] **Step 1: Lint the C++ you touched**

Run: `python apps/codestyle/codestyle-cpp.py`
Expected: `Everything looks good`

- [ ] **Step 2: Confirm a clean per-target build**

Run: `cmake --build D:/Games/azerothcore-wotlk/build --target worldserver --config RelWithDebInfo`
Expected: `worldserver.vcxproj -> ...worldserver.exe`, no errors.

- [ ] **Step 3: Commit the char-enum brick + crypto fix together**

```bash
git add src/common/Cryptography/Authentication/WorldPacketCrypt.h \
  src/common/Cryptography/Authentication/WorldPacketCrypt.cpp \
  src/server/game/Server/Packets/CharacterPackets.h \
  src/server/game/Server/Packets/CharacterPackets.cpp \
  src/server/game/Server/Packets/MiscPackets.h \
  src/server/game/Server/Packets/MiscPackets.cpp \
  src/server/game/Handlers/CharacterHandler.cpp \
  src/server/game/Handlers/MiscHandler.cpp \
  src/server/game/Server/Protocol/Opcodes.cpp \
  src/server/game/Server/WorldSession.h \
  src/server/game/Server/WorldSession.cpp
git commit   # message: feat(world-protocol): char enumeration + GCM nonce forward-gap tolerance (stable char-select)
```
Body should record: the modern SMSG_ENUM_CHARACTERS_RESULT/SMSG_SERVER_TIME_OFFSET bindings; the byte-accounting-proven nonce-gap root cause + the forward-window fix; the SendAddonsInfo removal. End with the project's `Co-Authored-By` trailer. **Ask the user before committing if repo policy requires it** (CLAUDE.md forbids unrequested state-changing git) — the user has been committing milestones explicitly.

---

## Task 2: Verify how the 3.4.3 reference handles the client→server nonce gap

**Goal:** Decide whether the forward-gap tolerance in `DecryptRecv` is the *correct* fix or a band-aid that masks a missing `SMSG_AUTH_RESPONSE` field. The reference servers (Xian55) use **strictly sequential** counters (confirmed: `_clientCounter++` per decrypt, no window) and work with the same client — so either (a) the reference's complete AuthResponse stops the client from cancelling its queued store packet (no gap ever appears), or (b) the reference tolerates gaps somewhere we haven't found.

**Files (read-only reference):**
- `D:/Games/xian55-3.4.3_Source/src/common/Cryptography/Authentication/WorldPacketCrypt.cpp`
- `D:/Games/xian55-3.4.3_Source/src/server/game/Server/WorldSocket.cpp` (ReadHandler/ReadDataHandler)
- `D:/Games/xian55-3.4.3_Source/src/server/game/Server/Packets/AuthenticationPackets.cpp` (`AuthResponse::Write` + `SuccessInfo`)
- `extract-test/hermes_PacketCrypt.cs`, `extract-test/hermes_worldsocket.cs`
- AC: `src/server/game/Handlers/AuthHandler.cpp::SendAuthResponse`, `src/server/game/Server/Packets/AuthenticationPackets.{h,cpp}`

- [ ] **Step 1: Re-confirm the reference counter model is strictly sequential**

Grep both reference crypts for the counter increment. Confirm neither has a forward window (we already verified Hermes `++_clientCounter` and Xian55 `++_clientCounter`). Document: "reference = strict sequential; therefore the client does NOT skip nonces against the reference."

- [ ] **Step 2: Diff AC's AuthResponse against the reference's AuthResponse::SuccessInfo**

Compare `extract-test/tc_AuthenticationPackets.cpp` (and Xian55's `AuthenticationPackets.cpp`) `EnumCharactersResult`-era `AuthResponse::Write` field-by-field against AC's `SendAuthResponse`. List every `SuccessInfo` field AC omits (currencies, GameTimeInfo, NumPlayersHorde/Alliance, expansion-trial flags, race/class `AvailableClasses`, `Templates`, `IsExpansionTrial`, `ForceCharacterTemplate`, `NumPlayers*`, etc.). The hypothesis to test: a field the client reads to decide "is the Battle.net store available?" is missing/zero, so the client cancels its queued `CMSG_BATTLE_PAY_*` send (advancing its nonce without transmitting).

- [ ] **Step 3: Build a complete AuthResponse on a throwaway branch and test live**

Implement the full `SuccessInfo` (port from the reference) into `SendAuthResponse`. Then **temporarily set `WorldPacketCrypt::MaxRecvCounterSkip = 0`** (forces strict sequential) and have the user connect. 
- If char-select is reached with skip=0 → the complete AuthResponse eliminated the nonce gap; the forward-window fix is unnecessary and should be **reverted in favour of the AuthResponse fix** (cleaner, matches reference exactly).
- If it still disconnects with skip=0 → the gap is genuine client behaviour the reference also relies on tolerating elsewhere, or our AuthResponse is still incomplete; **keep the forward-window fix** and document why.

- [ ] **Step 4: Record the verdict**

Write the decision (AuthResponse-fix vs forward-window-fix, with the live evidence) into `docs/wotlk-classic-3.4.3/KNOWLEDGE.md` under a new "GCM nonce sequencing" heading. Keep whichever fix won; delete the other.

---

## Task 3: Scan AzerothCore for fundamental protocol divergences from a real 3.4.3 core

**Goal:** Produce a prioritized divergence report so Phase 2+ doesn't rediscover structural gaps one disconnect at a time. Each finding = {area, AC state, reference state, severity, file:line}.

**Reference:** Xian55 (`D:/Games/xian55-3.4.3_Source`) + Hermes (`extract-test/hermes_*`). Dispatch parallel read-only `Explore`/`general-purpose` subagents per area; collate into one report.

- [ ] **Step 1: Opcode coverage & values**

Compare `src/server/game/Server/Protocol/Opcodes.{h,cpp}` to `extract-test/hermes_opcode_54261.cs` (authoritative values) and `extract-test/tc_opcodes_wotlk_classic.*`. Report: count of `STATUS_UNHANDLED` stubs; any opcode whose AC *value* disagrees with Hermes 54261; client opcodes the client actually sends at char-select/world-entry that are still stubs.

- [ ] **Step 2: Send-side framing & compression**

Diff AC `WorldSocket::WritePacketToBuffer`/`Update` against `tc_WorldSocket.cpp` + `hermes_worldsocket.cs` (SendPacket/Encrypt). Confirm: header is `[Size(4)][Tag(12)]` outbound (16 bytes) and `[Size][Tag][EncOpcode(2)]` inbound (18, peek-forward); `MinSizeForCompression`/`SMSG_COMPRESSED_PACKET` threshold matches; compression gating (`ExpansionVersion < 3`) is correct for WotLK.

- [ ] **Step 3: Core SMSG/CMSG packet layouts (WDC4 vs WDC5 risk)**

For the packets needed in Phase 2 (char create result, char-login/world-enter set: `SMSG_LOGIN_VERIFY_WORLD`, `SMSG_FEATURE_SYSTEM_STATUS`, `SMSG_TUTORIAL_FLAGS`, `SMSG_UPDATE_OBJECT`, `SMSG_INIT_WORLD_STATES`, time-sync), diff AC's struct (if any) vs the reference, flagging any field gated on build ≥ 54261/61581. Note which are still legacy 3.3.5a byte-layout in AC and need a modern rewrite.

- [ ] **Step 4: Character DB schema gap**

Document the delta between AC's legacy `characters` table (skin/face/hairStyle/... columns, `equipmentCache` string) and the modern model the 54261 enum/create/login path expects (`character_customizations` table of ChrCustomizationOption/Choice, `playerFlagsEx`, `personalTabard*`, `lastLoginBuild`, `c.slot`/`c.order`, DB2 `ChrSpecialization`/`ChrCustomization`). This is the long pole for Phase 2.

- [ ] **Step 5: DB2 / hotfix layer**

Confirm what DB2 stores exist (the design spec mentions a DB2/hotfix rewrite) and which the char-create/world-enter path requires (`ChrCustomization*`, `ChrModel`, `ChrRaceXChrModel`, `ChrClasses`, `ChrSpecialization`, `CharStartOutfit`, `Item*Appearance`). Report what's loaded vs missing.

- [ ] **Step 6: Write the divergence report**

Save to `docs/wotlk-classic-3.4.3/divergence-report-2026-06-27.md` (table form, sorted by severity). Feed the BLOCKER/Phase-2 items into Task 5.

---

## Task 4: Update KNOWLEDGE.md

- [ ] Append to `docs/wotlk-classic-3.4.3/KNOWLEDGE.md`: (1) the GCM nonce forward-gap finding + the Task 2 verdict; (2) the char-enum brick (opcodes, packet structs, empty-list rationale, deferred customization work); (3) the `SendAddonsInfo` removal; (4) a pointer to the Task 3 divergence report. Also drop a one-line auto-memory pointer if the finding is reusable across sessions.

---

## Task 5+: Phase 2 roadmap (next conversation starts here)

Implement in this order; each produces a live-testable client state. Use subagent-driven-development with byte-layout verification vs Xian55/Hermes for every new packet.

- [ ] **Phase 2a — Real character enumeration.** Add the `character_customizations` schema (SQL update under `pending_db_characters`), load customizations + equipment into `CharacterInfo`, wire the minimal DB2 `ChrCustomization`/`ChrSpecialization` lookups. Result: existing characters render correctly on char-select.
- [ ] **Phase 2b — Character creation.** `CMSG_CREATE_CHARACTER` (modern body: name + ChrCustomizationChoice array) → validate (race/class/DB2) → insert (legacy `characters` row + `character_customizations`) → `SMSG_CREATE_CHAR`. Result: create a character and see it in the list.
- [ ] **Phase 2c — World entry.** `CMSG_PLAYER_LOGIN` → the modern world-enter packet sequence (`SMSG_LOGIN_VERIFY_WORLD`, account data times, `SMSG_FEATURE_SYSTEM_STATUS`, action buttons, initial `SMSG_UPDATE_OBJECT` for the player, `SMSG_INIT_WORLD_STATES`, time sync). This is the large milestone — expect more 54261 layout deltas. Result: zone in and stand in the world.
- [ ] **Phase 2 instance connection / time-sync / movement** follow once world entry holds.

---

## Self-Review notes (for the executor)

- Task 1 must run before Tasks 2–4 so the verification happens against committed, known-good code.
- Task 2 can *replace* the Task 1 crypto change — if the AuthResponse fix wins, revert `WorldPacketCrypt` to strict sequential and re-commit. Do not skip Task 2: shipping the forward-window without confirming it's not masking an AuthResponse gap is the one open risk in this stack.
- All packet work: verify byte layout against **both** Xian55 (server intent) and Hermes (client-proven) before declaring done. When they disagree, Hermes wins for the wire format (it's validated against the live 54261 client).
- Build discipline is non-negotiable: per-target builds only, never `unit_tests`, never full build. Stop `worldserver.exe` before relinking (exe lock on Windows).
