# Task 1c.4 (UpdateFields → 3.4.3.54261) — Remaining Work

Status as of 2026-06-28. The structured `UF::UpdateField` object model is fully in and **builds clean**
(`game.lib` + `scripts.lib` compile; `worldserver.exe` links once it is not locked by a running server;
`codestyle-cpp.py` passes). This file tracks everything still open.

## 1. The behavioral gate (STEP 5 — Phase E) — user-driven

Not yet validated against a live client. To close 1c.4:

1. Boot the rebuilt `worldserver` (auto-applies the new `character_customizations` SQL via `Updates.AutoSetup=1`).
2. Log in with the patched **3.4.3.54261** client; confirm a well-formed `SMSG_UPDATE_OBJECT` create block and
   that the character enters the world without a disconnect.
3. If the client disconnects, bisect the create block against a HermesProxy capture of a real 3.4.3 session.

**Movement wire-order risks to verify in the capture** (`Object::BuildMovementUpdate`): 3rd movement-flag word = 0,
`StepUpStartElevation` = 0, transport `prevTime ← time2` (+ vehicleId absent), the rune-cooldown byte formula, and
the AnimKit / WorldEffect zero-fills.

**Boot-validation needed:** the newly ported ChrCustomization DB2 stores
(`sChrCustomizationChoiceStore` / `…DisplayInfoStore` / `…ElementStore`) have `DB2LoadInfo` layout hashes
transcribed from xian55. If a hash/field-count is off, the store fails to load at boot — confirm they load
("DB2 Stores loaded") on first start.

## 2. Co-dependencies stuck in the other agent's Phase-2 files (NOT in this commit)

The working tree shares some files with the in-flight Phase-2 (Battle.net / packet-path) work. These 1c.4 edits
are intermixed at the hunk level with Phase-2 changes and were therefore **left out of the 1c.4 commit** — they must
ride along when the Phase-2 work is committed/merged, or 1c.4 will not compile standalone:

- **`src/server/game/Server/WorldSession.h`** — `CharacterCreateInfo` gains
  `std::vector<WorldPackets::Character::ChrCustomizationChoice> Customizations;` (+ a forward-decl and `<vector>`).
  Required by `Player::Create` / the char-create handler.
- **`src/server/game/Handlers/MiscHandler.cpp:~1053`** — one `// [1c.4] TODO:` line (TodayContribution → send 0).

## 3. Large / coupled bricks deferred (real features, scoped but not started)

- **Currency subsystem** — In 3.4.3 honor/arena points are *currencies*
  (`GetHonorPoints() == GetCurrencyQuantity(CURRENCY_TYPE_HONOR_POINTS)`). We implemented the **server-side** fix
  (`m_honorPoints`/`m_arenaPoints` members backed by the existing `characters.totalHonorPoints`/`arenaPoints`
  columns), so PvP rewards / vendor logic work. **Client-visible** display still needs the full Currency brick:
  `ModifyCurrency` / `GetCurrencyQuantity`, a `character_currency` table, the currency packets
  (`SMSG_SET_CURRENCY` / `SMSG_INIT_CURRENCY`), and CurrencyTypes-DB2 caps/weekly-caps. Multi-day; standalone.
- **Guild `ObjectGuid` (`HighGuid::Guild`)** — AC keeps 64-bit guids with no `HighGuid::Guild`, so the mirror-image,
  char-enum and corpse `GuildGUID` fields are sent as `ObjectGuid::Empty`. Adding a real guild guid is coupled to
  Phase-2's ObjectGuid **wire serialization** — do it with the Phase-2 agent, not in isolation.

## 4. Genuinely absent in 3.4.3 — nothing to implement (xian55-confirmed)

These have **no** 3.4.3 player update-field; the corresponding `// [1c.4] TODO:` stubs are correct and final:

- **Arena-team info** (`PLAYER_FIELD_ARENA_TEAM_INFO_*`) — removed from update-fields; 3.4.3 arena teams use query
  *packets*, not inline fields.
- **RAF "can grant level" flag**, **rune-regen** (`PLAYER_RUNE_REGEN_*`), **second talent points**
  (`PLAYER_CHARACTER_POINTS2`) — no equivalent field.
- **Raw-index field setters** — `cs_debug` setvalue/getvalue and `MapScripts` `SCRIPT_COMMAND_FIELD_SET` /
  `FLAG_SET` / `FLAG_REMOVE` set update fields by raw numeric index, which the structured model has no access path
  for by design. Supporting them would need a reverse index→field map (debug-only; low value).

## 5. Best-effort / data-gated cosmetics still partial

- **Shapeshift model by hair/skin color** — `ObjectMgr::GetModelForShapeshift` now reads the player's real
  `Customizations` (skin = `[0]`, hair = `[3]`) per xian55's `GetModelForForm` switch, with the legacy
  `_playerShapeshiftModel` table kept as a fallback. Existing 3.3.5 characters (no customization rows) take the
  fallback until they have customizations.
- **Legacy `characters.skin/face/hairStyle/hairColor/facialStyle` columns** are kept vestigial (`NOT NULL DEFAULT 0`,
  dropped from the INS/UPD bind but **not** `DROP`ped). A future cleanup can drop them once a byte→choice migration
  for pre-existing characters exists (needs the ChrCustomization choice mapping).

## 6. How the appearance round-trip works now (reference)

Appearance is native `ChrCustomizationChoice` (Variant A): `character_customizations` /
`corpse_customizations` tables; `Player::SetCustomizations` / `GetCustomizationChoice` / `_SaveCustomizations`;
loaded at login (`PLAYER_LOGIN_QUERY_LOAD_CUSTOMIZATIONS`), populated on create from the 3.4.3
`CMSG_CREATE_CHARACTER`, persisted on save, copied to corpses/bones, fed to the char-select enum (chained async
query) and the barbershop (`CMSG_ALTER_APPEARANCE` → `SetCustomizations` + `SetNativeGender`, cost via
`sChrCustomizationOptionStore`).
