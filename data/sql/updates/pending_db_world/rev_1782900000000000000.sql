-- Class-by-expansion availability for 3.4.3 (54261). The legacy DBC ChrClasses `expansion`
-- field is gone in the DB2 layout; class gating now comes from this table (ObjectMgr::
-- LoadClassExpansionRequirements). WotLK: only Death Knight (class 6) requires the
-- Wrath of the Lich King expansion (level 2), for every playable race.
CREATE TABLE IF NOT EXISTS `class_expansion_requirement` (
    `ClassID` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `RaceID` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `ActiveExpansionLevel` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `AccountExpansionLevel` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`RaceID`, `ClassID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

DELETE FROM `class_expansion_requirement` WHERE `ClassID` = 6;
INSERT INTO `class_expansion_requirement` (`ClassID`, `RaceID`, `ActiveExpansionLevel`, `AccountExpansionLevel`) VALUES
(6, 1, 2, 2),
(6, 2, 2, 2),
(6, 3, 2, 2),
(6, 4, 2, 2),
(6, 5, 2, 2),
(6, 6, 2, 2),
(6, 7, 2, 2),
(6, 8, 2, 2),
(6, 10, 2, 2),
(6, 11, 2, 2);
