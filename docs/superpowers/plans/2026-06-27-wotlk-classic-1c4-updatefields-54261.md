# Task 1c.4 — UpdateFields migration to build 54261 (structured UF model) Implementation Plan

> **EXECUTION STATUS (2026-06-28, work-in-place on `feature/wotlk-classic-3.4.3`, UNCOMMITTED, NOT yet built):**
> - **Phase A ✅** — TypeID(14-type)/TypeMask in `ObjectGuid.h`; templated `UpdateMask<Bits>`; `UF::UpdateField.{h,cpp}`.
> - **Phase B ✅** — `UpdateFields.{h,cpp}` (selective), `ViewerDependentValues.h`, `QuaternionData.h` shim, minimal
>   packet-common types (`ItemBonusKey`/`DungeonScoreSummary`/`PerksVendorItem`). 3.4.3 has NO Azerite structs; omitted
>   AreaTrigger/Scene/Conversation; no field renumbering (wire byte-identical to xian55).
> - **Phase C1+C2 ✅** — `Object.{h,cpp}` rewired to `m_values`+`m_objectData`, helper templates, build path,
>   `CreateObjectBits m_updateFlag`, **`BuildMovementUpdate` transcribed verbatim from xian55**; `UpdateData.{h,cpp}`
>   to 3.4.3 shape; deleted `UpdateFieldFlags.{h,cpp}`; 9 `UpdateData(map)` ctor sites fixed.
> - **REMAINING:** C3 (per-entity holders + BuildValues* for Item/Bag/Unit/Player/GameObject/DynamicObject/Corpse),
>   the **Phase-D reconciliation worklist** below, first `game` build, then Phase E behavioral gate.
>
> **PHASE-D RECONCILIATION WORKLIST** (AC lacks these APIs that the verbatim C1/C2 port references — fix at build):
> - *BuildMovementUpdate (Object.cpp) — the hard one, needs AC-specific rewrite to emit 3.4.3 wire format from AC's
>   3.3.5 MovementInfo:* `GetExtraUnitMovementFlags2` (AC has only `…Flags()`→uint16), `IsSplineEnabled` (use
>   `MOVEMENTFLAG_SPLINE_ENABLED`), `GetMovementForces`/`MovementForces`/`MovementForce` (absent), `IsPlayingHoverAnim`
>   (absent), MovementInfo `inertia`/`advFlying`/`standingOnGameObjectGUID`/`stepUpStartElevation` (absent),
>   `jump.fallTime` (AC: top-level `fallTime`), no `<<` for `MovementInfo::TransportInfo`, `WorldPackets::Movement::
>   CommonMovement::*` spline writers (use `Movement::PacketBuilder::WriteCreate(*movespline, data)`), VehicleInfo
>   field `ID`→`m_ID`. **Remove the dead AreaTrigger/SceneObject/Conversation/SmoothPhasing movement blocks** (no AC
>   entity — they reference `ToAreaTrigger()` etc. that don't exist; the `CreateObjectBits` for them are always false).
> - *GameObject/WorldObject:* `GetPauseTimes`/`GetWorldEffectID`/`GetPackedLocalRotation`(AC:`GetPackedWorldRotation`→
>   int64)/`GetAIAnimKitId`/`GetMovementAnimKitId`/`GetMeleeAnimKitId`/`GetSmoothPhasing` — absent.
> - *ActivePlayer:* `Player::GetSceneMgr` absent (drop SceneObject/Scene block); verify rune/actionbutton accessors.
> - *ViewerDependentValues.h (Phase B):* `GetCreatureIdVisibleToSummoner`, `hasLootRecipient`/`isTappedBy`,
>   `GetGoStateFor`, `BuildAuraStateUpdateForTarget`, `MeetsInteractCondition`, `GetDifficultyID`, flag enums
>   `UNIT_FLAG_UNINTERACTIBLE`/`UNIT_FLAG3_ALREADY_SKINNED`/`GO_DYNFLAG_LO_*` — reconcile to AC equivalents.
> - *Cross-type changes from C1/C2 to propagate (Phase D):* `_Create(ObjectGuid const&)` sig change breaks derived
>   `_Create` callers; `AddToObjectUpdate()` `void`→`bool` (fix Item/other overrides); `GetDynamicFlags`/
>   `ReplaceAllDynamicFlags` now non-virtual (remove derived `virtual` overrides in Unit/GameObject); `DestroyForPlayer`
>   dropped `bool onDeath` + arena-destroy packet; 5 files still `#include "UpdateFieldFlags.h"` (Unit.cpp,
>   PlayerUpdates.cpp, PlayerStorage.cpp, GameObject.cpp, Group.cpp) — remove the include + old-API uses; ~9 UpdateData
>   sites still call removed `BuildPacket(WorldPacket&)`/`AddUpdateBlock(ByteBuffer)`/`SetFieldNotifyFlag`.

> **PHASE C3 + first-build done (2026-06-28):** all 7 entity holders + BuildValues* ported; cmake reconfigure
> SUCCEEDS (new UF/packet files globbed in); **`BuildMovementUpdate` REWRITTEN for AC** (keystone done — emits 3.4.3
> wire order from AC's 3.3.5 MovementInfo + `Movement::PacketBuilder::WriteCreate`; dead AreaTrigger/SmoothPhasing/
> Conversation blocks deleted; **wire-order items to validate at the Phase-E capture:** 3rd movement-flag word=0,
> StepUpStartElevation=0, transport prevTime←time2 + vehicleId absent, rune-cooldown byte formula, AnimKit/WorldEffect
> zero-fills); stale legacy methods cleaned (old `BuildValuesUpdate(uint8)` overrides removed, `AddToObjectUpdate`
> void→bool, redundant virtual DynamicFlags overrides removed, 5 stale `UpdateFieldFlags.h` includes removed). Build
> is still RED at the **PCH wall (Category A below)** — that gates everything.
>
> **REMAINING WORK — categorized:**
> - **A ✅ DONE (2026-06-28) — entity-HEADER inline flat accessors. THE PCH NOW COMPILES.** Converted ~140 accessors
>   across Unit.h/Player.h/Item.h/Bag.h/Corpse.h/GameObject.h/DynamicObject.h + Transport.h/Pet.h. Byte→named-field
>   maps (verified vs xian55): UNIT BYTES_0→ClassId/Sex/DisplayPower; BYTES_1 b0→StandState,b3→VisFlags; BYTES_2
>   b0→SheatheState,b1→PvpFlags,b3→ShapeshiftForm; Pet BYTES_1 b1→PetTalentPoints. Quest-slot block →
>   PlayerData::QuestLog[].QuestID/StateFlags/EndTime/ObjectiveProgress. **TODO-stubs left (WotLK fields absent from
>   3.4.3 ActivePlayerData — return 0/discard, tagged `// [1c.4] TODO:`):** GetFreePrimaryProfessionPoints/
>   SetFreePrimaryProfessions, GetHonorPoints/GetArenaPoints, SetInGuild/GetGuildId (guild id round-tripped through
>   GuildGUID counter — wire may need a real guild guid later).
> - **A′ (2 surgical Player.h fixes, next-wave blockers):** `Player.h:67` `class UpdateMask;` forward-decl is now a
>   template (remove the forward-decl / include UpdateMask.h); `Player.h:2044-45` re-sync Player's
>   `BuildCreateUpdateBlockForPlayer` (→ `const`) and `DestroyForPlayer` (drop `bool`) to Object's new virtual sigs.
> - **C — `ViewerDependentValues.h` (~15 APIs):** re-home the deleted `BuildValuesUpdate(uint8)` viewer logic;
>   reconcile `GetCreatureIdVisibleToSummoner`, `hasLootRecipient`/`isTappedBy`, `GetGoStateFor`,
>   `MeetsInteractCondition`, `Map::GetDifficultyID`, `UNIT_FLAG_UNINTERACTIBLE`, `UNIT_FLAG3_ALREADY_SKINNED`,
>   `BuildAuraStateUpdateForTarget`, `GO_DYNFLAG_LO_*` to AC equivalents.
> - **D (~16) — remove orphaned 3.3.5 value-cache:** `PatchValuesUpdate`, `_valuesUpdateCache`,
>   `InvalidateValuesUpdateCache()`, `ShouldTrackValuesUpdatePosByIndex` (Unit.h/Unit.cpp + callers). No structured
>   analogue — delete.
> - **B (~937 game + ~321 scripts + ~1394 field-index enum refs) — impl call-site sweep:** bucket by dir; convert
>   per xian55. Top methods: SetUInt32Value 277, GetUInt32Value 208, SetGuidValue 64, SetFloatValue 60, SetByteValue
>   58, GetFloatValue 54, GetByteValue 50, GetGuidValue 34, SetFlag 31, RemoveFlag 29.
> - **E (~19) — residual:** `BuildPacket(packet)`→`BuildPacket(&packet)` (~13), remaining `Set/RemoveFieldNotifyFlag`.
>
> **⚠️ DATA-SAFETY HAZARD discovered in the 2026-06-28 sweep — Player/Corpse appearance.** 3.4.3 moved
> `PLAYER_BYTES`/corpse bytes (skin/face/hair/hairColor/facialStyle) into `ChrCustomizationChoice` dynamic data.
> The character **DB save/load blocks in `PlayerStorage.cpp` + `Player::Create`/`CreateCorpse` were intentionally
> LEFT UNCONVERTED** — naively writing 0 to the structured fields would **wipe the DB appearance columns**. These
> require porting the ChrCustomizationChoice round-trip (cache appearance ↔ DB) BEFORE conversion. Do NOT stub these.
>
> **Cat-B sweep status (2026-06-28, parallel buckets):** Entities/Unit+Creature+Pet+Totem+Object — DONE (Cat D
> value-cache removed; Cat C ViewerDependentValues reconciled w/ documented stubs). Entities/Player+Item+Bag+GO+
> DynObj+Corpse+Transport — mostly DONE incl. the 2 Player.h fixes; **~158 sites REMAIN in `Player.cpp`**:
> `InitStatsForLevel` (~50, needs Unit stat field-sets), `Player::Create` (~20), the DB-save appearance blocks (~30,
> the hazard above), Pet `UNIT_FIELD_*` setters (~9), RestInfo/rune-regen/no-reagent/anim-tier/inebriation/hair.
> Remaining buckets dispatched: Spells/, other game dirs, scripts/. **Known stubs to revisit (WotLK features absent
> from 3.4.3 ActivePlayerData):** arena-team info, honor today/yesterday contribution, known currencies, RAF,
> `PLAYER_SELF_RES_SPELL`→SelfResSpells list, `PLAYER_RUNE_REGEN_*`, guild-GUID on Corpse (no HighGuid::Guild).
>
> **CAT-B SWEEP COMPLETE (2026-06-28) — ALL buckets done (~600+ sites: Unit/Creature/Pet/Object, Player/Item/Bag/
> GO/DynObj/Corpse/Transport, Spells, Handlers+Globals+Maps+BGs+AI, scripts). NEXT, in order:**
>
> **STEP 1 (DO FIRST) — add the missing PUBLIC typed-wrapper helpers** the sweep call sites depend on (the
> underlying UF fields all exist; only the wrappers are missing). Access-control note: `Object::SetUpdateFieldValue*`
> are PROTECTED (friends: Map on Object, WorldSession on Player), so non-entity/script code REQUIRES these public
> wrappers. Reference xian55's same-named entity headers for each body:
> - **Unit.h:** `SetChannelSpellId`, `ClearChannelObjects`, `AddChannelObject`, `SetChannelObject(slot,guid)`,
>   `ApplyModPowerCostPCT`, `ApplyModManaCostModifier`, `GetEmoteState()`, `SetRace`, power-regen flat-mod setters,
>   `GetTrackCreatureMask()`.
> - **Player.h:** `Add/RemoveAuraVision`, `ApplyModTargetResistance`, `ApplyModTargetPhysicalResistance`,
>   `ApplyModDamageDonePos/Neg`, `SetModDamageDonePercent`, `SetNoRegentCostMask`, `ApplyModFakeInebriation`,
>   `SetOverrideSpellsId`, `Set/RemovePlayerLocalFlag` + `enum PlayerLocalFlags` (TRACK_STEALTHED, RELEASE_TIMER),
>   `SetPetSpellPower`, melee/spell crit% getters, honor/kills/contribution setters, `ExploredZones` bit+bulk setter,
>   `KnownTitles` bulk-mask setter, `NativeSex` setter.
> - **GameObject:** `ParentRotation` via QuaternionData (2 ICC Arthas-platform sites currently TODO).
> - Note: `ForceValuesUpdateAtIndex` is GONE (resend is automatic now) — the 5 script TODO stubs can stay no-op.
>
> **STEP 2 — finish ~158 remaining `Player.cpp` sites** (InitStatsForLevel ~50, Player::Create ~20, Pet setters ~9,
> RestInfo/rune-regen/anim-tier/inebriation). **STEP 3 — port ChrCustomizationChoice** to safely convert the
> DB-save appearance blocks (the data-safety hazard above) — do NOT stub those. **STEP 4 — central build**
> (`cmake -S . -B build` then `cmake --build build --target worldserver --config RelWithDebInfo`) and iterate on
> residual errors (expect: TODO-stub sites, any wrapper still missing, ADL/serialize issues in UpdateFields.cpp).
> **STEP 5 — Phase E** packet-capture validation (movement wire-order risks listed above). **Then revisit the
> WotLK-absent stubs** (arena team info, honor contribution, known currencies, RAF, SelfResSpells list, rune-regen).
>
> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> This is a **sub-plan** expanding Task 1c.4 of `2026-06-26-wotlk-classic-3.4.3-phase0-phase1.md`. It is the single
> largest change in the port. Read that parent plan's "How to use this plan" (reference-fetch pattern, AzerothCore
> adaptation rules, build base command) first — they apply verbatim here.

**Goal:** Replace AzerothCore's flat `uint32 m_uint32Values[]` object-value model with the 3.4.3.54261 structured
`UF::UpdateField<T>` model, so the worldserver serializes `SMSG_UPDATE_OBJECT` create/values blocks in the format the
patched WotLK Classic client expects.

**Architecture:** Port the generic UpdateField machinery (templated `UpdateMask<Bits>`, `UpdateField.{h,cpp}`) from the
`D:\Games\xian55-3.4.3_Source` reference (== build 54261), extend the `TypeID` enum to the 14-type 3.4.3 layout, port
the per-entity UF data structures **selectively** (only the entity types AzerothCore actually has), then rewire `Object`
and each entity (`Item`/`Bag`/`Unit`/`Player`/`GameObject`/`DynamicObject`/`Corpse`) to own typed holders and serialize
through `WriteCreate`/`WriteUpdate`. Finally, sweep the ~779 raw flat-accessor call sites to the structured model.

**Tech Stack:** C++20, MSVC/RelWithDebInfo, AzerothCore game lib + scripts. Reference: xian55 3.4.3 (TrinityCore
`wotlk_classic` lineage). No new third-party deps.

---

## How to use this sub-plan

**This is an unsplittable big-bang.** The new `UpdateField.h` includes `UpdateMask.h` and uses a *templated*
`UpdateMask<Bits>`; AC's current `UpdateMask.h` defines a *non-template* `class UpdateMask` consumed by the existing
`Object.cpp`/`UpdateData.cpp`. The two cannot coexist. The moment Task A2 lands, every consumer of the old flat model
stops compiling and **the build stays red until Tasks C and D complete.** Plan accordingly:

- **Work in an isolated worktree** (REQUIRED SUB-SKILL: superpowers:using-git-worktrees), off current `HEAD`. This keeps
  the *other agent's* uncommitted Phase-2 edits (`AuthHandler.cpp`, `WorldSession.*`, `WorldSocket.cpp`, `Opcodes.*`,
  `MiscHandler.cpp`) out of this branch. Coordinate the eventual merge — `Object`/`Unit`/`Player` are touched by both.
- **The verification gate is at the end** (Task E: full game + scripts build links, then boot + behavioral). The
  per-task "gate" lines are *best-effort partial* checks (does this file parse in isolation?), not green-build gates.
  Build the `game` target frequently inside the worktree to shrink the error surface (`cmake --build build --target
  game --config RelWithDebInfo`).
- **Reference-fetch, don't fabricate.** The bulk ports (`UpdateField.h`, `UpdateMask.h`, the UF structures, the
  per-entity accessor bodies) are transcribed from the named xian55 file with the parent plan's adaptation rules
  applied. This plan inlines authored *glue* code (enum, `Object` skeleton, CMake) and gives **selection + adaptation
  rules** for the verbatim ports rather than reproducing thousands of lines. This mirrors the parent plan's explicit
  convention.

**Entity scope (CRITICAL).** AzerothCore's client-object entities are: `Object`, `Item`, `Bag` (=Container), `Unit`
(`Creature`/`Pet`/`TempSummon`/`Totem`/`Vehicle`), `Player`, `GameObject`, `DynamicObject`, `Corpse`. AC has **no**
`AreaTrigger`/`Conversation`/`SceneObject` *entities* and **no** Azerite items. Therefore port **only** these UF
structures and **omit** the rest:

| Port (AC has the entity) | Omit (AC has no such entity) |
|---|---|
| ObjectData, ItemData (+ItemEnchantment/ItemModList/SocketedGem), ContainerData | AzeriteEmpoweredItemData, AzeriteItemData |
| UnitData (+VisibleItem, ChrCustomizationChoice), PlayerData, ActivePlayerData (+QuestLog/SkillInfo/RestInfo/PVPInfo/ArenaCooldown/…) | AreaTriggerData (+ScaleCurve/VisualAnim), SceneObjectData, ConversationData |
| GameObjectData, DynamicObjectData, CorpseData | |

When a ported struct references an omitted one, drop that field (and its bit) — but keep the **bit numbering of the
remaining fields identical to xian55** (the client reads fields by bit position). Where a field maps to data AC's WotLK
content lacks, keep the field and serialize a default (0) so the wire layout stays correct. **Wire layout fidelity to
the 54261 client beats internal tidiness — never renumber bits.**

**Verified primitive inventory (do NOT re-port these — they exist in AC):**

| Primitive | AC location | Note |
|---|---|---|
| `Optional<T>` | `src/common/Utilities/Optional.h` | `using Optional = std::optional<T>` |
| `EnumFlag` / `DEFINE_ENUM_FLAG` | `src/common/Utilities/EnumFlag.h` | identical interface to xian55 |
| `flag96` / `flag128` / `FlagsArray` | `src/common/Utilities/Util.h` | already used by SpellInfo |
| `DBCPosition3D` (and 2D) | `src/server/game/DataStores/DB2Structure.h` (DBCEnums.h) | added during 1c.1–1c.3 |
| `QuaternionData` | `src/server/game/Entities/GameObject/GameObjectData.h` | see Task A4 for include shim |
| `ByteBuffer` bit ops (`WriteBit`/`WriteBits`/`FlushBits`/`HasUnfinishedBitPack`) | `src/server/shared/Packets/ByteBuffer.h` | identical interface |
| `ObjectGuid` | `src/server/game/Entities/Object/ObjectGuid.h` | |

---

## File structure

**New files (created):**
- `src/server/game/Entities/Object/Updates/UpdateField.h` — generic UF machinery (templates).
- `src/server/game/Entities/Object/Updates/UpdateField.cpp` — `WriteDynamicFieldUpdateMask` helpers.
- `src/server/game/Entities/Object/Updates/ViewerDependentValues.h` — per-target value overrides (selective).
- `src/server/game/Entities/Object/Updates/QuaternionData.h` — include shim (Task A4), only if needed.

**Replaced files (full rewrite to 3.4.3 form):**
- `src/server/game/Entities/Object/Updates/UpdateMask.h` — non-template → `template<uint32 Bits> class UpdateMask`.
- `src/server/game/Entities/Object/Updates/UpdateFields.h` — flat enums → UF data structs (selective).
- `src/server/game/Entities/Object/Updates/UpdateFields.cpp` — **new** (selective serializers; AC has none today).
- `src/server/game/Entities/Object/Updates/UpdateData.{h,cpp}` — 3.4.3 packet shape (map id, destroy/oor split).
- `src/server/game/Entities/Object/Object.{h,cpp}` — holder members, helper templates, build path.

**Deleted file:**
- `src/server/game/Entities/Object/Updates/UpdateFieldFlags.{h,cpp}` — the 3.3.5 per-index visibility-flag tables
  (139 KB) are replaced by `GetUpdateFieldFlagsFor()` + per-field flags baked into the UF structs.

**Modified (holders + accessor bodies + call sites):**
- `src/server/game/Entities/Object/ObjectGuid.h` — extend `TypeID` enum + `NUM_CLIENT_OBJECT_TYPES`.
- Entity headers/impls: `Item.{h,cpp}`, `Bag.{h,cpp}`, `Unit.{h,cpp}`, `Player.{h,cpp}`, `GameObject.{h,cpp}`,
  `DynamicObject.{h,cpp}`, `Corpse.{h,cpp}`, plus `Creature`/`Pet`/`Totem`/`Vehicle`/`TempSummon` where they set fields.
- ~173 files across `src/server/game` + `src/server/scripts` with raw flat-accessor call sites.

CMake: `src/server/game/CMakeLists.txt` uses recursive `CollectSourceFiles` glob — **new `.cpp` files are picked up
automatically**; no CMake edit needed beyond a clean re-configure.

---

## Phase A — Foundation (machinery + type system)

### Task A1: Extend the `TypeID` enum to the 14-type 3.4.3 layout

**Files:**
- Modify: `src/server/game/Entities/Object/ObjectGuid.h:30-42` (enum `TypeID` + `NUM_CLIENT_OBJECT_TYPES`)
- Reference: `D:\Games\xian55-3.4.3_Source\src\server\game\Entities\Object\ObjectGuid.h:33-51`

- [ ] **Step 1: Replace** the enum body. The values must match the client exactly (holder block-bits derive from them):

```cpp
enum TypeID
{
    TYPEID_OBJECT                 = 0,
    TYPEID_ITEM                   = 1,
    TYPEID_CONTAINER              = 2,
    TYPEID_AZERITE_EMPOWERED_ITEM = 3,   // no AC entity — reserved for wire-layout parity
    TYPEID_AZERITE_ITEM           = 4,   // no AC entity — reserved for wire-layout parity
    TYPEID_UNIT                   = 5,
    TYPEID_PLAYER                 = 6,
    TYPEID_ACTIVE_PLAYER          = 7,
    TYPEID_GAMEOBJECT             = 8,
    TYPEID_DYNAMICOBJECT          = 9,
    TYPEID_CORPSE                 = 10,
    TYPEID_AREATRIGGER            = 11,   // no AC entity — reserved
    TYPEID_SCENEOBJECT            = 12,   // no AC entity — reserved
    TYPEID_CONVERSATION           = 13    // no AC entity — reserved
};

#define NUM_CLIENT_OBJECT_TYPES             14
```

- [ ] **Step 2: Audit `TYPEMASK_*`.** Find the `TypeMask` enum (same header / `Object.h`). The masks are `1 <<
  TYPEID_*`; with the new values they shift. Verify `TYPEMASK_UNIT`/`TYPEMASK_PLAYER`/etc. are defined as
  `(1 << TYPEID_x)` (recompute) and not hard-coded literals. Fix any hard-coded mask literals to the `1 << TYPEID_x`
  form. Grep: `grep -rn "TYPEMASK_" src/server/game/Entities/Object/`.

- [ ] **Step 3: Find switch/array hazards.** Grep `GetByteValue`-free switches over `GetTypeId()` and any array sized
  `[NUM_CLIENT_OBJECT_TYPES]` or `[TYPEID_CORPSE + 1]`. Run:
  `grep -rn "NUM_CLIENT_OBJECT_TYPES\|TYPEID_CORPSE + 1\|case TYPEID_" src/server/game | head -60`.
  Note each for the call-site sweep (Task D); no edit yet beyond ones in this header.

- [ ] **Step 4: Partial-parse gate.** `ObjectGuid.h` still compiles on its own (it's a leaf header). Defer the real
  gate to Task E.

- [ ] **Step 5: Commit** — "datastores: extend TypeID enum to 14-type 3.4.3 layout (1c.4-A)".

### Task A2: Port the templated `UpdateMask<Bits>`

**Files:**
- Replace: `src/server/game/Entities/Object/Updates/UpdateMask.h`
- Reference: `D:\Games\xian55-3.4.3_Source\src\server\game\Entities\Object\Updates\UpdateMask.h` (164 lines)

- [ ] **Step 1: Overwrite** `UpdateMask.h` with the xian55 file verbatim. The only adaptation is the license header
  (`TrinityCore` → `AzerothCore` per repo convention; the rest of the file has no `Trinity::`/`TC_*` symbols). Key
  shape it must end with: namespace `UpdateMaskHelpers` (`GetBlockIndex`/`GetBlockFlag`) + `template<uint32 Bits> class
  UpdateMask` with `Set/Reset/SetAll/ResetAll/GetBlock/GetBlocksMask/IsAnySet/operator[]/operator&=/operator|=`.

- [ ] **Step 2: Confirm** no remaining `#include` needs adaptation — it includes only `"Define.h"` + `<algorithm>`,
  both present in AC.

- [ ] **Step 3: Note the break.** From here the old `UpdateMask` API (`SetBit`/`GetBlockCount`/`AppendToPacket`/
  `SetCount`) no longer exists; `Object.cpp`, `UpdateData.cpp` will not compile until Task C. Expected.

- [ ] **Step 4: Commit** — "datastores: templated UpdateMask<Bits> for 54261 (1c.4-A)".

### Task A3: Port `UpdateField.h` + `UpdateField.cpp`

**Files:**
- Create: `src/server/game/Entities/Object/Updates/UpdateField.h`
- Create: `src/server/game/Entities/Object/Updates/UpdateField.cpp`
- Reference: xian55 `.../Updates/UpdateField.h` (991 lines) + `UpdateField.cpp` (64 lines)

- [ ] **Step 1: Transcribe** `UpdateField.h` verbatim. Adaptation checklist (the file is already `UF::`-namespaced and
  clean):
  - License header → AzerothCore.
  - Includes resolve as-is: `"ObjectGuid.h"`, `"Optional.h"`, `"UpdateMask.h"` all exist in AC.
  - `DEFINE_ENUM_FLAG(UpdateFieldFlag)` resolves via `src/common/Utilities/EnumFlag.h` — ensure that header is
    transitively included (it is, via `Define.h`/`EnumFlag.h`; add `#include "EnumFlag.h"` explicitly at top if MSVC
    can't see the macro).
  - `UpdateMask<NUM_CLIENT_OBJECT_TYPES>` in `UpdateFieldHolder` (line ~697) resolves to 14 after Task A1.
  - Forward decls `class ByteBuffer; class Object;` stay.

- [ ] **Step 2: Transcribe** `UpdateField.cpp` verbatim (the two `UF::WriteDynamicFieldUpdateMask` /
  `WriteCompleteDynamicFieldUpdateMask` helpers). Adaptation: license header; `#include "ByteBuffer.h"` — verify the
  include path resolves (AC's ByteBuffer is at `src/server/shared/Packets/ByteBuffer.h`, on the game include path).

- [ ] **Step 3: Isolated-compile gate.** Author a throwaway TU that includes `UpdateField.h` and instantiates
  `UF::UpdateMask<4> m; m.Set(2);` — confirm it parses. (Optional; can fold into Task E.) Delete the throwaway.

- [ ] **Step 4: Commit** — "datastores: port UF::UpdateField machinery (1c.4-A)".

### Task A4: `QuaternionData.h` include shim + primitive verification

**Files:**
- Create (if needed): `src/server/game/Entities/Object/Updates/QuaternionData.h`
- Modify (alternative): the `#include` line in the new `UpdateFields.h`

- [ ] **Step 1: Locate** AC's `QuaternionData`. It is defined in
  `src/server/game/Entities/GameObject/GameObjectData.h`. xian55's `UpdateFields.h` does `#include "QuaternionData.h"`.
  Choose ONE:
  - **(a)** Create a 6-line `QuaternionData.h` that `#include`s `GameObjectData.h` (or moves the struct), **or**
  - **(b)** Change the `UpdateFields.h` include to `"GameObjectData.h"`.
  Prefer (b) — it avoids a header that drags GameObject into Item/Unit TUs only if `GameObjectData.h` is lightweight;
  if it isn't, do (a) and move the `struct QuaternionData { float X,Y,Z,W; ... }` into the new header, re-included by
  `GameObjectData.h`. Pick (a) if a grep shows `GameObjectData.h` pulls heavy deps.

- [ ] **Step 2: Verify primitives** referenced by the UF structs exist (one grep each; all expected PRESENT per the
  inventory table): `flag128`/`flag96` (`src/common/Utilities/Util.h`), `DBCPosition2D`/`DBCPosition3D`
  (`DB2Structure.h`/`DBCEnums.h`), `Optional` (`src/common/Utilities/Optional.h`). If `DBCPosition2D` is absent while
  3D is present, add the 4-line packed `struct DBCPosition2D { float X; float Y; };` next to 3D.

- [ ] **Step 3: Commit** — "datastores: QuaternionData include shim for UF structures (1c.4-A)".

---

## Phase B — Per-entity UF data structures (selective port)

> These two tasks port the structure definitions + serializers for AC's entity set only (see "Entity scope"). They
> reference omitted entities in xian55, so they are a **filtered transcription**, not a copy. The build remains red.

### Task B1: Port `UpdateFields.h` (structure declarations, selective)

**Files:**
- Replace: `src/server/game/Entities/Object/Updates/UpdateFields.h`
- Reference: xian55 `.../Updates/UpdateFields.h` (943 lines)

- [ ] **Step 1: Transcribe** the `namespace UF { ... }` declarations for the **Port** column structs only (ObjectData,
  ItemEnchantment, ItemModList, SocketedGem, ItemData, ContainerData, VisibleItem, ChrCustomizationChoice, UnitData,
  QuestLog, ArenaCooldown, PlayerData, SkillInfo, RestInfo, PVPInfo, CompletedProject, ResearchHistory, TraitConfig,
  StablePetInfo, StableInfo, ActivePlayerData, GameObjectData, DynamicObjectData, CorpseData). Each is a
  `struct X : public ... HasChangesMask<N> { ... fields ...; void WriteCreate(...); void WriteUpdate(...); void
  ClearChangesMask(); };`.

- [ ] **Step 2: Drop omitted structs** (AzeriteEmpoweredItemData, AzeriteItemData, AreaTriggerData, ScaleCurve,
  VisualAnim, SceneObjectData, ConversationData) **and any field referencing them.** When you remove a field, leave a
  comment `// [54261] <FieldName> omitted (no AC entity) — bit N reserved` and **do not renumber** the remaining
  fields' `Bit`/`BlockBit`/`FirstElementBit` template args. The `HasChangesMask<N>` bit count stays as xian55 has it.

- [ ] **Step 3: Fix includes** — replace `"QuaternionData.h"` per Task A4; keep `"EnumFlag.h"`, `"ObjectGuid.h"`,
  `"Position.h"`, `"UpdateField.h"`, `"UpdateMask.h"`. Drop `"ItemPacketsCommon.h"`/`"MythicPlusPacketsCommon.h"`/
  `"PerksProgramPacketsCommon.h"` **only if** the structs that needed them were omitted; otherwise port the small
  referenced type (e.g. `UF::ItemEnchantment` lives in UpdateFields itself; `ItemPacketsCommon` types like
  `ItemBonuses`/`ItemMod` may be needed by ItemData — port those small structs into a new
  `src/server/game/Server/Packets/ItemPacketsCommon.h` if ItemData uses them).

- [ ] **Step 4: Reconcile field types** against AC. Where a field uses a DB2/enum type, confirm AC's name matches
  (e.g. `ChrCustomizationChoice`, `QuaternionData`, `ObjectGuid`, `flag128`). Adjust `Acore::`-namespaced where needed.

- [ ] **Step 5: Commit** — "datastores: UF structure declarations for 54261 (selective) (1c.4-B)".

### Task B2: Port `UpdateFields.cpp` + `ViewerDependentValues.h` (serializers, selective)

**Files:**
- Create: `src/server/game/Entities/Object/Updates/UpdateFields.cpp`
- Create: `src/server/game/Entities/Object/Updates/ViewerDependentValues.h`
- Reference: xian55 `.../Updates/UpdateFields.cpp` (5097 lines) + `ViewerDependentValues.h`

- [ ] **Step 1: Transcribe** the `WriteCreate`/`WriteUpdate`/`ClearChangesMask` bodies for the ported structs only
  (mirror the Task B1 selection). Each body is generated bit-serialization (`data << field; data.WriteBits(...);
  data.FlushBits();`) — transcribe exactly; the wire order is the contract with the client.

- [ ] **Step 2: Drop** the bodies of omitted structs and any `if (fieldVisibility...) WriteX(omittedField)` lines.
  Where an omitted struct was a field of a kept struct (Step B1 dropped the field), delete the matching serialization
  line — **but** if the client expects bytes at that position, write the default instead (`data << uint32(0);`) to
  preserve offset. Decide per field by checking xian55's WriteCreate order: a removed *dynamic/optional* field whose
  bit is cleared needs no bytes; a removed *fixed* field needs a zero placeholder.

- [ ] **Step 3: Port `ViewerDependentValues.h`** selectively — it defines per-field viewer overrides (e.g.
  `UnitData::DisplayID` faction morph, `Field::Flags` visibility). Drop entries for omitted entities. Adapt
  `Trinity::` → `Acore::`, `sWorld->getBoolConfig` → `sWorld->getBoolConfig` (AC name; verify), DB2 store accessors
  (`sCreatureDisplayInfoStore` etc.) to AC's now-DB2 stores from 1c.2/1c.3.

- [ ] **Step 4: Fix includes** — `#include "UpdateFields.h"` + the entity headers it serializes against
  (`Player.h`, `Item.h`, `Bag.h`, `Unit.h`, `GameObject.h`, `DynamicObject.h`, `Corpse.h`, `ByteBuffer.h`,
  `ViewerDependentValues.h`). Drop `AreaTrigger.h`/`Corpse.h`-adjacent omitted includes.

- [ ] **Step 5: Commit** — "datastores: UF serializers + ViewerDependentValues for 54261 (selective) (1c.4-B)".

---

## Phase C — Rewire Object + entities (the cutover)

### Task C1: Rewire the `Object` base class

**Files:**
- Modify: `src/server/game/Entities/Object/Object.h`
- Modify: `src/server/game/Entities/Object/Object.cpp`
- Reference: xian55 `Object.h:75,111-144,177-181,267-268,284-395` + `Object.cpp:133-805`

- [ ] **Step 1: Replace the value-storage members** in `Object.h`. Remove the `union { int32* m_int32Values; ... }`,
  `UpdateMask _changesMask;`, `uint16 m_valuesCount;`, `uint16 _fieldNotifyFlags;`. Add:

```cpp
public:
    UF::UpdateFieldHolder m_values;
    UF::UpdateField<UF::ObjectData, 0, TYPEID_OBJECT> m_objectData;
protected:
    bool m_objectUpdated{ false };
```

- [ ] **Step 2: Add the helper templates** (verbatim from xian55 `Object.h:111-144` free functions in `namespace UF`,
  and the protected member templates `Object.h:284-360`: `SetUpdateFieldValue`, `SetUpdateFieldFlagValue`,
  `RemoveUpdateFieldValue`, `AddDynamicUpdateFieldValue`, `InsertDynamicUpdateFieldValue`,
  `RemoveDynamicUpdateFieldValue`, `ClearDynamicUpdateFieldValues`, `RemoveOptionalUpdateFieldValue`,
  `SetUpdateFieldStatValue`). These call `AddToObjectUpdateIfNeeded()`.

- [ ] **Step 3: Replace the public accessor API.** Delete the flat `Get/SetUInt32Value(uint16 index, …)` family and the
  `SetFlag(index,…)` family from `Object.h`/`Object.cpp`. (Call sites move to typed accessors in Task D.) Keep
  `BuildCreateUpdateBlockForPlayer`, add the 3.4.3 signatures:

```cpp
    virtual void BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const;
    void BuildValuesUpdateBlockForPlayer(UpdateData* data, Player const* target) const;
    void BuildValuesUpdateBlockForPlayerWithFlag(UpdateData* data, UF::UpdateFieldFlag flags, Player const* target) const;
    void BuildMovementUpdate(ByteBuffer* data, CreateObjectBits flags, Player* target) const;
    virtual UF::UpdateFieldFlag GetUpdateFieldFlagsFor(Player const* target) const;
    virtual void BuildValuesCreate(ByteBuffer* data, Player const* target) const = 0;
    virtual void BuildValuesUpdate(ByteBuffer* data, Player const* target) const = 0;
    virtual void BuildValuesUpdateWithFlag(ByteBuffer* data, UF::UpdateFieldFlag flags, Player const* target) const;
    virtual void ClearUpdateMask(bool remove);
```

- [ ] **Step 4: Rewrite `Object.cpp`** bodies from xian55: ctor `Object() : m_values(this)`; delete `_InitValues`,
  `_LoadIntoDataField`, `GetUpdateFieldData`, `_SetCreateBits`, `_SetUpdateBits`, and the flat `BuildValuesUpdate`.
  Port `BuildCreateUpdateBlockForPlayer` (uses `CreateObjectBits flags = m_updateFlag`), `BuildMovementUpdate`
  (CreateObjectBits form), `BuildValuesUpdateBlockForPlayer*`, `PrepareValuesUpdateBuffer`, `ClearUpdateMask`
  (`m_values.ClearChangesMask(&Object::m_objectData)`), `GetUpdateFieldFlagsFor` (returns `None`). Note: `CreateObjectBits`
  / `m_updateFlag` is the 3.4.3 movement-flag bitfield — port its struct from xian55 `Object.h` if AC's `m_updateFlag`
  is still the old `uint16`.

- [ ] **Step 5: Gate** — `Object.{h,cpp}` parse; build `game` to surface the first wave of entity errors (expected:
  every entity missing `BuildValuesCreate`/`BuildValuesUpdate`). Proceed to C3.

- [ ] **Step 6: Commit** — "object: rewire Object base to UF holder model (1c.4-C)".

### Task C2: Rewire `UpdateData.{h,cpp}` to the 3.4.3 packet shape

**Files:**
- Modify: `src/server/game/Entities/Object/Updates/UpdateData.{h,cpp}`
- Reference: xian55 `.../Updates/UpdateData.{h,cpp}`

- [ ] **Step 1: Port** the 3.4.3 class: ctor `UpdateData(uint32 map)`, `AddDestroyObject`, split
  `m_destroyGUIDs`/`m_outOfRangeGUIDs`, `AddUpdateBlock()` (count-only; the buffer is written via `GetBuffer()`),
  `ByteBuffer& GetBuffer()`, `BuildPacket(WorldPacket*)` writing `uint32 blockCount`, `uint16 map`, the destroy/oor bit
  + lists, then `uint32 dataSize` + `append(m_data)`.

- [ ] **Step 2: Fix `UpdateData` construction call sites.** The ctor now needs a map id. Grep
  `grep -rn "UpdateData " src/server/game | grep -v "//"` and pass `GetMapId()` (or `player->GetMapId()`). Track these
  for Task D; do the obvious ones here.

- [ ] **Step 3: Commit** — "object: UpdateData 54261 packet shape (1c.4-C)".

### Task C3: Per-entity holders, `BuildValuesCreate/Update`, ctor init

> Repeat this pattern for **each** entity in order: Item → Bag → Unit → Player → GameObject → DynamicObject → Corpse.
> Each is one task; do not batch. Reference the matching xian55 entity `.h`/`.cpp`.

**Files (per entity, e.g. Item):**
- Modify: `src/server/game/Entities/Item/Item.h` (add holder), `Item.cpp` (ctor init + Build bodies)
- Reference: xian55 `Item.h:360`, `Item.cpp` `BuildValuesCreate`/`BuildValuesUpdate`/`BuildValuesUpdateWithFlag`/
  `ClearUpdateMask`/`GetUpdateFieldFlagsFor`

- [ ] **Step 1: Add the holder member** to the entity header (exact decl from the explore report):
  - Item: `UF::UpdateField<UF::ItemData, 0, TYPEID_ITEM> m_itemData;`
  - Bag: `UF::UpdateField<UF::ContainerData, 0, TYPEID_CONTAINER> m_containerData;`
  - Unit: `UF::UpdateField<UF::UnitData, 0, TYPEID_UNIT> m_unitData;`
  - Player: `UF::UpdateField<UF::PlayerData, 0, TYPEID_PLAYER> m_playerData;` **and**
    `UF::UpdateField<UF::ActivePlayerData, 0, TYPEID_ACTIVE_PLAYER> m_activePlayerData;`
  - GameObject: `UF::UpdateField<UF::GameObjectData, 0, TYPEID_GAMEOBJECT> m_gameObjectData;`
  - DynamicObject: `UF::UpdateField<UF::DynamicObjectData, 0, TYPEID_DYNAMICOBJECT> m_dynamicObjectData;`
  - Corpse: `UF::UpdateField<UF::CorpseData, 0, TYPEID_CORPSE> m_corpseData;`

- [ ] **Step 2: Port `BuildValuesCreate`/`BuildValuesUpdate`/`BuildValuesUpdateWithFlag`/`ClearUpdateMask`** for that
  entity from xian55 verbatim. Shape (Item example):

```cpp
void Item::BuildValuesCreate(ByteBuffer* data, Player const* target) const
{
    UF::UpdateFieldFlag flags = GetUpdateFieldFlagsFor(target);
    std::size_t sizePos = data->wpos();
    *data << uint32(0);
    *data << uint8(flags);
    m_objectData->WriteCreate(*data, flags, this, target);
    m_itemData->WriteCreate(*data, flags, this, target);
    data->put<uint32>(sizePos, data->wpos() - sizePos - 4);
}
void Item::BuildValuesUpdate(ByteBuffer* data, Player const* target) const
{
    UF::UpdateFieldFlag flags = GetUpdateFieldFlagsFor(target);
    std::size_t sizePos = data->wpos();
    *data << uint32(0);
    *data << uint32(m_values.GetChangedObjectTypeMask());
    if (m_values.HasChanged(TYPEID_OBJECT))
        m_objectData->WriteUpdate(*data, flags, this, target);
    if (m_values.HasChanged(TYPEID_ITEM))
        m_itemData->WriteUpdate(*data, flags, this, target);
    data->put<uint32>(sizePos, data->wpos() - sizePos - 4);
}
```
  For Unit/Player also port `GetUpdateFieldFlagsFor` (owner/party/empath logic) and the `ClearUpdateMask` that clears
  every holder this entity owns.

- [ ] **Step 3: Initialize fixed-on-create fields** in the ctor / `Create()` via `SetUpdateFieldValue`. Port the
  initial sets xian55 does in `Item::Create`, `Player::Create`, `Unit` ctor, etc. (e.g. `SetEntry`, `SetObjectScale`,
  the type-specific defaults). The holder member itself needs no init-list entry — `UF::UpdateField<T>` value-inits.

- [ ] **Step 4: Gate** — build `game`; this entity's `BuildValues*` pure-virtuals are now satisfied. Iterate.

- [ ] **Step 5: Commit** — "object: <Entity> UF holder + build path (1c.4-C)".

---

## Phase D — Call-site sweep (~779 raw flat accessors, 173 files)

> The long tail. Most external code uses **typed wrappers** (`GetHealth()`, `SetLevel()`, `GetGUID()`), not raw
> `GetUInt32Value(INDEX)`. The wrappers move first (Task D1), which fixes the majority of call sites transitively; the
> residual raw sites are swept by domain (Task D2..).

### Task D1: Reimplement typed accessor wrappers over the holders

**Files:** `Unit.{h,cpp}`, `Player.{h,cpp}`, `Item.{h,cpp}`, `GameObject.{h,cpp}`, `Corpse.{h,cpp}`, `Object` (guid/scale/entry)
- Reference: the corresponding xian55 entity — every wrapper body is already converted there.

- [ ] **Step 1: For each typed getter/setter** (e.g. `Unit::GetLevel`, `SetHealth`, `GetFaction`, `Item::GetCount`),
  replace the body to read/write the UF field. Pattern:

```cpp
// getter
uint32 Unit::GetLevel() const { return m_unitData->Level; }
// setter
void Unit::SetLevel(uint8 lvl, bool /*showLevelChange*/)
{
    SetUpdateFieldValue(m_values.ModifyValue(&Unit::m_unitData).ModifyValue(&UF::UnitData::Level), lvl);
    // ... keep the surrounding non-field logic (group update flags, etc.)
}
```
  Copy the field mapping (which flat index → which `UF::*Data::Field`) from xian55's converted bodies — do not guess
  the field name.

- [ ] **Step 2: Build `game` after each entity's wrappers** to measure error reduction. Commit per entity:
  "object: port <Entity> typed accessors to UF fields (1c.4-D)".

### Task D2: Sweep residual raw flat-accessor sites by domain

**Files:** the ~173 files from `grep -rIl "GetUInt32Value\|SetUInt32Value\|GetFloatValue\|SetFloatValue\|GetByteValue\|
SetByteValue\|SetFlag(\|RemoveFlag(\|HasFlag(" src/server/game src/server/scripts`

- [ ] **Step 1: Bucket** the file list by directory (Entities, Spells, Handlers, scripts/<region>, …). Process one
  bucket per task to keep diffs reviewable.

- [ ] **Step 2: For each raw site,** map the flat index to its UF field and rewrite. Read transform:
  `obj->GetUInt32Value(UNIT_FIELD_FLAGS)` → `unit->m_unitData->Flags`. Write transform:
  `obj->SetUInt32Value(UNIT_FIELD_BYTES_1, v)` → `unit->SetUpdateFieldValue(unit->m_values.ModifyValue(&Unit::m_unitData)
  .ModifyValue(&UF::UnitData::...), v)` — usually via the typed wrapper from D1 when one exists. Prefer the wrapper.
  Use xian55's same call site as the authority for the target field.

- [ ] **Step 3: Flag/byte accessors** (`SetByteValue(UNIT_FIELD_BYTES_0, 0, race)`): in 3.4.3 these are discrete UF
  fields (`m_unitData->Race`, `->ClassId`, `->Sex`). Map each byte-offset to its named field per xian55.

- [ ] **Step 4: Build `game`, then the full solution (scripts).** Commit per bucket:
  "sweep: port <bucket> flat accessors to UF (1c.4-D)".

- [ ] **Step 5: Repeat** until `grep -rn "GetUInt32Value\|SetUInt32Value" src/server` returns only intentional
  non-object uses (if any). `log()`/note any site intentionally left (with reason).

---

## Phase E — Verification gate

### Task E: Build, link, and behavioral check

- [ ] **Step 1: Clean configure + full build** in the worktree:
  `cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSCRIPTS=static -DTOOLS_BUILD=none
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake` then
  `cmake --build build --target worldserver --config RelWithDebInfo`. Expected: **links clean**.

- [ ] **Step 2: Codestyle** — `python apps/codestyle/codestyle-cpp.py`. Fix violations.

- [ ] **Step 3: Boot** worldserver against the test DB. Expected: full "World Initialized", no assert in
  `Object::BuildValuesCreate`.

- [ ] **Step 4: Behavioral (THE gate).** Patched 3.4.3.54261 client logs in to a character. Capture the packet log;
  confirm a well-formed `SMSG_UPDATE_OBJECT` create block for the player (no client disconnect on parse). This resolves
  the parent plan's 1c.4 gate. If the client disconnects, bisect the create block against a HermesProxy capture of a
  real 3.4.3 session (per parent plan risk note).

- [ ] **Step 5: Checkpoint** — "world: UpdateFields + create block for 54261 (1c.4 complete)". Update the parent plan's
  progress log; merge the worktree, coordinating with the Phase-2 agent on `Object`/`Unit`/`Player`.

---

## Self-review notes

- **Spec coverage:** parent Task 1c.4 Steps 1–4 map to Phases A+B (Step 1 "replace UpdateFields.h"), C (Step 2 "adapt
  BuildValuesUpdate / mask writing"), E Step 4 (Step 3 "well-formed SMSG_UPDATE_OBJECT"), E Step 5 (Step 4 checkpoint).
- **Scope honesty:** the call-site sweep (D) is presented as bucketed iteration, not fabricated per-file code, because
  779 sites cannot be enumerated as literal steps; xian55's converted bodies are the per-site authority. This matches
  the parent plan's sanctioned reference-port convention.
- **Type consistency:** holder member names (`m_objectData`/`m_itemData`/`m_unitData`/`m_playerData`/
  `m_activePlayerData`/`m_gameObjectData`/`m_dynamicObjectData`/`m_corpseData`/`m_containerData`) and TypeID names are
  used identically across A1, C1, C3, D. `GetChangedObjectTypeMask`/`HasChanged`/`ClearChangesMask`/`ModifyValue`/
  `SetUpdateFieldValue` match the ported `UpdateField.h` API.
- **Known risk:** Phase B Step 2 (omitted-field byte-placeholder decisions) is the highest-uncertainty work — wire
  offset must match the client. Verify against a real capture in E Step 4; default to xian55's exact byte order when in
  doubt.
