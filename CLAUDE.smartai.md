# CLAUDE.smartai.md

SmartAI / `smart_scripts` reference for AzerothCore, derived from the engine source
(`src/server/game/AI/SmartScripts/`). Use this when a bugfix needs creature/gameobject
behaviour, waypoints, escorts, timed sequences, or gossip wired through data instead of C++.

Authoritative sources (read these if something here is ambiguous or you suspect drift):

- `SmartScriptMgr.h` — every event/action/target enum + the per-type parameter `struct`s (the real meaning of each `*_paramN`).
- `SmartScriptMgr.cpp` — `LoadFromDB`/`LoadSmartAIFromDB` (exact column order), `IsEventValid`/`IsTargetValid` (validation rules), waypoint loading.
- `SmartScript.cpp` — `ProcessAction()` (what each action actually does at runtime) and `ProcessEvent()`.
- `SmartAI.cpp` — movement/escort/waypoint runtime.

> **Prefer SmartAI over a new `CreatureScript`** for new creature/GO behaviour (per `CLAUDE.md`).
> Reach for C++ only when the event/action/target vocabulary below genuinely can't express the behaviour.

---

## 1. How SmartAI is enabled

A creature/gameobject only runs SmartAI when its template points at it:

```sql
UPDATE `creature_template`   SET `AIName` = 'SmartAI'         WHERE `entry` = <entry>;
UPDATE `gameobject_template` SET `AIName` = 'SmartGameObjectAI' WHERE `entry` = <entry>;
```

Then rows in `smart_scripts` drive it. Each **row = one event** (trigger → action → target).

---

## 2. The `smart_scripts` table

Column order **is** the load order in `SmartScriptMgr.cpp` (`LoadSmartAIFromDB`). Full INSERT column list:

```
entryorguid, source_type, id, link,
event_type, event_phase_mask, event_chance, event_flags,
  event_param1..event_param6,
action_type, action_param1..action_param6,
target_type, target_param1..target_param4,
target_x, target_y, target_z, target_o,
comment
```

| Column | Meaning |
|---|---|
| `entryorguid` | Positive = creature/GO **template entry** (applies to all spawns). Negative = a specific **spawn guid** (`-guid`, overrides the entry-level script for that one spawn). For `source_type = 9` it's a synthetic actionlist id (convention: `entry * 100 + n`). |
| `source_type` | What kind of object/script this is (see §3). |
| `id` | Row id within this `(entryorguid, source_type)`. Start at 0, increment. Also the execution order for actionlists. |
| `link` | If non-zero, the `id` of another row to fire **immediately and unconditionally** after this one (see §9). |
| `event_type` | When it fires (§5). |
| `event_phase_mask` | Bitmask of phases this event is allowed in; `0` = all phases (§7). |
| `event_chance` | 0–100% roll to fire. Use `100` for deterministic. |
| `event_flags` | Bitmask (§8): not-repeatable, difficulty gating, etc. |
| `event_param1..6` | Event parameters — meaning depends on `event_type` (§5). |
| `action_type` | What to do (§6). |
| `action_param1..6` | Action parameters — meaning depends on `action_type` (§6). |
| `target_type` | Who/what the action applies to (§4). |
| `target_param1..4` | Target parameters — meaning depends on `target_type` (§4). |
| `target_x/y/z/o` | World coords/orientation, used by position-based targets/actions (e.g. `SMART_TARGET_POSITION`, `MOVE_TO_POS`). |
| `comment` | Human-readable. Convention: `"<Creature name> - <event> - <action>"`. |

All `paramN` columns are stored as one shared `raw` union and reinterpreted per type — so an
unused param **must be 0**. The validator (`CheckUnusedEventParams` etc.) warns on non-zero junk.

---

## 3. `source_type` (SmartScriptType)

| Value | Type | Notes |
|---|---|---|
| 0 | `CREATURE` | Most common. `entryorguid` = creature entry or `-guid`. |
| 1 | `GAMEOBJECT` | Requires `AIName='SmartGameObjectAI'`. |
| 2 | `AREATRIGGER` | `entryorguid` = areatrigger id. |
| 3 | `EVENT` | |
| 4 | `GOSSIP` | |
| 5 | `QUEST` | |
| 6 | `SPELL` | |
| 7 | `TRANSPORT` | |
| 8 | `INSTANCE` | |
| 9 | `TIMED_ACTIONLIST` | A sequenced list invoked by action 80/87/88 (§10). Not attached to an object directly. |

Each event type is only valid on certain source types — see `SmartAIEventMask` in `SmartScriptMgr.h`
if an event mysteriously refuses to load.

---

## 4. Targets (`target_type`, `SMARTAI_TARGETS`)

The action's targets. `target_param1..4` meaning is per-type (from the `SmartTarget` union in `SmartScriptMgr.h`).

| ID | Name | params (p1..p4) |
|---|---|---|
| 0 | `NONE` | — (often "self" for self-affecting actions) |
| 1 | `SELF` | — |
| 2 | `VICTIM` | current top-threat target |
| 3 | `HOSTILE_SECOND_AGGRO` | maxDist, playerOnly, powerType+1, aura |
| 4 | `HOSTILE_LAST_AGGRO` | maxDist, playerOnly, powerType+1, aura |
| 5 | `HOSTILE_RANDOM` | maxDist, playerOnly, powerType+1, aura |
| 6 | `HOSTILE_RANDOM_NOT_TOP` | maxDist, playerOnly, powerType+1, aura |
| 7 | `ACTION_INVOKER` | the unit that triggered the event |
| 8 | `POSITION` | uses `target_x/y/z/o` |
| 9 | `CREATURE_RANGE` | entry(0=any), minDist, maxDist, alive(0 both/1 alive/2 dead) |
| 10 | `CREATURE_GUID` | guid, entry |
| 11 | `CREATURE_DISTANCE` | entry(0=any), maxDist, alive |
| 12 | `STORED` | id (a list saved earlier via action 64 `STORE_TARGET_LIST`) |
| 13 | `GAMEOBJECT_RANGE` | entry(0=any), minDist, maxDist |
| 14 | `GAMEOBJECT_GUID` | guid, entry |
| 15 | `GAMEOBJECT_DISTANCE` | entry(0=any), maxDist |
| 16 | `INVOKER_PARTY` | includePets(0/1) |
| 17 | `PLAYER_RANGE` | minDist, maxDist, maxCount |
| 18 | `PLAYER_DISTANCE` | maxDist |
| 19 | `CLOSEST_CREATURE` | entry(0=any), maxDist, dead? |
| 20 | `CLOSEST_GAMEOBJECT` | entry(0=any), maxDist |
| 21 | `CLOSEST_PLAYER` | maxDist |
| 22 | `ACTION_INVOKER_VEHICLE` | the invoker's vehicle |
| 23 | `OWNER_OR_SUMMONER` | useCharmerOrOwner |
| 24 | `THREAT_LIST` | maxDist (0=all), playerOnly |
| 25 | `CLOSEST_ENEMY` | maxDist, playerOnly |
| 26 | `CLOSEST_FRIENDLY` | maxDist, playerOnly |
| 27 | `LOOT_RECIPIENTS` | players who tagged this creature |
| 28 | `FARTHEST` | maxDist, playerOnly, isInLos, minDist |
| 29 | `VEHICLE_PASSENGER` | seatMask |
| **AC-only** | | |
| 201 | `PLAYER_WITH_AURA` | spellId, negation, maxDist, minDist |
| 202 | `RANDOM_POINT` | range, amount, self-as-middle(0/1 else xyz) |
| 203 | `ROLE_SELECTION` | maxDist, roleMask (tank1/heal2/dmg4), resize |
| 204 | `SUMMONED_CREATURES` | entry |
| 205 | `INSTANCE_STORAGE` | instance data index, type (creature1/go2) |
| 206 | `FORMATION` | type (0 members/1 leader/2 all), entry(0=any), excludeSelf |

---

## 5. Events (`event_type`, `SMART_EVENT`)

Most numeric/timed events take `(param3, param4)` as `repeatMin, repeatMax` (0,0 = fire once / no repeat).
Cooldown-style events use `(cooldownMin, cooldownMax)`. Comments below are the param layout from `SmartScriptMgr.h`.

| ID | Name | params |
|---|---|---|
| 0 | `UPDATE_IC` | InitialMin, InitialMax, RepeatMin, RepeatMax — **in combat** timer |
| 1 | `UPDATE_OOC` | InitialMin, InitialMax, RepeatMin, RepeatMax — **out of combat** timer |
| 2 | `HEALTH_PCT` | HPMin%, HPMax%, RepeatMin, RepeatMax |
| 3 | `MANA_PCT` | ManaMin%, ManaMax%, RepeatMin, RepeatMax |
| 4 | `AGGRO` | — (on entering combat) |
| 5 | `KILL` | CooldownMin, CooldownMax, playerOnly, creatureEntry |
| 6 | `DEATH` | — (on just died) |
| 7 | `EVADE` | — |
| 8 | `SPELLHIT` | spellId, school, CooldownMin, CooldownMax |
| 9 | `RANGE` | min, max, repeatMin, repeatMax (distance to victim) |
| 10 | `OOC_LOS` | hostilityMode(0 hostile/1 friendly/2 any), maxRange, CooldownMin, CooldownMax, playerOnly |
| 11 | `RESPAWN` | type (0 none/1 map/2 area), mapId, zoneId |
| 12 | `TARGET_HEALTH_PCT` | HPMin%, HPMax%, RepeatMin, RepeatMax |
| 13 | `VICTIM_CASTING` | RepeatMin, RepeatMax, spellId |
| 14 | `FRIENDLY_HEALTH` | hpDeficit, radius, RepeatMin, RepeatMax |
| 15 | `FRIENDLY_IS_CC` | radius, RepeatMin, RepeatMax |
| 16 | `FRIENDLY_MISSING_BUFF` | spellId, radius, RepeatMin, RepeatMax, onlyInCombat |
| 17 | `SUMMONED_UNIT` | creatureId(0=all), CooldownMin, CooldownMax |
| 18 | `TARGET_MANA_PCT` | ManaMin%, ManaMax%, RepeatMin, RepeatMax |
| 19 | `ACCEPTED_QUEST` | questId(0=any), CooldownMin, CooldownMax |
| 20 | `REWARD_QUEST` | questId(0=any), CooldownMin, CooldownMax |
| 21 | `REACHED_HOME` | — |
| 22 | `RECEIVE_EMOTE` | emoteId, CooldownMin, CooldownMax, condition, v1, v2, v3 |
| 23 | `HAS_AURA` | spellId, stackAmount, RepeatMin, RepeatMax |
| 24 | `TARGET_BUFFED` | spellId, stackAmount, RepeatMin, RepeatMax |
| 25 | `RESET` | — (after combat / on respawn+spawn) |
| 26 | `IC_LOS` | hostilityMode, maxRange, CooldownMin, CooldownMax, playerOnly |
| 27 | `PASSENGER_BOARDED` | CooldownMin, CooldownMax |
| 28 | `PASSENGER_REMOVED` | CooldownMin, CooldownMax |
| 29 | `CHARMED` | onRemove (0 apply/1 remove) |
| 30 | `CHARMED_TARGET` | — |
| 31 | `SPELLHIT_TARGET` | spellId, school, CooldownMin, CooldownMax |
| 32 | `DAMAGED` | minDmg, maxDmg, CooldownMin, CooldownMax |
| 33 | `DAMAGED_TARGET` | minDmg, maxDmg, CooldownMin, CooldownMax |
| 34 | `MOVEMENTINFORM` | movementType, pointId, pathId(0=any) |
| 35 | `SUMMON_DESPAWNED` | entry, CooldownMin, CooldownMax |
| 36 | `CORPSE_REMOVED` | — |
| 37 | `AI_INIT` | — |
| 38 | `DATA_SET` | id, value, CooldownMin, CooldownMax |
| 39 | `ESCORT_START` | — |
| 40 | `ESCORT_REACHED` | pointId(0=any), pathId(0=any) |
| 41–44 | `TRANSPORT_*` | transport hooks |
| 45 | `INSTANCE_PLAYER_ENTER` | team(0=any), CooldownMin, CooldownMax |
| 46 | `AREATRIGGER_ONTRIGGER` | triggerId(0=any) |
| 47–51 | `QUEST_*` | quest source_type hooks |
| 52 | `TEXT_OVER` | groupId (creature_text), creatureEntry(0=any) — fires when a `TALK` finishes |
| 53 | `RECEIVE_HEAL` | minHeal, maxHeal, CooldownMin, CooldownMax |
| 54 | `JUST_SUMMONED` | — |
| 55–58 | `ESCORT_PAUSED/RESUMED/STOPPED/ENDED` | pointId, pathId |
| 59 | `TIMED_EVENT_TRIGGERED` | id (from action 67/73) |
| 60 | `UPDATE` | InitialMin, InitialMax, RepeatMin, RepeatMax (IC or OOC) |
| 61 | `LINK` | — internal; the row pointed at by another row's `link`. **Never fires on its own.** |
| 62 | `GOSSIP_SELECT` | menuId, actionId |
| 63 | `JUST_CREATED` | — |
| 64 | `GOSSIP_HELLO` | filter (0 any/1 gossipHello only/2 reportUse only) |
| 65 | `FOLLOW_COMPLETED` | — |
| 66 | `EVENT_PHASE_CHANGE` | phase mask |
| 67 | `IS_BEHIND_TARGET` | min, max, repeatMin, repeatMax |
| 68 | `GAME_EVENT_START` | game_event id |
| 69 | `GAME_EVENT_END` | game_event id |
| 70 | `GO_STATE_CHANGED` | go state |
| 71 | `GO_EVENT_INFORM` | eventId |
| 72 | `ACTION_DONE` | eventId |
| 73 | `ON_SPELLCLICK` | — (clicker = invoker) |
| 74 | `FRIENDLY_HEALTH_PCT` | min, max, repeatMin, repeatMax, hpPct, radius |
| 75 | `DISTANCE_CREATURE` | guid, entry, distance, repeat |
| 76 | `DISTANCE_GAMEOBJECT` | guid, entry, distance, repeat |
| 77 | `COUNTER_SET` | id, value, CooldownMin, CooldownMax |
| 82 | `SUMMONED_UNIT_DIES` | creatureId(0=all), CooldownMin, CooldownMax |
| **AC-only** | | |
| 101 | `NEAR_PLAYERS` | minCount, radius, firstTimer, repeatMin, repeatMax |
| 102 | `NEAR_PLAYERS_NEGATION` | maxCount, radius, firstTimer, repeatMin, repeatMax |
| 103 | `NEAR_UNIT` | type(0 creature/1 go), entry, count, range, timer |
| 104 | `NEAR_UNIT_NEGATION` | type, entry, count, range, timer |
| 105 | `AREA_CASTING` | min, max, repeatMin, repeatMax, rangeMin, rangeMax |
| 106 | `AREA_RANGE` | min, max, repeatMin, repeatMax, rangeMin, rangeMax |
| 107 | `SUMMONED_UNIT_EVADE` | creatureId(0=all), CooldownMin, CooldownMax |
| 108 | `WAYPOINT_REACHED` | pointId(0=any), pathId(0=any) — **AC waypoint reached** |
| 109 | `WAYPOINT_ENDED` | pointId(0=any), pathId(0=any) |
| 110 | `IS_IN_MELEE_RANGE` | min, max, repeatMin, repeatMax, dist, invert(0/1) |

> **Note:** events 78–81 (`SCENE_*`) exist but **do not work on 3.3.5a** — don't use them.

---

## 6. Actions (`action_type`, `SMART_ACTION`)

Param meaning is from the `SmartAction` union in `SmartScriptMgr.h`; runtime behaviour is in
`SmartScript.cpp::ProcessAction`. Most-used ones first, then the full table.

**Common:**

| ID | Name | params |
|---|---|---|
| 1 | `TALK` | groupId (creature_text), duration→`TEXT_OVER`, useTalkTarget, delay |
| 11 | `CAST` | spellId, castFlags (§ cast flags), triggerFlags, targetsLimit |
| 12 | `SUMMON_CREATURE` | creatureId, summonType, duration(ms), attackInvoker, attackScriptOwner, flags |
| 22 | `SET_EVENT_PHASE` | phase |
| 23 | `INC_EVENT_PHASE` | inc, dec |
| 41 | `FORCE_DESPAWN` | delay(ms), forceRespawnTimer, removeFromWorld |
| 45 | `SET_DATA` | field, data (drives `DATA_SET` events on the target) |
| 53 | `ESCORT_START` | run/walk, pathId, canRepeat, quest, despawnTime, reactState |
| 69 | `MOVE_TO_POS` | pointId (uses x/y/z), transport, controlled, contactDistance |
| 80 | `CALL_TIMED_ACTIONLIST` | actionlistId, timerType(0 OOC/1 IC/2 always), allowOverride — **runs a `source_type=9` list** (§10) |
| 232 | `WAYPOINT_START` | pathId, repeat, pathSource (0 `waypoint_data`/1 `waypoints` table) (§11) |

**Full list (abbreviated params; consult the union for exact fields):**

| ID | Name | ID | Name |
|---|---|---|---|
| 0 | NONE | 1 | TALK |
| 2 | SET_FACTION | 3 | MORPH_TO_ENTRY_OR_MODEL (entry, model; 0/0 demorph) |
| 4 | SOUND (soundId, onlySelf, dist) | 5 | PLAY_EMOTE (emoteId) |
| 6 | FAIL_QUEST | 7 | OFFER_QUEST (questId, directAdd) |
| 8 | SET_REACT_STATE (0 passive/1 defensive/2 aggressive) | 9 | ACTIVATE_GOBJECT |
| 10 | RANDOM_EMOTE | 11 | CAST |
| 12 | SUMMON_CREATURE | 13 | THREAT_SINGLE_PCT |
| 14 | THREAT_ALL_PCT | 15 | CALL_AREAEXPLOREDOREVENTHAPPENS (questId) |
| 17 | SET_EMOTE_STATE (emoteId) | 18 | SET_UNIT_FLAG (flags, type) |
| 19 | REMOVE_UNIT_FLAG | 20 | AUTO_ATTACK (0 stop/1 continue) |
| 21 | ALLOW_COMBAT_MOVEMENT (0/1) | 22 | SET_EVENT_PHASE |
| 23 | INC_EVENT_PHASE | 24 | EVADE |
| 25 | FLEE_FOR_ASSIST | 26 | CALL_GROUPEVENTHAPPENS (questId) |
| 27 | COMBAT_STOP | 28 | REMOVEAURASFROMSPELL (spellId 0=all, charges) |
| 29 | FOLLOW (dist, angle, entry, credit, creditType, aliveState) | 30 | RANDOM_PHASE |
| 31 | RANDOM_PHASE_RANGE (min,max) | 32 | RESET_GOBJECT |
| 33 | CALL_KILLEDMONSTER (creatureId) | 34 | SET_INST_DATA (field, data, type) |
| 35 | SET_INST_DATA64 (field) | 36 | UPDATE_TEMPLATE (entry, updateLevel) |
| 37 | DIE (ms) | 38 | SET_IN_COMBAT_WITH_ZONE (range) |
| 39 | CALL_FOR_HELP (radius, withEmote) | 40 | SET_SHEATH (0 unarmed/1 melee/2 ranged) |
| 41 | FORCE_DESPAWN | 42 | SET_INVINCIBILITY_HP_LEVEL (minHP, percent) |
| 43 | MOUNT_TO_ENTRY_OR_MODEL (entry, model; 0/0 dismount) | 44 | SET_INGAME_PHASE_MASK |
| 45 | SET_DATA | 46 | MOVE_FORWARD (dist) |
| 47 | SET_VISIBILITY (0/1) | 48 | SET_ACTIVE (0/1) |
| 49 | ATTACK_START | 50 | SUMMON_GO (goId, despawnTime, targetSummon, summonType) |
| 51 | KILL_UNIT | 52 | ACTIVATE_TAXI (taxiId) |
| 53 | ESCORT_START | 54 | ESCORT_PAUSE (time) |
| 55 | ESCORT_STOP | 56 | ADD_ITEM (itemId, count) |
| 57 | REMOVE_ITEM (itemId, count) | 58 | INSTALL_AI_TEMPLATE |
| 59 | SET_RUN (0/1) | 60 | SET_FLY (0/1, speed, disableGravity) |
| 61 | SET_SWIM (0/1) | 62 | TELEPORT (mapId, uses x/y/z/o) |
| 63 | SET_COUNTER (id, value, reset, subtract) | 64 | STORE_TARGET_LIST (varId) |
| 65 | ESCORT_RESUME | 66 | SET_ORIENTATION (quickChange, random, turnAngle) |
| 67 | CREATE_TIMED_EVENT (id, initMin, initMax, repMin, repMax, chance) | 68 | PLAYMOVIE (entry) |
| 69 | MOVE_TO_POS | 70 | RESPAWN_TARGET (goRespawnTime) |
| 71 | EQUIP (entry, mask, slot1-3) | 72 | CLOSE_GOSSIP |
| 73 | TRIGGER_TIMED_EVENT (id) | 74 | REMOVE_TIMED_EVENT (id) |
| 75 | ADD_AURA (spellId) | 76 | OVERRIDE_SCRIPT_BASE_OBJECT (⚠ can crash) |
| 77 | RESET_SCRIPT_BASE_OBJECT | 78 | CALL_SCRIPT_RESET |
| 79 | SET_RANGED_MOVEMENT (dist, angle) | 80 | CALL_TIMED_ACTIONLIST |
| 81 | SET_NPC_FLAG | 82 | ADD_NPC_FLAG |
| 83 | REMOVE_NPC_FLAG | 84 | SIMPLE_TALK (groupId — target says it, no TEXT_OVER) |
| 85 | SELF_CAST (spellId, castFlags, triggerFlags, targetsLimit) | 86 | CROSS_CAST |
| 87 | CALL_RANDOM_TIMED_ACTIONLIST (ids 1-6) | 88 | CALL_RANDOM_RANGE_TIMED_ACTIONLIST (min, max) |
| 89 | RANDOM_MOVE (maxDist) | 90 | SET_UNIT_FIELD_BYTES_1 (bytes, target) |
| 91 | REMOVE_UNIT_FIELD_BYTES_1 (bytes, target) | 92 | INTERRUPT_SPELL |
| 93 | SEND_GO_CUSTOM_ANIM (animId) | 94 | SET_DYNAMIC_FLAG |
| 95 | ADD_DYNAMIC_FLAG | 96 | REMOVE_DYNAMIC_FLAG |
| 97 | JUMP_TO_POS (speedXY, speedZ, selfJump) | 98 | SEND_GOSSIP_MENU (menuId, npcTextId) |
| 99 | GO_SET_LOOT_STATE (state) | 100 | SEND_TARGET_TO_TARGET (id) |
| 101 | SET_HOME_POS (uses x/y/z/o or target) | 102 | SET_HEALTH_REGEN (0/1) |
| 103 | SET_ROOT (0/1) | 104 | SET_GO_FLAG |
| 105 | ADD_GO_FLAG | 106 | REMOVE_GO_FLAG |
| 107 | SUMMON_CREATURE_GROUP (group, attackInvoker, attackScriptOwner) | 108 | SET_POWER (powerType, newPower) |
| 109 | ADD_POWER | 110 | REMOVE_POWER |
| 111 | GAME_EVENT_STOP (id) | 112 | GAME_EVENT_START (id) |
| 113 | START_CLOSEST_WAYPOINT (wp1..wp7) | 114 | RISE_UP (dist) |
| 115 | RANDOM_SOUND | 116 | SET_CORPSE_DELAY (timer) |
| 117 | DISABLE_EVADE (1 disabled/0 enabled) | 118 | GO_SET_GO_STATE (state) |
| 121 | SET_SIGHT_DIST | 122 | FLEE (fleeTime) |
| 123 | ADD_THREAT (+threat, -threat) | 124 | LOAD_EQUIPMENT (id) |
| 125 | TRIGGER_RANDOM_TIMED_EVENT (idMin, idMax) | 126 | REMOVE_ALL_GAMEOBJECTS |
| 131 | SPAWN_SPAWNGROUP (groupId, ignoreRespawn, force) | 132 | DESPAWN_SPAWNGROUP (groupId, deleteRespawnTimes) |
| 134 | INVOKER_CAST (spellId, castFlags, triggerFlags, targetsLimit) | 135 | PLAY_CINEMATIC (entry) |
| 136 | SET_MOVEMENT_SPEED (type, speedInt, speedFrac) | 142 | SET_HEALTH_PCT (percent) |
| **AC-only (200+)** | | | |
| 201 | MOVE_TO_POS_TARGET (pointId) | 203 | EXIT_VEHICLE |
| 204 | SET_UNIT_MOVEMENT_FLAGS | 205 | SET_COMBAT_DISTANCE (dist) |
| 206 | DISMOUNT | 207 | SET_HOVER (0/1) |
| 208 | ADD_IMMUNITY (type, id, value) | 209 | REMOVE_IMMUNITY (type, id, value) |
| 210 | FALL | 211 | SET_EVENT_FLAG_RESET (0/1) |
| 212 | STOP_MOTION (stopMoving, movementExpired) | 213 | NO_ENVIRONMENT_UPDATE |
| 214 | ZONE_UNDER_ATTACK | 215 | LOAD_GRID |
| 216 | MUSIC (soundId, onlySelf, type) | 217 | RANDOM_MUSIC |
| 218 | CUSTOM_CAST (spellId, castFlags, bp0, bp1, bp2) | 219 | CONE_SUMMON |
| 220 | PLAYER_TALK (acore_string entry, yell 0/1) | 221 | VORTEX_SUMMON |
| 222 | CU_ENCOUNTER_START (reset cds + remove Heroism debuff) | 223 | DO_ACTION (actionId) |
| 224 | ATTACK_STOP | 225 | SET_GUID |
| 226 | SCRIPTED_SPAWN | 227 | SET_SCALE (scale) |
| 228 | SUMMON_RADIAL | 229 | PLAY_SPELL_VISUAL |
| 230 | FOLLOW_GROUP (state, type, dist) | 231 | SET_ORIENTATION_TARGET |
| 232 | WAYPOINT_START (pathId, repeat, pathSource) | 233 | WAYPOINT_DATA_RANDOM (pathId1, pathId2, repeat) |
| 234 | MOVEMENT_STOP | 235 | MOVEMENT_PAUSE (timer) |
| 236 | MOVEMENT_RESUME (timerOverride) | 237 | WORLD_SCRIPT (eventId, param) |
| 238 | DISABLE_REWARD (reputation 0/1, loot 0/1) | 239 | SET_ANIM_TIER |
| 240 | SET_GOSSIP_MENU (gossipMenuId) | 241 | SUMMON_GAMEOBJECT_GROUP (group) |
| 242 | INC_DATA (field, increment — wipe-safe across evade) | | |

> Actions marked `NOT SUPPORTED YET` in the header (119/120/127/133) and 3.3.5a-invalid ones
> (128–130 `ANIMKIT`/`SCENE`) should not be used.

---

## 7. Phases (`event_phase_mask`)

Phases let one creature run different event sets at different fight stages. A row only fires if
its `event_phase_mask` includes the current phase. `0` = always (all phases).

Set the current phase with action 22 `SET_EVENT_PHASE` (phase 1–12) or move it with 23 `INC_EVENT_PHASE`.
`event_phase_mask` is a **bitmask**, so phase 1 = `1`, phase 2 = `2`, phase 3 = `4`, phase 4 = `8`, …
(`SMART_EVENT_PHASE_*_BIT`). To match phases 1 **and** 2, use `3`.

---

## 8. `event_flags` (SmartEventFlags)

| Bit | Name | Meaning |
|---|---|---|
| 0x001 | `NOT_REPEATABLE` | Fires at most once |
| 0x002 | `DIFFICULTY_0` | 5-man normal / 10-man normal only |
| 0x004 | `DIFFICULTY_1` | 5-man heroic / 25-man normal only |
| 0x008 | `DIFFICULTY_2` | 10-man heroic only |
| 0x010 | `DIFFICULTY_3` | 25-man heroic only |
| 0x080 | `DEBUG_ONLY` | Debug builds only |
| 0x100 | `DONT_RESET` | Not reset on `OnReset()` |
| 0x200 | `WHILE_CHARMED` | Still fires while AI owner is charmed |

Combine difficulty bits to gate an event to specific modes (e.g. `0x018` = both heroic modes).

---

## 9. Linking events (`link`)

Set row A's `link` to the `id` of row B. When A fires, B runs **immediately**, with no event roll of
its own. B's `event_type` must be `61` (`SMART_EVENT_LINK`). Use this to attach several actions to one
trigger (one event → many actions), or to give the linked action a **different target** than the first.

```
(N, 0, 0, 1, 4,  0,100,0, ... , <action A>, ... 'Aggro - do A'),   -- id 0, link→1, on AGGRO
(N, 0, 1, 0, 61, 0,100,0, ... , <action B>, ... 'Link - do B'),    -- id 1, LINK
```

---

## 10. Timed action lists (`source_type = 9`)

A timed actionlist is an ordered, self-timed sequence of actions. It is **not** attached to an object;
it is invoked from a normal script row via:

- action 80 `CALL_TIMED_ACTIONLIST` (target = who runs it),
- action 87 `CALL_RANDOM_TIMED_ACTIONLIST`, or
- action 88 `CALL_RANDOM_RANGE_TIMED_ACTIONLIST`.

Conventions:

- `entryorguid` of the list = a synthetic id, by convention `creatureEntry * 100 + n` (so they don't collide).
- Rows are executed in `id` order. For each row, `event_param1/param2` are the **wait (min/max ms) before that row runs**, measured from the previous row — i.e. it's a delay cascade, not absolute timestamps.
- `event_type` is ignored/0 for list rows; the `action_*`/`target_*` columns work as usual.
- The invoker passed to action 80 becomes the list's `GetLastInvoker()`, so `ACTION_INVOKER` targets inside the list resolve to whoever triggered the outer event.

This is the idiomatic way to do "do X, wait 2s, say Y, wait 1s, morph, then walk off" without
chaining a dozen timed events.

---

## 11. Waypoints

Two waypoint systems exist; **pick the right table and the matching `pathSource`**.

| System | Table | `pathSource` | Reached/Ended events | Started by |
|---|---|---|---|---|
| Core waypoint mgr | `waypoint_data` | 0 (`WAYPOINT_MGR`) | MOVEMENTINFORM (34) | `MovePath` / formations |
| **SmartAI waypoints** | `waypoints` | 1 (`SMART_WAYPOINT_MGR`) | `WAYPOINT_REACHED` (108), `WAYPOINT_ENDED` (109) | action 232 `WAYPOINT_START` |
| Escort | `waypoints` (escort path) | — | `ESCORT_*` (39/40/55-58) | action 53 `ESCORT_START` |

The `waypoints` table (loaded by `SmartWaypointMgr`):

```sql
DELETE FROM `waypoints` WHERE `entry` = <pathId>;
INSERT INTO `waypoints`
  (`entry`, `pointid`, `position_x`, `position_y`, `position_z`, `orientation`, `delay`, `point_comment`)
VALUES
  (<pathId>, 1, x, y, z, 0, 0, 'comment'),
  (<pathId>, 2, x, y, z, 0, 0, 'comment');
```

- `entry` here is the **path id**, not a creature entry. Reference it from action 232 param1.
- `pointid` is 1-based and sequential.
- `orientation` 0 = face along movement; `delay` (ms) = pause at that point.

To start a non-escort SmartAI path and react to its end:

```
-- action 232: pathId=<path>, repeat=0, pathSource=1 (waypoints table)
(<who>, 9, k, 0, 0,0,100,0, 0,0,0,0,0,0, 232, <path>, 0, 1, 0,0,0, 1, 0,0,0,0, 0,0,0,0, 'Start Waypoint <path>'),
-- event 108: WAYPOINT_REACHED pointId=<lastPoint>, pathId=<path> → e.g. despawn
(-guid, 0, m, 0, 108,0,100,0, <lastPoint>, <path>, 0,0,0,0, 41, 0,0,0,0,0,0, 1, 0,0,0,0, 0,0,0,0, 'On WP reached - Despawn'),
```

> If a creature should walk rather than run, set `SET_RUN`(59)=0 before starting the path.

---

## 12. Talk / text

Action 1 `TALK` and 84 `SIMPLE_TALK` reference a `groupid` in the **`creature_text`** table
(not raw strings). `creature_text` carries the text id, type (say/yell/whisper/emote), language,
emote, sound, and broadcast text id. Action 1 additionally fires a `TEXT_OVER` (event 52) after
`duration` ms, which is how you chain "finish talking → next step". `SIMPLE_TALK` does **not** fire
`TEXT_OVER`. Add `creature_text` rows in the same SQL update when introducing new lines.

---

## 13. Worked example (this branch — Ursal the Mauler quest fix)

The pending update `rev_1781977612345678900.sql` is a clean end-to-end example combining most of the
above: an `aiDataSet` broadcast, per-guid scripts, timed actionlists, morph, and a SmartAI flee path.

Flow:

1. **Ursal (2039)** — `event 6 DEATH` → `action 45 SET_DATA(2,2)` targeting nearby Enslaved Druids (`target 11 CREATURE_DISTANCE`, entry 2852, 100yd). This broadcasts a data-set the druids listen for.
2. **Each druid (`-guid`)** — `event 38 DATA_SET(id=2,value=2)` → `action 80 CALL_TIMED_ACTIONLIST` invoking a per-druid list (`285200/285201/285202`), self target.
3. **Timed lists (`source_type 9`)** — wake (remove sleep standstate), morph to Freed Druid (2853), and `action 232 WAYPOINT_START` on path `22817` (`pathSource=1`, the `waypoints` table). The speaker's list (`285202`) also `TALK`s the free line. Per-row `event_param1/2` give the staggered delays (2s/3s/4s/5s) so they don't snap away in unison.
4. **Each druid** — `event 108 WAYPOINT_REACHED(point 3, path 22817)` → `action 41 FORCE_DESPAWN`.

Reading that file alongside this doc is the fastest way to internalise the column layout.

---

## 14. SQL conventions (enforced — see `CLAUDE.md`)

- Author only in `data/sql/updates/pending_db_world/`; generate the file with `./create_sql.sh`.
- Every `INSERT` needs a preceding matching `DELETE` for idempotency. For `smart_scripts`, delete by
  `source_type` + `entryorguid`; for `waypoints`, delete by `entry` (path id).
- 4-space indent, no tabs, trailing newline, no double `;;`, no multiple blank lines, InnoDB.
- Run `python apps/codestyle/codestyle-sql.py` before claiming done.
- After writing scripts, the server validates them on load (`IsEventValid`/`IsTargetValid`/`CheckUnused*Params`)
  and logs `sql.sql` errors for bad entries, non-existent spell/creature/quest references, min>max, or
  non-zero unused params. A correct script loads silently.
