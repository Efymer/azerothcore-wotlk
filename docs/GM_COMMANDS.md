# AzerothCore GM Command Reference

Complete list of in-game / console commands (699 total), generated from `data/sql/base/db_world/command.sql`.

Prefix commands with `.` in-game (e.g. `.account create`) or type them directly at the `AC>` server console (without the dot).

## Security levels

| Level | Name | Commands |
|-------|------|----------|
| 0 | Player | 22 |
| 1 | Moderator | 69 |
| 2 | Game Master | 252 |
| 3 | Administrator | 341 |
| 4 | Console only | 15 |

> Level = minimum account security required (legacy `command.security`). Actual access is governed by RBAC permissions, which default to these levels. Level 4 commands are reserved for the server console and cannot be granted to a player account.

## Table of contents

[`account`](#account) · [`achievement`](#achievement) · [`additem`](#additem) · [`announce`](#announce) · [`appear`](#appear) · [`arena`](#arena) · [`aura`](#aura) · [`autobroadcast`](#autobroadcast) · [`bags`](#bags) · [`ban`](#ban) · [`baninfo`](#baninfo) · [`banlist`](#banlist) · [`bf`](#bf) · [`bindsight`](#bindsight) · [`bm`](#bm) · [`cache`](#cache) · [`cast`](#cast) · [`character`](#character) · [`chatfilter`](#chatfilter) · [`cheat`](#cheat) · [`combatstop`](#combatstop) · [`cometome`](#cometome) · [`commands`](#commands) · [`commentator`](#commentator) · [`cooldown`](#cooldown) · [`damage`](#damage) · [`debug`](#debug) · [`deserter`](#deserter) · [`dev`](#dev) · [`die`](#die) · [`disable`](#disable) · [`dismount`](#dismount) · [`distance`](#distance) · [`event`](#event) · [`flusharenapoints`](#flusharenapoints) · [`freeze`](#freeze) · [`gear`](#gear) · [`gm`](#gm) · [`gmannounce`](#gmannounce) · [`gmnameannounce`](#gmnameannounce) · [`gmnotify`](#gmnotify) · [`go`](#go) · [`gobject`](#gobject) · [`gps`](#gps) · [`group`](#group) · [`groupsummon`](#groupsummon) · [`guid`](#guid) · [`guild`](#guild) · [`help`](#help) · [`hidearea`](#hidearea) · [`honor`](#honor) · [`instance`](#instance) · [`inventory`](#inventory) · [`item`](#item) · [`kick`](#kick) · [`learn`](#learn) · [`levelup`](#levelup) · [`lfg`](#lfg) · [`linkgrave`](#linkgrave) · [`list`](#list) · [`lookup`](#lookup) · [`mail`](#mail) · [`mailbox`](#mailbox) · [`maxskill`](#maxskill) · [`mmap`](#mmap) · [`modify`](#modify) · [`morph`](#morph) · [`movegens`](#movegens) · [`mute`](#mute) · [`mutehistory`](#mutehistory) · [`nameannounce`](#nameannounce) · [`neargrave`](#neargrave) · [`notify`](#notify) · [`npc`](#npc) · [`opendoor`](#opendoor) · [`packetlog`](#packetlog) · [`pdump`](#pdump) · [`pet`](#pet) · [`pinfo`](#pinfo) · [`playall`](#playall) · [`player`](#player) · [`pool`](#pool) · [`pooltools`](#pooltools) · [`possess`](#possess) · [`quest`](#quest) · [`rbac`](#rbac) · [`recall`](#recall) · [`reload`](#reload) · [`reset`](#reset) · [`respawn`](#respawn) · [`revive`](#revive) · [`save`](#save) · [`saveall`](#saveall) · [`send`](#send) · [`server`](#server) · [`setskill`](#setskill) · [`settings`](#settings) · [`showarea`](#showarea) · [`skirmish`](#skirmish) · [`spect`](#spect) · [`spellinfo`](#spellinfo) · [`string`](#string) · [`summon`](#summon) · [`teleport`](#teleport) · [`ticket`](#ticket) · [`titles`](#titles) · [`unaura`](#unaura) · [`unban`](#unban) · [`unbindsight`](#unbindsight) · [`unfreeze`](#unfreeze) · [`unlearn`](#unlearn) · [`unmute`](#unmute) · [`unpossess`](#unpossess) · [`unstuck`](#unstuck) · [`wchange`](#wchange) · [`whispers`](#whispers) · [`worldstate`](#worldstate) · [`wp`](#wp) · [`wpgps`](#wpgps)

---

## account

| Command | Level | Description |
|---------|-------|-------------|
| `.account` | 0 (Player) | Syntax: .account Display the access level of your account and the email adress if you possess the necessary permissions. |
| `.account 2fa` | 0 (Player) | Syntax: .account 2fa <setup/remove> |
| `.account 2fa remove` | 0 (Player) | Syntax: .account 2fa remove <token> Disables two-factor authentication for this account, if enabled. |
| `.account 2fa setup` | 0 (Player) | Syntax: .account 2fa setup Sets up two-factor authentication for this account. |
| `.account addon` | 1 (Moderator) | Syntax: .account addon #addon Set expansion addon level allowed. Addon values: 0 - normal, 1 - tbc, 2 - wotlk. |
| `.account create` | 4 (Console only) | Syntax: .account create $account $password $email Create account and set password to it. $email is optional, can be left blank. |
| `.account delete` | 4 (Console only) | Syntax: .account delete $account Delete account with all characters. |
| `.account lock country` | 0 (Player) | Syntax: .account lock country [on\|off] Allow login from account only from current used Country or remove this requirement. |
| `.account lock ip` | 0 (Player) | Syntax: .account lock ip [on\|off] Allow login from account only from current used IP or remove this requirement. |
| `.account onlinelist` | 4 (Console only) | Syntax: .account onlinelist Show list of online accounts. |
| `.account password` | 0 (Player) | Syntax: .account password $old_password $new_password $new_password [$email] Change your account password. You may need to check the actual security mode to see if email input is necessary. |
| `.account remove country` | 3 (Administrator) | Syntax: .account remove country <account> Removes the country information associated with the specified account. |
| `.account set` | 2 (Game Master) | Syntax: .account set $subcommand Type .account set to see the list of possible subcommands or .help account set $subcommand to see info on subcommands |
| `.account set 2fa` | 0 (Player) | Syntax: .account set 2fa <account> <secret/off> Provide a base32 encoded secret to setup two-factor authentication for the account. Specify 'off' to disable two-factor authentication for the account. |
| `.account set addon` | 2 (Game Master) | Syntax: .account set addon [$account] #addon Set user (possible targeted) expansion addon level allowed. Addon values: 0 - normal, 1 - tbc, 2 - wotlk. |
| `.account set email` | 3 (Administrator) | Syntax: .account set email $account $email $email_confirmation Add or change an email to the account. |
| `.account set gmlevel` | 3 (Administrator) | Syntax: .account set gmlevel [$account] #level [#realmid] Set the security level for targeted player (can't be used at self) or for account $name to a level of #level on the realm #realmID. #level may range from 0 to 3. #reamID may be -1 for all realms. |
| `.account set password` | 3 (Administrator) | Syntax: .account set password $account $password $password Set password for account. |

## achievement

| Command | Level | Description |
|---------|-------|-------------|
| `.achievement` | 2 (Game Master) | Syntax: .achievement $subcommand Type .achievement to see the list of possible subcommands or .help achievement $subcommand to see info on subcommands |
| `.achievement add` | 2 (Game Master) | Syntax: .achievement add $achievement Add an achievement to the targeted player. $achievement: can be either achievement id or achievement link |
| `.achievement checkall` | 3 (Administrator) | Syntax: .achievement checkall Check all achievement criteria of the selected player. |

## additem

| Command | Level | Description |
|---------|-------|-------------|
| `.additem` | 2 (Game Master) | Syntax: .additem Optional(playerName/playerGUID) #itemID/[#itemName]/#itemLink #itemCount Adds the specified item to you, the selected character or the specifed character name/GUID. If #itemCount is negative, you will remove #itemID. |
| `.additem set` | 2 (Game Master) | Syntax: .additemset #itemsetid Add items from itemset of id #itemsetid to your or selected character inventory. Will add by one example each item from itemset. |

## announce

| Command | Level | Description |
|---------|-------|-------------|
| `.announce` | 2 (Game Master) | Syntax: .announce $MessageToBroadcast Send a global message to all players online in chat log. |

## appear

| Command | Level | Description |
|---------|-------|-------------|
| `.appear` | 1 (Moderator) | Syntax: .appear [$charactername] Teleport to the given character. Either specify the character name or click on the character's portrait,e.g. when you are in a group. Character can be offline. |

## arena

| Command | Level | Description |
|---------|-------|-------------|
| `.arena captain` | 3 (Administrator) | Syntax: .arena captain #TeamID $name A command to set new captain to the team $name must be in the team |
| `.arena create` | 3 (Administrator) | Syntax: .arena create $name "arena name" #type A command to create a new Arena-team in game. #type  = [2/3/5] |
| `.arena disband` | 3 (Administrator) | Syntax: .arena disband #TeamID A command to disband Arena-team in game. |
| `.arena info` | 2 (Game Master) | Syntax: .arena info #TeamID A command that show info about arena team |
| `.arena lookup` | 2 (Game Master) | Syntax: .arena lookup $name A command that give a list of arenateam with the given $name |
| `.arena rename` | 3 (Administrator) | Syntax: .arena rename "oldname" "newname" A command to rename Arena-team name. |
| `.arena season deleteteams` | 3 (Administrator) | Syntax: .arena season deleteteams Deletes ALL arena teams. |
| `.arena season reward` | 3 (Administrator) | Syntax: .arena season reward $brackets Builds a ladder by combining team brackets and provides rewards from the arena_season_reward table. Example usage: # Combine all brackets, build a ladder, and distribute rewards among them .arena season reward all # Build ladders separately for 2v2, 3v3, and... |
| `.arena season set state` | 3 (Administrator) | Syntax: .arena season set state $state Changes the state for the current season. Available states: 0 - disabled. Players can't queue for the arena. 1 - in progress. Players can use arena-related functionality. |
| `.arena season start` | 3 (Administrator) | Syntax: .arena season start $season_id Starts a new arena season, places the correct vendors, and sets the new season state to IN PROGRESS. |

## aura

| Command | Level | Description |
|---------|-------|-------------|
| `.aura` | 2 (Game Master) | Syntax: .aura #spellid Add the aura from spell #spellid to the selected Unit. |
| `.aura stack` | 2 (Game Master) | Syntax: .aurastack #spellid #stacks Modify #stacks of an already applied #spellid to the selected Unit. |

## autobroadcast

| Command | Level | Description |
|---------|-------|-------------|
| `.autobroadcast` | 2 (Game Master) | Syntax: .autobroadcast $subcommand Type .autobroadcast to see a list of subcommands or .help autobroadcast $subcommand to see info on subcommands. |
| `.autobroadcast add` | 3 (Administrator) | Syntax: .autobroadcast add $weight $text Add a new autobroadcast entry with the given weight and text. |
| `.autobroadcast list` | 2 (Game Master) | Syntax: .autobroadcast list List all autobroadcast entries. |
| `.autobroadcast locale` | 3 (Administrator) | Syntax: .autobroadcast locale $id $locale $text Add or replace a localized text for the autobroadcast entry with the given ID. |
| `.autobroadcast remove` | 3 (Administrator) | Syntax: .autobroadcast remove $id Remove the autobroadcast entry with the given ID and its locale entries. |

## bags

| Command | Level | Description |
|---------|-------|-------------|
| `.bags` | 2 (Game Master) | Syntax: .bags $subcommand Type .bags to see the list of possible subcommands or .help bags $subcommand to see info on subcommands |
| `.bags clear` | 2 (Game Master) | Syntax: .bags clear $itemQuality Clear from players' bags all items including and below $itemQuality (or all items if used .bags clear all). |

## ban

| Command | Level | Description |
|---------|-------|-------------|
| `.ban` | 2 (Game Master) | Syntax: .ban $subcommand Type .ban to see the list of possible subcommands or .help ban $subcommand to see info on subcommands |
| `.ban account` | 2 (Game Master) | Syntax: .ban account $Name $bantime $reason Ban account kick player. $bantime: negative value leads to permban, otherwise use a timestring like "4d20h3s". |
| `.ban character` | 2 (Game Master) | Syntax: .ban character $Name $bantime $reason Ban character and kick player. $bantime: negative value leads to permban, otherwise use a timestring like "4d20h3s". |
| `.ban ip` | 2 (Game Master) | Syntax: .ban ip $Ip $bantime $reason Ban IP. $bantime: negative value leads to permban, otherwise use a timestring like "4d20h3s". |
| `.ban playeraccount` | 2 (Game Master) | Syntax: .ban playeraccount $Name $bantime $reason Ban account and kick player. $bantime: negative value leads to permban, otherwise use a timestring like "4d20h3s". |

## baninfo

| Command | Level | Description |
|---------|-------|-------------|
| `.baninfo` | 2 (Game Master) | Syntax: .baninfo $subcommand Type .baninfo to see the list of possible subcommands or .help baninfo $subcommand to see info on subcommands |
| `.baninfo account` | 2 (Game Master) | Syntax: .baninfo account $accountid Watch full information about a specific ban. |
| `.baninfo character` | 2 (Game Master) | Syntax: .baninfo character $charactername Watch full information about a specific ban. |
| `.baninfo ip` | 2 (Game Master) | Syntax: .baninfo ip $ip Watch full information about a specific ban. |

## banlist

| Command | Level | Description |
|---------|-------|-------------|
| `.banlist` | 2 (Game Master) | Syntax: .banlist $subcommand Type .banlist to see the list of possible subcommands or .help banlist $subcommand to see info on subcommands |
| `.banlist account` | 2 (Game Master) | Syntax: .banlist account [$Name] Searches the banlist for a account name pattern or show full list account bans. |
| `.banlist character` | 2 (Game Master) | Syntax: .banlist character $Name Searches the banlist for a character name pattern. Pattern required. |
| `.banlist ip` | 2 (Game Master) | Syntax: .banlist ip [$Ip] Searches the banlist for a IP pattern or show full list of IP bans. |

## bf

| Command | Level | Description |
|---------|-------|-------------|
| `.bf` | 3 (Administrator) | Syntax: .bf $subcommand Type .bf to see the list of possible subcommands or .help bf $subcommand to see info on subcommands. |
| `.bf enable` | 3 (Administrator) | Syntax: .bf enable [#battleid] #battleid is optional and defaults to 1 (Wintergrasp). |
| `.bf queue` | 2 (Game Master) | Syntax: .bf queue [#battleid] Displays all players currently in queue, invited, or actively in war for the specified battlefield. #battleid is optional and defaults to 1 (Wintergrasp). |
| `.bf start` | 3 (Administrator) | Syntax: .bf start [#battleid] #battleid is optional and defaults to 1 (Wintergrasp). |
| `.bf stop` | 3 (Administrator) | Syntax: .bf stop [#battleid] #battleid is optional and defaults to 1 (Wintergrasp). |
| `.bf switch` | 3 (Administrator) | Syntax: .bf switch [#battleid] #battleid is optional and defaults to 1 (Wintergrasp). |
| `.bf timer` | 3 (Administrator) | Syntax: .bf timer [#battleid] #timer #battleid is optional and defaults to 1 (Wintergrasp). #timer: use a timestring like "1h15m30s". |

## bindsight

| Command | Level | Description |
|---------|-------|-------------|
| `.bindsight` | 3 (Administrator) | Syntax: .bindsight Binds vision to the selected unit indefinitely. Cannot be used while currently possessing a target. |

## bm

| Command | Level | Description |
|---------|-------|-------------|
| `.bm` | 2 (Game Master) | Syntax: .bm [on/off] Enable or Disable in game Beastmaster mode or show current state if on/off not provided. |

## cache

| Command | Level | Description |
|---------|-------|-------------|
| `.cache` | 1 (Moderator) | Character data cached during start up. Type .cache to see a list of subcommands or .help $subcommand to see info on subcommands. |
| `.cache delete` | 3 (Administrator) | Syntax: .cache delete $playerName Deletes the cached data for the selected character. Use for debugging only! |
| `.cache info` | 2 (Game Master) | Syntax: .cache info $playerName Displays cached data for the selected character. |
| `.cache refresh` | 2 (Game Master) | Syntax: .cache refresh $playerName Deletes the current cache and refreshes it with updated data. |

## cast

| Command | Level | Description |
|---------|-------|-------------|
| `.cast` | 2 (Game Master) | Syntax: .cast #spellid [triggered] Cast #spellid to selected target. If no target selected cast to self. If 'triggered' or part provided then spell cast with triggered flag. |
| `.cast back` | 2 (Game Master) | Syntax: .cast back #spellid [triggered] Selected target will cast #spellid to your character. If 'triggered' or part provided then spell cast with triggered flag. |
| `.cast dest` | 2 (Game Master) | Syntax: .cast dest #spellid #x #y #z [triggered] Selected target will cast #spellid at provided destination. If 'triggered' or part provided then spell cast with triggered flag. |
| `.cast dist` | 2 (Game Master) | Syntax: .cast dist #spellid [#dist [triggered]] You will cast spell to point at distance #dist. If 'triggered' or part provided then spell cast with triggered flag. Not all spells can be cast as area spells. |
| `.cast self` | 2 (Game Master) | Syntax: .cast self #spellid [triggered] Cast #spellid by target at target itself. If 'triggered' or part provided then spell cast with triggered flag. |
| `.cast target` | 2 (Game Master) | Syntax: .cast target #spellid [triggered] Selected target will cast #spellid to his victim. If 'triggered' or part provided then spell cast with triggered flag. |

## character

| Command | Level | Description |
|---------|-------|-------------|
| `.character` | 2 (Game Master) | Syntax: character $subcommand Type .character to see a list of possible subcommands or .help character $subcommand to see info on the subcommand. |
| `.character changeaccount` | 3 (Administrator) | Syntax: .character changeaccount $NewAccountName $Name. Moves the specified character to the provided account. Kicks the player if the character is online. |
| `.character changefaction` | 2 (Game Master) | Syntax: .character changefaction $name Change character faction. |
| `.character changerace` | 2 (Game Master) | Syntax: .character changerace $name Change character race. |
| `.character check bag` | 2 (Game Master) | Syntax: .character check bag [$target_player] #bagSlot 1 - 4 |
| `.character check bank` | 2 (Game Master) | Syntax: .character check bank Show your bank inventory. |
| `.character check profession` | 2 (Game Master) | Syntax: .character check profession [$target_player] Show known professions list for selected player |
| `.character customize` | 2 (Game Master) | Syntax: .character customize [$name] Mark selected in game or by $name in command character for customize at next login. |
| `.character deleted delete` | 4 (Console only) | Syntax: .character deleted delete #guid\|$name Completely deletes the selected characters. If $name is supplied, only characters with that string in their name will be deleted, if #guid is supplied, only the character with that GUID will be deleted. |
| `.character deleted list` | 3 (Administrator) | Syntax: .character deleted list [#guid\|$name] Shows a list with all deleted characters. If $name is supplied, only characters with that string in their name will be selected, if #guid is supplied, only the character with that GUID will be selected. |
| `.character deleted purge` | 4 (Console only) | Syntax: .character deleted purge [#keepDays] Completely removes all characters from the database that where deleted more than #keepDays ago. If #keepDays not provided the used value from worldserver.conf option 'CharDelete.KeepDays'. If 'CharDelete.KeepDays' option is disabled (set to value 0) th... |
| `.character deleted restore` | 3 (Administrator) | Syntax: .character deleted restore #guid\|$name [$newname] [#new account] Restores deleted characters. If $name is supplied, only characters with that string in their name will be restored, if $guid is supplied, only the character with that GUID will be restored. If $newname is set, the character... |
| `.character erase` | 4 (Console only) | Syntax: .character erase $name Delete character $name. Character finally deleted in case any deleting options. |
| `.character level` | 2 (Game Master) | Syntax: .character level [$playername] [#level] Set the level of character with $playername (or the selected if not name provided) by #numberoflevels Or +1 if no #numberoflevels provided). If #numberoflevels is omitted, the level will be increase by 1. If #numberoflevels is 0, the same level will... |
| `.character rename` | 2 (Game Master) | Syntax: .character rename [$name] [reserveName] [$newName] Mark selected in game or by $name in command character for rename at next login. If [reserveName] is 1 then the player's current name is added to the list of reserved names. If [newName] then the player will be forced rename. |
| `.character reputation` | 2 (Game Master) | Syntax: .character reputation [$player_name] Show reputation information for selected player or player find by $player_name. |
| `.character titles` | 2 (Game Master) | Syntax: .character titles [$player_name] Show known titles list for selected player or player find by $player_name. |

## chatfilter

| Command | Level | Description |
|---------|-------|-------------|
| `.chatfilter` | 2 (Game Master) | Syntax: .chatfilter $subcommand Type .chatfilter to see a list of subcommands or .help chatfilter $subcommand to see info on subcommands. |
| `.chatfilter add` | 3 (Administrator) | Syntax: .chatfilter add $word Add $word to the `chat_filter` table and reload the chat filter. |
| `.chatfilter list` | 2 (Game Master) | Syntax: .chatfilter list List all entries from the `chat_filter` table. |
| `.chatfilter remove` | 3 (Administrator) | Syntax: .chatfilter remove $word Remove $word from the `chat_filter` table and reload the chat filter. |

## cheat

| Command | Level | Description |
|---------|-------|-------------|
| `.cheat` | 2 (Game Master) | Syntax: .cheat $subcommand Type .cheat to see the list of possible subcommands or .help cheat $subcommand to see info on subcommands |
| `.cheat casttime` | 2 (Game Master) | Syntax: .cheat casttime [on/off] Remove spells' casting time. |
| `.cheat cooldown` | 2 (Game Master) | Syntax: .cheat cooldown [on/off] Disable spells' cooldowns. |
| `.cheat explore` | 2 (Game Master) | Syntax: .cheat explore #flag Reveal or hide all maps for the selected player. If no player is selected, hide or reveal maps to you. Use a #flag of value 1 to reveal, use a #flag value of 0 to hide all maps. |
| `.cheat god` | 2 (Game Master) | Syntax: .cheat god [on/off] Turn the user invulnerable. |
| `.cheat power` | 2 (Game Master) | Syntax: .cheat power [on/off] Remove spells' cost (mana, energy, rage...). |
| `.cheat status` | 2 (Game Master) | Syntax: .cheat status Shows the cheats you currently have enabled. |
| `.cheat taxi` | 2 (Game Master) | Syntax: .cheat taxi on/off Temporary grant access to all taxi routes for the selected character. If no character is selected, hide or reveal all routes to you. Visited taxi nodes are still accessible after removing access. |
| `.cheat waterwalk` | 2 (Game Master) | Syntax: .cheat waterwalk on/off Allow to walk on water (self or selected character). |

## combatstop

| Command | Level | Description |
|---------|-------|-------------|
| `.combatstop` | 2 (Game Master) | Syntax: .combatstop [$playername] Stop combat for selected character. If selected non-player then command applied to self. If $playername provided then attempt applied to online player $playername. |

## cometome

| Command | Level | Description |
|---------|-------|-------------|
| `.cometome` | 3 (Administrator) | Syntax: .cometome $parameter Make selected creature come to your current location (new position not saved to DB). |

## commands

| Command | Level | Description |
|---------|-------|-------------|
| `.commands` | 0 (Player) | Syntax: .commands Display a list of available commands for your account level. |

## commentator

| Command | Level | Description |
|---------|-------|-------------|
| `.commentator` | 1 (Moderator) | Syntax: .commentator [on/off] Enable or Disable in game Commentator tag or show current state if on/off not provided. |

## cooldown

| Command | Level | Description |
|---------|-------|-------------|
| `.cooldown` | 2 (Game Master) | Syntax: .cooldown [#spell_id] Remove all (if spell_id not provided) or #spel_id spell cooldown from selected character or you (if no selection). |

## damage

| Command | Level | Description |
|---------|-------|-------------|
| `.damage` | 2 (Game Master) | Syntax: .damage $damage_amount [$school [$spellid]] Apply $damage to target. If not $school and $spellid provided then this flat clean melee damage without any modifiers. If $school provided then damage modified by armor reduction (if school physical), and target absorbing modifiers and result ap... |

## debug

| Command | Level | Description |
|---------|-------|-------------|
| `.debug` | 2 (Game Master) | Syntax: .debug $subcommand Type .debug to see the list of possible subcommands or .help debug $subcommand to see info on subcommands |
| `.debug Mod32Value` | 3 (Administrator) | Syntax: .debug Mod32Value #field #value Add #value to field #field of your character. |
| `.debug anim` | 3 (Administrator) | TODO |
| `.debug areatriggers` | 3 (Administrator) | Syntax: .debug areatriggers Toggle debug mode for areatriggers. In debug mode GM will be notified if reaching an areatrigger |
| `.debug arena` | 3 (Administrator) | Syntax: .debug arena Toggle debug mode for arenas. In debug mode GM can start arena with single player. |
| `.debug bg` | 3 (Administrator) | Syntax: .debug bg Toggle debug mode for battlegrounds. In debug mode GM can start battleground with single player. |
| `.debug boundary` | 3 (Administrator) | Syntax: .debug boundary [duration] [fill] [z] Optional arguments: - duration: Duration in ms (default: 5000, max: 180000). - fill: Fills the boundary with markers. - z: Includes z-axis in visualization. |
| `.debug combat` | 3 (Administrator) | Syntax: .debug combat Lists PvP and PvE combat references of the selected unit (or self). |
| `.debug cooldown` | 3 (Administrator) | Syntax: .debug cooldown #spellID #cooldownTime #itemID Apply a cooldown of the given duration (in milliseconds) for the given spell and item ID. |
| `.debug dummy` | 3 (Administrator) | Syntax: .debug dummy <???> Catch-all debug command. Does nothing by default. If you want it to do things for testing, add the things to its script in cs_debug.cpp. |
| `.debug entervehicle` | 3 (Administrator) | Syntax: .debug entervehicle #entry #seatID Enter the targeted or given vehicle ID in the given seat. |
| `.debug factionchange` | 3 (Administrator) | Syntax: .debug factionchange [$playerName] Checks all faction/race change requirements for the target player and reports pass/fail for each condition (AT_LOGIN flags, guild, arena captain, mail, auctions, gold limit). |
| `.debug getitemstate` | 3 (Administrator) | Syntax: .debug getitemstate #itemState [unchanged/changed/new/removed/queue/check_all] Returns all items in a given player's inventory with a given item state. |
| `.debug getitemvalue` | 3 (Administrator) | Syntax: .debug getitemvalue #GUID #index Returns the value of the given index for the given item GUID. |
| `.debug getvalue` | 3 (Administrator) | Syntax: .debug getvalue #index #isInt Returns either an integer or float value at a given index of your target. |
| `.debug hostile` | 3 (Administrator) | Syntax: .debug hostile Returns the hostile reference list of a given player. |
| `.debug itemexpire` | 3 (Administrator) | Syntax: .debug itemexpire #GUID Destroy an item with the given GUID. |
| `.debug lfg` | 3 (Administrator) | Syntax: .debug lfg Toggle debug mode for lfg. In debug mode GM can start lfg queue with one player. |
| `.debug loot` | 2 (Game Master) | Syntax: .debug loot <type> <id> [count] Simulates loot generation for the given loot type and ID, outputting the results to chat without creating items. Optional count (1-100) repeats the simulation and shows aggregated drop rates. Valid types: creature, gameobject, fishing, item, pickpocketing,... |
| `.debug lootrecipient` | 3 (Administrator) | Syntax: .debug lootrecipient Returns the loot recipient of the targeted creature. |
| `.debug los` | 3 (Administrator) | Syntax: .debug los Returns line of sight status between you and your target. |
| `.debug mapdata` | 3 (Administrator) | Syntax: .debug mapdata Displays debug information about the current map. |
| `.debug moveflags` | 3 (Administrator) | Syntax: .debug moveflags [$newMoveFlags [$newMoveFlags2]] No params given will output the current moveflags of the target |
| `.debug objectcount` | 3 (Administrator) | Syntax: .debug objectcount <optional map id> Shows the number of Creatures and GameObjects for the specified map id or for all maps if none is specified |
| `.debug play` | 1 (Moderator) | Syntax: .debug play $subcommand Type .debug play to see the list of possible subcommands or .help debug play $subcommand to see info on subcommands. |
| `.debug play cinematic` | 3 (Administrator) | Syntax: .debug play cinematic #cinematicid Play cinematic #cinematicid for you. You stay at place while your mind fly. |
| `.debug play movie` | 3 (Administrator) | Syntax: .debug play movie #movieid Play movie #movieid for you. |
| `.debug play music` | 3 (Administrator) | Syntax: .debug play music <musicId> Play music with <musicId>. Music will be played only for you. Other players will not hear this. |
| `.debug play sound` | 3 (Administrator) | Syntax: .debug play sound #soundid Play sound with #soundid. Sound will be play only for you. Other players do not hear this. Warning: client may have more 5000 sounds... |
| `.debug play visual` | 3 (Administrator) | Syntax: .debug play visual #visualid Play spell visual with #visualid. #visualid refers to the ID from SpellVisualKit.dbc |
| `.debug send` | 3 (Administrator) | Syntax: .debug send $subcommand Type .debug send to see the list of possible subcommands or .help debug send $subcommand to see info on subcommands. |
| `.debug send buyerror` | 3 (Administrator) | Syntax: .debug send buyerror #error Sends the given buy error result. |
| `.debug send channelnotify` | 3 (Administrator) | Syntax: .debug send channelnotify #type Sends a channel notify message of the given type. |
| `.debug send chatmessage` | 3 (Administrator) | Syntax: .debug send chatmessage #type Sends a chat message of the given type. |
| `.debug send equiperror` | 3 (Administrator) | Syntax: .debug send equiperror #error Sends the given equip error result. |
| `.debug send largepacket` | 3 (Administrator) | Syntax: .debug send largepacket Sends a system message of 128 kilobytes. |
| `.debug send opcode` | 3 (Administrator) | Syntax: .debug send opcode Sends opcodes contained in "opcode.txt". |
| `.debug send qinvalidmsg` | 3 (Administrator) | Syntax: .debug send qinvalidmsg #error Sends the given quest error result. |
| `.debug send qpartymsg` | 3 (Administrator) | Syntax: .debug send qpartymsg #message Sends the given party quest share message. |
| `.debug send sellerror` | 3 (Administrator) | Syntax: .debug send sellerror #error Sends the given sell error result. |
| `.debug send setphaseshift` | 3 (Administrator) | Syntax: .debug send setphaseshift #phaseShift Sends a phase shift message with the given phase shift value. |
| `.debug send spellfail` | 3 (Administrator) | Syntax: .debug send spellfail #result #failArgument1 #failArgument2 Sends a spell failure message with the given result and argument values. |
| `.debug setaurastate` | 3 (Administrator) | Syntax: .debug setaurastate #state #apply Sets the selected units aura state using the given apply value. |
| `.debug setbit` | 3 (Administrator) | Syntax: .debug setbit #index #bit Sets the unsigned 32-bit integer value of the target at the given index to the given bit. |
| `.debug setitemvalue` | 3 (Administrator) | Syntax: .debug setitemvalue #GUID #index #value Sets the value of the given index for the given item GUID to the given value. |
| `.debug setvalue` | 3 (Administrator) | Syntax: .debug setvalue #index #value Sets the unsigned 32-bit integer or float value of the target at the given index to the given value. |
| `.debug setvid` | 3 (Administrator) | Syntax: .debug setvid #ID Currently disabled. Sets the given target's vehicle ID to the given value. |
| `.debug spawnvehicle` | 3 (Administrator) | Syntax: .debug spawnvehicle #entry #ID Creates a vehicle with the given ID. |
| `.debug threat` | 3 (Administrator) | Syntax: .debug threat Returns the threat list of a given creature. |
| `.debug threatinfo` | 3 (Administrator) | Syntax: .debug threatinfo Displays various debug information about the target's threat state, modifiers, redirects and similar. |
| `.debug unitstate` | 3 (Administrator) | Syntax: .debug unitstate [#unitstate] Sets the unit state for the selected unit or displays current unit and react state. |
| `.debug update` | 3 (Administrator) | Syntax: .debug update #index #value Sets the unsigned 32-bit integer value of the target at the given index to the given bit. |
| `.debug uws` | 3 (Administrator) | Syntax: .debug uws #variable #value Sends a worldstate update for the given variable to the given value. |
| `.debug visibilitydata` | 3 (Administrator) | Syntax: .debug visibilitydata Displays debug information related to object visibility around the player. |
| `.debug zonestats` | 1 (Moderator) | .debug zonestats [$playerName] Displays the amount of players in the player's current zone. |

## deserter

| Command | Level | Description |
|---------|-------|-------------|
| `.deserter bg add` | 3 (Administrator) | Syntax: .deserter bg add $playerName <$time> Adds the bg deserter debuff to a player or your target with $time. Optional $time: use a timestring like "1h15m30s".Default: 15m |
| `.deserter bg remove` | 3 (Administrator) | Syntax: .deserter bg remove $playerName Removes the bg deserter debuff from a player or your target. |
| `.deserter bg remove all` | 3 (Administrator) | Syntax: .deserter bg remove all <$maxDuration> Removes the bg deserter debuff from all online and offline players. Optional $maxDuration sets the maximum duration to be removed. Use a timestring like "1h45m". "-1" for any duration. Default: 15m |
| `.deserter instance add` | 3 (Administrator) | Syntax: .deserter instance add $playerName <$time> Adds the instance deserter debuff to a player or your target with $time. Optional $time: use a timestring like "1h15m30s". Default: 30m |
| `.deserter instance remove` | 3 (Administrator) | Syntax: .deserter instance remove $playerName Removes the instance deserter debuff from a player or your target. |
| `.deserter instance remove all` | 3 (Administrator) | Syntax: .deserter instance remove all <$maxDuration> Removes the instance deserter debuff from all online and offline players. Optional $maxDuration sets the maximum duration to be removed. Use a timestring like "1h45m". "-1" for any duration. Default: 30m |

## dev

| Command | Level | Description |
|---------|-------|-------------|
| `.dev` | 3 (Administrator) | Syntax: .dev [on/off] Enable or Disable in game Dev tag or show current state if on/off not provided. |

## die

| Command | Level | Description |
|---------|-------|-------------|
| `.die` | 2 (Game Master) | Syntax: .die Kill the selected player. If no player is selected, it will kill you. |

## disable

| Command | Level | Description |
|---------|-------|-------------|
| `.disable add battleground` | 3 (Administrator) | Syntax: .disable add battleground $entry $flag $comment |
| `.disable add map` | 3 (Administrator) | Syntax: .disable add map $entry $flag $comment |
| `.disable add outdoorpvp` | 3 (Administrator) | Syntax: .disable add outdoorpvp $entry $flag $comment |
| `.disable add quest` | 3 (Administrator) | Syntax: .disable add quest $entry $flag $comment |
| `.disable add spell` | 3 (Administrator) | Syntax: .disable add spell $entry $flag $comment |
| `.disable add vmap` | 3 (Administrator) | Syntax: .disable add vmap $entry $flag $comment |
| `.disable remove battleground` | 3 (Administrator) | Syntax: .disable remove battleground $entry |
| `.disable remove map` | 3 (Administrator) | Syntax: .disable remove map $entry |
| `.disable remove outdoorpvp` | 3 (Administrator) | Syntax: .disable remove outdoorpvp $entry |
| `.disable remove quest` | 3 (Administrator) | Syntax: .disable remove quest $entry |
| `.disable remove spell` | 3 (Administrator) | Syntax: .disable remove spell $entry |
| `.disable remove vmap` | 3 (Administrator) | Syntax: .disable remove vmap $entry |

## dismount

| Command | Level | Description |
|---------|-------|-------------|
| `.dismount` | 0 (Player) | Syntax: .dismount Dismount you, if you are mounted. |

## distance

| Command | Level | Description |
|---------|-------|-------------|
| `.distance` | 3 (Administrator) | Syntax: .distance Display the distance from your character to the selected creature. |

## event

| Command | Level | Description |
|---------|-------|-------------|
| `.event` | 2 (Game Master) | Syntax: .event #event_id Show details about event with #event_id. |
| `.event activelist` | 2 (Game Master) | Syntax: .event activelist Show list of currently active events. |
| `.event info` | 2 (Game Master) | Syntax: .event info [event_id] Displays information about game events. |
| `.event start` | 2 (Game Master) | Syntax: .event start #event_id Start event #event_id. Set start time for event to current moment (change not saved in DB). |
| `.event stop` | 2 (Game Master) | Syntax: .event stop #event_id Stop event #event_id. Set start time for event to time in past that make current moment is event stop time (change not saved in DB). |

## flusharenapoints

| Command | Level | Description |
|---------|-------|-------------|
| `.flusharenapoints` | 3 (Administrator) | Syntax: .flusharenapoints Use it to distribute arena points based on arena team ratings, and start a new week. |

## freeze

| Command | Level | Description |
|---------|-------|-------------|
| `.freeze` | 2 (Game Master) | Syntax: .freeze (#player) "Freezes" #player and disables his chat. When using this without #name it will freeze your target. |

## gear

| Command | Level | Description |
|---------|-------|-------------|
| `.gear repair` | 2 (Game Master) | Syntax: .gear repair Repair all selected player's items. |
| `.gear stats` | 0 (Player) | Syntax: .gear stats |

## gm

| Command | Level | Description |
|---------|-------|-------------|
| `.gm` | 1 (Moderator) | Syntax: .gm [on/off] Enable or Disable in game GM MODE or show current state of on/off not provided. |
| `.gm chat` | 2 (Game Master) | Syntax: .gm chat [on/off] Enable or disable chat GM MODE (show gm badge in messages) or show current state of on/off not provided. |
| `.gm fly` | 2 (Game Master) | Syntax: .gm fly [on/off] Enable/disable gm fly mode. |
| `.gm ingame` | 0 (Player) | Syntax: .gm ingame Display a list of available in game Game Masters. |
| `.gm list` | 3 (Administrator) | Syntax: .gm list Display a list of all Game Masters accounts and security levels. |
| `.gm off` | 1 (Moderator) | Syntax: .gm off Turns off GM flag. |
| `.gm on` | 1 (Moderator) | Syntax: .gm on Turns on GM flag. |
| `.gm spectator` | 2 (Game Master) | Syntax: .gm spectator on\|off Requires .gm on. Allows the GM character to follow members of the opposite faction. You may need to change zones for the effect to apply. |
| `.gm visible` | 2 (Game Master) | Syntax: .gm visible on/off Output current visibility state or make GM visible(on) and invisible(off) for other players. |

## gmannounce

| Command | Level | Description |
|---------|-------|-------------|
| `.gmannounce` | 2 (Game Master) | Syntax: .gmannounce $announcement Send an announcement to online Gamemasters. |

## gmnameannounce

| Command | Level | Description |
|---------|-------|-------------|
| `.gmnameannounce` | 2 (Game Master) | Syntax: .gmnameannounce $announcement. Send an announcement to all online GM's, displaying the name of the sender. |

## gmnotify

| Command | Level | Description |
|---------|-------|-------------|
| `.gmnotify` | 2 (Game Master) | Syntax: .gmnotify $notification Displays a notification on the screen of all online GM's. |

## go

| Command | Level | Description |
|---------|-------|-------------|
| `.go` | 1 (Moderator) | Syntax: .go $subcommand Type .go to see the list of possible subcommands or .help go $subcommand to see info on subcommands |
| `.go creature` | 1 (Moderator) | Syntax: .go creature $creature.guid Teleports you to the creature using `guid` value of `creature` table. |
| `.go creature id` | 1 (Moderator) | Syntax: .go creature id #creature_entry [#spawn] Teleports you to first (if no #spawn provided) spawn the given creature entry. |
| `.go creature name` | 1 (Moderator) | Syntax: .go creature name $creature_template.name Teleports you to a creature using the `name` value of `creature_template` table. In the case of multiple creature of the same `name` existing in the world, you will be teleported to the lowest `guid` creature. When running the command for names wi... |
| `.go gameobject` | 1 (Moderator) | Syntax: .go gameobject $gameobject.guid Teleports you to the gameobject using `guid` value of `gameobject` table. |
| `.go gameobject id` | 1 (Moderator) | Syntax: .go gameobject id #gameobject_entry [#spawn] Teleports you to first (if no #spawn provided) spawn the given gameobject entry. |
| `.go graveyard` | 1 (Moderator) | Syntax: .go graveyard #graveyardId Teleport to graveyard with the graveyardId specified. |
| `.go grid` | 1 (Moderator) | Syntax: .go grid #gridX #gridY [#mapId] Teleport the gm to center of grid with provided indexes at map #mapId (or current map if it not provided). |
| `.go quest` | 1 (Moderator) | Syntax: .go quest <starter/ender> <quest>. Teleports you to the quest starter/ender creature or object. |
| `.go taxinode` | 1 (Moderator) | Syntax: .go taxinode #taxinode Teleport player to taxinode coordinates. You can look up zone using .lookup taxinode $namepart |
| `.go ticket` | 2 (Game Master) | Syntax: .go ticket #ticketid Teleports the user to the location where $ticketid was created. |
| `.go trigger` | 1 (Moderator) | Syntax: .go trigger #trigger_id Teleport your character to areatrigger with id #trigger_id. Character will be teleported to trigger target if selected areatrigger is telporting trigger. |
| `.go xyz` | 1 (Moderator) | Syntax: .go xyz #x #y [#z [#mapid [#orientation]]] Teleport player to point with (#x,#y,#z) coordinates at map #mapid with orientation #orientation. If #z is not provided, ground/water level will be used. If #mapid is not provided, the current map will be used. If #orientation is not provided, th... |
| `.go zonexy` | 1 (Moderator) | Syntax: .go zonexy #x #y [#zone] Teleport player to point with (#x,#y) client coordinates at ground(water) level in zone #zoneid or current zone if #zoneid not provided. You can look up zone using .lookup area $namepart |

## gobject

| Command | Level | Description |
|---------|-------|-------------|
| `.gobject` | 2 (Game Master) | Syntax: .gobject $subcommand Type .gobject to see the list of possible subcommands or .help gobject $subcommand to see info on subcommands |
| `.gobject activate` | 2 (Game Master) | Syntax: .gobject activate #guid Activates an object like a door or a button. |
| `.gobject add` | 3 (Administrator) | Syntax: .gobject add #id <spawntimeSecs> Add a game object from game object templates to the world at your current location using the #id. spawntimesecs sets the spawntime, it is optional. Note: this is a copy of .gameobject. |
| `.gobject add temp` | 2 (Game Master) | Adds a temporary gameobject that is not saved to DB. |
| `.gobject delete` | 3 (Administrator) | Syntax: .gobject delete #go_guid Delete gameobject with guid #go_guid. |
| `.gobject despawngroup` | 3 (Administrator) | Syntax: .gobject despawngroup #groupId Despawns all gameobjects in the given spawn group. |
| `.gobject info` | 1 (Moderator) | Syntax: .gobject info [$object_entry] Query Gameobject information for selected gameobject or given entry. |
| `.gobject load` | 3 (Administrator) | Syntax: .gobject load #spawnId Load a gameobject spawn from the database into the world by its GUID. |
| `.gobject move` | 3 (Administrator) | Syntax: .gobject move #goguid [#x #y #z] Move gameobject #goguid to character coordinates (or to (#x,#y,#z) coordinates if its provide). |
| `.gobject near` | 1 (Moderator) | Syntax: .gobject near  [#distance] Output gameobjects at distance #distance from player. Output gameobject guids and coordinates sorted by distance from character. If #distance not provided use 10 as default value. |
| `.gobject respawn` | 2 (Game Master) | Syntax: .gobject respawn #guid./nRespawns the target gameobject. |
| `.gobject set` | 3 (Administrator) | Syntax: .gobject set $subcommand Type .gobject set to see the list of possible subcommands or .help gobject set $subcommand to see info on subcommands. |
| `.gobject set phase` | 3 (Administrator) | Syntax: .gobject set phase #guid #phasemask Gameobject with DB guid #guid phasemask changed to #phasemask with related world vision update for players. Gameobject state saved to DB and persistent. |
| `.gobject set state` | 3 (Administrator) | Syntax: .gobject set state #GUIDLow, #objectType, #objectState Sets the byte value or sends a custom animation for a given gameobject GUID. |
| `.gobject spawngroup` | 3 (Administrator) | Syntax: .gobject spawngroup #groupId Spawns all gameobjects in the given spawn group. |
| `.gobject target` | 1 (Moderator) | Syntax: .gobject target [#go_id\|#go_name_part] Locate and show position nearest gameobject. If #go_id or #go_name_part provide then locate and show position of nearest gameobject with gameobject template id #go_id or name included #go_name_part as part. |
| `.gobject turn` | 3 (Administrator) | Syntax: .gobject turn #goguid Set for gameobject #goguid orientation same as current character orientation. |

## gps

| Command | Level | Description |
|---------|-------|-------------|
| `.gps` | 1 (Moderator) | Syntax: .gps [$name\|$shift-link] Display the position information for a selected character or creature (also if player name $name provided then for named player, or if creature/gameobject shift-link provided then pointed creature/gameobject if it loaded). Position information includes X, Y, Z, a... |

## group

| Command | Level | Description |
|---------|-------|-------------|
| `.group` | 2 (Game Master) | Syntax: .group $subcommand Type .group to see the list of possible subcommands or .help group $subcommand to see info on subcommands |
| `.group disband` | 2 (Game Master) | Syntax: .group disband [$characterName] Disbands the given character's group. |
| `.group join` | 2 (Game Master) | Syntax: .group join $AnyCharacterNameFromGroup [$CharacterName] Adds to group of player $AnyCharacterNameFromGroup player $CharacterName (or selected). |
| `.group leader` | 2 (Game Master) | Syntax: .group leader [$characterName] Sets the given character as his group's leader. |
| `.group list` | 2 (Game Master) | Syntax: .group list [$CharacterName] Lists all the members of the group/party the player is in. |
| `.group remove` | 2 (Game Master) | Syntax: .group remove [$characterName] Removes the given character from his group. |
| `.group revive` | 2 (Game Master) | Syntax: .group revive $characterName Revives all group members of the given character or self if not provided. |

## groupsummon

| Command | Level | Description |
|---------|-------|-------------|
| `.groupsummon` | 2 (Game Master) | Syntax: .groupsummon [$charactername] Teleport the given character and his group to you. Teleported only online characters but original selected group member can be offline. |

## guid

| Command | Level | Description |
|---------|-------|-------------|
| `.guid` | 2 (Game Master) | Syntax: .guid Display the GUID for the selected character. |

## guild

| Command | Level | Description |
|---------|-------|-------------|
| `.guild` | 2 (Game Master) | Syntax: .guild $subcommand Type .guild to see the list of possible subcommands or .help guild $subcommand to see info on subcommands |
| `.guild create` | 2 (Game Master) | Syntax: .guild create [$GuildLeaderName] "$GuildName" Create a guild named $GuildName with the player $GuildLeaderName (or selected) as leader.  Guild name must in quotes. |
| `.guild delete` | 2 (Game Master) | Syntax: .guild delete "$GuildName" Delete guild $GuildName. Guild name must in quotes. |
| `.guild info` | 2 (Game Master) | Shows information about the target's guild or a given Guild Id or Name. |
| `.guild invite` | 2 (Game Master) | Syntax: .guild invite [$CharacterName] "$GuildName" Add player $CharacterName (or selected) into a guild $GuildName. Guild name must in quotes. |
| `.guild rank` | 2 (Game Master) | Syntax: .guild rank [$CharacterName] #RankNumber Set for player $CharacterName (or selected) rank #Rank in a guild. Ranks value are numeric, 0 = Guild Master, 1 = Officer, etc... |
| `.guild rename` | 2 (Game Master) | Syntax: .guild rename "$GuildName" "$NewGuildName" Rename a guild named $GuildName with $NewGuildName. Guild name and new guild name must in quotes. |
| `.guild uninvite` | 2 (Game Master) | Syntax: .guild uninvite [$CharacterName] Remove player $CharacterName (or selected) from a guild. |

## help

| Command | Level | Description |
|---------|-------|-------------|
| `.help` | 0 (Player) | Syntax: .help [$command] Display usage instructions for the given $command. If no $command provided show list available commands. |

## hidearea

| Command | Level | Description |
|---------|-------|-------------|
| `.hidearea` | 3 (Administrator) | Syntax: .hidearea #areaid Hide the area of #areaid to the selected character. If no character is selected, hide this area to you. |

## honor

| Command | Level | Description |
|---------|-------|-------------|
| `.honor` | 2 (Game Master) | Syntax: .honor $subcommand Type .honor to see the list of possible subcommands or .help honor $subcommand to see info on subcommands |
| `.honor add` | 2 (Game Master) | Syntax: .honor add $amount Add a certain amount of honor (gained today) to the selected player. |
| `.honor add kill` | 2 (Game Master) | Syntax: .honor add kill Add the targeted unit as one of your pvp kills today (you only get honor if it's a racial leader or a player) |
| `.honor update` | 2 (Game Master) | Syntax: .honor update Force the yesterday's honor fields to be updated with today's data, which will get reset for the selected player. |

## instance

| Command | Level | Description |
|---------|-------|-------------|
| `.instance` | 1 (Moderator) | Syntax: .instance $subcommand Type .instance to see the list of possible subcommands or .help instance $subcommand to see info on subcommands |
| `.instance getbossstate` | 1 (Moderator) | Syntax: .instance getbossstate [$Name] Displays the state for every available encounter. If no character name is provided, the current map will be used as target. |
| `.instance listbinds` | 1 (Moderator) | Syntax: .instance listbinds Lists the binds of the selected player. |
| `.instance savedata` | 3 (Administrator) | Syntax: .instance savedata Save the InstanceData for the current player's map to the DB. |
| `.instance setbossstate` | 2 (Game Master) | Syntax: .instance setbossstate $bossId $encounterState [$Name] Sets the EncounterState for the given boss id to a new value. EncounterStates range from 0 to 5. If no character name is provided, the current map will be used as target. |
| `.instance stats` | 1 (Moderator) | Syntax: .instance stats Shows statistics about instances. |
| `.instance unbind` | 2 (Game Master) | Syntax: .instance unbind <mapid\|all> [difficulty] Clear all/some of player's binds |

## inventory

| Command | Level | Description |
|---------|-------|-------------|
| `.inventory` | 1 (Moderator) | Syntax: .inventory $subcommand Type .inventory to see the list of possible subcommands or .help inventory $subcommand to see info on subcommands |
| `.inventory count` | 1 (Moderator) | Syntax: .inventory count $playerName or $plaerGuid Count free slots in bags divided into different bag types. |

## item

| Command | Level | Description |
|---------|-------|-------------|
| `.item move` | 2 (Game Master) | Syntax: .itemmove #sourceslotid #destinationslotid Move an item from slots #sourceslotid to #destinationslotid in your inventory Not yet implemented |
| `.item refund` | 3 (Administrator) | Syntax: .item refund <name> <item> <extendedCost> Removes the item and restores honor/arena/items according to extended cost. |
| `.item restore` | 2 (Game Master) | Syntax: .item restore [#recoveryItemId] [#playername] Restore an disposed item for the specified player. Get recoveryId from ".item restore list" command. |
| `.item restore list` | 2 (Game Master) | Syntax: .item restore list [#playername] See restorable items for the specified player. |

## kick

| Command | Level | Description |
|---------|-------|-------------|
| `.kick` | 2 (Game Master) | Syntax: .kick [$charactername] [$reason] Kick the given character name from the world with or without reason. If no character name is provided then the selected player (except for yourself) will be kicked. If no reason is provided, default is "No Reason". |

## learn

| Command | Level | Description |
|---------|-------|-------------|
| `.learn` | 2 (Game Master) | Syntax: .learn #spell [all] Selected character learn a spell of id #spell. If 'all' provided then all ranks learned. |
| `.learn all` | 2 (Game Master) | Syntax: .learn all $subcommand Type .learn all to see the list of possible subcommands or .help learn all $subcommand to see info on subcommands. |
| `.learn all crafts` | 2 (Game Master) | Syntax: .learn crafts Learn all professions and recipes. |
| `.learn all default` | 2 (Game Master) | Syntax: .learn all default [$playername] Learn for selected/$playername player all default spells for his race/class and spells rewarded by completed quests. |
| `.learn all gm` | 2 (Game Master) | Syntax: .learn all gm Learn all default spells for Game Masters. |
| `.learn all lang` | 2 (Game Master) | Syntax: .learn all lang Learn all languages |
| `.learn all my` | 2 (Game Master) | Syntax: .learn all my $subcommand Type .learn all my to see the list of possible subcommands or .help learn all my $subcommand to see info on subcommands. |
| `.learn all my class` | 2 (Game Master) | Syntax: .learn all my class  Learn all spells (trainer, talent, and quest rewards) for your class. |
| `.learn all my pettalents` | 2 (Game Master) | Syntax: .learn all my pettalents Learn all talents for your pet available for his creature type (only for hunter pets). |
| `.learn all my quest` | 2 (Game Master) | Syntax: .learn all my quest  Learn all spells rewarded from quest for your class. |
| `.learn all my talents` | 2 (Game Master) | Syntax: .learn all my talents Learn all talents (and spells with first rank learned as talent) available for his class. |
| `.learn all my trainer` | 2 (Game Master) | Syntax: .learn all my trainer  Learn all spells taught by trainers for your class. |
| `.learn all recipes` | 2 (Game Master) | Syntax: .learn all recipes [$profession] Learns all recipes of specified profession and sets skill level to max. Example: .learn all recipes enchanting |

## levelup

| Command | Level | Description |
|---------|-------|-------------|
| `.levelup` | 2 (Game Master) | Syntax: .levelup [$playername] [#numberoflevels] Increase/decrease the level of character with $playername (or the selected if not name provided) by #numberoflevels Or +1 if no #numberoflevels provided). If #numberoflevels is omitted, the level will be increase by 1. If #numberoflevels is 0, the... |

## lfg

| Command | Level | Description |
|---------|-------|-------------|
| `.lfg` | 1 (Moderator) | Syntax: .lfg $subcommand Type .lfg to see the list of possible subcommands or .help lfg $subcommand to see info on subcommands. |
| `.lfg clean` | 3 (Administrator) | Syntax: .flg clean Cleans current queue, only for debugging purposes. |
| `.lfg cooldown` | 3 (Administrator) | Syntax: .lfg cooldown Clears all LFG dungeon cooldowns for all players. |
| `.lfg group` | 1 (Moderator) | Syntax: .lfg group Shows information about all players in the group  (state, roles, comment, dungeons selected). |
| `.lfg options` | 2 (Game Master) | Syntax: .lfg options [new value] Shows current lfg options. New value is set if extra param is present. |
| `.lfg player` | 1 (Moderator) | Syntax: .lfg player Shows information about player (state, roles, comment, dungeons selected). |
| `.lfg queue` | 1 (Moderator) | Syntax: .lfg queue Shows info about current lfg queues. |

## linkgrave

| Command | Level | Description |
|---------|-------|-------------|
| `.linkgrave` | 3 (Administrator) | Syntax: .linkgrave #graveyard_id [alliance\|horde] Link current zone to graveyard for any (or alliance/horde faction ghosts). This let character ghost from zone teleport to graveyard after die if graveyard is nearest from linked to zone and accept ghost of this faction. Add only single graveyard... |

## list

| Command | Level | Description |
|---------|-------|-------------|
| `.list` | 1 (Moderator) | Syntax: .list $subcommand Type .list to see the list of possible subcommands or .help list $subcommand to see info on subcommands |
| `.list auras` | 1 (Moderator) | Syntax: .list auras List auras (passive and active) of selected creature or player. If no creature or player is selected, list your own auras. |
| `.list auras id` | 1 (Moderator) | Syntax: .list auras id Lists all active auras on the selected unit by spell ID. |
| `.list auras name` | 1 (Moderator) | Syntax: .list auras name Lists all active auras on the selected unit by spell name. |
| `.list creature` | 1 (Moderator) | Syntax: .list creature #creature_id [#max_count] Output creatures with creature id #creature_id found in world. Output creature guids and coordinates sorted by distance from character. Will be output maximum #max_count creatures. If #max_count not provided use 10 as default value. |
| `.list item` | 1 (Moderator) | Syntax: .list item #item_id [#max_count] Output items with item id #item_id found in all character inventories, mails, auctions, and guild banks. Output item guids, item owner guid, owner account and owner name (guild name and guid in case guild bank). Will be output maximum #max_count items. If... |
| `.list object` | 1 (Moderator) | [DEPRECATED]: use ".list gobject" instead. Syntax: .go object #object_guid Teleport your character to gameobject with guid #object_guid |
| `.list respawns` | 2 (Game Master) | Syntax: .list respawns [entryId] In-game: shows all pending creature and gameobject respawns on the current map, filtered by entryId if provided. Console: .list respawns #mapId [#instanceId [#entryId]] - specify map and optional instance ID and entry filter. |

## lookup

| Command | Level | Description |
|---------|-------|-------------|
| `.lookup` | 1 (Moderator) | Syntax: .lookup $subcommand Type .lookup to see the list of possible subcommands or .help lookup $subcommand to see info on subcommands |
| `.lookup area` | 1 (Moderator) | Syntax: .lookup area $namepart Looks up an area by $namepart, and returns all matches with their area ID's. |
| `.lookup creature` | 1 (Moderator) | Syntax: .lookup creature $namepart Looks up a creature by $namepart, and returns all matches with their creature ID's. |
| `.lookup event` | 1 (Moderator) | Syntax: .lookup event $name Attempts to find the ID of the event with the provided $name. |
| `.lookup faction` | 1 (Moderator) | Syntax: .lookup faction $name Attempts to find the ID of the faction with the provided $name. |
| `.lookup gobject` | 1 (Moderator) | Syntax: .lookup object $objname Looks up an gameobject by $objname, and returns all matches with their Gameobject ID's. |
| `.lookup item` | 1 (Moderator) | Syntax: .lookup item $itemname Looks up an item by $itemname, and returns all matches with their Item ID's. |
| `.lookup item set` | 1 (Moderator) | Syntax: .lookup itemset $itemname Looks up an item set by $itemname, and returns all matches with their Item set ID's. |
| `.lookup map` | 1 (Moderator) | Syntax: .lookup map $namepart Looks up a map by $namepart, and returns all matches with their map ID's. |
| `.lookup object` | 1 (Moderator) | [DEPRECATED]: use ".lookup gobject" instead. Syntax: .go object #object_guid Teleport your character to gameobject with guid #object_guid |
| `.lookup player` | 2 (Game Master) | Syntax: .lookup player $subcommand Type .lookup player to see the list of possible subcommands or .help lookup player $subcommand to see info on subcommands. |
| `.lookup player account` | 2 (Game Master) | Syntax: .lookup player account $account ($limit) Searchs players, which account username is $account with optional parametr $limit of results. |
| `.lookup player email` | 2 (Game Master) | Syntax: .lookup player email $email ($limit) Searchs players, which account email is $email with optional parametr $limit of results. |
| `.lookup player ip` | 2 (Game Master) | Syntax: .lookup player ip $ip ($limit) Searchs players, which account ast_ip is $ip with optional parametr $limit of results. |
| `.lookup quest` | 1 (Moderator) | Syntax: .lookup quest $namepart Looks up a quest by $namepart, and returns all matches with their quest ID's. |
| `.lookup skill` | 1 (Moderator) | Syntax: .lookup skill $$namepart Looks up a skill by $namepart, and returns all matches with their skill ID's. |
| `.lookup spell` | 1 (Moderator) | Syntax: .lookup spell $namepart Looks up a spell by $namepart, and returns all matches with their spell ID's. |
| `.lookup spell id` | 1 (Moderator) | Syntax: .lookup spell id #spellid Looks up a spell by #spellid, and returns the match with its spell name. |
| `.lookup taxinode` | 1 (Moderator) | Syntax: .lookup taxinode $substring Search and output all taxinodes with provide $substring in name. |
| `.lookup teleport` | 1 (Moderator) | Syntax: .lookup teleport $substring Search and output all .teleport command locations with provide $substring in name. |
| `.lookup title` | 1 (Moderator) | Syntax: .lookup title $$namepart Looks up a title by $namepart, and returns all matches with their title ID's and index's. |

## mail

| Command | Level | Description |
|---------|-------|-------------|
| `.mail` | 2 (Game Master) | Syntax: .mail $subcommand Type .mail to see a list of subcommands or .help mail $subcommand to see info on subcommands. |
| `.mail list` | 2 (Game Master) | Syntax: .mail list [$player] Displays all mail data (except subject and body) for the target player. |
| `.mail return` | 2 (Game Master) | Syntax: .mail return $player $mailId Returns the specified mail to its original sender. |

## mailbox

| Command | Level | Description |
|---------|-------|-------------|
| `.mailbox` | 1 (Moderator) | Syntax: .mailbox Show your mailbox content. |

## maxskill

| Command | Level | Description |
|---------|-------|-------------|
| `.maxskill` | 2 (Game Master) | Syntax: .maxskill Sets all skills of the targeted player to their maximum values for its current level. |

## mmap

| Command | Level | Description |
|---------|-------|-------------|
| `.mmap` | 3 (Administrator) | Syntax: Syntax: .mmaps $subcommand Type .mmaps to see the list of possible subcommands or .help mmaps $subcommand to see info on subcommands |
| `.mmap loadedtiles` | 3 (Administrator) | Syntax: .mmap loadedtiles to show which tiles are currently loaded |
| `.mmap loc` | 3 (Administrator) | Syntax: .mmap loc to print on which tile one is |
| `.mmap path` | 3 (Administrator) | Syntax: .mmap path to calculate and show a path to current select unit |
| `.mmap stats` | 3 (Administrator) | Syntax: .mmap stats to show information about current state of mmaps |
| `.mmap testarea` | 3 (Administrator) | Syntax: .mmap testarea to calculate paths for all nearby npcs to player |

## modify

| Command | Level | Description |
|---------|-------|-------------|
| `.modify` | 2 (Game Master) | Syntax: .modify $subcommand Type .modify to see the list of possible subcommands or .help modify $subcommand to see info on subcommands |
| `.modify arenapoints` | 2 (Game Master) | Syntax: .modify arenapoints #value Add $amount arena points to the selected player. |
| `.modify bit` | 2 (Game Master) | Syntax: .modify bit #field #bit Toggle the #bit bit of the #field field for the selected player. If no player is selected, modify your character. |
| `.modify drunk` | 2 (Game Master) | Syntax: .modify drunk #value Set drunk level to #value (0..100). Value 0 remove drunk state, 100 is max drunked state. |
| `.modify energy` | 2 (Game Master) | Syntax: .modify energy #energy Modify the energy of the selected player. If no player is selected, modify your energy. |
| `.modify faction` | 3 (Administrator) | Syntax: .modify faction #factionid #flagid #npcflagid #dynamicflagid Modify the faction and flags of the selected creature. Without arguments, display the faction and flags of the selected creature. |
| `.modify gender` | 2 (Game Master) | Syntax: .modify gender male/female Change gender of selected player. |
| `.modify honor` | 2 (Game Master) | Syntax: .modify honor $amount Add $amount honor points to the selected player. |
| `.modify hp` | 2 (Game Master) | Syntax: .modify hp #newhp Modify the hp of the selected player. If no player is selected, modify your hp. |
| `.modify mana` | 2 (Game Master) | Syntax: .modify mana #newmana Modify the mana of the selected player. If no player is selected, modify your mana. |
| `.modify money` | 2 (Game Master) | Syntax: .modify money #money .money #money Add or remove money to the selected player. If no player is selected, modify your money. #gold can be negative to remove money. |
| `.modify mount` | 2 (Game Master) | Syntax: .modify mount #id #speed Set CreatureDisplayID as #id and set speed to #speed value between 0.1 - 50.0 |
| `.modify phase` | 2 (Game Master) | Syntax: .modify phase #phasemask Selected character phasemask changed to #phasemask with related world vision update. Change active until in game phase changed, or GM-mode enable/disable, or re-login. Character pts pasemask update to same value. |
| `.modify rage` | 2 (Game Master) | Syntax: .modify rage #newrage Modify the rage of the selected player. If no player is selected, modify your rage. |
| `.modify reputation` | 2 (Game Master) | Syntax: .modify reputation #repId (#repvalue \| $rankname [#delta]) Sets the selected players reputation with faction #repId to #repvalue or to $reprank. If the reputation rank name is provided, the resulting reputation will be the lowest reputation for that rank plus the delta amount, if specifi... |
| `.modify runicpower` | 2 (Game Master) | Syntax: .modify runicpower #newrunicpower Modify the runic power of the selected player. If no player is selected, modify your runic power. |
| `.modify scale` | 2 (Game Master) | .modify scale #scale Modify size of the selected player or creature to "normal scale"*rate. If no player or creature is selected, modify your size. #rate may range from 0.1 to 10. |
| `.modify speed` | 2 (Game Master) | Syntax: .modify speed $speedtype #rate Modify the running speed of the selected player to "normal base run speed"= 1. If no player is selected, modify your speed. $speedtypes may be fly, all, walk, backwalk, or swim. #rate may range from 0.1 to 50. |
| `.modify speed all` | 2 (Game Master) | Syntax: .modify aspeed #rate Modify all speeds -run,swim,run back,swim back- of the selected player to "normalbase speed for this move type"*rate. If no player is selected, modify your speed. #rate may range from 0.1 to 50. |
| `.modify speed backwalk` | 2 (Game Master) | Syntax: .modify speed backwalk #rate Modify the speed of the selected player while running backwards to "normal walk back speed"*rate. If no player is selected, modify your speed. #rate may range from 0.1 to 50. |
| `.modify speed fly` | 2 (Game Master) | .modify speed fly #rate Modify the flying speed of the selected player to "normal flying speed"*rate. If no player is selected, modify your speed. #rate may range from 0.1 to 50. |
| `.modify speed swim` | 2 (Game Master) | Syntax: .modify speed swim #rate Modify the swim speed of the selected player to "normal swim speed"*rate. If no player is selected, modify your speed. #rate may range from 0.1 to 50. |
| `.modify speed walk` | 2 (Game Master) | Syntax: .modify speed bwalk #rate Modify the speed of the selected player while running to "normal walk speed"*rate. If no player is selected, modify your speed. #rate may range from 0.1 to 50. |
| `.modify spell` | 4 (Console only) | TODO |
| `.modify standstate` | 2 (Game Master) | Syntax: .modify standstate #emoteid Change the emote of your character while standing to #emoteid. |
| `.modify talentpoints` | 2 (Game Master) | Syntax: .modify talentpoints #amount Set free talent points for selected character or character's pet. It will be reset to default expected at next levelup/login/quest reward. |

## morph

| Command | Level | Description |
|---------|-------|-------------|
| `.morph` | 1 (Moderator) | Syntax: .morph $subcommand Type .morph to see the list of possible subcommands or ".help morph" to see info on subcommands |
| `.morph mount` | 1 (Moderator) | Syntax: .morph mount #displayid - Change the selected target's mount's model ID to #displayid. |
| `.morph reset` | 1 (Moderator) | Syntax: .morph reset - Doesn't use any parameters to reset the selected target's model |
| `.morph target` | 1 (Moderator) | Syntax: .morph target #displayid - Change the selected target's current model id to #displayid. |

## movegens

| Command | Level | Description |
|---------|-------|-------------|
| `.movegens` | 3 (Administrator) | Syntax: .movegens Show movement generators stack for selected creature or player. |

## mute

| Command | Level | Description |
|---------|-------|-------------|
| `.mute` | 2 (Game Master) | Syntax: .mute [$playerName] $mutetime [$reason] Disible chat messaging for any character from account of character $playerName (or currently selected) at $mutetime time. Player can be offline. $mutetime: use a timestring like "1d15h33s". |

## mutehistory

| Command | Level | Description |
|---------|-------|-------------|
| `.mutehistory` | 2 (Game Master) | Syntax: .mutehistory $accountName. Shows mute history for an account. |

## nameannounce

| Command | Level | Description |
|---------|-------|-------------|
| `.nameannounce` | 2 (Game Master) | Syntax: .nameannounce $announcement. Send an announcement to all online players, displaying the name of the sender. |

## neargrave

| Command | Level | Description |
|---------|-------|-------------|
| `.neargrave` | 2 (Game Master) | Syntax: .neargrave [alliance\|horde] Find nearest graveyard linked to zone (or only nearest from accepts alliance or horde faction ghosts). |

## notify

| Command | Level | Description |
|---------|-------|-------------|
| `.notify` | 2 (Game Master) | Syntax: .notify $MessageToBroadcast Send a global message to all players online in screen. |

## npc

| Command | Level | Description |
|---------|-------|-------------|
| `.npc` | 2 (Game Master) | Syntax: .npc $subcommand Type .npc to see the list of possible subcommands or .help npc $subcommand to see info on subcommands |
| `.npc add` | 3 (Administrator) | Syntax: .npc add #creatureid Spawn a creature by the given template id of #creatureid. |
| `.npc add formation` | 3 (Administrator) | Syntax: .npc add formation $leader Add selected creature to a leader's formation. |
| `.npc add item` | 3 (Administrator) | Syntax: .npc add item #itemId <#maxcount><#incrtime><#extendedcost>r Add item #itemid to item list of selected vendor. Also optionally set max count item in vendor item list and time to item count restoring and items ExtendedCost. |
| `.npc add move` | 3 (Administrator) | Syntax: .npc add move #creature_guid [#waittime] Add your current location as a waypoint for creature with guid #creature_guid. And optional add wait time. |
| `.npc add temp` | 3 (Administrator) | Syntax: .npc add temp Adds temporary NPC, not saved to database. |
| `.npc delete` | 3 (Administrator) | Syntax: .npc delete [#guid] Delete creature with guid #guid (or the selected if no guid is provided) |
| `.npc delete item` | 3 (Administrator) | Syntax: .npc delete item #itemId Remove item #itemid from item list of selected vendor. |
| `.npc despawngroup` | 3 (Administrator) | Syntax: .npc despawngroup #groupId Despawns all creatures in the given spawn group. |
| `.npc do` | 3 (Administrator) | Syntax: .npc do $action Requests the NPC to perform DoAction with the specified ActionID. Used for testing scripts. |
| `.npc follow` | 2 (Game Master) | Syntax: .npc follow start Selected creature start follow you until death/fight/etc. |
| `.npc follow stop` | 2 (Game Master) | Syntax: .npc follow stop Selected creature (non pet) stop follow you. |
| `.npc guid` | 2 (Game Master) | Syntax: .npc guid Displays GUID, faction, NPC flags, Entry ID, Model ID for selected creature. |
| `.npc info` | 2 (Game Master) | Syntax: .npc info [#creature_guid] Display a list of details for the selected creature, or for the creature with the given GUID if no target is selected. When a creature is targeted or found in the current map, the list includes: - GUID, Faction, NPC flags, Entry ID, Model ID, - Level, - Health (... |
| `.npc load` | 3 (Administrator) | Syntax: .npc load #spawnId Load a creature spawn from the database into the world by its GUID. |
| `.npc move` | 2 (Game Master) | Syntax: .npc move [#creature_guid] Move the targeted creature spawn point to your coordinates. |
| `.npc near` | 2 (Game Master) | Syntax: .npc near #distance Returns all database creature spawns in a given distance. |
| `.npc playemote` | 2 (Game Master) | Syntax: .npc playemote #emoteid Make the selected creature emote with an emote of id #emoteid. |
| `.npc say` | 2 (Game Master) | Syntax: .npc say $message Make selected creature say specified message. |
| `.npc set` | 3 (Administrator) | Syntax: .npc set $subcommand Type .npc set to see the list of possible subcommands or .help npc set $subcommand to see info on subcommands. |
| `.npc set allowmove` | 3 (Administrator) | Syntax: .npc set allowmove Enable or disable movement creatures in world. Not implemented. |
| `.npc set data` | 3 (Administrator) | Syntax: .npc set data $field $data Sets data for the selected creature. Used for testing Scripting |
| `.npc set entry` | 3 (Administrator) | Syntax: .npc set entry $entry Switch selected creature with another entry from creature_template. - New creature.id value not saved to DB. |
| `.npc set faction original` | 3 (Administrator) | Syntax: .npc set faction original Revert the temporal faction of the selected creature. |
| `.npc set faction permanent` | 3 (Administrator) | Syntax: .npc set faction permanent #factionid Permanently set the faction of the selected creature to #factionid. |
| `.npc set faction temp` | 3 (Administrator) | Syntax: .npc set faction temp #factionid Temporarily set the faction of the selected creature to #factionid. |
| `.npc set flag` | 3 (Administrator) | Syntax: .npc set flag #npcflag Set the NPC flags of creature template of the selected creature and selected creature to #npcflag. NPC flags will applied to all creatures of selected creature template after server restart or grid unload/load. |
| `.npc set level` | 3 (Administrator) | Syntax: .npc set level #level Change the level of the selected creature to #level. #level may range from 1 to (CONFIG_MAX_PLAYER_LEVEL) + 3. |
| `.npc set link` | 3 (Administrator) | Syntax: .npc set link $creatureGUID Links respawn of selected creature to the condition that $creatureGUID defined is alive. |
| `.npc set model` | 3 (Administrator) | Syntax: .npc set model #displayid Change the model id of the selected creature to #displayid. |
| `.npc set movetype` | 3 (Administrator) | Syntax: .npc set movetype [#creature_guid] stay/random/way [NODEL] Set for creature pointed by #creature_guid (or selected if #creature_guid not provided) movement type and move it to respawn position (if creature alive). Any existing waypoints for creature will be removed from the database if yo... |
| `.npc set phase` | 3 (Administrator) | Syntax: .npc set phase #phasemask Selected unit or pet phasemask changed to #phasemask with related world vision update for players. In creature case state saved to DB and persistent. In pet case change active until in game phase changed for owner, owner re-login, or GM-mode enable/disable.. |
| `.npc set spawntime` | 3 (Administrator) | Syntax: .npc set spawntime #time Adjust spawntime of selected creature to #time. #time: use a timestring like "10m30s". |
| `.npc set wanderdistance` | 3 (Administrator) | Syntax: .npc set wanderdistance #dist Adjust wander distance of selected creature to dist. |
| `.npc spawngroup` | 3 (Administrator) | Syntax: .npc spawngroup #groupId Spawns all creatures in the given spawn group. |
| `.npc tame` | 2 (Game Master) | Syntax: .npc tame Creates a player pet of the targeted creature. |
| `.npc textemote` | 2 (Game Master) | Syntax: .npc textemote #emoteid Make the selected creature to do textemote with an emote of id #emoteid. |
| `.npc whisper` | 2 (Game Master) | Syntax: .npc whisper #playername #text Make the selected npc whisper #text to  #playername. |
| `.npc yell` | 2 (Game Master) | Syntax: .npc yell $message Make selected creature yell specified message. |

## opendoor

| Command | Level | Description |
|---------|-------|-------------|
| `.opendoor` | 2 (Game Master) | Syntax: .opendoor [$range] Opens the nearest door within the range provided (default 5.0yd) |

## packetlog

| Command | Level | Description |
|---------|-------|-------------|
| `.packetlog` | 2 (Game Master) | Syntax: .packetlog [on/off] Toggles to allow the character using the command to start to log their packets into the server, PacketLogFile needs to be set with a valid filename |

## pdump

| Command | Level | Description |
|---------|-------|-------------|
| `.pdump` | 3 (Administrator) | Syntax: .pdump $subcommand Type .pdump to see the list of possible subcommands or .help pdump $subcommand to see info on subcommands |
| `.pdump copy` | 3 (Administrator) | Syntax: .pdump copy $playerNameOrGUID $account [$newname] [$newguid] Copy character with name/guid $playerNameOrGUID into character list of $account with $newname, with first free or $newguid guid. |
| `.pdump load` | 3 (Administrator) | Syntax: .pdump load $filename $account [$newname] [$newguid] Load character dump from dump file into character list of $account with saved or $newname, with saved (or first free) or $newguid guid. |
| `.pdump write` | 3 (Administrator) | Syntax: .pdump write $filename $playerNameOrGUID Write character dump with name/guid $playerNameOrGUID to file $filename. |

## pet

| Command | Level | Description |
|---------|-------|-------------|
| `.pet` | 2 (Game Master) | Syntax: .pet $subcommand Type .pet to see the list of possible subcommands or .help pet $subcommand to see info on subcommands |
| `.pet create` | 2 (Game Master) | Syntax: .pet create Creates a pet of the selected creature. |
| `.pet delete` | 3 (Administrator) | Syntax: .pet delete $playerNameOrGUID #petNumber Deletes the pet with the given pet number belonging to the specified player. |
| `.pet learn` | 2 (Game Master) | Syntax: .pet learn Learn #spellid to pet. |
| `.pet list` | 1 (Moderator) | Syntax: .pet list $playerNameOrGUID Lists all pets owned by the specified player (id, entry, level, slot, name, type). |
| `.pet unlearn` | 2 (Game Master) | Syntax: .pet unlean unLearn #spellid to pet. |

## pinfo

| Command | Level | Description |
|---------|-------|-------------|
| `.pinfo` | 2 (Game Master) | Syntax: .pinfo [$player_name/#GUID] Output account information and guild information for selected player or player find by $player_name or #GUID. |

## playall

| Command | Level | Description |
|---------|-------|-------------|
| `.playall` | 2 (Game Master) | Syntax: .playall #soundid Player a sound to whole server. |

## player

| Command | Level | Description |
|---------|-------|-------------|
| `.player learn` | 2 (Game Master) | Syntax: .player learn #playername #spell [all]. |
| `.player unlearn` | 2 (Game Master) | Syntax: .player unlearn #playername #spell [all]. |

## pool

| Command | Level | Description |
|---------|-------|-------------|
| `.pool info` | 2 (Game Master) | Syntax: .pool info #poolId Shows pool details: description, max active, all creature/gameobject/sub-pool members with active/inactive status. |
| `.pool lookup` | 2 (Game Master) | Syntax: .pool lookup Target a creature or stand near a gameobject to find which pool it belongs to and its current spawn status. |

## pooltools

| Command | Level | Description |
|---------|-------|-------------|
| `.pooltools` | 3 (Administrator) | Syntax: .pooltools $subcommand Tools for creating gameobject pools ingame. To use pooltools, uncomment Appender.Dev and Logger.sql.dev in the worldserver config. |
| `.pooltools add` | 3 (Administrator) | Syntax: .pooltools add [radius] Adds nearby gameobjects to the pooling session. Default radius is 5y. |
| `.pooltools clear` | 3 (Administrator) | Syntax: .pooltools clear Clears the current pooling session. |
| `.pooltools def` | 3 (Administrator) | Syntax: .pooltools def #GameobjectID #Chance #GameobjectID #Chance (...) Defines the Gameobject entries to be detected along with their associated chances. |
| `.pooltools end` | 3 (Administrator) | Syntax: .pooltools end Logs the current pooling session and clears it. |
| `.pooltools remove` | 3 (Administrator) | Syntax: .pooltools remove Removes the last group from the pooling session. |
| `.pooltools start` | 3 (Administrator) | Syntax: .pooltools start #description Starts a pooling session with the specified description. |

## possess

| Command | Level | Description |
|---------|-------|-------------|
| `.possess` | 2 (Game Master) | Syntax: .possess Possesses indefinitely the selected creature. |

## quest

| Command | Level | Description |
|---------|-------|-------------|
| `.quest` | 2 (Game Master) | Syntax: .quest $subcommand Type .quest to see the list of possible subcommands or .help quest $subcommand to see info on subcommands |
| `.quest add` | 2 (Game Master) | Syntax: .quest add #quest_id Add to character quest log quest #quest_id. Quest started from item can't be added by this command but correct .additem call provided in command output. |
| `.quest complete` | 2 (Game Master) | Syntax: .quest complete #questid Mark all quest objectives as completed for target character active quest. After this target character can go and get quest reward. |
| `.quest remove` | 2 (Game Master) | Syntax: .quest remove #quest_id Set quest #quest_id state to not completed and not active (and remove from active quest list) for selected player. |
| `.quest reward` | 2 (Game Master) | Syntax: .quest reward #questId Grants quest reward to selected player and removes quest from his log (quest must be in completed state). |
| `.quest status` | 2 (Game Master) | Syntax: .quest status $id [$name]. Displays the selected player's status for the specified quest. |

## rbac

| Command | Level | Description |
|---------|-------|-------------|
| `.rbac list` | 3 (Administrator) | Syntax: .rbac list [#id] View list of all permissions. If #id is given will show only info for that permission. |

## recall

| Command | Level | Description |
|---------|-------|-------------|
| `.recall` | 2 (Game Master) | Syntax: .recall [$playername] Teleport $playername or selected player to the place where he has been before last use of a teleportation command. If no $playername is entered and no player is selected, it will teleport you. |

## reload

| Command | Level | Description |
|---------|-------|-------------|
| `.reload` | 3 (Administrator) | Syntax: .reload $subcommand Type .reload to see the list of possible subcommands or .help reload $subcommand to see info on subcommands |
| `.reload achievement_criteria_data` | 3 (Administrator) | Syntax: .reload achievement_criteria_data Reload achievement_criteria_data table. |
| `.reload achievement_reward` | 3 (Administrator) | Syntax: .reload achievement_reward Reload achievement_reward table. |
| `.reload achievement_reward_locale` | 3 (Administrator) | Syntax: .reload achievement_reward_locale Reload achievement_reward_locale table. |
| `.reload acore_string` | 3 (Administrator) | Syntax: .reload acore_string Reload acore_string table. |
| `.reload all` | 3 (Administrator) | Syntax: .reload all Reload all tables with reload support added and that can be _safe_ reloaded. |
| `.reload all achievement` | 3 (Administrator) | Syntax: .reload all achievement Reload achievement_reward, achievement_criteria_data tables. |
| `.reload all area` | 3 (Administrator) | Syntax: .reload all area Reload areatrigger_teleport, areatrigger_tavern, game_graveyard_zone tables. |
| `.reload all gossips` | 3 (Administrator) | Syntax: .reload all gossips Reload gossip_menu, gossip_menu_option, gossip_scripts, points_of_interest tables. |
| `.reload all item` | 3 (Administrator) | Syntax: .reload all item Reload page_text, item_enchantment_table tables. |
| `.reload all locales` | 3 (Administrator) | Syntax: .reload all locales Reload all `locales_*` tables with reload support added and that can be _safe_ reloaded. |
| `.reload all loot` | 3 (Administrator) | Syntax: .reload all loot Reload all `*_loot_template` tables. This can be slow operation with lags for server run. |
| `.reload all npc` | 3 (Administrator) | Syntax: .reload all npc Reload npc_option, npc_trainer, npc vendor, points of interest tables. |
| `.reload all quest` | 3 (Administrator) | Syntax: .reload all quest Reload all quest related tables if reload support added for this table and this table can be _safe_ reloaded. |
| `.reload all scripts` | 3 (Administrator) | Syntax: .reload all scripts Reload gameobject_scripts, event_scripts, quest_end_scripts, quest_start_scripts, spell_scripts, db_script_string, waypoint_scripts tables. |
| `.reload all spell` | 3 (Administrator) | Syntax: .reload all spell Reload all `spell_*` tables with reload support added and that can be _safe_ reloaded. |
| `.reload antidos_opcode_policies` | 3 (Administrator) | Syntax: .reload antidos_opcode_policies Reloads antidos_opcode_policies table. |
| `.reload areatrigger` | 3 (Administrator) | Syntax: .reload areatrigger Reloads areatrigger table. |
| `.reload areatrigger_involvedrelation` | 3 (Administrator) | Syntax: .reload areatrigger_involvedrelation Reload areatrigger_involvedrelation table. |
| `.reload areatrigger_tavern` | 3 (Administrator) | Syntax: .reload areatrigger_tavern Reload areatrigger_tavern table. |
| `.reload areatrigger_teleport` | 3 (Administrator) | Syntax: .reload areatrigger_teleport Reload areatrigger_teleport table. |
| `.reload auctions` | 3 (Administrator) | Syntax: .reload auctions Reload dynamic data tables from the database. |
| `.reload autobroadcast` | 3 (Administrator) | Syntax: .reload autobroadcast Reload autobroadcast table. |
| `.reload battleground_template` | 3 (Administrator) | Syntax: .reload battleground_template Reload Battleground Templates. |
| `.reload broadcast_text` | 3 (Administrator) | Syntax: .reload broadcast_text Reload broadcast_text table. |
| `.reload chat_filter` | 3 (Administrator) | Syntax: .reload chat_filter Reload the `chat_filter` table. |
| `.reload command` | 3 (Administrator) | Syntax: .reload command Reload command table. |
| `.reload conditions` | 3 (Administrator) | Reload conditions table. |
| `.reload config` | 3 (Administrator) | Syntax: .reload config Reload config settings (by default stored in worldserver.conf). Not all settings can be change at reload: some new setting values will be ignored until restart, some values will applied with delay or only to new objects/maps, some values will explicitly rejected to change a... |
| `.reload creature_linked_respawn` | 3 (Administrator) | Syntax: .reload creature_linked_respawn Reload creature_linked_respawn table. |
| `.reload creature_loot_template` | 3 (Administrator) | Syntax: .reload creature_loot_template Reload creature_loot_template table. |
| `.reload creature_movement_override` | 3 (Administrator) | Syntax: .reload creature_movement_override Reload creature_movement_override table. |
| `.reload creature_onkill_reputation` | 3 (Administrator) | Syntax: .reload creature_onkill_reputation Reload creature_onkill_reputation table. |
| `.reload creature_questender` | 3 (Administrator) | Syntax: .reload creature_questender Reload creature_questender table. |
| `.reload creature_queststarter` | 3 (Administrator) | Syntax: .reload creature_queststarter Reload creature_queststarter table. |
| `.reload creature_template` | 3 (Administrator) | Syntax: .reload creature_template $entry Reload the specified creature's template. |
| `.reload creature_template_locale` | 3 (Administrator) | Syntax: .reload creature_template_locale Reload creature_template_locale table. |
| `.reload creature_text` | 3 (Administrator) | Syntax: .reload creature_text Reload creature_text table. |
| `.reload creature_text_locale` | 3 (Administrator) | Syntax: .reload creature_text_locale Reload creature_text_locale Table. |
| `.reload disables` | 3 (Administrator) | Syntax: .reload disables Reload disables table. |
| `.reload disenchant_loot_template` | 3 (Administrator) | Syntax: .reload disenchant_loot_template Reload disenchant_loot_template table. |
| `.reload dungeon_access_requirements` | 3 (Administrator) | Syntax: .reload dungeon_access_requirements Reload dungeon_access_requirements table. |
| `.reload dungeon_access_template` | 3 (Administrator) | Syntax: .reload dungeon_access_template Reload dungeon_access_template table. |
| `.reload event_scripts` | 3 (Administrator) | Syntax: .reload event_scripts Reload event_scripts table. |
| `.reload fishing_loot_template` | 3 (Administrator) | Syntax: .reload fishing_loot_template Reload fishing_loot_template table. |
| `.reload game_event_npc_vendor` | 3 (Administrator) | Syntax: .reload game_event_npc_vendor Reload game_event_npc_vendor table. |
| `.reload game_graveyard` | 3 (Administrator) | Syntax: .reload game_graveyard Reload game_graveyard table. |
| `.reload game_tele` | 3 (Administrator) | Syntax: .reload game_tele Reload game_tele table. |
| `.reload gameobject_loot_template` | 3 (Administrator) | Syntax: .reload gameobject_loot_template Reload gameobject_loot_template table. |
| `.reload gameobject_questender` | 3 (Administrator) | Syntax: .reload gameobject_questender\nReload gameobject_questender table. |
| `.reload gameobject_queststarter` | 3 (Administrator) | Syntax: .reload gameobject_queststarter Reload gameobject_queststarter table. |
| `.reload gameobject_template_locale` | 3 (Administrator) | Syntax: .reload gameobject_template_locale Reload gameobject_template_locale table. |
| `.reload gm_tickets` | 3 (Administrator) | Syntax: .reload gm_tickets Reload gm_tickets table. |
| `.reload gossip_menu` | 3 (Administrator) | Syntax: .reload gossip_menu Reload gossip_menu table. |
| `.reload gossip_menu_option` | 3 (Administrator) | Syntax: .reload gossip_menu_option Reload gossip_menu_option table. |
| `.reload gossip_menu_option_locale` | 3 (Administrator) | Syntax: .reload gossip_menu_option_locale Reload gossip_menu_option_locale table. |
| `.reload graveyard_zone` | 3 (Administrator) | Syntax: .reload graveyard_zone |
| `.reload item_enchantment_template` | 3 (Administrator) | Syntax: .reload item_enchantment_template Reload item_enchantment_template table. |
| `.reload item_loot_template` | 3 (Administrator) | Syntax: .reload item_loot_template Reload item_loot_template table. |
| `.reload item_set_name_locale` | 3 (Administrator) | Syntax: .reload item_set_name_locale Reload item_set_name_locale table. |
| `.reload item_set_names` | 3 (Administrator) | Syntax: .reload item_set_names Reload item_set_names table. |
| `.reload item_template_locale` | 3 (Administrator) | Syntax: .reload item_template_locale Reload item_template_locale table. |
| `.reload lfg_dungeon_rewards` | 3 (Administrator) | Syntax: .reload lfg_dungeon_rewards Reload lfg_dungeon_rewards table. |
| `.reload mail_level_reward` | 3 (Administrator) | Syntax: .reload mail_level_reward Reload mail_level_reward table. |
| `.reload mail_loot_template` | 3 (Administrator) | Syntax: .reload quest_mail_loot_template Reload quest_mail_loot_template table. |
| `.reload mail_server_template` | 3 (Administrator) | Syntax: .reload mail_server_template Reload server_mail_template table. |
| `.reload milling_loot_template` | 3 (Administrator) | Syntax: .reload milling_loot_template Reload milling_loot_template table. |
| `.reload module_string` | 3 (Administrator) | Syntax: .reload module_string |
| `.reload motd` | 3 (Administrator) | Syntax: .reload motd Reload motd table. |
| `.reload npc_spellclick_spells` | 3 (Administrator) | Syntax: .reload npc_spellclick_spells Reload npc_spellclick_spells table. |
| `.reload npc_text_locale` | 3 (Administrator) | Syntax: .reload npc_text_locale Reload npc_text_locale table. |
| `.reload npc_vendor` | 3 (Administrator) | Syntax: .reload npc_vendor Reload npc_vendor table. |
| `.reload page_text` | 3 (Administrator) | Syntax: .reload page_text Reload page_text table. You need to delete your client cache or change the cache number in the config in order for your players see the changes. |
| `.reload page_text_locale` | 3 (Administrator) | Syntax: .reload page_text_locale Reload page_text_locale table. You need to delete your client cache or change the cache number in config in order for your players see the changes. |
| `.reload pickpocketing_loot_template` | 3 (Administrator) | Syntax: .reload pickpocketing_loot_template Reload pickpocketing_loot_template table. |
| `.reload player_loot_template` | 3 (Administrator) | Syntax: .reload player_loot_template Reload player_loot_template table. |
| `.reload points_of_interest` | 3 (Administrator) | Syntax: .reload points_of_interest Reload points_of_interest table. |
| `.reload points_of_interest_locale` | 3 (Administrator) | Syntax: .reload points_of_interest_locale Reload points_of_interest_locale table. |
| `.reload profanity_name` | 3 (Administrator) | Syntax: .reload profanity_name Reloads profanity_name table. |
| `.reload prospecting_loot_template` | 3 (Administrator) | Syntax: .reload prospecting_loot_template Reload prospecting_loot_template table. |
| `.reload quest_greeting` | 3 (Administrator) | Syntax: .reload quest_greeting Reload quest_greeting table. |
| `.reload quest_offer_reward_locale` | 3 (Administrator) | Syntax: .reload quest_offer_reward_locale Reloads quest_offer_reward_locale table. |
| `.reload quest_poi` | 3 (Administrator) | Syntax: .reload quest_poi Reload quest_poi table. |
| `.reload quest_request_item_locale` | 3 (Administrator) | Syntax: .reload quest_request_item_locale Reloads quest_request_item_locale table. |
| `.reload quest_template` | 3 (Administrator) | Syntax: .reload quest_template Reload quest_template table. |
| `.reload quest_template_locale` | 3 (Administrator) | Syntax: .reload quest_template_locale Reload quest_template_locale table. |
| `.reload rbac` | 3 (Administrator) | Syntax: .reload rbac Reload rbac system. |
| `.reload reference_loot_template` | 3 (Administrator) | Syntax: .reload reference_loot_template Reload reference_loot_template table. |
| `.reload reputation_reward_rate` | 3 (Administrator) | Syntax: .reload reputation_reward_rate Reloads reputation_reward_rate table. |
| `.reload reputation_spillover_template` | 3 (Administrator) | Syntax: .reload reputation_spillover_template Reloads reputation_spillover_template table. |
| `.reload reserved_name` | 3 (Administrator) | Syntax: .reload reserved_name Reload reserved_name table. |
| `.reload skill_discovery_template` | 3 (Administrator) | Syntax: .reload skill_discovery_template Reload skill_discovery_template table. |
| `.reload skill_extra_item_template` | 3 (Administrator) | Syntax: .reload skill_extra_item_template Reload skill_extra_item_template table. |
| `.reload skill_fishing_base_level` | 3 (Administrator) | Syntax: .reload skill_fishing_base_level Reload skill_fishing_base_level table. |
| `.reload skinning_loot_template` | 3 (Administrator) | Syntax: .reload skinning_loot_template Reload skinning_loot_template table. |
| `.reload smart_scripts` | 3 (Administrator) | Syntax: .reload smart_scripts Reload smart_scripts table. |
| `.reload spawn_group` | 3 (Administrator) | Syntax: .reload spawn_group Reloads the spawn_group_template and spawn_group tables. |
| `.reload spell_area` | 3 (Administrator) | Syntax: .reload spell_area Reload spell_area table. |
| `.reload spell_bonus_data` | 3 (Administrator) | Syntax: .reload spell_bonus_data Reload spell_bonus_data table. |
| `.reload spell_group` | 3 (Administrator) | Syntax: .reload spell_group Reload spell_group table. |
| `.reload spell_group_stack_rules` | 3 (Administrator) | Syntax: .reload spell_group Reload spell_group_stack_rules table. |
| `.reload spell_linked_spell` | 3 (Administrator) | Usage: .reload spell_linked_spell Reloads the spell_linked_spell DB table. |
| `.reload spell_loot_template` | 3 (Administrator) | Syntax: .reload spell_loot_template Reload spell_loot_template table. |
| `.reload spell_pet_auras` | 3 (Administrator) | Syntax: .reload spell_pet_auras Reload spell_pet_auras table. |
| `.reload spell_proc` | 3 (Administrator) | Syntax: .reload spell_proc Reload spell_proc table. |
| `.reload spell_required` | 3 (Administrator) | Syntax: .reload spell_required Reload spell_required table. |
| `.reload spell_scripts` | 3 (Administrator) | Syntax: .reload spell_scripts Reload spell_scripts table. |
| `.reload spell_target_position` | 3 (Administrator) | Syntax: .reload spell_target_position Reload spell_target_position table. |
| `.reload spell_threats` | 3 (Administrator) | Syntax: .reload spell_threats Reload spell_threats table. |
| `.reload trainer` | 3 (Administrator) | Syntax: .reload trainer Reloads trainer,trainer_locale,trainer_spell and creature_default_trainer tables. |
| `.reload vehicle_accessory` | 3 (Administrator) | Syntax: .reload vehicle_accessory Reloads GUID-based vehicle accessory definitions from the database. |
| `.reload vehicle_template_accessory` | 3 (Administrator) | Syntax: .reload vehicle_template_accessory Reloads entry-based vehicle accessory definitions from the database. |
| `.reload warden_action` | 3 (Administrator) | Syntax: .reload warden_action Reloads warden_action table. |
| `.reload waypoint_data` | 3 (Administrator) | Syntax: .reload waypoint_data will reload waypoint_data table. |
| `.reload waypoint_scripts` | 3 (Administrator) | Syntax: .reload waypoint_scripts Reload waypoint_scripts table. |

## reset

| Command | Level | Description |
|---------|-------|-------------|
| `.reset` | 3 (Administrator) | Syntax: .reset $subcommand Type .reset to see the list of possible subcommands or .help reset $subcommand to see info on subcommands |
| `.reset achievements` | 4 (Console only) | Syntax: .reset achievements [$playername] Reset achievements data for selected or named (online or offline) character. Achievements for persistance progress data like completed quests/etc re-filled at reset. Achievements for events like kills/casts/etc will lost. |
| `.reset all` | 4 (Console only) | Syntax: .reset all spells Syntax: .reset all talents Syntax: .reset all honor Syntax: .reset all arena Requests a reset of spells or talents (including talents for all of a character's pets, if any) at the next login for each existing character, or immediately resets honor points or arena points... |
| `.reset honor` | 3 (Administrator) | Syntax: .reset honor [Playername] Reset all honor data for targeted character. |
| `.reset items` | 3 (Administrator) | Syntax : .reset items equipped\|bags\|bank\|keyring\|currency\|vendor_buyback\|all\|allbags #playername Delete items in the player inventory (equipped, bank, bags etc...) depending on the chosen option. #playername : Optional target player name (if player is online only). If not provided the comm... |
| `.reset items all` | 3 (Administrator) | Syntax : .reset items all #playername Delete all items in the selected player's inventory (equipped, in bags, in bank, in keyring, in currency list and in vendor buy back tab). #playername : Optional target player name (if player is online only). If not provided the command will execute on the se... |
| `.reset items allbags` | 3 (Administrator) | Syntax : .reset items allbags #playername Delete all items in the selected player's inventory (equipped, in bags, in bank, in keyring, in currency list and in vendor buy back tab) This command also deletes the bags. #playername : Optional target player name (if player is online only). If not prov... |
| `.reset items bags` | 3 (Administrator) | Syntax : .reset items bags #playername Delete all items in the selected player's bags. #playername : Optional target player name (if player is online only). If not provided the command will execute on the selected target player. |
| `.reset items bank` | 3 (Administrator) | Syntax : .reset items bank #playername Delete all items in the selected player's bank. #playername : Optional target player name (if player is online only). If not provided the command will execute on the selected target player. |
| `.reset items currency` | 3 (Administrator) | Syntax : .reset items currency #playername Delete all items in the selected player's currencies list. #playername : Optional target player name (if player is online only). If not provided the command will execute on the selected target player. |
| `.reset items equipped` | 3 (Administrator) | Syntax : .reset items equipped #playername Delete all items equipped on the target player. #playername : Optional target player name (if player is online only). If not provided the command will execute on the selected target player. |
| `.reset items keyring` | 3 (Administrator) | Syntax : .reset items keyring #playername Delete all items in the selected player's keyring. #playername : Optional target player name (if player is online only). If not provided the command will execute on the selected target player. |
| `.reset items vendor_buyback` | 3 (Administrator) | Syntax : .reset items vendor_buyback #playername Delete all items in the selected player's vendor buyback tab. #playername : Optional target player name (if player is online only). If not provided the command will execute on the selected target player. |
| `.reset level` | 3 (Administrator) | Syntax: .reset level [Playername] Reset level to 1 including reset stats and talents.  Equipped items with greater level requirement can be lost. |
| `.reset spells` | 3 (Administrator) | Syntax: .reset spells [Playername] Removes all non-original spells from spellbook. . Playername can be name of offline character. |
| `.reset stats` | 3 (Administrator) | Syntax: .reset stats [Playername] Resets(recalculate) all stats of the targeted player to their original VALUESat current level. |
| `.reset talents` | 3 (Administrator) | Syntax: .reset talents [Playername] Removes all talents of the targeted player or pet or named player. Playername can be name of offline character. With player talents also will be reset talents for all character's pets if any. |

## respawn

| Command | Level | Description |
|---------|-------|-------------|
| `.respawn` | 2 (Game Master) | Syntax: .respawn Respawn the selected unit without waiting respawn time expiration. |
| `.respawn all` | 2 (Game Master) | Syntax: .respawn all Respawn all nearest creatures and GO without waiting respawn time expiration. |
| `.respawn creature entry` | 3 (Administrator) | Syntax: .respawn creature entry #entry [#mapId [#instanceId]] In-game: forces all creatures with the given #entry to respawn on the current map. Console: specify #mapId (required) and optionally #instanceId. Pooled creatures are skipped in the queue phase to prevent duplicate spawns. |
| `.respawn creature guid` | 3 (Administrator) | Syntax: .respawn creature guid #spawnGuid Forces the creature with the given spawn GUID (database GUID) to respawn. Usable from console. |
| `.respawn gameobject entry` | 3 (Administrator) | Syntax: .respawn gameobject entry #entry [#mapId [#instanceId]] In-game: forces all gameobjects with the given #entry to respawn on the current map. Console: specify #mapId (required) and optionally #instanceId. Pooled gameobjects are skipped in the queue phase to prevent duplicate spawns. |
| `.respawn gameobject guid` | 3 (Administrator) | Syntax: .respawn gameobject guid #spawnGuid Forces the gameobject with the given spawn GUID (database GUID) to respawn. Usable from console. |

## revive

| Command | Level | Description |
|---------|-------|-------------|
| `.revive` | 2 (Game Master) | Syntax: .revive Revive the selected player. If no player is selected, it will revive you. |

## save

| Command | Level | Description |
|---------|-------|-------------|
| `.save` | 0 (Player) | Syntax: .save Saves your character. |

## saveall

| Command | Level | Description |
|---------|-------|-------------|
| `.saveall` | 2 (Game Master) | Syntax: .saveall Save all characters in game. |

## send

| Command | Level | Description |
|---------|-------|-------------|
| `.send` | 2 (Game Master) | Syntax: send $subcommand Type .send to see a list of possible subcommands or .help send $subcommand to see info on the subcommand. |
| `.send items` | 2 (Game Master) | Syntax: .send items #playername "#subject" "#text" itemid1[:count1] itemid2[:count2] ... itemidN[:countN] Send a mail to a player. Subject and mail text must be in "". If for itemid not provided related count values then expected 1, if count > max items in stack then items will be send in require... |
| `.send mail` | 2 (Game Master) | Syntax: .send mail #playername "#subject" "#text" Send a mail to a player. Subject and mail text must be in "". |
| `.send message` | 3 (Administrator) | Syntax: .send message $playername $message Send screen message to player from ADMINISTRATOR. |
| `.send money` | 2 (Game Master) | Syntax: .send money #playername "#subject" "#text" #money Send mail with money to a player. Subject and mail text must be in "". |

## server

| Command | Level | Description |
|---------|-------|-------------|
| `.server` | 3 (Administrator) | Syntax: .server $subcommand Type .server to see the list of possible subcommands or .help server $subcommand to see info on subcommands |
| `.server corpses` | 2 (Game Master) | Syntax: .server corpses Triggering corpses expire check in world. |
| `.server debug` | 3 (Administrator) | Syntax: .server debug Shows detailed information about the server setup, useful when reporting a bug. |
| `.server exit` | 4 (Console only) | Syntax: .server exit Terminate AzerothCore NOW. Exit code 0. |
| `.server idlerestart` | 4 (Console only) | Syntax: .server idlerestart #delay Restart the server after #delay if no active connections are present (no players). Use #exist_code or 2 as program exist code. #delay: use a timestring like "1h15m30s". |
| `.server idlerestart cancel` | 3 (Administrator) | Syntax: .server idlerestart cancel Cancel the restart/shutdown timer if any. |
| `.server idleshutdown` | 4 (Console only) | Syntax: .server idleshutdown #delay [#exist_code] Shut the server down after #delay if no active connections are present (no players). Use #exist_code or 0 as program exist code. #delay: use a timestring like "1h15m30s". |
| `.server idleshutdown cancel` | 3 (Administrator) | Syntax: .server idleshutdown cancel Cancel the restart/shutdown timer if any. |
| `.server info` | 0 (Player) | Syntax: .server info Display server version and the number of connected players. |
| `.server motd` | 0 (Player) | Syntax: .server motd Show server Message of the day. |
| `.server restart` | 3 (Administrator) | Syntax: .server restart #delay Restart the server after #delay. Use #exist_code or 2 as program exist code. #delay: use a timestring like "1h15m30s". |
| `.server restart cancel` | 3 (Administrator) | Syntax: .server restart cancel Cancel the restart/shutdown timer if any. |
| `.server set closed` | 4 (Console only) | Syntax: server set closed on/off Sets whether the world accepts new client connectsions. |
| `.server set loglevel` | 4 (Console only) | Syntax: .server set loglevel $facility $name $loglevel. $facility can take the values: appender (a) or logger (l). $loglevel can take the values: disabled (0), trace (1), debug (2), info (3), warn (4), error (5) or fatal (6) |
| `.server set motd` | 3 (Administrator) | Syntax: .server set motd Optional($realmId) Optional($locale) $MOTD Set server Message of the day for the specified $realmId. If $realmId is not provided it will update for the current realm. Use $realmId -1 to set motd for all realms. If $locale is not provided enUS will be used. |
| `.server shutdown` | 3 (Administrator) | Syntax: .server shutdown #delay [#exit_code] Shut the server down after #delay. Use #exit_code or 0 as program exit code. #delay: use a timestring like "1h15m30s". |
| `.server shutdown cancel` | 3 (Administrator) | Syntax: .server shutdown cancel Cancel the restart/shutdown timer if any. |

## setskill

| Command | Level | Description |
|---------|-------|-------------|
| `.setskill` | 2 (Game Master) | Syntax: .setskill #skill #level [#max] Set a skill of id #skill with a current skill value of #level and a maximum value of #max (or equal current maximum if not provide) for the selected character. If no character is selected, you learn the skill. |

## settings

| Command | Level | Description |
|---------|-------|-------------|
| `.settings` | 1 (Moderator) | Syntax: .settings $subcommand Type .setting to see the list of all available commands. |
| `.settings announcer` | 1 (Moderator) | Syntax: .settings announcer <type> <on/off>. Disables receiving announcements. Valid announcement types are: 'autobroadcast', 'arena' and 'bg' |

## showarea

| Command | Level | Description |
|---------|-------|-------------|
| `.showarea` | 2 (Game Master) | Syntax: .showarea #areaid Reveal the area of #areaid to the selected character. If no character is selected, reveal this area to you. |

## skirmish

| Command | Level | Description |
|---------|-------|-------------|
| `.skirmish` | 3 (Administrator) | Syntax: .skirmish [arena] [XvX] [Nick1] [Nick2] ... [NickN] [arena] can be "all" or comma-separated list of possible arenas (NA, BE, RL, DS, RV). [XvX] can be 1v1, 2v2, 3v3, 5v5. After [XvX] specify enough nicknames for that mode. |

## spect

| Command | Level | Description |
|---------|-------|-------------|
| `.spect` | 0 (Player) | Syntax: .spect $subcommand Type .spect to see the list of possible subcommands or .help spect $subcommand to see info on subcommands. |
| `.spect leave` | 0 (Player) | Syntax: .spect leave Leave an arena you are spectating. |
| `.spect reset` | 0 (Player) | Syntax: .spect reset Reset various values related to spectating. |
| `.spect spectate` | 0 (Player) | Syntax: .spect spectate #name Begin spectating the given player. |
| `.spect version` | 0 (Player) | Syntax: .spect version #version Verify addon version for arena spectating. |
| `.spect watch` | 0 (Player) | Syntax: .spect watch #name Begin watching the given player. |

## spellinfo

| Command | Level | Description |
|---------|-------|-------------|
| `.spellinfo` | 2 (Game Master) | Syntax: .spellinfo $subcommand Type .spellinfo to see a list of subcommands or .help spellinfo $subcommand to see info on subcommands. |
| `.spellinfo all` | 2 (Game Master) | Syntax: .spellinfo all #spellid Displays all available data for spell #spellid including attributes, general properties, effects and targets. |
| `.spellinfo attributes` | 2 (Game Master) | Syntax: .spellinfo attributes #spellid Displays basic info and attribute flags for spell #spellid including SpellAttr0-7, custom attributes, stances, dispel type and mechanic. |
| `.spellinfo effects` | 2 (Game Master) | Syntax: .spellinfo effects #spellid Displays effect data for spell #spellid including effect type, aura type, base points, multipliers, misc values, mechanic, trigger spell, amplitude and class mask per effect. |
| `.spellinfo targets` | 2 (Game Master) | Syntax: .spellinfo targets #spellid Displays target data for spell #spellid including target mask, creature type, max affected targets, and per-effect TargetA, TargetB, radius and chain targets. |

## string

| Command | Level | Description |
|---------|-------|-------------|
| `.string` | 2 (Game Master) | Syntax: .string #id [#locale] |

## summon

| Command | Level | Description |
|---------|-------|-------------|
| `.summon` | 2 (Game Master) | Syntax: .summon [$charactername] Teleport the given character to you. Character can be offline. |

## teleport

| Command | Level | Description |
|---------|-------|-------------|
| `.teleport` | 2 (Game Master) | Syntax: .teleport #location Teleport player to a given location. |
| `.teleport add` | 3 (Administrator) | Syntax: .teleport add $name Add current your position to .teleport command target locations list with name $name. |
| `.teleport del` | 3 (Administrator) | Syntax: .teleport del $name Remove location with name $name for .teleport command locations list. |
| `.teleport group` | 2 (Game Master) | Syntax: .teleport group#location Teleport a selected player and his group members to a given location. |
| `.teleport name` | 2 (Game Master) | Syntax: .teleport name [#playername] #location Teleport the given character to a given location. Character can be offline. To teleport to homebind, set #location to "$home" (without quotes). |
| `.teleport name npc guid` | 2 (Game Master) | Syntax: .teleport name id #playername #creatureSpawnId Teleport the given character to creature with spawn id #creatureSpawnId. Character can be offline. |
| `.teleport name npc id` | 2 (Game Master) | Syntax: .teleport name id #playername #creatureId Teleport the given character to first found creature with id #creatureId. Character can be offline. |
| `.teleport name npc name` | 2 (Game Master) | Syntax: .teleport name id #playername #creatureName Teleport the given character to first found creature with name (must match exactly) #creatureName. Character can be offline. |

## ticket

| Command | Level | Description |
|---------|-------|-------------|
| `.ticket` | 2 (Game Master) | Syntax: .ticket $subcommand Type .ticket to see the list of possible subcommands or .help ticket $subcommand to see info on subcommands |
| `.ticket assign` | 2 (Game Master) | Usage: .ticket assign $ticketid $gmname. Assigns the specified ticket to the specified Game Master. |
| `.ticket close` | 2 (Game Master) | Usage: .ticket close $ticketid. Closes the specified ticket. Does not delete permanently. |
| `.ticket closedlist` | 2 (Game Master) | Displays a list of closed GM tickets. |
| `.ticket comment` | 2 (Game Master) | Usage: .ticket comment $ticketid $comment. Allows the adding or modifying of a comment to the specified ticket. |
| `.ticket complete` | 2 (Game Master) | Syntax: .ticket complete #ticketID Mark a ticket of the given ID as complete. |
| `.ticket delete` | 3 (Administrator) | Usage: .ticket delete $ticketid. Deletes the specified ticket permanently. Ticket must be closed first. |
| `.ticket escalate` | 2 (Game Master) | Syntax: .ticket escalate #ticketID Add a ticket of the given ID to the escalation queue. |
| `.ticket escalatedlist` | 2 (Game Master) | Syntax: .ticket escalatedlist Return all open tickets in the escalation queue. |
| `.ticket list` | 2 (Game Master) | Displays a list of open GM tickets. |
| `.ticket onlinelist` | 2 (Game Master) | Displays a list of open GM tickets whose owner is online. |
| `.ticket reset` | 4 (Console only) | Syntax: .ticket reset Removes all closed tickets and resets the counter, if no pending open tickets are existing. |
| `.ticket response` | 2 (Game Master) | Syntax: .ticket response $subcommand Type .ticket response to see the list of possible subcommands or .help ticket response $subcommand to see info on subcommands. |
| `.ticket response append` | 2 (Game Master) | Add a response Syntax: ticket response append $ticketId $response |
| `.ticket response appendln` | 2 (Game Master) | Add a response to a new line. Syntax: ticket response appendln $ticketId $response |
| `.ticket response delete` | 2 (Game Master) | Delete a ticket response Syntax: ticket response delete $ticketId |
| `.ticket response show` | 2 (Game Master) | Show a ticket response Syntax: ticket response show $ticketId |
| `.ticket togglesystem` | 3 (Administrator) | Syntax: .ticket togglesystem Toggle whether tickets are allowed or disallowed. |
| `.ticket unassign` | 2 (Game Master) | Usage: .ticket unassign $ticketid. Unassigns the specified ticket from the current assigned Game Master. |
| `.ticket viewid` | 2 (Game Master) | Usage: .ticket viewid $ticketid. Returns details about specified ticket. Ticket must be open and not deleted. |
| `.ticket viewname` | 2 (Game Master) | Usage: .ticket viewname $creatorname. Returns details about specified ticket. Ticket must be open and not deleted. |

## titles

| Command | Level | Description |
|---------|-------|-------------|
| `.titles` | 2 (Game Master) | Syntax: .titles $subcommand Type .titles to see the list of possible subcommands or .help titles $subcommand to see info on subcommands. |
| `.titles add` | 2 (Game Master) | Syntax: .titles add #title Add title #title (id or shift-link) to known titles list for selected player. |
| `.titles current` | 2 (Game Master) | Syntax: .titles current #title Set title #title (id or shift-link) as current selected titl for selected player. If title not in known title list for player then it will be added to list. |
| `.titles remove` | 2 (Game Master) | Syntax: .titles remove #title Remove title #title (id or shift-link) from known titles list for selected player. |
| `.titles set` | 2 (Game Master) | Syntax: .titles set $subcommand Type .titles set to see the list of possible subcommands or .help titles set $subcommand to see info on subcommands. |
| `.titles set mask` | 2 (Game Master) | Syntax: .titles set mask #mask Allows user to use all titles from #mask. #mask=0 disables the title-choose-field |

## unaura

| Command | Level | Description |
|---------|-------|-------------|
| `.unaura` | 2 (Game Master) | Syntax: .unaura #spellid Remove aura due to spell #spellid from the selected Unit. |

## unban

| Command | Level | Description |
|---------|-------|-------------|
| `.unban` | 3 (Administrator) | Syntax: .unban $subcommand Type .unban to see the list of possible subcommands or .help unban $subcommand to see info on subcommands |
| `.unban account` | 3 (Administrator) | Syntax: .unban account $Name Unban accounts for account name pattern. |
| `.unban character` | 3 (Administrator) | Syntax: .unban character $Name Unban accounts for character name pattern. |
| `.unban ip` | 3 (Administrator) | Syntax : .unban ip $Ip Unban accounts for IP pattern. |
| `.unban playeraccount` | 3 (Administrator) | Syntax: .unban playeraccount #name Unban accounts for character name pattern. |

## unbindsight

| Command | Level | Description |
|---------|-------|-------------|
| `.unbindsight` | 3 (Administrator) | Syntax: .unbindsight Removes bound vision. Cannot be used while currently possessing a target. |

## unfreeze

| Command | Level | Description |
|---------|-------|-------------|
| `.unfreeze` | 2 (Game Master) | Syntax: .unfreeze (#player) "Unfreezes" #player and enables his chat again. When using this without #name it will unfreeze your target. |

## unlearn

| Command | Level | Description |
|---------|-------|-------------|
| `.unlearn` | 2 (Game Master) | Syntax: .unlearn #spell [all] Unlearn for selected player a spell #spell.  If 'all' provided then all ranks unlearned. |

## unmute

| Command | Level | Description |
|---------|-------|-------------|
| `.unmute` | 2 (Game Master) | Syntax: .unmute [$playerName] Restore chat messaging for any character from account of character $playerName (or selected). Character can be ofline. |

## unpossess

| Command | Level | Description |
|---------|-------|-------------|
| `.unpossess` | 2 (Game Master) | Syntax: .unpossess If you are possessed, unpossesses yourself; otherwise unpossesses current possessed target. |

## unstuck

| Command | Level | Description |
|---------|-------|-------------|
| `.unstuck` | 2 (Game Master) | Syntax: .unstuck $playername [inn/graveyard/startzone] Teleports specified player to specified location. Default location is player's current hearth location. |

## wchange

| Command | Level | Description |
|---------|-------|-------------|
| `.wchange` | 3 (Administrator) | Syntax: .wchange #weathertype #grade Set current weather to #weathertype with an intensity of #grade. #weathertype can be 0 for fine, 1 for rain, 2 for snow, 3 for storm, 86 for thunders, 90 for blackrain. #grade is a float value from 0.0 (disabled) to 1.0 (maximum intensity). |

## whispers

| Command | Level | Description |
|---------|-------|-------------|
| `.whispers` | 1 (Moderator) | Syntax: .whispers on\|off Enable/disable accepting whispers by GM from players. By default use trinityd.conf setting. |

## worldstate

| Command | Level | Description |
|---------|-------|-------------|
| `.worldstate scourgeinvasion battleswon` | 3 (Administrator) | Syntax: .worldstate scourgeinvasion battleswon <value> Adjusts the Scourge Invasion battles won count by <value> (can be negative). |
| `.worldstate scourgeinvasion show` | 3 (Administrator) | Syntax: .worldstate scourgeinvasion show Displays the current status of the Scourge Invasion. |
| `.worldstate scourgeinvasion startzone` | 3 (Administrator) | Syntax: .worldstate scourgeinvasion startzone <id> Starts a Scourge Invasion event in the zone specified by <id>. Valid zone IDs: 0-7. |
| `.worldstate scourgeinvasion state` | 3 (Administrator) | Syntax: .worldstate scourgeinvasion state <value> Sets the Scourge Invasion state. Valid values: 0: Disabled 1: Enabled |
| `.worldstate sunsreach counter` | 3 (Administrator) | Syntax: .worldstate sunsreach counter <index> <value> Sets a Suns Reach worldstate counter and displays current Suns Reach status. |
| `.worldstate sunsreach gate` | 3 (Administrator) | Syntax: .worldstate sunsreach gate <gate>. Sets the phase of Sunwell Plateau Gate. Valid values are: 0: All Gates Closed 1: Gate 1 Agamath Open 2: Gate 2 Rohendar Open 3: Gate 3 Archonisus Open. |
| `.worldstate sunsreach gatecounter` | 3 (Administrator) | Syntax: .worldstate sunsreach gatecounter <index> <value> Sets a Sunwell gate progression counter and displays current Suns Reach gate status. |
| `.worldstate sunsreach phase` | 3 (Administrator) | Syntax: .worldstate sunsreach phase <value>. Sets the phase of Sun's Reach. Valid values are: 0: Staging Area 1: Sanctum 2: Armory 3: Harbor. |
| `.worldstate sunsreach status` | 3 (Administrator) | Syntax: .worldstate sunsreach status Displays current Suns Reach and Sunwell gate progression status. |
| `.worldstate sunsreach subphase` | 3 (Administrator) | Syntax: .worldstate sunsreach subphase <mask>. Sets the subphase mask of Sun's Reach. Valid values are: 1: Portal 2: Anvil 4: Alchemy Lab 8: Monument 15: All. |

## wp

| Command | Level | Description |
|---------|-------|-------------|
| `.wp` | 3 (Administrator) | Syntax: wp $subcommand Type .wp to see a list of possible subcommands or .help wp $subcommand to see info on the subcommand. |
| `.wp add` | 3 (Administrator) | Syntax: .wp add Add a waypoint for the selected creature at your current position. |
| `.wp event` | 3 (Administrator) | Syntax: .wp event $subcommand Type .path event to see the list of possible subcommands or .help path event $subcommand to see info on subcommands. |
| `.wp load` | 3 (Administrator) | Syntax: .wp load $pathid Load pathid number for selected creature. Creature must have no waypoint data. |
| `.wp modify` | 3 (Administrator) | Syntax: |
| `.wp reload` | 3 (Administrator) | Syntax: .wp reload $pathid Load path changes ingame - IMPORTANT: must be applied first for new paths before .wp load #pathid |
| `.wp show` | 3 (Administrator) | Syntax: .wp show $option Options: on $pathid (or selected creature with loaded path) - Show path off - Hide path info $selected_waypoint - Show info for selected waypoint. |
| `.wp unload` | 3 (Administrator) | Syntax: .wp unload Unload path for selected creature. |

## wpgps

| Command | Level | Description |
|---------|-------|-------------|
| `.wpgps` | 3 (Administrator) | Syntax: .wpgps Output current position to sql developer log as partial SQL query to be used in pathing (formated for waypoint_data table). Use .wpgps sai for waypoint (SAI) table format. |

