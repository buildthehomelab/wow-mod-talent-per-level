-- Migrate first-kill talent tracking from creature_entry to stable credit_id.
-- Allows config-driven allowlist reconcile and double Kel'Thuzad (classic + WotLK).

ALTER TABLE `aldr_first_kill_talent`
  ADD COLUMN `credit_id` VARCHAR(64) NOT NULL DEFAULT '' AFTER `guid`;

UPDATE `aldr_first_kill_talent` SET `credit_id` = CASE `creature_entry`
  WHEN 6109  THEN 'azuregos'
  WHEN 12397 THEN 'kazzak'
  WHEN 14887 THEN 'ysondre'
  WHEN 14888 THEN 'lethon'
  WHEN 14889 THEN 'emeriss'
  WHEN 14890 THEN 'taerar'
  WHEN 17711 THEN 'doomwalker'
  WHEN 18728 THEN 'doomlord_kazzak'
  WHEN 10184 THEN 'onyxia'
  WHEN 301000 THEN 'onyxia'
  WHEN 11502 THEN 'ragnaros'
  WHEN 11583 THEN 'nefarian'
  WHEN 14834 THEN 'hakkar'
  WHEN 15339 THEN 'ossirian'
  WHEN 15727 THEN 'cthun'
  WHEN 351019 THEN 'kt_classic'
  WHEN 15990 THEN 'kt_wotlk'
  WHEN 19044 THEN 'gruul'
  WHEN 17257 THEN 'magtheridon'
  WHEN 21212 THEN 'vashj'
  WHEN 19622 THEN 'kaelthas'
  WHEN 17968 THEN 'archimonde'
  WHEN 22917 THEN 'illidan'
  WHEN 23863 THEN 'zuljin'
  WHEN 25315 THEN 'kiljaeden'
  WHEN 28859 THEN 'malygos'
  WHEN 28860 THEN 'sartharion'
  WHEN 33288 THEN 'yogg_saron'
  WHEN 34564 THEN 'anubarak'
  WHEN 36597 THEN 'lich_king'
  WHEN 39863 THEN 'halion'
  WHEN 32871 THEN 'algalon'
  ELSE CONCAT('entry_', `creature_entry`)
END;

ALTER TABLE `aldr_first_kill_talent` DROP PRIMARY KEY;

ALTER TABLE `aldr_first_kill_talent`
  CHANGE COLUMN `creature_entry` `source_entry` INT UNSIGNED NOT NULL DEFAULT 0,
  ADD PRIMARY KEY (`guid`, `credit_id`);
