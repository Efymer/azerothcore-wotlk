-- Build 54261 (WotLK Classic 3.4.3) realm version advertisement.
-- The bnetserver derives each realm's advertised client version from build_info keyed by gamebuild.
-- Without a row for 54261 the modern client sees the realm as incompatible and stalls at realm select.
DELETE FROM `build_info` WHERE `build` = 54261;
INSERT INTO `build_info` (`build`, `majorVersion`, `minorVersion`, `bugfixVersion`, `hotfixVersion`, `winAuthSeed`, `win64AuthSeed`, `mac64AuthSeed`, `winChecksumSeed`, `macChecksumSeed`) VALUES
(54261, 3, 4, 3, '', NULL, '25FD812475DCF26F9F1383AED37FC99E', NULL, NULL, NULL);
