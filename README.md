# mod-talent-progression

Aldrynth custom AzerothCore module: talent points from level 1, plus configurable first-kill / retro boss talent credits.

## Features

1. **Curve** — 1 talent point per level for levels 1–`TalentProgression.MaxLevel` (default 80). Replaces Blizzard’s “talents start at 10” curve. Does **not** use `Rate.Talent = 2`.
2. **Boss credits** — +1 permanent bonus talent per enabled credit ID (first earn only). Tracked in CharDB `aldr_first_kill_talent` by `credit_id`. Awarded to every real player who shared the kill (group / loot / threat in range), not only the last hit — playerbots and add last-hits do not steal the credit.
3. **Config allowlist** — `TalentProgression.EnabledCredits` controls which bosses grant talents. Remove an ID → clawed back on next login reconcile.
4. **Retro** — on login, missing raid credits proven by **achievements** (actual kill) are granted. Individual Progression hidden quests are not used: IP fills skipped ladder states (killing Onyxia also stamps “MC complete”), which is not a Ragnaros kill. Outdoor world bosses are kill-only (no history).
5. **Double Kel'Thuzad** — `kt_classic` (IP entry `351019`) and `kt_wotlk` (`15990`) are separate credits.

## Config

| Key | Default | Meaning |
|-----|---------|---------|
| `TalentProgression.Enable` | 1 | Master switch |
| `TalentProgression.Announce` | 0 | Verbose on world load |
| `TalentProgression.MaxLevel` | 80 | Cap for 1:1 curve |
| `TalentProgression.FirstKillEnable` | 1 | Boss credit awards |
| `TalentProgression.FirstKillAnnounce` | 1 | Whisper on award / revoke |
| `TalentProgression.ReconcileOnLogin` | 1 | Sync DB credits to allowlist on login |
| `TalentProgression.RetroEnable` | 1 | Grant proven past raid clears on login |
| `TalentProgression.EnabledCredits` | (full catalog) | Comma-separated credit IDs |

## Credit catalog

| Credit ID | Boss | Kill entries | Retro |
|-----------|------|--------------|-------|
| `azuregos` … `doomlord_kazzak` | Outdoor world bosses | vanilla entries | kill only |
| `onyxia` | Onyxia | `10184`, `301000` (IP) | ach 684 |
| `ragnaros` … `cthun` | Vanilla ends | standard | matching kill achievement |
| `kt_classic` | Kel'Thuzad (Classic Naxx40) | `351019` | ach 533 |
| `kt_wotlk` | Kel'Thuzad (WotLK) | `15990` | ach 574/575 |
| `gruul` … `kiljaeden` | TBC ends | standard | matching kill achievement |
| `malygos` … `algalon` | WotLK ends | standard | matching kill achievement |

## Install

```bash
cd modules
git submodule add https://github.com/VenomekPL/mod-talent-progression.git mod-talent-progression
# reconfigure CMake, rebuild, copy conf.dist → conf, restart worldserver
```

CharDB: `data/sql/db-characters/base/` (create) + `updates/` (credit_id migration).

Reset a character’s credits:

```sql
DELETE FROM aldr_first_kill_talent WHERE guid = <char_guid>;
-- also lower characters.extraBonusTalentCount if you need a full talent wipe
```

To drop a boss from the realm allowlist, remove its credit ID from `EnabledCredits` and restart (or reload conf); players lose that +1 on next login.
