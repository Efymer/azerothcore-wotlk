# CLAUDE.bugfixing.md

Annex to `CLAUDE.md`. Workflow and reference sources for **fixing bugs in AzerothCore** by cross-referencing other 3.3.5a (WotLK) emulators and authoritative game data.

## Goal

When fixing a bug in AzerothCore (AC), use other emulators and Wowhead as reference points. AC is a fork of TrinityCore (TC); TC and (c)MaNGOS are often more polished/complete on specific mechanics. Comparing their source and DB content helps decide what "correct" behaviour is and how to implement the fix in AC's idioms. Compare always with TC and (c)MaNGOS when investigating issues.

**Important:** reference sources are for *understanding and comparison only*. Never blindly copy — port to AC conventions (see `CLAUDE.md` → namespace `Acore::`, typed helpers, logging, SQL update workflow). AC and TC diverge in APIs, table columns, and namespaces.

## Reference sources

### 1. TrinityCore (installed now)

- **Source:** `D:\Games\TrinityCore\src` (full clone, master branch).
  - Note: this is *outside* AC's configured working directories. Reads work; treat as read-only reference. Never edit TC source as part of an AC fix.
  - Layout mirrors AC: `src/server/game/`, `src/server/scripts/`, `src/server/database/`.
- **SQL files:** `D:\Games\TrinityCore\sql` (`base/`, `create/`, `updates/{auth,characters,world}`, `custom/`, `old/`).

### 2. Live MySQL databases (localhost)

Server: **MySQL 8.4** at `127.0.0.1:3306`, user `root`, password `1234`.
Client: `C:\Program Files\MySQL\MySQL Server 8.4\bin\mysql.exe` (on PATH as `mysql`).

| Emulator | auth | characters | world |
|---|---|---|---|
| AzerothCore | `acore_auth` | `acore_characters` | `acore_world` |
| TrinityCore | `auth` | `characters` | `world` |
| cMaNGOS WoTLK | *(n/a — not loaded)* | *(n/a — not loaded)* | `cmangos_world` |

> cMaNGOS is loaded as a **world-DB-only** reference (realmd/characters are runtime schemas, useless for content comparison). See section 4 for layout and schema caveats.

Example (read-only comparison):
```bash
mysql -h 127.0.0.1 -P 3306 -u root -p1234 -e \
  "SELECT * FROM acore_world.creature_template WHERE entry=12345 \G"
mysql -h 127.0.0.1 -P 3306 -u root -p1234 -e \
  "SELECT * FROM world.creature_template WHERE entry=12345 \G"
```

**DB safety rules:**
- Read queries (`SELECT`, `SHOW`, `DESCRIBE`) — run freely.
- Destructive statements (`INSERT`/`UPDATE`/`DELETE`/`DROP`/`ALTER`) — confirm with the user first.
- Never mutate the TrinityCore DBs (`world`/`characters`/`auth`) — they are reference only.
- DB schemas differ between AC and TC; compare columns before assuming a value maps over (`DESCRIBE world.creature_template` vs `acore_world.creature_template`).

### 3. Wowhead (WotLK)

- Base: https://www.wowhead.com/wotlk/
- Authoritative for in-game intended behaviour: spell coefficients, NPC abilities, quest steps, loot, item stats, drop rates.
- Use for "what *should* happen" when emulators disagree or both look wrong.
- Fetch via WebFetch/WebSearch when context is needed.

### 4. cMaNGOS mangos-wotlk (installed)

- **Source:** `D:\Games\mangos-wotlk` (shallow clone of `cmangos/mangos-wotlk`, master). Read-only reference; never edit. Layout: `src/`, `sql/base/`, `sql/updates/mangos/`, `sql/base/dbc/`, `sql/scriptdev2/`.
- **DB repo:** `D:\Games\wotlk-db` (shallow clone of `cmangos/wotlk-db`). Contains `Full_DB/`, `Updates/`, `ACID/`, `locales/`, and the upstream `InstallFullDB.sh`.
- **DBs:** in the same MySQL 8.4 (`127.0.0.1:3306`, `root`/`1234`): `cmangos_world` (content — for cross-reference), plus `cmangos_realmd`, `cmangos_characters`, `cmangos_logs` (created so the server can actually run — see "Running locally" below). Locales were skipped in the world DB (English content only).
- **Reinstall / refresh:** `bash D:\Games\wotlk-db\install_reference.sh` (custom non-interactive installer mirroring `InstallFullDB.sh`'s `apply_full_content_db` order: FullDB → Updates → core `mangos` updates >14092 → DBC `original_data`+`cmangos_fixes` → ScriptDev2 → ACID → `cmangos_custom`). It **drops and recreates** `cmangos_world`. The ~8 "Duplicate column / Unknown column required_*" warnings from the post-14092 core updates are expected (those revs are already bundled in the DB `Updates/` folder) and harmless.

**cMaNGOS schema caveats (differ from AC/TC — verify columns before assuming a value maps over):**
- cMaNGOS is **not** a TrinityCore fork; its DB schema and content-scripting engine differ substantially from AC/TC. Treat it as an independent second opinion, not a drop-in.
- `broadcast_text` columns are `Id`, `Text`, `Text1`, `LanguageID`, `EmoteID1`… — **not** AC's `ID`/`MaleText`/`FemaleText`. (The numeric IDs themselves match across emulators since they're DBC-derived — e.g. the Christoph Faral/Aedis Brom RP block is `Id` 399–432 in all three.)
- NPC behaviour/banter is **not** SmartAI. cMaNGOS uses `creature_ai_scripts`, the `dbscripts_on_*` family (`dbscripts_on_creature_movement`, `dbscripts_on_event`, `dbscripts_on_relay`, …), `dbscript_random_templates`, plus `script_texts`/ScriptDev2 C++ scripts. To trace a conversation: find the `dbscripts_on_*` rows, follow `dataint`/text ids into `script_texts`, and the waypoint-triggered ones via `creature_movement[_template]`.

### 5. GitHub (via `gh` CLI)

- **Always use the `gh` command-line tool** to check anything on GitHub — never guess or rely on memory.
- Use it to inspect upstream AzerothCore and reference-emulator repos: search merged PRs/commits for prior fixes, read issues for known bugs, view how a fix was implemented upstream.
- Examples:
  ```bash
  gh search prs --repo azerothcore/azerothcore-wotlk "Kologarn grip" --state merged
  gh search issues --repo azerothcore/azerothcore-wotlk "Assembly of Iron evade"
  gh pr view <number> --repo azerothcore/azerothcore-wotlk
  gh search commits --repo TrinityCore/TrinityCore "creature_template entry"
  gh api repos/azerothcore/azerothcore-wotlk/commits/<sha>
  ```
- A bug may already be fixed (or under discussion) upstream — check before implementing.

## Hard rule: never reconstruct an event from scratch 

**If neither TrinityCore nor cMaNGOS actually implements the behaviour, do not attempt to recreate it yourself.** Stop and report that no reference implementation exists. Do **not** author a fix by stitching together a transcript (Wowhead/Warcraft Wiki/videos) + broadcast-text rows + guessed coordinates/timings. A "reference" that only *spawns* the NPCs, leaves dialogue/movement unscripted, or is otherwise a stub does **not** count as an implementation — partial stubs are not a basis to build on.

The reference cores exist precisely so fixes are *ported and verified*, not invented. Self-authored events (untested coordinates, guessed timers, hand-mapped speakers) are high-risk and unverifiable here, and are out of scope regardless of how complete the canonical data looks.

Example: issue #10618 ("Matis the Cruel" quest 9711 turn-in execution event) — TC scripts none of it, cMaNGOS only summons two NPCs with no dialogue. Per this rule, the correct outcome was to **not** implement it and report the absence of a reference, rather than reconstruct the ~100s cutscene by hand.

## Bug-fixing workflow

1. **Reproduce / locate** the bug in AC source (`src/server/`) and/or `acore_world` data.
2. **Compare** against TC source and `world` DB; consult Wowhead for intended behaviour.
3. **Decide** the correct behaviour from the cross-reference.
   - **When the references differ in polish/completeness, prefer the more polished implementation** (then port it to AC idioms). If one emulator scripts a richer, more faithful version of the same event — e.g. staggered/timed steps, extra emotes/lines, smoother sequencing — adopt that behaviour rather than the minimal one, as long as it matches the authoritative intent (Wowhead/sniffs). Example: for quest 486 "Ursal the Mauler", TrinityCore transforms all four Enslaved Druids of the Talon simultaneously, while cMaNGOS wakes them together then morphs/flees them in a timed cascade (one speaks, then 2+1 peel off over ~5s) — the cMaNGOS cascade is the more polished version and was the one ported.
   - **Never skip the "thin" cosmetic details — emotes, orientation/facing, stand states, kneel/point gestures, NPC-flag toggling, return-to-home facing.** They are part of the authentic behaviour, not optional garnish; a fix that reproduces only the movement/text but drops the gestures is incomplete. Enumerate every step in *each* reference core (TC actionlists **and** cMaNGOS `dbscripts_*`) and port the union of the cosmetic steps, not just the ones the first source you read happened to include. Example (quest 9473 "An Alternative Alternative", Daedal 17215, issue chromiecraft#1407): the first pass added the walk but left out Daedal's kneel (emote 16) while applying the cure, the priestess pointing in alarm (emote 25), and turning him back to his spawn facing on arrival — all three are present in both TC and cMaNGOS and had to be added. A driver NPC's timed actionlist can carry a *second* NPC's emotes/lines via `PLAY_EMOTE`/`TALK` with a creature target, so adding them need not touch the other NPC's own script.
4. **Implement** in AC idioms:
   - Code fixes → follow `CLAUDE.md` style (Allman, `auto const&`, typed helpers, `LOG_*`, `fmt` `{}`).
   - Data/content fixes → prefer **SmartAI** + the SQL update workflow in `data/sql/updates/pending_db_*/` (never edit immutable SQL dirs).
5. **Verify** with linters (`apps/codestyle/codestyle-cpp.py`, `codestyle-sql.py`) before claiming done.
   - **Always double-check that deletions are safe before writing or applying them.** For every `DELETE` (and any destructive write), prove the blast radius first:
     - Scope the `WHERE` as narrowly as the fix needs (specific keys, not whole tables/groups).
     - Find everything that references the rows you're touching (other tables, `smart_scripts` targets/actionlists, `conditions`, `*_locale`, `gossip`, scripts in C++). A row that looks unused may be reached indirectly.
     - **Read every parameter, not just the first.** Multi-param actions fan out: e.g. SmartAI action 87 (`CALL_RANDOM_TIMED_ACTIONLIST`) uses `action_param1`–`param6`; checking only `param1/2` hid two actionlists once and led to a wrong "this group is unused" claim. Select all relevant columns.
     - Confirm whether sibling/locale data stays consistent after the change (see `creature_text_locale` note below).
     - When unsure, ask before applying — never run a destructive statement against a live DB without confirmation (and never against the TC reference DBs at all).
6. **Provide in-game test steps.** After every bugfix, give the user the GM commands needed to reproduce and verify the fix in-game. Pull exact commands/syntax from [`docs/GM_COMMANDS.md`](docs/GM_COMMANDS.md) (699-command reference). Typical building blocks:
   - `.go creature <guid>` / `.go xyz <x> <y> <z> <map>` — reach the affected NPC/location.
   - `.npc add <entry>` / `.respawn` / `.die` / `.revive` — spawn or reset the test target.
   - `.cast <spellid>` / `.aura <spellid>` / `.unaura <spellid>` — exercise spell/aura behaviour.
   - `.modify <stat> …`, `.cheat …`, `.tele …`, `.reset all` — set up the test state.
   - `.reload <table>` — apply DB-side fixes without a server restart.
   Give the concrete entry/spell/guid IDs relevant to the fix, in the order to run them.
7. Respect `CLAUDE.md` agent rules: don't build unless asked; don't run state-changing git commands.

### Git / commit rules
- **Never co-author commits with Claude.** Do not add `Co-Authored-By: Claude ...` (or any Claude/AI co-author trailer) to commit messages. Commits are authored solely by the user.
- **Never open pull requests unless the user explicitly asks.** Do not run `gh pr create` (or otherwise open a PR) on your own initiative; wait for an explicit instruction. The same applies to pushing branches/forking for the purpose of opening a PR.
- **Always follow the repository PR template** (`pull_request_template.md` in the repo root) when opening a PR — never substitute an ad-hoc body. Keep every section and checklist (`## Changes Proposed:`, `### AI-assisted Pull Requests`, `## Issues Addressed:`, `## SOURCE:`, `## Tests Performed:`, `## How to Test the Changes:`, `## Known Issues and TODO List:`, and the trailing `## How to Test AzerothCore PRs` boilerplate), filling the `- [ ]`/`- [x]` checkboxes honestly. Notes that apply here:
  - **AI disclosure is mandatory:** tick the AI-assisted box and name the model (e.g. "Claude Opus 4.8") — the template requires disclosing AI use.
  - For a fix derived from another emulator (cMaNGOS/TC), tick the "from another project" SOURCE box and credit the original author/commit in the text. A literal git cherry-pick must use `--author`; a re-implementation in a different engine (e.g. porting cMaNGOS `dbscripts` to AC SmartAI) is not a cherry-pick — credit by mention instead (per the user's standing "mention, don't co-author" preference).
  - Branch a fix off the latest `origin/master` so the PR contains only its own commit, and stage only the fix file(s) — keep local workflow files (`CLAUDE*.md`, `docs/`) out of the PR.

## Quick reference

```bash
# Diff a creature between emulators
mysql -h 127.0.0.1 -u root -p1234 -e "SELECT * FROM acore_world.creature_template WHERE entry=N \G"
mysql -h 127.0.0.1 -u root -p1234 -e "SELECT * FROM world.creature_template WHERE entry=N \G"          # TrinityCore
mysql -h 127.0.0.1 -u root -p1234 -e "SELECT * FROM cmangos_world.creature_template WHERE entry=N \G"  # cMaNGOS

# Compare a script's logic
#   AC:      D:\Games\azerothcore-wotlk\src\server\scripts\...
#   TC:      D:\Games\TrinityCore\src\server\scripts\...
#   cMaNGOS: D:\Games\mangos-wotlk\src\game\... (+ sql/scriptdev2/)

# Wowhead for intended behaviour
#   https://www.wowhead.com/wotlk/npc=N  |  /spell=N  |  /quest=N  |  /item=N
```

## Lessons & gotchas (accumulated)

### SmartAI (`smart_scripts`)
- **Trust the C++ param struct, not the enum's inline comment, for action/event param order.** The comments in `SmartScriptMgr.h` are sometimes stale/misordered. For `SMART_ACTION_CALL_TIMED_ACTIONLIST` (80) the comment reads "ID, stop after combat?(0/1), timer update type", but the actual struct is `timedActionList { id; timerType; allowOverride; }` → **param1 = actionlist id, param2 = timerType (0 OOC / 1 IC / 2 ALWAYS), param3 = allowOverride (0/1 bool)**. Putting the timerType (2) in param3 makes the loader reject the row at startup: *"Action 80 uses param value of type Boolean with value 2, valid values are 0 or 1, skipped."* Cross-check the `union SmartAction`/`SmartEvent` structs in `SmartScriptMgr.h` and the handler in `SmartScript.cpp` before trusting a column's meaning.
- **Invalid `smart_scripts` rows fail silently in-game — check `Errors.log`.** Bad rows are dropped at load with a `SmartAIMgr:` line in `build/bin/<cfg>/Errors.log` (and `Server.log`), not surfaced anywhere in the client. When "nothing happens" after a SmartAI fix, `grep` the entries/guids in `Errors.log` first.
- **`.reload smart_scripts` only refreshes the global cache; it does NOT re-init already-spawned creatures.** `HandleReloadSmartScripts` just calls `sSmartScriptMgr->LoadSmartAIFromDB()`. A live creature keeps the event list it cached in `SmartScript::OnInitialize` at spawn. To apply a `smart_scripts` change to creatures already in the world, **respawn them** (or restart worldserver). Changing `creature_template.AIName` likewise needs a respawn/restart.
- **`TARGET_CREATURE_DISTANCE` (11) returns ALL matching creatures in range, not the closest** (AC's "closest" is `TARGET_CLOSEST_CREATURE` = 19). Note AC vs TC enum drift here: TC's target 11 is also creature-range, but TC's waypoint action `53`/event `40` are *escort* ops in AC — port waypoint paths to AC's `WAYPOINT_START` (232) + `WAYPOINT_REACHED` (108).
- **`WAYPOINT_START` (232) param3 = `pathSource`, and it selects WHICH TABLE the path is read from: `0` = `waypoint_data` (sWaypointMgr), `1` = `waypoints` (sSmartWaypointMgr).** Put the path in the table matching the pathSource, or `MoveWaypoint` silently fails to move with `WaypointMovementGenerator::DoInitialize: creature ... doesn't have waypoint path id: N` in `Errors.log`. The `waypoints` table (SmartAI-specific, point ids must start at 1 and be sequential — `node.Id` = the `pointid`, so a `WAYPOINT_REACHED` despawn keys off that same number) is the natural home for SmartAI flee/intro paths → use pathSource `1`. (params: param1 = pathId, param2 = repeat 0/1, param3 = pathSource.)

### SmartAI escort vs waypoint movement (walk-out / RP / walk-back)
For a "walk over, do a roleplay, walk back" event prefer the **escort** family — `ESCORT_START` (action 53) + `ESCORT_REACHED` (event 40) + `ESCORT_PAUSE` (action 54) + `ESCORT_ENDED` (event 57) — over `WAYPOINT_START`/`WAYPOINT_REACHED`, because only the escort path supports pausing at a node and auto-handles NPC flags. Both read the path from the **`waypoints`** table (escort uses `pathSource = SMART_WAYPOINT_MGR` by default). Verified on Daedal (17215) for quest 9473:
- **`ESCORT_START` (53) params** = `forcedMovement, pathID, repeat, quest, despawnTime, reactState` (the `wpStart` struct). **`forcedMovement = 1` (`FORCED_MOVEMENT_WALK`) forces walking** (`EscortMovementGenerator` → `init.SetWalk(true)`); `0` (`NONE`) keeps the creature's current run/walk state, which usually means it **runs**. `reactState` (param6) is applied unconditionally via `SetReactState` and never reset — pass `2` (aggressive, the creature default) unless you actually want it passive.
- **Escort auto-removes/restores NPC flags** when started with a **player invoker** (`SmartAI::StartPath` 254-258 saves `GetNpcFlags()` + sets `UNIT_NPC_FLAG_NONE`; `EndPath` 359-363 restores them, guarded by `if (mEscortNPCFlags)`). A turn-in (`REWARD_QUEST`, event 20) carries the player as invoker, so a plain non-pausing escort gets this for free and the flags come back at the final waypoint. **But if the escort PAUSES mid-path for an RP, the auto-flag is unreliable — observed flipping the flags back ON at the pause and OFF again on resume** (the pause's `MoveIdle` + the RP's `SET_ORIENTATION` facing spline disturb the escort movement state). For a paused/RP escort, **take manual control instead**: `SET_NPC_FLAG 0` on the reward *before* `ESCORT_START` (this also makes the escort save an empty set, so its own restore becomes a no-op and can't toggle them), then `SET_NPC_FLAG <orig>` on the **final `WAYPOINT_REACHED`** (event 40, same place you set the home facing). Don't restore on `ESCORT_ENDED` (57): a `STOPPED`/failed escort skips it, and the event-40-at-last-point timing is the proven idiom anyway. (Daedal 17215 / quest 9473: the escort auto-flag toggled at the WP4 pause; manual remove-on-reward + restore-on-home-WP fixed it.)
- **Stop at a node for the RP with `ESCORT_REACHED` (40) → `ESCORT_PAUSE` (54, param1 = delay ms).** When `ESCORT_PAUSE` runs from the `ESCORT_REACHED` event it is `forced=false` and `MovepointReached` cleanly stops the creature at the node; when run from a called actionlist it is `forced=true`. The escort auto-resumes (walks the rest of the path) when the pause timer expires.
- **`ESCORT_REACHED`/`PAUSED`/`STOPPED`/`ENDED` matching:** `event_param1` (pointID) is checked against the current WP and `event_param2` (pathID) against `GetScript()->GetPathId()` (the active path) — so keying `(point, pathId)` is valid; `0` = any.
- **Restoring facing after a walk-out-and-return: set orientation on the final `WAYPOINT_REACHED`/`ESCORT_REACHED` (event 40), NOT on `ESCORT_ENDED` (57).** This is the established idiom — ~58 scripts do it (Watcher Cutford, Keeper Remulos "Set Orientation Home Position", Sunreaver War Mage, Captured Farmer…); effectively none use `ESCORT_ENDED`. `MovepointReached` fires `ESCORT_REACHED` at the instant of arrival, while the creature is stopped at the node and the arrival spline has finalised, so a `SET_ORIENTATION` (66) turn launches cleanly there; fired from inside `EndPath` (which is what `ESCORT_ENDED` is) the turn does not stick, because `EndPath` doesn't apply the node `orientation` and the spline lifecycle is different. **Use a `POSITION` target with a fixed `target_o`** (the spawn angle), not target `SELF`/home: the escort **corrupts the home orientation** — `MovepointReached` runs `SetHomePosition(GetPosition())` *right after* firing event 40 (line ~658), overwriting it with the travel direction (and overwriting any `SET_HOME_POS` you tried to do on `ESCORT_REACHED`). Some scripts get away with target `SELF` only because their last node's travel direction happens to match; when it doesn't (e.g. Daedal 17215 arriving at his home WP facing ~0.87 rad but needing spawn facing 3.80482), pin it with `POSITION` + `target_o`. First attempt here used `ESCORT_ENDED` (57) + `SET_ORIENTATION` and the facing came out wrong every time.
- **`waypoints.orientation`:** `NULL` = no facing change at that node; a value (**including `0.0`**) forces that facing when the node is a *stop/pause* (`PausePath` applies `node.Orientation` if it `has_value()`). Set the pause node's orientation toward the RP target to avoid a one-frame facing flash before the actionlist re-faces it.
- **One actionlist can drive a second NPC's gestures:** `PLAY_EMOTE` (5) emotes everyone in `targets` and `TALK` (1) makes the *target* creature speak its own `creature_text` — so a `target_type 19` (closest) row on the driver's list adds the other NPC's emotes/lines without editing that NPC's own `smart_scripts`.

### NPC dialogue / `creature_text`
- **`BroadcastTextId` is what the client actually displays**, not the `Text` column. `Text` is effectively a human-readable comment; if `BroadcastTextId` is set and valid, the player sees the broadcast text's content. A row whose `Text` looks right can still show the wrong line if its `BroadcastTextId` is wrong (e.g. Wright Williams 28355 group 3 had the "too much Deathweed" `Text` but `BroadcastTextId` 27809 = "Skadi the Ruthless is within range…").
- **Localization (`creature_text_locale`) is only read when `BroadcastTextId == 0`.** Source: `CreatureTextMgr::GetLocalizedChatString` (`src/server/game/Texts/CreatureTextMgr.cpp`) — `if (bct) baseText = bct->GetText(locale, gender); ... if (locale != DEFAULT_LOCALE && !bct) { read creature_text_locale }`. So when a valid `BroadcastTextId` is present, localized strings come from `broadcast_text`/`broadcast_text_locale`, and any `creature_text_locale` rows for that entry/group are inert dead data. Reordering/scrambling the English groups does **not** desync localized display in that case — but the locale rows may be left stale; deleting them to match TC is an optional tidy-up.
- **NPC banter is data-driven, not C++.** It's `smart_scripts` (often `action_type 87 CALL_RANDOM_TIMED_ACTIONLIST` on one NPC) driving `creature_text` groups, with the *other* NPC spoken to via `target_type 19` (closest creature by entry). To trace a conversation: find the actionlists (`source_type = 9`), map each `Say Line N` to the target creature's `creature_text` group N, then resolve each group's `BroadcastTextId` to the displayed line.
- **TC vs AC `creature_text` can differ in both group ordering and `BroadcastTextId` mapping** even when the `smart_scripts`/actionlists are byte-identical. Compare the *resolved displayed lines per group*, not just row counts.
