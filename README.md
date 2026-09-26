# mod-talent-progression

AzerothCore module: 1 talent point per level from level 1, so a level 80 character has 80 points instead of 71.

Stripped-down fork of [VenomekPL/mod-talent-progression](https://github.com/VenomekPL/mod-talent-progression) with only the talent curve. The boss first-kill credits, login reconcile and retro awards were removed, so there is no database table.

## How it works

Hooks `OnPlayerCalculateTalentsPoints` and replaces Blizzard's `level - 9` (starting at level 10) with `level`, capped at `TalentProgression.MaxLevel`. Bonus talents from `characters.extraBonusTalentCount` still stack on top. `Rate.Talent` in `worldserver.conf` still multiplies the result, so leave it at `1`.

## Config

| Key | Default | Meaning |
|-----|---------|---------|
| `TalentProgression.Enable` | 1 | Master switch |
| `TalentProgression.MaxLevel` | 80 | Cap for the 1:1 curve |

## Install

```bash
cd modules
git clone https://github.com/buildthehomelab/wow-mod-talent-progression.git mod-talent-progression
# reconfigure CMake, rebuild, copy conf.dist → conf, restart worldserver
```

Clone into `mod-talent-progression` (without the `wow-` prefix): AzerothCore derives the script loader name from the folder name.
