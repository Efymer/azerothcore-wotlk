# AzerothCore → WotLK Classic 3.4.3 — Project Knowledge Base

> Living knowledge from porting AzerothCore (a WoW 3.3.5a / build 12340 emulator) to natively
> support the **WoW WotLK Classic 3.4.3.54261** client. Captures the architecture, the protocol
> facts, the environment, what's built, the gotchas, and a glossary. Read this before resuming.

---

## 1. Goal & locked decisions

- **Goal:** make AzerothCore speak the **modern WotLK Classic 3.4.3.54261** client protocol natively
  — a **fork & replace** of the legacy 3.3.5a (build 12340) protocol. WotLK *content* (the
  `acore_world` DB, scripts, gameplay) is preserved; only the **wire protocol, login stack, and
  client-data layer** change.
- **Architecture:** native (not a translation proxy). The user weighed a HermesProxy-style proxy
  and chose native fork.
- **Target build:** **3.4.3.54261** (the final Live WotLK Classic build, Apr 2024). The user's
  client is at `D:\Games\World of Warcraft 3.4.3.54261`.
- **Auth:** patched client (via `wow-patcher`) + a **native modern `bnetserver`** (REST + protobuf +
  TLS). The grunt SRP6 `authserver` is NOT usable by a modern client.
- **Branch:** `feature/wotlk-classic-3.4.3`. Remotes: `fork` = `Efymer/azerothcore-wotlk` (ours,
  push here), `origin` = `azerothcore/azerothcore-wotlk` (upstream, **never** push there).

### The single most important strategic fact — build divergence

**TrinityCore's `wotlk_classic` branch targets 3.4.4.61581 (WDC5), NOT 3.4.3.54261 (WDC4).** It's
actually a Cata-Classic (4.4.x) branch that *added* WotLK Classic 3.4.4 in May 2025; it never
targeted 3.4.3 and never used WDC4. Consequence: **we back-port format/protocol deltas wherever
TC's 61581 code differs from our 54261 client.** This recurs (it bit us on the DB2 format and the
DB2 table metadata; it will bite again on opcodes/UpdateFields/packet layouts). TC remains the
primary porting reference, but its build is not ours.

---

## 2. Glossary (terms used throughout)

### Client / content
- **WotLK Classic / 3.4.x** — Blizzard's re-release of Wrath of the Lich King content running on a
  *modern* 64-bit client engine (BfA/Shadowlands-era), not the original 32-bit 3.3.5a client.
- **Client build** — the exact client version. `12340` = original 3.3.5a (what stock AzerothCore
  speaks). `54261` = 3.4.3 final Live (our target). `61581` = 3.4.4 (what TC targets).
- **CASC** (Content Addressable Storage Container) — the modern client's on-disk content store
  (under `Data/`), replacing the old **MPQ** archives. Read via **CascLib**.
- **NGDP / TACT** — Blizzard's content-distribution system (the CDN protocol the client/Agent use to
  download data). `.build.info` configures CDN hosts. Private servers can redirect this (Whitemane
  points it at `cascstream.gamefreedom.org`; wow-patcher can redirect to the Arctium CDN).
- **Agent** — Blizzard's `Agent.exe` helper process (listens on localhost **:1120**); handles
  updates/launching. Irrelevant to server emulation; the client talks to it for housekeeping.
- **WTF/Config.wtf** — client config. `SET portal "X"` selects the login datacenter (a region code
  like `US`, or an IP). With an IP portal the client connects directly; with a region code it
  appends the portal suffix (see wow-patcher).

### Client database files
- **DBC** (DataBase Client) — the *old* 3.3.5a static-data file format (`*.dbc`). AzerothCore reads
  these via `DBCStores`.
- **DB2** — the *modern* client static-data format (`*.db2`), replacing DBC. Holds the same kinds of
  data (Map, ChrRaces, Item, …) but a different on-disk layout, with compression/encryption.
- **WDC4 / WDC5** — the DB2 file-format **version** (the 4-byte magic `WDC4`/`WDC5` at the start of
  each `.db2`). "WDC" ≈ Warcraft Database Client. **WDC5 = WDC4 + a 132-byte preamble** (`uint32
  Version` + `char Schema[128]`) inserted right after the magic; everything from `RecordCount`
  onward is byte-identical. **3.4.3.54261 ships WDC4; 3.4.4.61581 + retail ≥ ~10.2.7 ship WDC5.**
  Our reader was ported from TC (WDC5-only) and patched to also read WDC4.
- **LayoutHash** — a hash of a DB2 table's column layout, stored in the file header and used to
  validate that the server's metadata matches the client's table. Differs per build when a table's
  columns change — the root cause of "Invalid X.db2 format" when using 61581 metadata against a
  54261 file.
- **FileDataID** — the numeric ID identifying a file inside CASC (e.g. a `.db2`, a model, a map). The
  extractor opens files by FileDataID.
- **WDB / ADB** — client-side runtime caches (`Cache/WDB`, `Cache/ADB`); not server data.
- **WoWDBDefs** — the community project (`github.com/wowdev/WoWDBDefs`) of **`.dbd`** files, each
  defining a DB2 table's columns and **per-build LAYOUT blocks** (tagged with build lists like
  `…, 3.4.3.54261, …`). The LAYOUT tag *is* the LayoutHash. **This is the authoritative source for
  54261-correct DB2 metadata** (TC's hardcoded metadata is for 61581).

### Map / model extraction
- **ADT / WDT** — terrain tile / map-table files. **WMO** — world map objects (buildings). **M2** —
  doodad/creature models. The extractors read these from CASC to produce server data.
- **map / vmap / mmap** — the *server's* extracted data: `.map` = terrain height/liquid tiles (for
  positioning), `vmap` = collision/line-of-sight geometry, `mmap` = navigation meshes (pathfinding).
  `.map` output **must** match AzerothCore's binary format so the worldserver's `GridTerrainLoader`
  reads it (we kept AC's writer format, not TC's).

### Login / Battle.net
- **bnetserver** — the modern **Battle.net** login server. AzerothCore had none (its `authserver` is
  the legacy grunt SRP6 logon). We built one. Listens on **:1119** (bnet TCP) + **:8081** (HTTPS REST
  login portal).
- **BGS / bgs.protocol** — Blizzard Game Service: the **Protocol Buffers** RPC schema the modern
  client speaks to bnetserver (services: Authentication, Account, Connection, GameUtilities). The
  `proto` lib holds the generated code.
- **SRP6 / SRP6v1 / SRP6v2** — Secure Remote Password, the password-proof protocol. The modern bnet
  login uses **BnetSRP6** (v1 = SHA256-based, v2 = PBKDF2-HMAC-SHA512, 15000 iters). The **SRP
  username is `HexStr(SHA256(uppercased-email))`** — registration and login MUST compute it the same
  way (a mismatch = error BLZ51900003).
- **REST login portal** — the bnetserver serves an HTTPS `/bnetserver/login/` form (JSON) and an
  `/bnetserver/login/srp/` endpoint where the client runs the SRP exchange. Built on a modern HTTP
  server (`Acore::Net::Http`) + TLS + **ProtobufJSON** (protobuf↔JSON via rapidjson).
- **Realm list / RealmHandle / sub-region** — after login, the client requests the realm list over
  bnet RPC. `Battlenet::RealmHandle` = Region/Site/Realm. The realm advertises a `gamebuild`; if it
  ≠ the client build the client shows the realm **Incompatible** (set `realmlist.gamebuild = 54261`).

### World connection (the next, unbuilt frontier)
- **WorldSocket / WorldSession / opcodes / WorldPacket** — the worldserver's packet layer. The
  modern client uses **uint32 opcodes** (AC's are uint16), a different packet header, and a modern
  `WorldPacket` with a `ConnectionType`.
- **WorldPacketCrypt** — the modern **AES-256** world-packet session encryption (replaces the legacy
  ARC4 **`AuthCrypt`**). Ported (world-protocol brick 1).
- **ConnectTo** — the bnet→world handoff: the realm-join hands the client a worldserver address +
  session key; the worldserver validates it during the world auth handshake.
- **ENTER_ENCRYPTED_MODE** — the modern world handshake step (`SMSG_ENTER_ENCRYPTED_MODE` /
  `CMSG_ENTER_ENCRYPTED_MODE_ACK`) that switches on `WorldPacketCrypt` after key derivation
  (HMAC-SHA512 of the session key with fixed seeds).
- **Connection V2 / join ticket** — the modern world handshake opens with a
  `WORLD OF WARCRAFT CONNECTION … V2` banner, and `CMSG_AUTH_SESSION` carries a **protobuf RealmList
  join ticket** (not the 3.3.5a seed→SHA1 digest flow).

### Client patchers
- **wow-patcher** (`wowemulation-dev/wow-patcher`, Rust) — **statically** patches the client `.exe`:
  nulls/redirects the BGS portal suffix (`.actual.battle.net` → `.actual.wowemu.dev`), swaps the
  **RSA modulus** (ConnectTo, world handoff) to TrinityCore's key, and redirects the Version/CDN URLs.
  Requires the server's **TLS cert to chain to a trusted root**. **This is what we use** — transparent
  and debuggable.
- **Arctium WoW Launcher** — patches the client **in-memory at launch** (runtime), routing via a
  local proxy. Opaque/launcher-controlled (hard to debug). What we initially tried and abandoned.
- **RSA modulus / Ed25519 key** — keys the client uses to verify the server (RSA = ConnectTo / world
  handoff; Ed25519 = module signing, *not used by Classic clients*). wow-patcher swaps the RSA to
  TC's; matters for the *world* connection, **not** for login.
- **Whitemane / GameFreedom** — a real Cata-Classic (4.4.2.60895) private server. Inspected as a
  reference: its `WowClassic.exe` is **stock/unpatched** — it patches **in-memory via its launcher**
  (Arctium-based) and redirects CDN via `.build.info`. Confirms two viable strategies (static-patch
  vs in-memory launcher); we chose static.

### References
- **TrinityCore `wotlk_classic`** — the primary porting reference (same codebase lineage as AC).
  Targets 3.4.4.61581/WDC5; "very raw" per TC. Pin commit at port time.
- **TrinityCore `cata_classic` / `master`** — fallbacks; master's *history* (≤ ~10.2.x, 2023) had a
  native **WDC4** reader (confirms the WDC4 path) but its metadata is Dragonflight content (not 3.4.3).
- **HermesProxy** — a modern-client↔legacy-server translation proxy; behavioral reference for the
  modern bnet/world protocols (it implements a modern BNetServer + WorldServer).

---

## 3. Environment & build

- **OS/shell:** Windows 10, PowerShell (+ Git Bash). MySQL 8.4 running (`127.0.0.1:3306 root/1234`),
  databases `acore_auth` / `acore_characters` / `acore_world` exist; we added **`acore_hotfixes`**.
- **vcpkg** bootstrapped at `C:\vcpkg`; installed `protobuf` + `casclib` (x64-windows). **Every CMake
  configure must pass** `-DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake`.
- **Configure (per-target build, multi-config generator):**
  ```
  cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSCRIPTS=none -DTOOLS_BUILD=all \
    -DBUILD_TESTING=ON -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
  cmake --build build --target <target> --config RelWithDebInfo
  ```
- **⚠ Build discipline:** **NEVER build the `unit_tests` target (it HANGS in this env — orphaned
  `cl.exe` at ~3% CPU) and avoid the full default build.** Build the specific changed target only
  (`common`, `database`, `proto`, `bnetserver`, `extractor_common`, `map_extractor`, …). The `game`
  lib (and `worldserver`) are enormous — comparable cost. Verify unit-test-shaped logic by
  inspection/isolated compile, not by building `unit_tests`.
- **New dependencies added:** `deps/protobuf` (vendored **protobuf 2.6.1** — see gotcha #2),
  `deps/casc` (CascLib via vcpkg), `deps/rapidjson` (vendored header-only).
- **Logging caveat:** bnetserver's **console appender uses the Windows Console API** (NOT capturable
  via stdout redirection / Bash) and its **file appender buffers until graceful shutdown** (a
  force-kill loses it). So `Bnet.log` is often empty. **Diagnose from the DB, TCP connection state,
  and the client's error code — not from server logs.**

---

## 4. What was built this session (by phase)

### Phase 0 — client-data toolchain (DONE, proven against the real client)
- `deps/protobuf` (2.6.1), `deps/casc`, `deps/rapidjson` vendored.
- `acore_hotfixes` 4th database (`DATABASE_HOTFIX=8`, `HotfixDatabase` pool mirroring World).
- CASC `extractor_common` (`CascHandles`, `DB2CascFileSource`, `ExtractorDB2LoadInfo`).
- **DB2 (WDC) reader** in `src/common/DataStores/` (`DB2FileLoader`/`DB2FileSystemSource`/`DB2Meta`),
  **patched to read WDC4** (54261) as well as WDC5.
- `map_extractor` + `vmap4_extractor` ported to **CASC** (output kept in **AzerothCore's** `.map`/vmap
  binary format so the worldserver reads it unmodified).
- **54261 DB2 table metadata** generated from **WoWDBDefs** for the 6 extractor tables
  (`src/tools/extractor_common/ExtractorDB2LoadInfo.h`).
- **Verified:** extracts **5,714+ `.map` tiles** and **2,526 building models** + 976 DB2 files from
  the real client. `map_extractor.exe -i <client> -o <out> -e 1|2`, `vmap4_extractor.exe -d <client>`.

### Phase 1a — native Battle.net login (DONE, a real client logs in)
- **`proto` lib** — TC's `bgs.protocol` generated `.pb.*` (vendored protobuf 2.6.1) + `ServiceBase`.
- **TLS** — `SslContext`/`SslStream`; AC's `Socket<T>` made stream-templated (TCP default preserved).
- **`Acore::Net`** — TC's **modern networking** subsystem ported (`src/common/network/`: modern
  `Socket<Stream>`, connection-initializer chain, full **HTTP server**, SslStream) coexisting with
  AC's legacy `Socket<T>`.
- **BnetSRP6 v1/v2** + **ProtobufJSON** + vendored **rapidjson**; `HMAC_SHA512`, `BigNumber::GetNumBits`.
- **Battlenet auth schema** (`battlenet_accounts`, `account` linkage) + `LOGIN_*_BNET_*` statements.
- **`Battlenet::RealmHandle` + `BnetRealmList` RPC** (`Battlenet::RealmListRpc::*` free functions),
  `ClientBuildInfo`, `Locales`/`Timezone`/`SecretMgr`-owner shims.
- **The `bnetserver` app** — `src/server/apps/bnetserver/` (Main, Session, SessionManager, REST
  portal, the 4 BGS services). Plus a **`--register-bnet <email> <password>`** account-create mode.
- **Verified end-to-end:** a wow-patcher-patched 54261 client completes SRP6v2 login and shows the
  realm list. (See §6 to reproduce.)

### Phase 1b — world protocol (STARTED; foundations only)
- **`WorldPacketCrypt`** (AES-256 world session crypt) + AES extended to AES-256.
- **`HMAC_SHA512`** (world key derivation).
- The rest is mapped but unbuilt — see §7.

---

## 5. Key gotchas & lessons (read before resuming)

1. **Build divergence (54261 vs 61581)** — the recurring tax of staying on 54261. Always verify
   format/layout against the *real client*, not TC's build. **⚠ The biggest instance (proven live,
   2026-06-27): OPCODES. The 54261 client uses FLAT uint16 opcodes (e.g. `CMSG_AUTH_SESSION = 14181 =
   0x3765`, `SMSG_AUTH_CHALLENGE = 12360 = 0x3048`), NOT TC 61581's grouped `0xGGGGIIII` uint32 scheme
   (`0x3B0001`).** The wire opcode is **2 bytes**, not 4. Bricks A/B were built on TC 61581 → wrong for
   the 54261 wire. **Authoritative 54261 opcode source: HermesProxy-WOTLK's `World/Enums/V3_4_3_54261/
   Opcode.cs`** (901 opcodes, flat `enum : uint`; saved at `extract-test/hermes_opcode_54261.cs`) —
   **its values are byte-exact against the live client** (the recon that earlier "dismissed" them was
   wrong; they ARE the wire format). The fix (brick B-redux): read/write the wire opcode as uint16,
   re-value the opcode table to the 54261 flat set (name-matched to our existing TC names so the
   handler/packet/call-site work survives), flat table indexing (not `GetOpcodeArrayIndex`), and a
   no-op `CMSG_LOG_DISCONNECT` (14185/0x3769 — the client's *first* post-AuthChallenge packet) handler.
   Discovered when the live client's first world packet read as garbage opcode `0x033769` (= uint16
   `0x3769` + 2 payload bytes misread as a uint32). Header is `[int32 Size][12B GCM tag]`, body =
   `[uint16 opcode][payload]` AES-GCM-encrypted; banner strings + crypto + framing were all correct.
2. **Protobuf must be 2.6.1, vendored.** TC's `bgs.protocol` ships *pre-generated* `.pb.*` locked to
   protobuf 2.6.1 (only ~8 of ~100 services have `.proto` source). Modern protobuf 6.x is
   incompatible. We vendor TC's 2.6.1 and use the checked-in generated code (no `protoc`).
3. **DB2 metadata comes from WoWDBDefs**, keyed to build 54261 — not TC's 61581 metadata. WebFetch
   misreports `.dbd` LayoutHashes; cross-check by extracting the exact LAYOUT block deterministically.
4. **SRP username = `HexStr(SHA256(uppercased-email))`** in BOTH registration and login. The original
   `--register-bnet` used the raw email → every login failed `BLZ51900003`. Fixed.
5. **wow-patcher needs a trusted TLS cert.** Self-signed works only if imported to the trusted-root
   store, and the **cert subject must match the portal** (we use `CN=127.0.0.1` with `portal "127.0.0.1"`).
   Installed to **CurrentUser Root** (user-authorized; remove via `certmgr.msc`).
6. **The modern client only connects on "Log In" click**, not at the menu — don't expect a server
   connection just from launching.
7. **bnetserver logging is effectively unreadable** (see §3). Diagnose from DB + connection state +
   client error codes.
8. **Account model**: a bnet account (`battlenet_accounts`) links to one+ grunt game accounts
   (`account.battlenet_account` / `battlenet_index`), named `<bnetId>#1`. The realm list needs a
   linked game account to be useful.
9. **Known stubs to finish for world entry**: `account.session_key` is `binary(40)` but the bnet
   session key is **64 bytes** (needs a wider column / dedicated bnet session table); `realmlist`
   lacks region/battlegroup columns; `ClientBuildInfo::AuthKeys` is empty; `Timezone` is a UTC stub.

---

## 6. How to run the login (reproduce)

1. **Build** the bnetserver: `cmake --build build --target bnetserver --config RelWithDebInfo`.
2. **Config** at `build/bin/RelWithDebInfo/configs/bnetserver.conf` →
   `LoginDatabaseInfo = "127.0.0.1;3306;root;1234;acore_auth"`, `Updates.EnableDatabases = 1`.
3. **TLS cert** (subject must match the portal): `openssl req -x509 -newkey rsa:2048 -keyout
   bnetserver.key.pem -out bnetserver.cert.pem -days 3650 -nodes -subj "/CN=127.0.0.1"
   -addext "subjectAltName=IP:127.0.0.1"` → place next to `bnetserver.exe`. Import the cert to the
   **CurrentUser Trusted Root** store (via `X509Store("Root","CurrentUser").Add` — the UI-prompt path
   fails non-interactively).
4. **Register a test account:** `bnetserver.exe --register-bnet test@test.com test12345`
   (creates `battlenet_accounts` row + linked game account `<id>#1`).
5. **Patch the client (once):** build wow-patcher (`cargo build --release`), then
   `wow-patcher.exe -l "<client>\_classic_\WowClassic.exe" -o "<client>\_classic_\WowClassic_patched.exe"`.
   Set `Config.wtf` → `SET portal "127.0.0.1"`.
6. **Realm:** `UPDATE acore_auth.realmlist SET gamebuild = 54261;` (so the realm shows compatible).
7. **Run** `bnetserver.exe`, launch `WowClassic_patched.exe`, log in `test@test.com` / `test12345`
   → realm list. (Entering the world fails: the 3.4.3 world protocol isn't built — see §7.)

---

## 7. World-protocol roadmap (the remaining epic)

The modern 3.4.3 world handshake: a `WORLD OF WARCRAFT CONNECTION … V2` banner → server `AuthChallenge`
+ DOS challenge → client `CMSG_AUTH_SESSION` carrying a **protobuf RealmList join ticket** →
**HMAC-SHA512** key derivation (`AuthCheckSeed`/`SessionKeySeed`/`EncryptionKeySeed`) →
`SMSG_ENTER_ENCRYPTED_MODE`/`CMSG_..._ACK` switches on `WorldPacketCrypt`; plus
`CMSG_AUTH_CONTINUED_SESSION` for instance sockets.

It must be built in this order (each is large; the opcode/WorldPacket bricks ripple through the whole
game and require the enormous `game` build):

- ✅ **Brick 1 (done):** `WorldPacketCrypt` (AES-256) + `HMAC_SHA512`.
- ✅ **A — modern `WorldPacket`/`Packet` framework (done):** ported TC `wotlk_classic`'s bit-stream
  into AC's `ByteBuffer` (`WriteBit`/`ReadBit`/`WriteBits`/`ReadBits`/`FlushBits`/`ResetBitPos`/
  `PutBits`/`bitwpos`, MSB-first, verbatim — AC had none); `WorldPacket` opcode `uint16`→`uint32`
  (default `UNKNOWN_OPCODE = 0xBBAADD`) + new `ConnectionType _connection` + `GetConnection()`;
  `ConnectionType` enum (`REALM=0`/`INSTANCE=1`/`MAX`/`DEFAULT=-1`) added to `Opcodes.h`; `Packet`
  base gained `GetConnection()`, `ServerPacket` a `connection` param, plus `WorldPackets::Null`.
  Verified: `shared` + `game` both compile. **Seam with brick B:** opcode *values* and `OpcodeTable`
  are deliberately UNTOUCHED — only storage/types widened so B's sparse 32-bit values won't truncate.
  ⚠ **Brick-B landmine (commented in-code):** 3 legacy `uint16(...)` opcode truncations remain at
  the 3.3.5 wire/DoS boundaries — `WorldSession.cpp` AntiDos+throttle (`:1363`/`:1372`) and
  `WorldSocket.cpp` `ServerPktHeader` (`:176`); brick B (modern wire header + opcode table) must
  widen these. AntiDos/throttle maps are keyed by `uint16` and will need widening too.
- ✅ **B — modern opcode table + 3.4.3 opcode enum (done):** replaced `enum Opcodes : uint16` with
  sparse `OpcodeClient`/`OpcodeServer : uint32` (packed `0xGGGGIIII`, full TC 61581 set verbatim —
  values to be empirically confirmed for 54261 at brick E), the two `GetOpcodeArrayIndex` switches +
  split `_internalTable{Client,Server}` + `IsValid` (kept AC's virtual `PacketHandler::Call` design,
  not TC's free-fn). `Initialize` registers the handshake + essentials real and **everything else
  STATUS_UNHANDLED (CMSG) / server-side (SMSG)** — AC's ~400 handlers are NOT rebound (deferred to
  per-feature tasks). Then migrated **all ~2,494 opcode-name call sites** across game+scripts to the
  3.4.3 names (full fork & replace, the user's chosen path); `game` compiles green. Plan + decomposition:
  `docs/superpowers/plans/2026-06-27-wotlk-classic-brick-b-opcodes.md`. Commits: `b8deffe1f` (enum+table),
  `a2fe41795` (PCH), `71a2ddb98` (Packets), `75e1a4c1c` (movement), `21cff25d0` (rest), + review fixes.
  ⚠ **Deferred (compiles, runtime-correctness owed to feature tasks):** ~45 removed opcodes stubbed to
  `UNKNOWN_OPCODE` with `TODO(3.4.3 brick-B)` (Warden, GM-ticket, `SMSG_DESTROY_OBJECT`, vehicle-data,
  battlefield-mgr, …); movement protocol-redesign TODOs (force-ack/spline/`SMSG_MOVE_UPDATE` broadcasts,
  Phase-1d); a few coarse-but-directional renames (item-query→`SMSG_DB_REPLY`, quests-completed→
  `SMSG_ALL_ACHIEVEMENT_DATA`, combo→`SMSG_AURA_UPDATE`). Opcode VALUES are TC-61581; **the ~10 handshake
  opcodes must be verified against the real 54261 client at brick E** (HermesProxy values are not wire-format).
- ✅ **C — `AuthenticationPackets.{h,cpp}` (done):** ported TC's full `WorldPackets::Auth` set
  (Ping/Pong, AuthChallenge, AuthSession, AuthContinuedSession, ConnectTo, EnterEncryptedMode,
  AuthResponse, WaitQueue*, ResumeComms, ConnectToFailed, QueuedMessagesEnd) byte-faithfully; `game`
  compiles. **New crypto wrappers** (C1): `Acore::Crypto::RsaSignature` (RSA-2048/SHA-256, for
  `SMSG_CONNECT_TO`) + `Acore::Crypto::Ed25519` (`SignWithContext` = Ed25519ctx, for
  `SMSG_ENTER_ENCRYPTED_MODE`); `HMAC_SHA512` already existed. Added `PacketUtilities` `Bits<N>`/`As<T>`/
  `Timestamp<T>` + `ByteBuffer::WriteString`/`ReadString`. RSA PEM + Ed25519 key + EnableEncryption
  seed(32)/context(16) constants copied **verbatim** from TC (pair with the patched client's embedded
  keys). Commits `ef16cf8aa` (C1), `daa629cbd` (C2), + Ed25519 OpenSSL≥3.2 guard. **Ed25519ctx KAT:**
  validated `SignWithContext` against the RFC 8032 §7.2 vector — **byte-exact PASS** (the one
  re-implemented, not-transcribed crypto path; de-risks the EnterEncryptedMode handshake). ⚠ AuthResponse
  game-data (class/template/virtual-realm arrays) is stubbed empty — populate at brick D / login-sequence.
  ⚠ Ed25519ctx uses OpenSSL 3.2+ EVP params (build is 3.6.3), guarded by `static_assert`.
- ✅ **D — modern `WorldSession` + account schema (done; login-holder wiring deferred to E):**
  - **D1 (schema, `16f213f5c`):** added `account.session_key_bnet binary(64)` + `client_build` +
    `timezone_offset`; fixed `BnetRealmList::JoinRealm` to store the **full 64-byte** key + build + tz
    (was truncating to 40 — resolves 🔴 D1/C1); pre-staged the brick-E LoginDatabase statements
    (`LOGIN_SEL_ACCOUNT_INFO_FOR_WORLD_AUTH`, `LOGIN_SEL/UPD_ACCOUNT_INFO_CONTINUED_SESSION`).
  - **D2 (WorldSession, `23d048542`):** `m_Socket` → `m_Socket[MAX_CONNECTION_TYPES]` (realm+instance);
    added `ConnectToKey` union + `_instanceConnectKey` + `SendConnectToInstance` + static
    `AddInstanceConnection` (faithful) + minimal `HandleContinuePlayerLogin`/`AbortLogin`; union ctor
    with `battlenetAccountId`/`os`/`timezoneOffset`/`build`/`clientBuildVariant`; reused
    `ClientBuild::VariantId`. `game` compiles. **Brick-E owes:** split `HandlePlayerLoginOpcode` →
    `SendConnectToInstance(WorldAttempt1)` + defer the LoginQueryHolder into `HandleContinuePlayerLogin`
    (and make `m_playerLoading` an `ObjectGuid`); real ctor values from the handshake; `SendPacket`
    routing by `ConnectionType`; wire `AddInstanceConnection` on the 2nd socket's `CMSG_AUTH_CONTINUED_SESSION`.
- **E — the `WorldSocket` + `WorldSocketMgr`/`Main` on `Acore::Net`** (depends on A–D; **user chose the
  full Acore::Net migration**). Prep subsystems already existed from the bnet work: `SessionKeyGenerator`,
  the JSON `RealmList::RealmJoinTicket` proto + `ProtobufJSON`, `ClientBuild::VariantId/AuthKey/Info` (✅).
  - ✅ **E1 (Acore::Net migration + framing, `715ef5608`):** rebased `WorldSocket` on
    `Acore::Net::Socket<>` (plain TCP — world isn't TLS); the **V2 banner** (exact strings, via AC's
    existing `SocketConnectionInitializer` chain); modern wire headers `PacketHeader{Size,Tag[12]}` (16B
    out) / `IncomingPacketHeader{…,EncryptedOpcode}` (20B in); the `ReadHeaderHandler`/`ReadDataHandler`
    AES-GCM flow + `WritePacketToBuffer`; swapped `AuthCrypt`(ARC4)→`WorldPacketCrypt`(AES-256-GCM);
    `WorldSocketMgr` on `Acore::Net::SocketMgr` (custom `WorldSocketThread`). **`worldserver.exe` builds
    + links green.** Also fixed deferred **N2** (legacy vs modern `Socket.h` collision: isolated the RA
    acceptor into `RemoteAccess/RAAcceptor.*` + include ordering). Auth crypto stubbed (`TODO brick-E2`).
  - ✅ **E2a (realm-socket auth handshake, `fc7cea331`):** `SendAuthSession` → real `AuthChallenge`
    (`_serverChallenge` 32B); `HandleAuthSession` parses `WorldPackets::Auth::AuthSession` + JSON
    `RealmJoinTicket`, queries `LOGIN_SEL_ACCOUNT_INFO_FOR_WORLD_AUTH`; `HandleAuthSessionCallback` does
    the digest `HMAC_SHA512(SHA512(KeyData‖authKey))(LocalChallenge‖serverChallenge‖AuthCheckSeed)`,
    session-key `SessionKeyGenerator<SHA512>(HMAC_SHA512(SHA512(KeyData))(serverChallenge‖LocalChallenge‖
    SessionKeySeed))` → 40B, encrypt-key `HMAC_SHA512(_sessionKey)(LocalChallenge‖serverChallenge‖
    EncryptionKeySeed)`[:32], persists via `LOGIN_UPD_ACCOUNT_INFO_CONTINUED_SESSION`, builds the modern
    WorldSession, sends `SMSG_ENTER_ENCRYPTED_MODE`; `HandleEnterEncryptedModeAck` → `_authCrypt.Init` +
    `AddSession`. **4 seeds copied verbatim; crypto reviewed byte-faithful to TC** (incl. the reversed
    session-key operand order). Also wired `ConnectTo/EnterEncryptedMode::InitializeEncryption()` into
    `Main.cpp` (RSA/Ed25519 signers). `game`+`worldserver` link green. **This is the first live-testable
    milestone** (realm-auth → character-select / Phase-1b gate).
  - 🔜 **E2b (world-entry two-socket flow):** `HandleAuthContinuedSession[Callback]` (uses
    `ContinuedSessionSeed`, already added), `WorldSessionMgr::AddInstanceSocket` registry, the
    **login-holder split** (`HandlePlayerLoginOpcode` → `SendConnectToInstance(WorldAttempt1)` → 2nd
    socket `CMSG_AUTH_CONTINUED_SESSION` → `AddInstanceConnection`/`HandleContinuePlayerLogin` holder load;
    `m_playerLoading` bool→`ObjectGuid`), Bnet RpcErrorCode mapping. Needed for *entering the world with a
    character* (Phase-1c), not for char-select. **Do this AFTER the E2a live test confirms auth works.**
  - 🔜 **E-verify — live test (do NOW after E2a).** Open gates this resolves: **(1)** does 54261 require a
    non-empty per-variant `build_auth_key`? (we derive with an empty key + WARN log — if the client needs
    one, the digest mismatches and it disconnects); **(2)** `KeyData` byte-order from the bnet `JoinRealm`
    handoff (writer `LOGIN_UPD_BNET_GAME_ACCOUNT_WORLD_HANDOFF` vs this reader); **(3)** the TC-61581 opcode
    VALUES + the RSA/Ed25519 signatures. Digest-mismatch is self-diagnosing (logs authKeyLen + first bytes).
    ⚠ **worldserver-boot caveat:** the worldserver loads DBC/map data at boot before `StartNetwork`; the
    3.4.3 server-side DB2 store layer is Phase 1c (unbuilt). It may need the stock 3.3.5a `dbc/`+`maps/`
    present just to boot far enough to accept the world connection (the handshake itself doesn't use DBC).
    Proxy-protocol parity (N3) remains its own deferred task. Session key + crypt live on the **socket**.
- Then (Phase 1c): **server-side DB2 store layer** (~30-40 world-entry tables, WoWDBDefs method),
  regenerated **UpdateFields** for 54261, login-sequence packets, movement, chat, object spawning.

The `WorldSession↔WorldSocket` compat surface is small (`CloseSocket`/`SendPacket`/`IsOpen`/
`SetPacketLogging`) — that part is NOT the blocker; the WorldPacket/opcode rewrite is.

---

## 8. Companion docs & references

- Design spec: `docs/superpowers/specs/2026-06-26-azerothcore-wotlk-classic-3.4.3-design.md`
- Implementation plan: `docs/superpowers/plans/2026-06-26-wotlk-classic-3.4.3-phase0-phase1.md`
- Branch `feature/wotlk-classic-3.4.3` on the `fork` remote (~24 commits).
- Local TC reference checkout: `C:\Users\deves\AppData\Local\tc-wotlk-ref` (sparse).
- wow-patcher build: `D:\workspace\wow-patcher-build\src\target\release\wow-patcher.exe`.
