# Design: Native AzerothCore fork to WotLK Classic 3.4.3 (build 54261)

**Date:** 2026-06-26
**Status:** Approved design — pending implementation plan
**Author:** brainstorming session (Claude + user)

---

## 1. Summary

Convert AzerothCore — today a single-protocol WoW 3.3.5a (build **12340**) emulator — into a
**native fork that speaks only the WoW WotLK Classic 3.4.3 client protocol (build 54261)**.
3.3.5a support is intentionally dropped (no runtime version dispatch). WotLK **content** (the
`acore_world` data, scripts, gameplay) is preserved; only the **wire protocol**, **login stack**,
and **client data layer** change.

The 3.4.3 client is a 64-bit modern client (BfA-era engine). It does **not** speak AzerothCore's
existing SRP6 "grunt" logon, does **not** read `.dbc` files the 3.3.5 way, and uses a different
opcode space, update-field layout, movement format, and world handshake. This document scopes the
full effort in phases, with a deliberately small **Phase 1 milestone**: a character that logs in via
modern Battle.net auth, enters the world, and can **move + chat** with nearby objects spawning.

## 2. Locked decisions

| Decision | Choice | Rationale |
|---|---|---|
| Architecture | **Native fork & replace** (3.4.3 only) | No runtime branching; leanest native path. Cost: lose 3.3.5a client/tooling. |
| Target build | **3.4.3.54261** (final WotLK Classic, Live, April 13 2024) | The last 3.4.3 live build per warcraft.wiki.gg/Public_client_builds; the client we actually serve. Local install: `D:\Games\World of Warcraft 3.4.3.54261`. (TC `wotlk_classic` references sit at nearby 3.4.3.x/3.4.4 builds — protocol deltas within 3.4.3 are minimal, so its sources remain valid; the *expected build constant* we set is 54261.) |
| Client + auth | **Patched client (`wow-patcher`) + modern bnetserver** | The modern client *mandates* Battle.net-style login; grunt SRP6 is not an option. |
| Phase-1 done | **Login → world → move/chat** | Prove the hardest path (auth + handshake + data) end-to-end before breadth. |

> **Correction recorded during design:** the original assumption "keep SRP6 grunt logon, small auth
> surface" is **not viable**. The 3.4.3 client requires a modern login server (REST + protobuf +
> TLS). AzerothCore has none. This makes `bnetserver` a first-class Phase-1 workstream.

## 3. Reality model — two protocol surfaces, both new

### A. Login (entirely absent from AzerothCore today)
```
client → HTTPS REST login portal (TLS, SRP6a)        ← LoginRESTService
       → bnetserver TCP (protobuf bgs.protocol RPC)   ← Account/Authentication/Connection/GameUtilities
       → realm list + game account + "connect to"     ← mints world session key
       → world socket
```
`wow-patcher` patches the client's **portal hostname**, **RSA modulus**, and **Ed25519 key**, and
**requires a real hostname + a TLS cert chaining to a trusted root CA** (no bare IPs).

### B. Worldserver wire protocol (rewrite of the serialization layer)
- New **opcode table** (3.4.3 numbering — e.g. `SMSG_HOTFIX_CONNECT = 0x460003`; nothing reuses 12340 values).
- Regenerated **UpdateFields** (different field indices/sizes/masks — the single largest coupling point).
- Modern **movement** (de)serialization (bit-packed).
- Modern **world packet header + crypt**, and a different `SMSG_AUTH_CHALLENGE` / `CMSG_AUTH_SESSION` handshake.

### C. Client data layer — **DB2 + hotfixes** (absent from AzerothCore today)
- 3.4.3 uses **DB2** stores (not 3.3.5 `.dbc`). AzerothCore has only `DBCStores` — **zero DB2**.
- Server logic reads these stores at runtime; 3.4.3 schemas differ from 3.3.5 DBC, so the data-access
  layer must move to DB2.
- The server keeps client data in sync via the **hotfix** protocol (`SMSG_HOTFIX_CONNECT` /
  `SMSG_HOTFIX_MESSAGE`) backed by a new **`acore_hotfixes`** database.

## 4. Architecture & new components

| Component | Action | Primary reference (TC `wotlk_classic`) |
|---|---|---|
| `bnetserver` (new app) | Port REST `LoginRESTService` + `SslContext` + `Session`/`SessionManager` + `ServiceDispatcher` + 4 services | `src/server/bnetserver` |
| `proto/` (new) | Vendor `bgs.protocol` + Login/RealmList `.proto`; add **Protobuf** build dep | `src/server/proto` |
| Auth DB model | Migrate `acore_auth` to battlenet + game-account model | TC auth DB |
| `acore_hotfixes` DB (new, 4th DB) | Port `hotfixes_database.sql` (`hotfix_data`); wire into schema-updater, config, connection pool | TC `sql/base/hotfixes_database.sql` |
| WorldSocket handshake | Replace auth challenge/session + world crypt with 3.4.x flow | `WorldSocket.*` |
| Opcodes | Regenerate enum + handler table for 54261 | `Opcodes.*` |
| UpdateFields | Regenerate field layout/masks for 54261 | UpdateFields + WowPacketParser |
| Movement | Port 3.4.x movement (de)serialization | Movement packets |
| DB2 subsystem (new) | Port `shared/DataStores/{DB2DatabaseLoader,DB2Store,DBStorageIterator}` + `game/DataStores/{DB2Stores,DB2Structure,DB2Metadata,DB2LoadInfo,DB2HotfixGenerator,GameTables}` | `*/DataStores` |
| Extraction toolchain (new) | Port `extractor_common` (CASC) + DB2 extractor; adapt map/vmap/mmap extractors to CASC | `src/tools` |
| Build/version plumbing | Expected build 54261; realm build info; version checks | `RealmList` / SharedDefines |

## 5. Reference-source map

- **TrinityCore `wotlk_classic`** — master reference for *every* protocol/auth/data piece (same codebase
  lineage as AzerothCore). Dominant source. Noted by TC as "very raw," so expect gaps.
- **TrinityCore `cata_classic` / `master`** — fallback where `wotlk_classic` is unfinished; `bnetserver`
  internals are largely shared across modern branches.
- **`wowemulation-dev/wow-patcher`** — client patching (portal/RSA/Ed25519); TLS+hostname requirements.
- **`wowemulation-dev/warcraft-rs`** — WoW file-format tooling (CASC/DB2/M2) for the extraction track.
- **`WowLegacyCore/HermesProxy`** — behavioral oracle for the modern-client-facing bnet + world
  translation semantics (it implements a modern `BNetServer` + `WorldServer` on its client side), even
  though we go native.

> Working method: use the `gh` CLI to scan these repos directly (`gh api .../contents?ref=<branch>`,
> `gh search code`) rather than inferring.

## 6. Full multi-phase plan

Each phase is independently testable with an explicit exit gate. Phases are ordered to prove the
hardest path (auth → handshake → data → world) before breadth.

### Phase 0 — Prerequisites, toolchain, and spike
**Goal:** have 3.4.3 server data + a patched client that reaches our portal.
- Add **Protobuf** to CMake; vendor `proto/` and confirm it generates.
- Port the **CASC extraction toolchain** (`extractor_common`) + a **DB2 extractor**; adapt
  map/vmap/mmap extractors to read the 3.4.3 CASC client. Extract maps + DB2 from a 3.4.3 install.
- Generate a **TLS cert** for a local resolvable hostname; run a 3.4.3 client through `wow-patcher`.
- **Exit:** patched client performs a TLS handshake against our (stub) REST endpoint; extracted DB2 +
  map data present on disk.

### Phase 1a — bnetserver login
**Goal:** client logs in and sees the realm list.
- Port REST login (SRP6a) + `bnetserver` session/dispatcher + Account/Authentication/Connection/
  GameUtilities services + realm list RPC.
- Migrate `acore_auth` to the battlenet/game-account model.
- **Exit:** client passes login; realm list shown.

### Phase 1b — World handshake
**Goal:** reach character enumeration.
- Implement 3.4.3 `SMSG_AUTH_CHALLENGE` / `CMSG_AUTH_SESSION` + world crypt; session-key handoff from
  bnetserver "connect to"; build check for 54261.
- **Exit:** character-list screen renders (even if empty).

### Phase 1c — Enter world
**Goal:** a character spawns and stays connected.
- Port the **opcode table** and **regenerated UpdateFields** for 54261.
- Implement the login sequence packets (`SMSG_LOGIN_VERIFY_WORLD`, time sync, account data, tutorial,
  action buttons, etc.).
- Stand up the **DB2 read layer** for the world-entry store subset (see §7) + the `acore_hotfixes`
  DB; validate whether the client demands a (possibly empty) **hotfix handshake** at login.
- **Exit:** character stands in the world without disconnecting; nearby static state correct.

### Phase 1d — Move, chat, nearby objects  *(Phase-1 milestone)*
**Goal:** the defined milestone.
- Port movement (de)serialization; chat opcodes; object create/update for nearby creatures/players.
- **Exit:** character moves, chats, and sees nearby objects spawn correctly.

### Phase 2 — Core gameplay loop
- Combat (melee/ranged), basic spellcasting + aura application, threat, death/resurrect.
- Loot, inventory operations, vendor/trainer/gossip, quest accept/complete.
- Broaden DB2 store coverage as features demand (spells, items, talents).
- **Exit:** a character can quest and fight through a starting zone.

### Phase 3 — Systems breadth
- Groups/raids, instances + difficulties, talents/glyphs, professions, mail, AH, guilds, achievements.
- **Exit:** parity with day-to-day 3.3.5a play for grouped/instanced content.

### Phase 4 — Hotfix content pipeline & custom content
- Full `DB2HotfixGenerator` pipeline: regenerate hotfix blobs for any server data that diverges from
  client-baked DB2 (custom items, tuned stats, new creatures' client-visible data).
- **Exit:** custom/modified content displays correctly on unmodified 3.4.3 clients.

### Phase 5 — Hardening & maintainability
- Warden/anticheat posture, packet fuzz/abuse resistance, performance pass.
- Establish the **upstream-merge strategy** (how AzerothCore `master` content/bugfixes flow into the fork).
- **Exit:** a documented, repeatable update process; stable multi-player test realm.

## 7. Phase-1c concrete DB2 store list

The full worldserver loads **209** DB2 stores on `wotlk_classic`. Phase 1c needs a **subset (~30–40)**
— the stores the character-enumerate / enter-world / move / chat path reads. The remainder are ported
incrementally as later phases touch them. (Structs for referenced stores must compile even if not yet
populated; the loader can be stubbed to populate only the subset first.)

**Character enumerate / create:**
`sChrRacesStore`, `sChrClassesStore`, `sChrCustomizationOptionStore`, `sChrCustomizationReqStore`,
`sCharacterLoadoutStore`, `sCharacterLoadoutItemStore` (modern `CharStartOutfit`), `sCharTitlesStore`,
`sFactionStore`, `sFactionTemplateStore`, `sPowerTypeStore`, `sPowerDisplayStore`,
`sCinematicSequencesStore`, `sCinematicCameraStore`.

**Enter world / map & positioning:**
`sMapStore`, `sMapDifficultyStore`, `sAreaTableStore`, `sAreaTriggerStore`, `sLightStore`,
`sWorldMapOverlayStore`, `sPlayerConditionStore`, `sTaxiNodesStore`, `sTaxiPathStore`,
`sTaxiPathNodeStore`.

**Skills (character setup at login):**
`sSkillLineStore`, `sSkillLineAbilityStore`, `sSkillRaceClassInfoStore`.

**Starting gear / inventory display:**
`sItemStore`, `sItemSparseStore`, `sItemEffectStore`, `sItemAppearanceStore`,
`sItemModifiedAppearanceStore` (largest Phase-1 group; needed to render equipped starting items).

**Spell name resolution (minimal):** `sSpellNameStore`.
**Currency init:** `sCurrencyTypesStore`.

> Chat needs essentially no DB2 stores — it's opcode/packet work in Phase 1d. The exact mandatory set
> is confirmed empirically in 1c by watching which missing stores cause the client to refuse world
> entry; this list is the curated starting point for the implementation plan.

## 8. Prerequisites & external dependencies

- **Build deps:** Protobuf (+ generator), OpenSSL (already present) for TLS.
- **Runtime/dev:** a resolvable **hostname** + **TLS cert** trusted by the client host (for `wow-patcher`).
- **Client:** a 3.4.3.54261 install (available at `D:\Games\World of Warcraft 3.4.3.54261`) + `wow-patcher`.
- **Data:** extracted DB2 + maps/vmaps/mmaps from the 3.4.3 CASC client (new toolchain, Phase 0).
- **New database:** `acore_hotfixes` alongside `acore_auth` / `acore_characters` / `acore_world`.

## 9. Risks & open questions

1. **DB2/hotfix iceberg** — biggest single workstream (`DB2Stores.cpp` ~142 KB, `DB2Structure.h`
   ~103 KB upstream). Mitigation: port the world-entry subset first (§7); defer the full set + hotfix
   *content* pipeline to Phases 2/4.
2. **`wotlk_classic` is "very raw"** (per TC) — expect to backfill from `cata_classic`/`master`.
3. **bnetserver port friction** — AzerothCore lacks the modern `shared/Networking`/`Packets` stack and
   uses `Acore::` namespace + helpers; the port is not copy-paste.
4. **TLS/hostname requirement** complicates local dev (cert + DNS or hosts entry).
5. **Empty-hotfix handshake** — unknown whether the client demands `SMSG_HOTFIX_CONNECT` even with no
   custom data; resolve in Phase 1c.
6. **Maintainability / divergence** — forking this hard from AzerothCore `master` means inheriting
   content/bugfix updates manually; decide the long-term merge story by Phase 5.
7. **Warden** — modern clients ship anticheat; `wow-patcher` works without in-client memory edits, but
   Warden posture must be addressed before any public realm (Phase 5).

## 10. Codebase coupling points (from reconnaissance)

Current AzerothCore is hard-wired to 12340 with **no version abstraction**:
- `src/server/apps/authserver/**` — grunt SRP6 logon (to be replaced by `bnetserver`).
- `src/server/shared/Realms/RealmList.*`, `apps/authserver/Authentication/AuthCodes.cpp`,
  `data/sql/base/db_auth/build_info.sql` — build/version checks (retarget to 54261).
- `src/server/game/Server/WorldSocket.*`, `Protocol/ServerPktHeader.h`,
  `common/Cryptography/Authentication/AuthCrypt.*` — world handshake/header/crypt.
- `src/server/game/Server/Protocol/Opcodes.{h,cpp}` — single hardcoded opcode table.
- `src/server/game/Entities/Object/Updates/UpdateFields.h` (+ `Object.cpp` `BuildValuesUpdate`) —
  auto-generated for 12340; the dominant serialization coupling.
- `src/server/game/DataStores/` — `DBCStores` only; **no DB2** (to be replaced/augmented).
- `src/tools/` — MPQ-based extractors (to be ported to CASC + DB2).

## 11. Long-term strategy note

Because this is a *fork & replace*, AzerothCore upstream content and bugfixes do not flow in
automatically. Two viable models, to be decided by Phase 5:
- **Vendor protocol/login/data as a layer** kept as isolated as practical, periodically merging
  `master` content — minimizes drift but constrains how invasive the protocol changes can be.
- **Hard fork**, cherry-picking upstream content fixes manually — maximum freedom, higher maintenance.

The phasing above keeps protocol/login/data changes as self-contained as the codebase allows to keep
the first option open.
