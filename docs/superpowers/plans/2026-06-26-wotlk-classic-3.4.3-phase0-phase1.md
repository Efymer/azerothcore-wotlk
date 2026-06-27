# WotLK Classic 3.4.3 Native Fork — Phase 0 + Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Take AzerothCore from a 3.3.5a (12340) emulator to one where a patched WotLK Classic **3.4.3.54261** client completes modern Battle.net login, enters the world, and can move/chat with nearby objects spawning.

**Architecture:** Native fork & replace (3.4.3-only, no runtime version dispatch). Add a modern `bnetserver` (REST+TLS+protobuf) for login, rewrite the worldserver wire protocol (opcodes/UpdateFields/movement/crypt/handshake), and add a DB2 + hotfix client-data layer plus a CASC-based extraction toolchain. WotLK content/DB is preserved.

**Tech Stack:** C++20, CMake, MySQL, Boost.Asio, OpenSSL (TLS), **Protobuf (new dep)**, **CascLib (new dep)**, Google Test. Primary reference: **TrinityCore `wotlk_classic` branch** (same codebase lineage).

---

## How to use this plan

**Reference-fetch pattern.** Many tasks port a pinned upstream file. Fetch the reference verbatim before adapting:
```bash
gh api "repos/TrinityCore/TrinityCore/contents/<path>?ref=wotlk_classic" --jq '.content' | base64 -d > /tmp/ref_<name>
```
Pin to the branch head recorded at plan time: **TC `wotlk_classic` @ `12c81a6f86cddbd47710b4e27aeff3f4eb7c4ced`** (`gh api repos/TrinityCore/TrinityCore/branches/wotlk_classic` to refresh).

**AzerothCore adaptation rules (apply to every ported file):**
- Namespace `Trinity::` → `Acore::`; `TC_LOG_*`/`sLog->` → `LOG_*("category", "...", args)` with `{}` placeholders.
- `*_API` export macros → the matching AzerothCore macro (`AC_DATABASE_API`, `AC_COMMON_API`, etc.).
- Random/string/format helpers → `Acore::` equivalents (`urand`, `Acore::StringFormat`).
- Match AzerothCore code style (4-space, Allman braces, `auto const&`, `Type const*`); run `python apps/codestyle/codestyle-cpp.py` before each checkpoint.

**Verification gates.** Porting tasks can't be unit-tested step-by-step; each ends in a concrete gate — either *compiles + links* or a *behavioral observation* (client reaches state X; packet log shows opcode Y). Build only the target you changed (e.g. `cmake --build build --target bnetserver`) to keep cycles short.

**Commit checkpoints.** Where a step says "**Checkpoint**", stage and commit per your team's git workflow. (Literal git commands are intentionally omitted per repo policy.)

**Build base command (referenced throughout):** This environment uses **vcpkg** (bootstrapped at `C:\vcpkg`) for the new native deps (Protobuf, CascLib), so every configure MUST pass the toolchain file. Windows is a multi-config generator — pass `--config` on build.
```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSCRIPTS=none -DTOOLS_BUILD=all -DBUILD_TESTING=ON `
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --target <target> --config RelWithDebInfo
```

---

## Progress log (reconstructed 2026-06-27)

Checkboxes below were not maintained during the brick-based execution; actual state reconstructed from the
code + git history:

- **Phase 0** — ✅ done. Protobuf (`deps/protobuf`), CascLib (`deps/casc`), `extractor_common`, CASC+DB2
  `map_extractor` all in place; **763 `.db2` files extracted for build 54261**. Loose ends (non-blocking):
  `mmaps_generator` still on MPQ (0.5 explicitly defers correctness); `apps/dev/gen-bnet-cert.sh` helper never
  created (its gate — patched client over TLS — passed live anyway).
- **Phase 1a** — ✅ done. bnetserver login + realm list working live.
- **Phase 1b** — ✅ done. Native world handshake → **stable empty character-select**, proven live
  (`75410cbbb`, `271c8c5d2`). Opcode table is flat uint16 54261.
- **Phase 1c** — 🟡 in progress.
  - **1c.1 store layer** — DB2 *file-reader* (`src/common/DataStores/DB2FileLoader*`, `DB2Meta*`,
    `DB2FileSystemSource*`) + unit test already existed. Added the **store layer on top**:
    `src/server/shared/DataStores/{DBStorageIterator.h, DB2Store.{h,cpp}, DB2DatabaseLoader.{h,cpp}}` (ported from
    TC `wotlk_classic` via the local `D:/Games/xian55-3.4.3_Source` clone), plus the two
    `HOTFIX_*_STMT_OFFSET` constants in `HotfixDatabase.h`. ✅ **Verified: `shared.lib` compiles + links**
    (RelWithDebInfo/MSVC), codestyle clean. Adaptations: TC `Field::GetUInt32()`/`setBool` → AC `Field::Get<T>()`/
    `PreparedStatement::SetData`; `ABORT_MSG` → `ABORT` (`{}`-style); `LocalizedString::operator[]` →
    `.Str[locale]` (AC's has no `operator[]`); added `Field.h`+`QueryResult.h` includes.
  - **1c.2 (in progress)** — store layer + boot wiring in `src/server/game/DataStores/`
    `{DB2Structure.h, DB2LoadInfo.h, DB2Stores.{h,cpp}}`, `LoadDB2Stores()` called after `LoadDBCStores` in
    `World.cpp`. **6 DB2-only stores load live** (boot log, no errors): LiquidMaterial(3), SpellName(446921),
    CharacterLoadout(1828), CharacterLoadoutItem(25516), ChrCustomizationOption(1932), ChrCustomizationReq(599),
    ItemEffect(181364), ItemAppearance(83055), ItemModifiedAppearance(194434), PowerType(139), ItemSparse(211852,
    73 fields/130 entries) — **11 stores live**. The boot-time `sizeof(T)==GetRecordSize()` + LayoutHash + sign
    checks make even large transcriptions self-validating. **All DB2-only subset stores done — 12 live**
    (added PlayerCondition: 122129, 81 fields/147 entries). **Remaining subset is entirely DBC-colliding** (store
    or struct name clashes with DBC: Map, ChrRaces, ChrClasses, AreaTable, Faction, FactionTemplate, SkillLine,
    SkillLineAbility, SkillRaceClassInfo, CurrencyTypes, CharTitles, CinematicCamera, CinematicSequences,
    PowerDisplay, MapDifficulty, WorldMapOverlay, TaxiNodes, TaxiPath, TaxiPathNode, Light, Item) → **Task 1c.3**:
    each added while removing its DBC twin and repointing consumers (overlaps the other agent's world-entry path —
    coordinate).
    Method: xian55 == 54261 for these (LayoutHash in each extracted .db2 @offset 24 matches xian55 metadata
    exactly), so structs/metadata ported verbatim from xian55, hash-validated. Key correctness facts captured:
    DB2 structs MUST be `#pragma pack(push,1)` (loader produces packed records, stride = `GetRecordSize()`);
    added a boot-time `sizeof(T) == GetRecordSize()` guard in `DB2Stores.cpp`; `DB2Meta` written designated-init
    (NOT TC's constructor — that would break the extractor's designated-init metadata); ParentIndexField's
    LoadInfo field must be `IsSigned=false`. DBC-colliding subset stores deferred to 1c.3.
  - **✅ BLOCKER RESOLVED (was a DB2 format bug, root cause confirmed vs xian55):** a prior revision wrongly
    added `uint32 Version` + `std::array<char,128> Schema` to `DB2Header` (`src/common/DataStores/DB2FileLoader.h`)
    modelling a speculative WDC5 preamble. The real 3.4.3.54261 client ships **WDC4 with no such preamble**
    (verified: xian55's `DB2Header` has neither; it reads `Read(&_header, sizeof(DB2Header))` in one shot, magic
    `WDC4` only). The phantom 132-byte preamble shifted every field and made `ExtractDB2File` emit corrupt,
    body-less db2 files. **Fix:** removed `Version`+`Schema` from `DB2Header`; simplified `DB2FileLoader::LoadHeaders`
    to a single-shot read + `WDC4` signature check (matching xian55); updated the WDC5 unit test to WDC4. **No
    `ExtractDB2File` logic change needed** — the corrected struct makes the existing extractor faithful.
    Re-extracted DB2 with `map_extractor -e 2` (763 files); `LiquidMaterial.db2` now 180 B with `LayoutHash`
    `0x2CFFEA40` at offset 24. **Behavioral gate PASSED:** worldserver boot logs `>> DB2 LiquidMaterial.db2 loaded
    3 records` (index-table size; 2 real rows at IDs 1-2), no layout-hash error. 1c.1 store layer + 1c.2 boot
    wiring proven end-to-end.
  - **1c.3 (in progress)** — DBC→DB2 repoint **pattern proven end-to-end** on `PowerDisplay` (clean, model-stable,
    single non-overlapping consumer `Vehicle.cpp`): added the DB2 store, removed the DBC `PowerDisplayEntry` +
    `sPowerDisplayStore` (DBCStructure.h / DBCStores.{h,cpp} / DBCfmt.h), repointed `Vehicle.cpp` (`->PowerType` →
    `->ActualType`). Boots loading PowerDisplay.db2 (143 records, 13 DB2 stores), DBC version gone. **Key 1c.3
    scoping findings:** (1) the high-value world-entry stores are **entangled** — `ChrRaces`/`ChrClasses` consumers
    include `CharacterHandler.cpp` (the *other agent's* active file); (2) several stores are **model-changed**, not
    mechanical repoints: `CurrencyTypes` (3.3.5 indexes by ItemId + player bitmask `BitIndex` → 3.4.3 indexes by
    currency ID, no item link), `ChrRaces` (race display models moved to a separate `ChrRaceXChrModel` store). So
    the remaining 1c.3 splits into: *clean mechanical* peripheral stores (do solo) vs *model-rework + coordinate*
    world-entry stores (CurrencyTypes-style reworks + the other agent's char path). **Follow-up survey result:
    clean mechanical repoints are essentially exhausted — PowerDisplay was the rare clean case.** The rest are all
    model-changed and need rework, not a remap: `CinematicCamera` (DBC `Model` filename → DB2 `FileDataID`, breaks
    M2Stores filename-based camera loading), `MapDifficulty` (DBC `(MapId,Difficulty)` composite key + `resetTime`
    seconds → DB2 `ID` + `ResetInterval` enum), `WorldMapOverlay` (different struct; only an area-array rename is
    salvageable), plus the already-noted `CurrencyTypes`/`ChrRaces`. **Recommendation:** stop the solo DB2 data-layer
    grind here (foundation is solid: 13 stores + loader/extractor fix + the proven repoint pattern); sequence the
    remaining model-rework stores deliberately alongside their gameplay consumers / the other agent's Phase 2.
  - **1c.3 — first world-entry store migrated: `ChrClasses` (DBC→DB2).** 14 DB2 stores live; boots to full "World
    Initialized" with Player-Create class validation clean. Field repoints: `powerType`→`DisplayPower`
    (Player.cpp ×3, cs_reset.cpp), `CinematicSequence`→`CinematicSequenceID` (CharacterHandler.cpp). Model-change
    handled: the DBC `expansion` field is gone in DB2 (3.4.3 uses a `class_expansion_requirement` DB table) — kept
    a minimal Death-Knight-only stopgap at the char-create gate until that table is ported. Removed the DBC store
    (DBCStructure/Stores/fmt) and added `DB2Stores.h` includes to the 5 consumers. CharacterHandler.cpp edits are
    minimal/forced; the other agent's modern char-create will supersede them.
  - **Completed the ChrClasses model rework (no deferral): ported the `class_expansion_requirement` mechanism.**
    Added `ObjectMgr` structs (`ClassAvailability`/`RaceClassAvailability`), `LoadClassExpansionRequirements()` +
    `GetClassExpansionRequirement(race,class)` / `GetClassExpansionRequirementFallback(class)` (ported from xian55),
    wired into boot after `LoadPlayerInfo`, and a new `class_expansion_requirement` world table (Death Knight → WotLK
    for all 10 races). Replaced the char-create stopgap with the real accessor. Verified live: SQL update applies,
    loader logs "Loaded 10 class expansion requirements", full World Initialized. Also exposes
    `GetClassExpansionRequirements()` for the other agent's SMSG_AUTH_RESPONSE AvailableClasses block.
  - **`ChrRaces` migrated DBC→DB2 (no deferral).** 57-field struct; loads 22 records, byte-exact (size-check).
    Repoints: `TeamID`(7/1)→`Alliance`(0/1) in `TeamIdForRace`, `model_m/f`→`MaleDisplayID/FemaleDisplayID`
    (display IDs stayed in ChrRaces — no ChrRaceXChrModel needed), `RaceID`→`ID`, `Flags&NOT_PLAYABLE`→
    `PlayableRaceBit<0`, `HasFlag(CAN_MOUNT)`→`Flags&0x04`, `name[i]`→`Name.Str[i]` (DetectDBCLang),
    `CinematicSequence`→`CinematicSequenceID`. Built the **`race_unlock_requirement`** mechanism (struct + loader +
    accessor + world table; Draenei/Blood Elf → TBC) and wired the char-create race gate to it. **Ordering fix:**
    `LoadDB2Stores()` now runs before `DetectDBCLang()` (which reads the now-DB2 race names) — caught a null-deref
    crash via boot-testing. Verified live: ChrRaces + 2 race-unlock rows load, full World Initialized.
  - **`SkillLine` migrated DBC→DB2 (no deferral).** Loads 789 records; 16 DB2 stores live. Repoints across ~10
    files: `id`→`ID`, `categoryId`→`CategoryID`, `canLink`→`CanLink`, `name[i]`→`DisplayName.Str[i]`. Moved
    `LoadDB2Stores()` before `LoadDBCStores()` (a DBC post-load loop validates SkillID against the now-DB2 store).
    Resolved an `FT_*` enum clash (DB2FileLoader's `DBCFormer` vs DBCFileLoader's `DbcFieldFormat`) by keeping
    `DBCStores.cpp` off the DB2 headers — dropped a redundant SkillRaceClassInfo validity check there. Verified
    live: full World Initialized, sSkillLineStore exercised across SpellMgr/ObjectMgr with no errors.
  - **`FT_*` enum systemic unblock** (`e19b7f02f`): the DBC `DbcFieldFormat` and DB2 `DBCFormer` enums share
    `FT_*` enumerators, so any TU pulling both fails — which blocked `DBCStores.cpp` (post-load) from referencing
    *any* migrated DB2 store. Moved `DbcFieldFormat` to its own `DBCFieldFormat.h` (included only where `FT_*` is
    used by name); `DBCStores.h` no longer leaks it. Unblocks all remaining DBC→DB2 stores.
  - **`SkillLineAbility` migrated DBC→DB2 (no deferral).** Loads 50603 records; 17 DB2 stores live. DBC and DB2
    field names match (SkillLine/Spell/RaceMask/ClassMask/AcquireMethod/TrivialSkillLineRank*), so most consumers
    compiled unchanged (RaceMask is int64 now). Handled the one semantic inversion: DBC `SupercededBySpell`
    (forward pointer) → DB2 `SupercedesSpell` (back pointer) — the paladin double-Seal guard in `Player.cpp` now
    searches the skill's abilities for the one that supercedes the current spell. `DBCStores.cpp` post-load
    (pet-family spells, ability index) now reads the DB2 store; `SkillLineAbilityEntry` forward-declared in
    `DBCStores.h` (pointer-only use). Verified live: full World Initialized.
  - **1c.4–1c.5** — not started. ⚠️ Note: AC's `DB2Meta` stays a designated-init aggregate (do NOT add TC's
    constructor — it would break the extractor's + runtime's designated-init metadata).
- **Phase 1d** — not started.

> The separate `2026-06-27-...-phase2-handover.md` plan (nonce verification + char enum/create/world-entry
> packet path) is owned by a **different agent**. Their uncommitted edits to `AuthHandler.cpp`, `WorldSession.*`,
> `WorldPacketCrypt.h` are not part of this plan — do not touch them.

---

## Phase 0 — Prerequisites, toolchain, and spike

**Exit:** patched 3.4.3 client performs a TLS handshake against our stub REST endpoint; extracted DB2 + map data present on disk; Protobuf generates in-build.

### Task 0.1: Vendor Protobuf as a build dependency

**Files:**
- Create: `deps/protobuf/CMakeLists.txt`
- Modify: `deps/CMakeLists.txt:19-27` (add `add_subdirectory(protobuf)`)
- Modify: `deps/PackageList.txt` (record protobuf version)

- [ ] **Step 1: Decide acquisition strategy.** Prefer system Protobuf via `find_package(Protobuf REQUIRED)` wrapped in an interface target, falling back to vendored sources. AzerothCore already wraps deps as interface libs (see `deps/openssl`). Model `deps/protobuf/CMakeLists.txt` on `deps/openssl/CMakeLists.txt` (read it first).
- [ ] **Step 2: Expose an `acore-protobuf` interface target** that provides include dirs + libs + the `protoc` executable path, mirroring how `deps/boost` exposes `boost`.
- [ ] **Step 3: Wire into `deps/CMakeLists.txt`.** Add `add_subdirectory(protobuf)` after line 21 (`boost`). Guard with the same condition group as apps if it should only build when apps build.
- [ ] **Step 4: Verify.** Run the base `cmake -S . -B build ...`. Expected: configure succeeds and prints a protobuf version; `protoc` path resolves. No target built yet.
- [ ] **Step 5: Checkpoint** — "build: vendor protobuf dependency".

### Task 0.2: Vendor CascLib for CASC client reading

**Files:**
- Create: `deps/casc/CMakeLists.txt`
- Modify: `deps/CMakeLists.txt:45-49` (add `add_subdirectory(casc)` inside the `BUILD_TOOLS_MAPS` block)
- Modify: `deps/PackageList.txt`

- [ ] **Step 1: Add CascLib** (the standard `ladislav-zezula/CascLib`) the same way `deps/libmpq` is vendored — read `deps/libmpq/CMakeLists.txt` first and mirror it for an `casc` static lib target.
- [ ] **Step 2: Wire into the maps-tools block** of `deps/CMakeLists.txt` (lines 45-49, next to `libmpq`).
- [ ] **Step 3: Verify** with `cmake -S . -B build -DTOOLS_BUILD=all ...`. Expected: `casc` target configures.
- [ ] **Step 4: Checkpoint** — "build: vendor CascLib dependency".

### Task 0.3: Create the CASC extractor_common library

**Files:**
- Create: `src/tools/extractor_common/` (port from TC)
- Reference: `gh api ".../contents/src/tools/extractor_common?ref=wotlk_classic"`

- [ ] **Step 1: Fetch** the TC `extractor_common` directory listing and each file (CascHandles, etc.).
- [ ] **Step 2: Port** into `src/tools/extractor_common/`, applying the adaptation rules. This provides the shared CASC-open/locale/build-detection helpers the extractors need.
- [ ] **Step 3: Add to the tools build.** In `src/tools/CMakeLists.txt`, extractors currently link `mpq` (line 140). Add a path where CASC-based tools link `casc` + `extractor_common` instead. Keep it additive (don't delete the mpq link yet — the DB2 extractor and map extractor will switch over in 0.4/0.5).
- [ ] **Step 4: Verify** `cmake --build build --target <a trivial extractor_common test>` or that the library archives. Expected: `extractor_common` compiles.
- [ ] **Step 5: Checkpoint** — "tools: add CASC extractor_common".

### Task 0.4: Port the DB2 extractor (`map_extractor` → CASC + DB2)

> **SEQUENCING UPDATE (discovered during 0.3):** the DB2 extractor depends on
> `extractor_common`'s `DB2CascFileSource`, which needs the **DB2 file-format reader**
> (`src/common/DataStores/{DB2FileLoader,DB2FileSystemSource,DB2Meta}` — Task **1c.1**).
> So **Task 1c.1 was moved up to run before 0.4.** 0.3 shipped `extractor_common` with
> only `CascHandles`; once 1c.1 lands, re-add `DB2CascFileSource` + `ExtractorDB2LoadInfo`
> to `extractor_common`, then do 0.4. (TC's DB2 reader lives in `src/common/DataStores`,
> not `src/server/shared/DataStores` as the original §6/§7 implied.)

**Files:**
- Modify/replace: `src/tools/map_extractor/` (CASC + DB2 dump)
- Reference: TC `src/tools/map_extractor` @ wotlk_classic

- [ ] **Step 1: Fetch** TC `map_extractor` (it already does CASC map + DB2 dump on `wotlk_classic`).
- [ ] **Step 2: Port** it over AzerothCore's `map_extractor`, switching from `libmpq` to `casc`/`extractor_common`. Output: `dbc/` (now DB2) + `maps/` for build 54261.
- [ ] **Step 3: Update** `src/tools/CMakeLists.txt` so `map_extractor` links `casc`+`extractor_common` (not `mpq`).
- [ ] **Step 4: Verify (behavioral).** Point it at the real 3.4.3.54261 client install at `D:\Games\World of Warcraft 3.4.3.54261`. Expected: it enumerates the CASC storage and writes DB2 files + map tiles. Record the DB2 file count.
- [ ] **Step 5: Checkpoint** — "tools: port map/DB2 extractor to CASC (3.4.3)".

### Task 0.5: Confirm vmap/mmap extraction against 3.4.3 (assess-only gate)

**Files:** `src/tools/vmap4_extractor`, `src/tools/mmaps_generator`

- [ ] **Step 1:** Diff AzerothCore's `vmap4_extractor` against TC `wotlk_classic`'s. If model/WMO formats are unchanged from 3.3.5, only the CASC file-access layer needs swapping; if changed, port the TC version.
- [ ] **Step 2:** Port the CASC access path only (reuse `extractor_common`). Defer full vmap/mmap correctness — Phase 1 needs map tiles (0.4), not collision/pathing.
- [ ] **Step 3: Verify** vmap4_extractor runs against the 3.4.3 client without CASC errors (output correctness deferred to Phase 2).
- [ ] **Step 4: Checkpoint** — "tools: CASC access for vmap/mmap (correctness deferred)".

### Task 0.6: Local TLS cert + patched-client spike (stub REST endpoint)

**Files:**
- Create: `apps/dev/gen-bnet-cert.sh` (dev helper)
- Create: throwaway stub listener (can live in a scratch branch or a minimal `bnetserver` Main.cpp stub)

- [ ] **Step 1:** Generate a self-signed cert for a resolvable dev hostname (e.g. add `127.0.0.1 bnet.localtest` to hosts) and install it into the client host's trusted root store (wow-patcher requires a chain to a trusted root).
- [ ] **Step 2:** Patch a copy of the 3.4.3 client with `wow-patcher`, setting the portal to `bnet.localtest` and the RSA modulus/Ed25519 keys to the values our bnetserver will use (decide keys now; reuse TC's standard keys, which wow-patcher defaults to).
- [ ] **Step 3:** Stand up a minimal TLS listener on the bnet REST port that logs the incoming TLS ClientHello / HTTP request and returns 200.
- [ ] **Step 4: Verify (behavioral, THE Phase-0 gate).** Launch the patched client. Expected: the stub logs an inbound TLS handshake + an HTTP login request from the client. This proves portal patching + TLS trust end-to-end.
- [ ] **Step 5: Checkpoint** — "dev: 3.4.3 patched-client reaches stub REST over TLS".

---

## Phase 1a — bnetserver login

**Exit:** client passes login and shows the realm list.

### Task 1a.1: Vendor the bgs.protocol definitions and generate

**Files:**
- Create: `src/server/proto/` (port from TC `src/server/proto`)
- Modify: `src/server/CMakeLists.txt` (add `proto` subdir before `bnetserver`)

- [ ] **Step 1: Fetch** TC `src/server/proto` (contains `Client/`, `Login/`, `RealmList/`, `ServiceBase.*`, `BattlenetRpcErrorCodes.h`, `CMakeLists.txt`).
- [ ] **Step 2: Port** the directory; wire its `CMakeLists.txt` to invoke `protoc` (from Task 0.1's `acore-protobuf`) generating into the build dir, producing an `proto`/`acore-proto` static lib.
- [ ] **Step 3: Verify** `cmake --build build --target proto`. Expected: `.pb.cc`/`.pb.h` generate and the lib archives.
- [ ] **Step 4: Checkpoint** — "proto: vendor bgs.protocol + generate".

### Task 1a.2: Auth DB model — battlenet + game accounts + hotfixes DB registration

**Files:**
- Create: `data/sql/updates/pending_db_auth/rev_<ts>.sql` (battlenet account tables)
- Modify: `src/server/database/Database/DatabaseLoader.h:44-53` (add `DATABASE_HOTFIX = 8` + extend `DATABASE_MASK_ALL`)
- Modify: `src/server/database/Database/DatabaseEnvFwd.h` / `DatabaseEnv.*` (declare `HotfixDatabasePool`)
- Reference: TC auth DB battlenet schema; TC `sql/base/hotfixes_database.sql`

- [ ] **Step 1: Write** the auth-DB update adding `battlenet_accounts` + game-account linkage (model on TC; idempotent `DELETE`+`INSERT`, 4-space, InnoDB — see repo SQL conventions). Use `data/sql/updates/pending_db_auth/create_sql.sh` to generate the file.
- [ ] **Step 2: Add** `DATABASE_HOTFIX = 8` to the `DatabaseTypeFlags` enum and include it in `DATABASE_MASK_ALL`.
- [ ] **Step 3: Declare** a `HotfixDatabasePool` (mirror the World pool wiring) so worldserver can later load DB2 hotfix rows.
- [ ] **Step 4: Verify** SQL with `python apps/codestyle/codestyle-sql.py`; build `database` target.
- [ ] **Step 5: Checkpoint** — "db: battlenet account model + hotfixes DB registration".

### Task 1a.3: Create the bnetserver application skeleton

**Files:**
- Create: `src/server/apps/bnetserver/{Main.cpp, bnetserver.conf.dist, CMakeLists deps}`
- Create: `src/server/apps/bnetserver/{Server/,REST/,Services/}` (ported)
- Modify: `src/server/apps/CMakeLists.txt:130-154` (add a `MATCHES "bnetserver"` link block)
- Reference: TC `src/server/bnetserver/**`

- [ ] **Step 1: Fetch** all of TC `src/server/bnetserver` (`Main.cpp`, `Server/{Session,SessionManager,SslContext}`, `REST/{LoginRESTService,LoginHttpSession}`, `Services/{Account,Authentication,Connection,GameUtilities,Service,ServiceDispatcher}`, `bnetserver.conf.dist`).
- [ ] **Step 2: Create** `src/server/apps/bnetserver/` (AzerothCore auto-discovers apps from this directory — see `apps/CMakeLists.txt:91`). Port files with the adaptation rules.
- [ ] **Step 3: Add link block** in `src/server/apps/CMakeLists.txt` next to the authserver block (line 130): `bnetserver` links `PUBLIC shared proto` + `acore-protobuf` + OpenSSL TLS.
- [ ] **Step 4: Port the REST + TLS layer first** (`LoginRESTService`, `SslContext`), pointing at the cert from Task 0.6.
- [ ] **Step 5: Verify (behavioral).** Run `bnetserver`; launch the patched client. Expected: the client completes the REST login form POST and receives a login ticket (SRP6a). Watch logs for a successful `LoginRESTService` exchange.
- [ ] **Step 6: Checkpoint** — "bnetserver: REST/TLS login skeleton".

### Task 1a.4: Port the protobuf RPC services + realm list

**Files:** `src/server/apps/bnetserver/Services/*`, `src/server/shared/Realms/*`

- [ ] **Step 1: Port** `ServiceDispatcher` + `AuthenticationService`, `AccountService`, `ConnectionService`, `GameUtilitiesService`. These ride the bnet TCP session (port the `Server/Session` framing).
- [ ] **Step 2: Wire realm list** through the bnet `RealmList` proto service. AzerothCore's `RealmList` (src/server/shared/Realms) currently feeds the grunt protocol; adapt it to emit bnet realm entries for build 54261.
- [ ] **Step 3: Verify (behavioral, THE Phase-1a gate).** Patched client → after login, the realm list screen lists our realm. Confirm via on-screen realm + bnetserver log of the realm-list RPC.
- [ ] **Step 4: Checkpoint** — "bnetserver: protobuf services + realm list".

---

## Phase 1b — World handshake

**Exit:** character-list screen renders (even if empty).

### Task 1b.1: Set the expected client build to 54261

**Files:**
- Modify: `data/sql/updates/pending_db_auth/rev_<ts>.sql` (build_info row for 54261)
- Modify: build/version constants (grep `12340` across `src/server/shared/Realms`, `src/common`, `src/server/game/Server`)

- [ ] **Step 1: Grep** `rg -n "12340"` and catalog each site (RealmList build info, WorldSocket build check, PacketLog header, UpdateFields header comment).
- [ ] **Step 2: Add** a `build_info` row for `54261,3,4,3,...` and set the expected/allowed build constant to 54261. Remove 12340 from accepted builds (fork & replace).
- [ ] **Step 3: Verify** SQL codestyle + `database`/`shared` build.
- [ ] **Step 4: Checkpoint** — "core: target build 54261".

### Task 1b.2: Replace the world auth handshake + crypt

**Files:**
- Modify: `src/server/game/Server/WorldSocket.{h,cpp}` (HandleSendAuthSession / HandleAuthSession)
- Modify: `src/server/game/Server/Protocol/ServerPktHeader.h`, `WorldSocket.h:57-65` (ClientPktHeader)
- Modify: `src/common/Cryptography/Authentication/AuthCrypt.{h,cpp}` (or replace with modern `WorldCrypt`)
- Reference: TC `wotlk_classic` `WorldSocket.*`, `WorldCrypt`/`PacketCrypt`

- [ ] **Step 1: Fetch** TC `wotlk_classic` `WorldSocket.*` + its world crypt class.
- [ ] **Step 2: Port** the modern `SMSG_AUTH_CHALLENGE` (3.4.3 format/seed) and `CMSG_AUTH_SESSION` parsing, plus the modern header struct and crypt init (the 3.4.x world crypt differs from 3.3.5 ARC4 — port TC's). The session key arrives via the bnetserver "connect to" handoff (ConnectionService), not the grunt path.
- [ ] **Step 3: Verify (behavioral).** Patched client → selecting the realm connects to worldserver and passes auth (no immediate disconnect). Use a packet log to confirm `SMSG_AUTH_RESPONSE`/equivalent success.
- [ ] **Step 4: Checkpoint** — "world: 3.4.3 auth handshake + crypt".

### Task 1b.3: Regenerate the opcode table for 54261

**Files:**
- Modify: `src/server/game/Server/Protocol/Opcodes.{h,cpp}`
- Reference: TC `wotlk_classic` `Opcodes.{h,cpp}`

- [ ] **Step 1: Fetch** TC `wotlk_classic` `Opcodes.h/.cpp`. Note the modern numbering (e.g. `SMSG_HOTFIX_CONNECT = 0x460003`) and the per-direction split.
- [ ] **Step 2: Replace** AzerothCore's opcode enum + `OpcodeTable::Initialize` registrations with the 3.4.3 set. Keep handler bindings to existing `WorldSession::Handle*` where the handler still exists; stub-register unimplemented ones to `STATUS_UNHANDLED`.
- [ ] **Step 3: Verify** `game` target compiles and the handler table builds without duplicate-opcode asserts. Behavioral: char-list packet now uses the correct opcode (check packet log).
- [ ] **Step 4: Checkpoint** — "world: 3.4.3 opcode table".

### Task 1b.4: Character enumeration over 3.4.3

**Files:** `src/server/game/Handlers/CharacterHandler.cpp` (enum builder), char-list packet builder

- [ ] **Step 1:** Update the `SMSG_ENUM_CHARACTERS_RESULT` (3.4.3) builder to the modern structure (port the packet layout from TC `wotlk_classic` `CharacterPackets`).
- [ ] **Step 2: Verify (behavioral, THE Phase-1b gate).** Patched client reaches the character-selection screen (empty list is success).
- [ ] **Step 3: Checkpoint** — "world: character enumeration (3.4.3)".

---

## Phase 1c — Enter world + DB2 read layer

**Exit:** a character stands in the world without disconnecting.

### Task 1c.1: Port the DB2 file-reader + DB-backed loader (shared/DataStores)

**Files:**
- Create: `src/server/shared/DataStores/{DB2DatabaseLoader,DB2Store,DBStorageIterator}.{h,cpp}`
- Reference: TC `wotlk_classic` `src/server/shared/DataStores/*`

- [ ] **Step 1: Fetch + port** the three files. This is the WDC-format DB2 reader + the loader that overlays `acore_hotfixes` rows.
- [ ] **Step 2: Write a unit test** for the DB2 header parser (TDD — this piece IS testable). Create `src/test/DataStores/DB2StoreTest.cpp`:
```cpp
#include "gtest/gtest.h"
#include "DB2Store.h"
// Minimal WDC header fixture (magic 'WDC5'/'WDC4' per 3.4.3), 1 record, 1 field.
TEST(DB2Store, ParsesHeaderRecordCount)
{
    // Build an in-memory DB2 blob matching the target WDC version's header layout.
    std::vector<uint8> blob = MakeMinimalDb2Fixture(/*records=*/3, /*fields=*/1);
    DB2FileLoader loader;
    ASSERT_TRUE(loader.LoadHeaders(blob.data(), blob.size()));
    EXPECT_EQ(loader.GetRecordCount(), 3u);
}
```
- [ ] **Step 3: Run** `cmake --build build --target unit_tests && ./build/src/test/unit_tests --gtest_filter=DB2Store.*`. Expected: FAIL (no `MakeMinimalDb2Fixture`/loader yet).
- [ ] **Step 4: Implement** `MakeMinimalDb2Fixture` (test helper) and ensure `DB2FileLoader::LoadHeaders` parses the real WDC version emitted by Task 0.4's extractor. Re-run: PASS.
- [ ] **Step 5: Verify** the loader reads one real extracted DB2 (e.g. `Map.db2`) and returns the expected record count (log it).
- [ ] **Step 6: Checkpoint** — "datastores: DB2 reader + DB-backed loader + header test".

### Task 1c.2: Define DB2 structures/metadata for the world-entry store subset

**Files:**
- Create: `src/server/game/DataStores/{DB2Structure.h, DB2Metadata.h, DB2LoadInfo.h, DB2Stores.{h,cpp}, DBCEnums.h}` (subset)
- Reference: TC `wotlk_classic` `src/server/game/DataStores/*`

- [ ] **Step 1: Fetch** the TC files. Port **only the Phase-1c subset** of stores (the rest are added in later phases). Subset (from the spec, §7):
  - Char enum/create: `sChrRacesStore`, `sChrClassesStore`, `sChrCustomizationOptionStore`, `sChrCustomizationReqStore`, `sCharacterLoadoutStore`, `sCharacterLoadoutItemStore`, `sCharTitlesStore`, `sFactionStore`, `sFactionTemplateStore`, `sPowerTypeStore`, `sPowerDisplayStore`, `sCinematicSequencesStore`, `sCinematicCameraStore`.
  - Map/positioning: `sMapStore`, `sMapDifficultyStore`, `sAreaTableStore`, `sAreaTriggerStore`, `sLightStore`, `sWorldMapOverlayStore`, `sPlayerConditionStore`, `sTaxiNodesStore`, `sTaxiPathStore`, `sTaxiPathNodeStore`.
  - Skills: `sSkillLineStore`, `sSkillLineAbilityStore`, `sSkillRaceClassInfoStore`.
  - Starting gear: `sItemStore`, `sItemSparseStore`, `sItemEffectStore`, `sItemAppearanceStore`, `sItemModifiedAppearanceStore`.
  - Spell/currency: `sSpellNameStore`, `sCurrencyTypesStore`.
- [ ] **Step 2:** For each store, port its `struct` (DB2Structure.h), `DB2Metadata`, and `DB2LoadInfo` entry, plus its `DB2Storage<...>` extern + `LoadDB2(...)` call in `DB2Stores.cpp::LoadDB2Stores`. Keep a single `LoadDB2Stores()` that loads only the subset (guarded list), logging any that fail to load.
- [ ] **Step 3: Verify** `game` compiles and a worldserver boot loads all subset stores from extracted DB2 without fatal errors (log line per store with record count).
- [ ] **Step 4: Checkpoint** — "datastores: world-entry DB2 store subset".

### Task 1c.3: Switch world-entry code paths from DBC to DB2

**Files:** call sites of `sChrRacesStore`/`sMapStore`/etc. in `src/server/game/Entities/Player`, `World`, `Maps`

- [ ] **Step 1: Grep** the world-entry path (`Player::Create`, `Player::LoadFromDB`, `HandlePlayerLogin`, map/area lookups) for `sDBCStores`-era accessors that the subset now replaces.
- [ ] **Step 2: Repoint** those reads to the new DB2 stores. Field names/IDs differ — fix per the ported struct definitions. Leave non-world-entry DBC reads alone for now (later phases).
- [ ] **Step 3: Verify** `game` compiles; worldserver boots and validates a test character's race/class/map against DB2 data (no crash on login attempt).
- [ ] **Step 4: Checkpoint** — "world: repoint world-entry reads to DB2".

### Task 1c.4: Regenerate UpdateFields for 54261 + player create block

**Files:**
- Modify: `src/server/game/Entities/Object/Updates/UpdateFields.h`
- Modify: `src/server/game/Entities/Object/Object.cpp` (`BuildValuesUpdate`, `BuildMovementUpdate`)
- Reference: TC `wotlk_classic` UpdateFields + `Object` build path

- [ ] **Step 1: Replace** `UpdateFields.h` with the 54261 field layout (indices/sizes/masks). This is the dominant serialization change.
- [ ] **Step 2: Adapt** `Object::BuildValuesUpdate` / mask writing to the 3.4.3 update-block format (port from TC — the mask block + create-object block changed).
- [ ] **Step 3: Verify (behavioral).** On login, the worldserver builds the player create-object update without asserting; packet log shows a well-formed `SMSG_UPDATE_OBJECT`.
- [ ] **Step 4: Checkpoint** — "world: UpdateFields + create block for 54261".

### Task 1c.5: Login sequence packets + hotfix handshake probe

**Files:** `src/server/game/Handlers/CharacterHandler.cpp` (HandlePlayerLogin), login opcodes; `src/server/game/Server/Packets/HotfixPackets.*` (new)

- [ ] **Step 1: Port** the modern login sequence (`SMSG_LOGIN_VERIFY_WORLD`, time sync, account-data times, tutorial flags, action buttons, initial spells) in the 3.4.3 packet formats.
- [ ] **Step 2: Add** `HotfixPackets` + `SMSG_HOTFIX_CONNECT`/`SMSG_HOTFIX_MESSAGE` handlers. Send an **empty/minimal** hotfix response at login.
- [ ] **Step 3: Verify (behavioral, THE Phase-1c gate).** Patched client → enter world with a character that stands in the starting zone and does NOT disconnect. Note whether the client demanded the hotfix handshake (resolves spec risk #5).
- [ ] **Step 4: Checkpoint** — "world: login sequence + hotfix handshake".

---

## Phase 1d — Move, chat, nearby objects (Phase-1 milestone)

**Exit:** character moves, chats, and sees nearby objects spawn correctly.

### Task 1d.1: Movement (de)serialization for 3.4.3

**Files:**
- Modify: `src/server/game/Entities/Object/MovementInfo` read/write; `src/server/game/Movement/**`; movement opcode handlers
- Reference: TC `wotlk_classic` movement packets + `MovementInfo`

- [ ] **Step 1: Port** the 3.4.3 `MovementInfo` bit-packed read/write and the movement opcode handlers (move start/stop/heartbeat, facing).
- [ ] **Step 2: Verify (behavioral).** Character walks/runs/turns; position updates round-trip (server echoes movement to nearby observers). Confirm no rubber-banding to spawn point.
- [ ] **Step 3: Checkpoint** — "world: 3.4.3 movement".

### Task 1d.2: Chat opcodes

**Files:** `src/server/game/Handlers/ChatHandler.cpp`, chat packets

- [ ] **Step 1: Port** `CMSG_MESSAGECHAT_*` / `SMSG_CHAT` (3.4.3 formats — modern chat split per type).
- [ ] **Step 2: Verify (behavioral).** `/say` text appears above the character and in the chat frame; a second client in range receives it.
- [ ] **Step 3: Checkpoint** — "world: 3.4.3 chat".

### Task 1d.3: Nearby object create/update (creatures + players)

**Files:** grid visibility → `Object::BuildCreateUpdateBlockForPlayer` path (already touched in 1c.4), creature/player movement update flags

- [ ] **Step 1: Verify** that grid-visible creatures and a second player produce correct create-object blocks under the 3.4.3 format (build on 1c.4). Fix update-flag/movement-block deltas for non-self units.
- [ ] **Step 2: Verify (behavioral, THE Phase-1 MILESTONE GATE).** Stand in a starting zone: nearby creatures and players spawn at correct positions, animate, and update as they move. Two clients see each other move and chat.
- [ ] **Step 3: Checkpoint** — "world: nearby object spawning — PHASE 1 COMPLETE".

---

## Phases 2–5 — high-level outline (planned in detail after Phase 1 proves the path)

- **Phase 2 — Core gameplay loop:** combat (melee/ranged), basic spellcast + aura apply, threat, death/res; loot, inventory ops, vendor/trainer/gossip, quest accept/complete. Broaden DB2 store coverage (spells/items/talents) as features demand.
- **Phase 3 — Systems breadth:** groups/raids, instances + difficulties, talents/glyphs, professions, mail, AH, guilds, achievements.
- **Phase 4 — Hotfix content pipeline:** full `DB2HotfixGenerator` so server-authored/modified data (custom items, tuned stats) renders on unmodified clients.
- **Phase 5 — Hardening & maintainability:** Warden/anticheat posture, packet abuse resistance, performance pass, and the documented AzerothCore-`master` → fork content-merge process.

---

## Deferred work & technical debt (found during implementation)

> Gaps, stubs, and limits discovered while building Phase 0 + the bnetserver this session. Each is an
> actionable deferred task: **where it surfaced**, **files**, **what to do**. **🔴 = blocks world
> entry** (must be done before/with the world-protocol bricks); 🟡 = correctness/quality; ⚪ = cleanup.
> Cross-reference: `docs/wotlk-classic-3.4.3/KNOWLEDGE.md` §5.

### Database / schema

- [ ] **🔴 D1 — Widen `account.session_key` for the 64-byte bnet key.** Surfaced in `BnetRealmList::JoinRealm`
  (brick D+E): `account.session_key` is `binary(40)` (legacy grunt SHA1 key) but the modern bnet session
  key is **64 bytes**, so `JoinRealm` truncates it. **Files:** new `data/sql/updates/pending_db_auth/rev_*.sql`;
  `src/server/shared/Realms/BnetRealmList.cpp`; `src/server/database/Database/Implementation/LoginDatabase.{h,cpp}`.
  **Do:** add `session_key_bnet binary(64)`, `client_build int unsigned`, `timezone_offset int` columns to
  `account` (TC's layout); store the full 64-byte key + build + tz in `JoinRealm`; the world-auth handshake
  (world brick D/E) reads them. Unblocks D3's `LOGIN_UPD_BNET_GAME_ACCOUNT_LOGIN_INFO`.

- [ ] **🟡 D2 — Add `region`/`battlegroup` to `realmlist`.** Surfaced in brick D+E: `Battlenet::RealmHandle`
  Region/Site default to 0 (single "0-0-0" sub-region) and character-count queries report region/bg as 1.
  **Files:** `data/sql/updates/pending_db_auth/rev_*.sql`; `LOGIN_SEL_REALMLIST` in `LoginDatabase.cpp`;
  `src/server/shared/Realms/{Realm,RealmList,BnetRealmList}.*`. **Do:** add the columns, extend the SELECT,
  populate `RealmHandle` from them so multiple bnet sub-regions work.

- [ ] **🟡 D3 — Un-stub the 9 deferred BNET prepared statements.** Surfaced bricks 1 & 3. Currently absent or
  stubbed: `LOGIN_UPD_BNET_GAME_ACCOUNT_LOGIN_INFO` (needs D1 columns), the `LOGIN_SEL_BNET_LAST_PLAYER_CHARACTERS`
  family (stubbed `... AND 0`, needs an `account_last_played_character` table), `LOGIN_SEL_BNET_CHARACTER_COUNTS_BY_*`
  (needs D2). **Files:** `LoginDatabase.{h,cpp}`; new schema. **Do:** add the schema (D1/D2 + last-played table) and
  replace the stubs with real queries.

- [ ] **🟡 D4 — Build the hotfix *delivery* protocol.** Surfaced 1a.2: the `acore_hotfixes` DB + `HotfixDatabase`
  pool exist, but `SMSG_HOTFIX_CONNECT`/`SMSG_HOTFIX_MESSAGE` + `DB2HotfixGenerator` are not built. **Files:**
  `src/server/game/Server/Packets/HotfixPackets.*` (new); `src/server/game/DataStores/DB2HotfixGenerator.*` (port).
  **Do:** Phase 1c.5 / Phase 4 — needed once we serve custom/modified data to unmodified clients.

- [ ] **⚪ D5 — Resolve the `codestyle-sql` base-dir warning for `db_hotfixes`.** Surfaced 1a.2/1a.4: adding
  `data/sql/base/db_hotfixes/` trips the linter's "notify a maintainer" warning (normal for any base-dir add).
  **Do:** maintainer ack, or align with the proper base-DB registration path if upstreaming to the fork's CI.

### bnetserver stubs

- [ ] **🟡 B1 — `ClientBuildInfo::AuthKeys` is an empty stub.** Surfaced brick D+E: AC has no `build_auth_key`
  table, so the client auth-key/CRC version check is effectively skipped. **Files:** `src/server/shared/Realms/ClientBuildInfo.{h,cpp}`;
  new `build_auth_key` schema. **Do:** populate the auth keys + enable the check if strict version validation is wanted.

- [ ] **🟡 B2 — `Acore::Timezone::GetOffsetByHash` returns UTC for everything.** Surfaced brick D+E (stub in
  `src/common/Utilities/Timezone.h`). **Do:** port TC's real timezone-offset table for correct client time display.

- [ ] **⚪ B3 — `Battlenet::SslContext::UsesDevWildcardCertificate()` returns `false`.** Surfaced brick 3
  (`src/server/shared/Network/SslContext.h`). **Do:** implement only if a dev wildcard / http-fallback path is wanted.

### ConnectTo / world handoff

- [ ] **🔴 C1 — Complete the bnet→world ConnectTo handoff.** Surfaced brick D+E: `JoinRealm` mints a join key
  but (a) stores only 40/64 bytes (see D1) and (b) the worldserver doesn't validate it (3.4.3 world auth handshake
  unbuilt). **Files:** `BnetRealmList.cpp`; world brick D/E (WorldSocket handshake). **Do:** D1 + read the protobuf
  join ticket in the modern WorldSocket handshake. This is the link between login (done) and world entry.

### Networking / cleanup

- [ ] **⚪ N1 — Remove redundant Brick-2 `SslStream`/`SslContext`.** Surfaced brick A: the modern `Acore::Net`
  stack (`src/common/network/SslStream.h`) supersedes the simplified `src/server/shared/Network/SslStream.h` +
  `SslContext.{h,cpp}` from brick 2. **Do:** confirm nothing (bnetserver/authserver) references the Brick-2 files,
  then delete them; keep `Battlenet::SslContext` only where actually used.

- [ ] **⚪ N2 — De-fragile the two colliding `Socket.h`.** Surfaced brick 3: legacy `src/server/shared/Network/Socket.h`
  (global `Socket<T>`) and modern `src/common/network/Socket.h` (`Acore::Net::Socket`) share bare filenames; the
  bnetserver CMake prepends the modern include dir to resolve `#include "Socket.h"`. **Do:** rename/namespace one set
  (e.g. modern → `NetSocket.h` or include via `network/Socket.h`) to remove the include-order dependency.

- [ ] **🔴 N3 — Proxy-protocol parity for `WorldSocketMgr` on `Acore::Net`.** Surfaced world-brick-2 scoping: the
  modern `Acore::Net::NetworkThread` lacks `EnableProxyProtocol` that AC's `WorldSocketMgr::CreateThreads` uses.
  **Files:** `src/common/network/NetworkThread.h`; `src/server/game/Server/WorldSocketMgr.cpp`. **Do:** port proxy-protocol
  support to the modern NetworkThread, or drop the feature when moving WorldSocketMgr onto Acore::Net (world brick E/F).

### Verify (low-risk but unconfirmed)

- [ ] **⚪ V1 — Confirm the `CONNECTION_BOTH` change is harmless.** Surfaced in the register-account brick:
  `LOGIN_INS_ACCOUNT` + `LOGIN_UPD_BNET_GAME_ACCOUNT_LINK` were moved `CONNECTION_ASYNC` → `CONNECTION_BOTH` so the
  `--register-bnet` CLI can `DirectExecute` them. **Do:** confirm the async worldserver paths still work (they should —
  `CONNECTION_BOTH` prepares on both pools).

- [ ] **⚪ V2 — Runtime-test the grunt auth path (or confirm irrelevant).** Surfaced bricks 1/2/D+E: the battlenet
  account-model additions are link-verified (authserver compiles+links) but the legacy grunt login wasn't run
  end-to-end. **Do:** smoke-test grunt login, or document it as intentionally-abandoned (fork-replace auth).

### Server-side DB2 (the Phase 1c on-ramp)

- [ ] **🔴 S1 — Generate 54261 DB2 metadata for the ~30-40 world-entry server tables.** Surfaced in Phase 0: only the
  6 *extractor* tables have 54261 metadata (`ExtractorDB2LoadInfo.h`). The server runtime store layer (tasks 1c.1-1c.3,
  spec §7 list) needs the world-entry set via the **same proven WoWDBDefs method** (`.dbd` LAYOUT block for 54261 →
  `DB2Meta`/`LoadInfo`, cross-checking hashes deterministically — WebFetch misreports them). **Do:** Phase 1c DB2 store
  layer using WoWDBDefs for build 54261.

### Observability

- [ ] **⚪ O1 — Make bnetserver logs readable at runtime.** Surfaced during login debugging: the console appender uses
  the Windows Console API (uncapturable via stdout redirect) and the file appender buffers until graceful shutdown, so
  `Bnet.log` is empty on a force-kill. **Do:** add a flush-on-write / line-buffered file-appender option (or a clean
  shutdown path) so runtime diagnosis doesn't rely on DB + TCP-connection-state forensics.

---

## Self-review notes

- **Spec coverage:** every Phase-0/1 spec section maps to tasks — bnetserver (1a.3–1a.4), proto dep (0.1/1a.1), TLS/cert (0.6), CASC+DB2 toolchain (0.2–0.5), 4th DB (1a.2), world handshake/crypt (1b.2), opcodes (1b.3), UpdateFields (1c.4), DB2 read layer + subset (1c.1–1c.3), hotfix handshake (1c.5), movement/chat/objects (1d). Build 54261 (1b.1). Phases 2–5 outlined per the "full multi-phase, Phase 0/1 in depth" instruction.
- **Placeholder scan:** porting tasks intentionally reference pinned upstream files + adaptation rules + behavioral gates rather than fabricated C++ — this is a deliberate, honest choice for a multi-thousand-line port, not a hidden TODO. The one genuinely unit-testable unit (DB2 header parse) carries real TDD steps.
- **Type consistency:** store names in 1c.2 match the spec §7 list; `DATABASE_HOTFIX`, `HotfixDatabasePool`, `DB2FileLoader`, `LoadDB2Stores()` are used consistently across tasks.
- **Known risk to watch:** Task 1c.4 (UpdateFields) is the highest-uncertainty task; if the client disconnects on first update, bisect via packet log against a HermesProxy capture of a real 3.4.3 session.
