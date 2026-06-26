-- Battle.net account model for the modern bnetserver login flow.
-- Creates the battlenet_accounts and battlenet_account_bans tables and links
-- the existing grunt `account` table to a battlenet account via two columns.

DROP TABLE IF EXISTS `battlenet_accounts`;
CREATE TABLE `battlenet_accounts` (
    `id` int unsigned NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
    `email` varchar(320) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL,
    `srp_version` tinyint NOT NULL DEFAULT '1',
    `salt` binary(32) NOT NULL,
    `verifier` blob NOT NULL,
    `joindate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `last_ip` varchar(64) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL DEFAULT '127.0.0.1',
    `failed_logins` int unsigned NOT NULL DEFAULT '0',
    `locked` tinyint unsigned NOT NULL DEFAULT '0',
    `lock_country` varchar(2) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL DEFAULT '00',
    `last_login` timestamp NULL DEFAULT NULL,
    `online` tinyint unsigned NOT NULL DEFAULT '0',
    `locale` tinyint unsigned NOT NULL DEFAULT '0',
    `os` varchar(4) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL DEFAULT '',
    `LastCharacterUndelete` int unsigned NOT NULL DEFAULT '0',
    `LoginTicket` varchar(64) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` DEFAULT NULL,
    `LoginTicketExpiry` int unsigned DEFAULT NULL,
    PRIMARY KEY (`id`),
    UNIQUE KEY `idx_email` (`email`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Battle.net Account System';

DROP TABLE IF EXISTS `battlenet_account_bans`;
CREATE TABLE `battlenet_account_bans` (
    `id` int unsigned NOT NULL DEFAULT '0' COMMENT 'Account id',
    `bandate` int unsigned NOT NULL DEFAULT '0',
    `unbandate` int unsigned NOT NULL DEFAULT '0',
    `bannedby` varchar(50) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL,
    `banreason` varchar(255) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL,
    PRIMARY KEY (`id`,`bandate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Battle.net Ban List';

ALTER TABLE `account`
    ADD COLUMN `battlenet_account` int unsigned DEFAULT NULL AFTER `totaltime`,
    ADD COLUMN `battlenet_index` tinyint unsigned DEFAULT NULL AFTER `battlenet_account`,
    ADD UNIQUE KEY `uk_bnet_acc` (`battlenet_account`,`battlenet_index`),
    ADD CONSTRAINT `fk_bnet_acc` FOREIGN KEY (`battlenet_account`) REFERENCES `battlenet_accounts` (`id`);
