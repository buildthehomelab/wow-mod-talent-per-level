# mod-talent-progression

Aldrynth custom AzerothCore module: talent points from level 1, plus first-kill boss talent rewards.

## Features

1. **Curve** — 1 talent point per level for levels 1–`TalentProgression.MaxLevel` (default 80). Replaces Blizzard’s “talents start at 10” curve. Does **not** use `Rate.Talent = 2`.
2. **First-kill bonuses** — +1 permanent bonus talent the first time a character kills an allowlisted outdoor world boss or canon raid endboss. Tracked in CharDB `aldr_first_kill_talent`.

## Config

| Key | Default | Meaning |
|-----|---------|---------|
| `TalentProgression.Enable` | 1 | Master switch |
| `TalentProgression.Announce` | 0 | Log on world load |
| `TalentProgression.MaxLevel` | 80 | Cap for 1:1 curve |
| `TalentProgression.FirstKillEnable` | 1 | Boss first-kill awards |
| `TalentProgression.FirstKillAnnounce` | 1 | Whisper killer on award |

## Allowlist (~28 bosses)

Outdoor: Azuregos, Lord Kazzak, green dragons, Doomwalker, Doom Lord Kazzak.

Raid endbosses: Onyxia, Ragnaros, Nefarian, Hakkar, Ossirian, C'Thun, Kel'Thuzad, Gruul, Magtheridon, Vashj, Kael'thas, Archimonde, Illidan, Zul'jin, Kil'jaeden, Malygos, Sartharion, Yogg-Saron, Anub'arak, Lich King, Halion, Algalon.

Edit `FirstKillBossEntries` in `src/TalentProgression_scripts.cpp` to expand/trim (requires rebuild).

## Install

```bash
cd modules
git submodule add https://github.com/VenomekPL/mod-talent-progression.git mod-talent-progression
# reconfigure CMake, rebuild, copy conf.dist → conf, restart worldserver
```

Character DB table is applied via module SQL updater (`data/sql/db-characters/base/`).

Reset a character’s first-kill awards during redesign:

```sql
DELETE FROM aldr_first_kill_talent WHERE guid = <char_guid>;
-- also clear characters.extraBonusTalentCount if you need a full talent wipe
```
