-- Native ChrCustomizationChoice appearance storage (3.4.3 Variant A)
DROP TABLE IF EXISTS `character_customizations`;
CREATE TABLE `character_customizations` (
    `guid` INT UNSIGNED NOT NULL,
    `chrCustomizationOptionID` INT UNSIGNED NOT NULL DEFAULT '0',
    `chrCustomizationChoiceID` INT UNSIGNED NOT NULL DEFAULT '0',
    PRIMARY KEY (`guid`, `chrCustomizationOptionID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DROP TABLE IF EXISTS `corpse_customizations`;
CREATE TABLE `corpse_customizations` (
    `ownerGuid` INT UNSIGNED NOT NULL,
    `chrCustomizationOptionID` INT UNSIGNED NOT NULL DEFAULT '0',
    `chrCustomizationChoiceID` INT UNSIGNED NOT NULL DEFAULT '0',
    PRIMARY KEY (`ownerGuid`, `chrCustomizationOptionID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
