-- Widen account.os for the modern WotLK Classic 3.4.3 client OS strings.
-- The legacy 3.3.5a values were 3 chars ("Win"/"OSX"); the modern client reports 4-char forms
-- ("Wn64" = Win64, "Mc64" = macOS64). varchar(3) caused JoinRealm's UPDATE to fail with
-- MySQL error 1406 (Data too long), so session_key_bnet was never written -> world auth "unknown account".
ALTER TABLE `account` MODIFY COLUMN `os` varchar(10) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '';
