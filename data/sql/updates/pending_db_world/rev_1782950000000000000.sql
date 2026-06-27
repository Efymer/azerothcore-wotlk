-- Race-by-expansion availability for 3.4.3 (54261). The legacy DBC ChrRaces `expansion` field
-- is gone in the DB2 layout; race gating now comes from this table (ObjectMgr::
-- LoadRaceUnlockRequirements). WotLK: Draenei (11) and Blood Elf (10) require The Burning
-- Crusade (expansion 1); the original races need no row (ungated).
CREATE TABLE IF NOT EXISTS `race_unlock_requirement` (
    `raceID` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `expansion` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `achievementId` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`raceID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DELETE FROM `race_unlock_requirement` WHERE `raceID` IN (10, 11);
INSERT INTO `race_unlock_requirement` (`raceID`, `expansion`, `achievementId`) VALUES
(10, 1, 0),
(11, 1, 0);
