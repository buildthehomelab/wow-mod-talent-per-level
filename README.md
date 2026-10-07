# mod-talent-per-level

AzerothCore module: 1 talent point per level from level 1, so a level 80 character has 80 points instead of 71.

Based on [VenomekPL/mod-talent-progression](https://github.com/VenomekPL/mod-talent-progression), cut down to only the talent curve. The boss first-kill credits, login reconcile and retro awards were removed, so there is no database table.

## How it works

Hooks `OnPlayerCalculateTalentsPoints` and replaces Blizzard's `level - 9` (starting at level 10) with `level`, capped at `TalentPerLevel.MaxLevel`. Bonus talents from `characters.extraBonusTalentCount` still stack on top. `Rate.Talent` in `worldserver.conf` still multiplies the result, so leave it at `1`.

## Config

| Key | Default | Meaning |
|-----|---------|---------|
| `TalentPerLevel.Enable` | 1 | Master switch |
| `TalentPerLevel.MaxLevel` | 80 | Cap for the 1:1 curve |

## Requirements

- An [AzerothCore](https://github.com/azerothcore/azerothcore-wotlk) WotLK (master) server,
  recent enough to have the `OnPlayerCalculateTalentsPoints` player hook. The module has no other
  module or core-fork dependencies.
- A WoW 3.3.5a (12340) client. No client patch or addon is needed.
- `Rate.Talent = 1` in `worldserver.conf`.

## Install

```bash
cd modules
git clone https://github.com/buildthehomelab/wow-mod-talent-per-level.git mod-talent-per-level
# reconfigure CMake, rebuild, copy conf.dist → conf, restart worldserver
```

Clone into `mod-talent-per-level` (without the `wow-` prefix): AzerothCore derives the script loader name from the folder name.

## Troubleshooting

- **Characters still have the Blizzard talent count**: check `TalentPerLevel.Enable = 1`, that
  the folder is named `mod-talent-per-level` (otherwise AzerothCore never calls the loader), and
  that the worldserver log shows the "1 talent/level" line at startup.
- **The point total is not 1 per level**: `Rate.Talent` in `worldserver.conf` multiplies the
  result, so leave it at `1`. Points stop at `TalentPerLevel.MaxLevel`.
- **A character has extra points**: bonus talents from `characters.extraBonusTalentCount` stack
  on top of the curve.

## Credits

Author: [buildthehomelab](https://github.com/buildthehomelab)

Talent curve by [VenomekPL](https://github.com/VenomekPL) (Aldrynth), MIT licensed, from
[VenomekPL/mod-talent-progression](https://github.com/VenomekPL/mod-talent-progression).

## License

MIT, see [LICENSE](LICENSE).
