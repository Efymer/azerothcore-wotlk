-- WotLK Classic 3.4.3 world handshake: store the full 64-byte bnet session key
-- negotiated in BnetRealmList::JoinRealm (clientSecret || serverSecret), plus the
-- client build and timezone offset, so the worldserver can derive and validate the
-- 40-byte world session key at the modern handshake. `session_key_bnet` later gets
-- overwritten with the derived 40-byte key by the world auth path.
ALTER TABLE `account`
    ADD COLUMN `session_key_bnet` binary(64) DEFAULT NULL AFTER `session_key`,
    ADD COLUMN `client_build` int unsigned DEFAULT NULL AFTER `session_key_bnet`,
    ADD COLUMN `timezone_offset` smallint DEFAULT NULL AFTER `client_build`;
