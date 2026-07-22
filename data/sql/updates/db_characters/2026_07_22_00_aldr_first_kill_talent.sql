-- Per-character first-kill talent awards (mod-talent-progression)
CREATE TABLE IF NOT EXISTS `aldr_first_kill_talent` (
    `guid` INT UNSIGNED NOT NULL,
    `creature_entry` INT UNSIGNED NOT NULL,
    `killed_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`guid`, `creature_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
