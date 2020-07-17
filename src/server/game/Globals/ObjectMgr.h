/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _OBJECTMGR_H
#define _OBJECTMGR_H

#include "Log.h"
#include "Object.h"
#include "Bag.h"
#include "Creature.h"
#include "DynamicObject.h"
#include "GameObject.h"
#include "TemporarySummon.h"
#include "Corpse.h"
#include "QuestDef.h"
#include "ItemPrototype.h"
#include "NPCHandler.h"
#include "DatabaseEnv.h"
#include "Mail.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectDefines.h"
#include "VehicleDefines.h"
#include "ItemIdGenerator.hpp"
#include "Utilities/Span.h"
#include <string>
#include <map>
#include <limits>
#include "ConditionMgr.h"
#include <functional>
#include <queue>

class Item;
struct AccessRequirement;
struct PlayerClassInfo;
struct PlayerClassLevelInfo;
struct PlayerInfo;
struct PlayerLevelInfo;

struct PageText
{
    std::string Text;
    uint16 NextPage;
};

/// Key for storing temp summon data in TempSummonDataContainer
struct TempSummonGroupKey
{
    TempSummonGroupKey(uint32 summonerEntry, SummonerType summonerType, uint8 group)
        : _summonerEntry(summonerEntry), _summonerType(summonerType), _summonGroup(group)
    {
    }

    bool operator<(TempSummonGroupKey const& rhs) const
    {
        return std::tie(_summonerEntry, _summonerType, _summonGroup) < std::tie(rhs._summonerEntry, rhs._summonerType, rhs._summonGroup);
    }

private:
    uint32 _summonerEntry;      ///< Summoner's entry
    SummonerType _summonerType; ///< Summoner's type, see SummonerType for available types
    uint8 _summonGroup;         ///< Summon's group id
};

// DB scripting commands
enum ScriptCommands
{
    SCRIPT_COMMAND_TALK                  = 0,                // source/target = Creature, target = any, datalong = talk type (0=say, 1=whisper, 2=yell, 3=emote text, 4=boss emote text), datalong2 & 1 = player talk (instead of creature), dataint = string_id
    SCRIPT_COMMAND_EMOTE                 = 1,                // source/target = Creature, datalong = emote id, datalong2 = 0: set emote state; > 0: play emote state
    SCRIPT_COMMAND_FIELD_SET             = 2,                // source/target = Creature, datalong = field id, datalog2 = value
    SCRIPT_COMMAND_MOVE_TO               = 3,                // source/target = Creature, datalong2 = time to reach, x/y/z = destination
    SCRIPT_COMMAND_FLAG_SET              = 4,                // source/target = Creature, datalong = field id, datalog2 = bitmask
    SCRIPT_COMMAND_FLAG_REMOVE           = 5,                // source/target = Creature, datalong = field id, datalog2 = bitmask
    SCRIPT_COMMAND_TELEPORT_TO           = 6,                // source/target = Creature/Player (see datalong2), datalong = map_id, datalong2 = 0: Player; 1: Creature, x/y/z = destination, o = orientation
    SCRIPT_COMMAND_QUEST_EXPLORED        = 7,                // target/source = Player, target/source = GO/Creature, datalong = quest id, datalong2 = distance or 0
    SCRIPT_COMMAND_KILL_CREDIT           = 8,                // target/source = Player, datalong = creature entry, datalong2 = 0: personal credit, 1: group credit
    SCRIPT_COMMAND_RESPAWN_GAMEOBJECT    = 9,                // source = WorldObject (summoner), datalong = GO guid, datalong2 = despawn delay
    SCRIPT_COMMAND_TEMP_SUMMON_CREATURE  = 10,               // source = WorldObject (summoner), datalong = creature entry, datalong2 = despawn delay, x/y/z = summon position, o = orientation
    SCRIPT_COMMAND_OPEN_DOOR             = 11,               // source = Unit, datalong = GO guid, datalong2 = reset delay (min 15)
    SCRIPT_COMMAND_CLOSE_DOOR            = 12,               // source = Unit, datalong = GO guid, datalong2 = reset delay (min 15)
    SCRIPT_COMMAND_ACTIVATE_OBJECT       = 13,               // source = Unit, target = GO
    SCRIPT_COMMAND_REMOVE_AURA           = 14,               // source (datalong2 != 0) or target (datalong2 == 0) = Unit, datalong = spell id
    SCRIPT_COMMAND_CAST_SPELL            = 15,               // source and/or target = Unit, datalong2 = cast direction (0: s->t 1: s->s 2: t->t 3: t->s 4: s->creature with dataint entry), dataint & 1 = triggered flag
    SCRIPT_COMMAND_PLAY_SOUND            = 16,               // source = WorldObject, target = none/Player, datalong = sound id, datalong2 (bitmask: 0/1=anyone/player, 0/2=without/with distance dependency, so 1|2 = 3 is target with distance dependency)
    SCRIPT_COMMAND_CREATE_ITEM           = 17,               // target/source = Player, datalong = item entry, datalong2 = amount
    SCRIPT_COMMAND_DESPAWN_SELF          = 18,               // target/source = Creature, datalong = despawn delay

    SCRIPT_COMMAND_LOAD_PATH             = 20,               // source = Unit, datalong = path id, datalong2 = is repeatable
    SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT    = 21,               // source = WorldObject (if present used as a search center), datalong = script id, datalong2 = unit lowguid, dataint = script table to use (see ScriptsType)
    SCRIPT_COMMAND_KILL                  = 22,               // source/target = Creature, dataint = remove corpse attribute

    // TrinityCore only
    SCRIPT_COMMAND_ORIENTATION           = 30,               // source = Unit, target (datalong > 0) = Unit, datalong = > 0 turn source to face target, o = orientation
    SCRIPT_COMMAND_EQUIP                 = 31,               // soucre = Creature, datalong = equipment id
    SCRIPT_COMMAND_MODEL                 = 32,               // source = Creature, datalong = model id
    SCRIPT_COMMAND_CLOSE_GOSSIP          = 33,               // source = Player
    SCRIPT_COMMAND_PLAYMOVIE             = 34,               // source = Player, datalong = movie id
    SCRIPT_COMMAND_UNITSTATE             = 35                // source = Creature, datalong = unitstate
};

// Benchmarked: Faster than std::unordered_map (insert/find)
typedef std::map<uint32, PageText> PageTextContainer;

// Benchmarked: Faster than std::map (insert/find)
typedef std::unordered_map<uint16, InstanceTemplate> InstanceTemplateContainer;

struct GameTele
{
    float  position_x;
    float  position_y;
    float  position_z;
    float  orientation;
    uint32 mapId;
    std::string name;
    std::wstring wnameLow;
    std::list<std::wstring> names;
};

typedef std::unordered_map<uint32, GameTele> GameTeleContainer;

enum ScriptsType
{
    SCRIPTS_FIRST = 1,

    SCRIPTS_SPELL = SCRIPTS_FIRST,
    SCRIPTS_EVENT,
    SCRIPTS_WAYPOINT,

    SCRIPTS_LAST
};

enum eScriptFlags
{
    // Talk Flags
    SF_TALK_USE_PLAYER          = 0x1,

    // Emote flags
    SF_EMOTE_USE_STATE          = 0x1,

    // TeleportTo flags
    SF_TELEPORT_USE_CREATURE    = 0x1,

    // KillCredit flags
    SF_KILLCREDIT_REWARD_GROUP  = 0x1,

    // RemoveAura flags
    SF_REMOVEAURA_REVERSE       = 0x1,

    // CastSpell flags
    SF_CASTSPELL_SOURCE_TO_TARGET = 0,
    SF_CASTSPELL_SOURCE_TO_SOURCE = 1,
    SF_CASTSPELL_TARGET_TO_TARGET = 2,
    SF_CASTSPELL_TARGET_TO_SOURCE = 3,
    SF_CASTSPELL_SEARCH_CREATURE  = 4,
    SF_CASTSPELL_TRIGGERED      = 0x1,

    // PlaySound flags
    SF_PLAYSOUND_TARGET_PLAYER  = 0x1,
    SF_PLAYSOUND_DISTANCE_SOUND = 0x2,

    // Orientation flags
    SF_ORIENTATION_FACE_TARGET  = 0x1
};

struct ScriptInfo
{
    ScriptsType type;
    uint32 id;
    uint32 delay;
    ScriptCommands command;

    union
    {
        struct
        {
            uint32 nData[3];
            float  fData[4];
        } Raw;

        struct                      // SCRIPT_COMMAND_TALK (0)
        {
            uint32 ChatType;        // datalong
            uint32 Flags;           // datalong2
            int32  TextID;          // dataint
        } Talk;

        struct                      // SCRIPT_COMMAND_EMOTE (1)
        {
            uint32 EmoteID;         // datalong
            uint32 Flags;           // datalong2
        } Emote;

        struct                      // SCRIPT_COMMAND_FIELD_SET (2)
        {
            uint32 FieldID;         // datalong
            uint32 FieldValue;      // datalong2
        } FieldSet;

        struct                      // SCRIPT_COMMAND_MOVE_TO (3)
        {
            uint32 Unused1;         // datalong
            uint32 TravelTime;      // datalong2
            int32  Unused2;         // dataint

            float DestX;
            float DestY;
            float DestZ;
        } MoveTo;

        struct                      // SCRIPT_COMMAND_FLAG_SET (4)
                                    // SCRIPT_COMMAND_FLAG_REMOVE (5)
        {
            uint32 FieldID;         // datalong
            uint32 FieldValue;      // datalong2
        } FlagToggle;

        struct                      // SCRIPT_COMMAND_TELEPORT_TO (6)
        {
            uint32 MapID;           // datalong
            uint32 Flags;           // datalong2
            int32  Unused1;         // dataint

            float DestX;
            float DestY;
            float DestZ;
            float Orientation;
        } TeleportTo;

        struct                      // SCRIPT_COMMAND_QUEST_EXPLORED (7)
        {
            uint32 QuestID;         // datalong
            uint32 Distance;        // datalong2
        } QuestExplored;

        struct                      // SCRIPT_COMMAND_KILL_CREDIT (8)
        {
            uint32 CreatureEntry;   // datalong
            uint32 Flags;           // datalong2
        } KillCredit;

        struct                      // SCRIPT_COMMAND_RESPAWN_GAMEOBJECT (9)
        {
            uint32 GOGuid;          // datalong
            uint32 DespawnDelay;    // datalong2
        } RespawnGameobject;

        struct                      // SCRIPT_COMMAND_TEMP_SUMMON_CREATURE (10)
        {
            uint32 CreatureEntry;   // datalong
            uint32 DespawnDelay;    // datalong2
            int32  Unused1;         // dataint

            float PosX;
            float PosY;
            float PosZ;
            float Orientation;
        } TempSummonCreature;

        struct                      // SCRIPT_COMMAND_CLOSE_DOOR (12)
                                    // SCRIPT_COMMAND_OPEN_DOOR (11)
        {
            uint32 GOGuid;          // datalong
            uint32 ResetDelay;      // datalong2
        } ToggleDoor;

                                    // SCRIPT_COMMAND_ACTIVATE_OBJECT (13)

        struct                      // SCRIPT_COMMAND_REMOVE_AURA (14)
        {
            uint32 SpellID;         // datalong
            uint32 Flags;           // datalong2
        } RemoveAura;

        struct                      // SCRIPT_COMMAND_CAST_SPELL (15)
        {
            uint32 SpellID;         // datalong
            uint32 Flags;           // datalong2
            int32  CreatureEntry;   // dataint

            float SearchRadius;
        } CastSpell;

        struct                      // SCRIPT_COMMAND_PLAY_SOUND (16)
        {
            uint32 SoundID;         // datalong
            uint32 Flags;           // datalong2
        } PlaySound;

        struct                      // SCRIPT_COMMAND_CREATE_ITEM (17)
        {
            uint32 ItemEntry;       // datalong
            uint32 Amount;          // datalong2
        } CreateItem;

        struct                      // SCRIPT_COMMAND_DESPAWN_SELF (18)
        {
            uint32 DespawnDelay;    // datalong
        } DespawnSelf;

        struct                      // SCRIPT_COMMAND_LOAD_PATH (20)
        {
            uint32 PathID;          // datalong
            uint32 IsRepeatable;    // datalong2
        } LoadPath;

        struct                      // SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT (21)
        {
            uint32 CreatureEntry;   // datalong
            uint32 ScriptID;        // datalong2
            uint32 ScriptType;      // dataint
        } CallScript;

        struct                      // SCRIPT_COMMAND_KILL (22)
        {
            uint32 Unused1;         // datalong
            uint32 Unused2;         // datalong2
            int32  RemoveCorpse;    // dataint
        } Kill;

        struct                      // SCRIPT_COMMAND_ORIENTATION (30)
        {
            uint32 Flags;           // datalong
            uint32 Unused1;         // datalong2
            int32  Unused2;         // dataint

            float Unused3;
            float Unused4;
            float Unused5;
            float Orientation;
        } Orientation;

        struct                      // SCRIPT_COMMAND_EQUIP (31)
        {
            uint32 EquipmentID;     // datalong
        } Equip;

        struct                      // SCRIPT_COMMAND_MODEL (32)
        {
            uint32 ModelID;         // datalong
        } Model;

                                    // SCRIPT_COMMAND_CLOSE_GOSSIP (33)

        struct                      // SCRIPT_COMMAND_PLAYMOVIE (34)
        {
            uint32 MovieID;         // datalong
        } PlayMovie;

        struct                      // SCRIPT_COMMAND_UNITSTATE (35)
        {
            uint8 UnitstandID;      // datalong
        } UnitStandState;
    };

    std::string GetDebugInfo() const;
};

typedef std::multimap<uint32, ScriptInfo> ScriptMap;
typedef std::map<uint32, ScriptMap > ScriptMapMap;
typedef std::multimap<uint32, uint32> SpellScriptsContainer;
typedef std::pair<SpellScriptsContainer::iterator, SpellScriptsContainer::iterator> SpellScriptsBounds;
extern ScriptMapMap sSpellScripts;
extern ScriptMapMap sEventScripts;
extern ScriptMapMap sWaypointScripts;

std::string GetScriptsTableNameByType(ScriptsType type);
ScriptMapMap* GetScriptsMapByType(ScriptsType type);
std::string GetScriptCommandName(ScriptCommands command);

struct SpellClickInfo
{
    uint32 spellId;
    uint8 castFlags;
    SpellClickUserTypes userType;

    // helpers
    bool IsFitToRequirements(Unit const* clicker, Unit const* clickee) const;
};

typedef std::multimap<uint32, SpellClickInfo> SpellClickInfoContainer;
typedef std::pair<SpellClickInfoContainer::const_iterator, SpellClickInfoContainer::const_iterator> SpellClickInfoMapBounds;

struct AreaTrigger
{
    uint32 target_mapId;
    float  target_X;
    float  target_Y;
    float  target_Z;
    float  target_Orientation;
};

struct BroadcastText
{
    BroadcastText() : Id(0), Language(0), EmoteId0(0), EmoteId1(0), EmoteId2(0),
                      EmoteDelay0(0), EmoteDelay1(0), EmoteDelay2(0), SoundId(0), Unk1(0), Unk2(0)
    {
        MaleText.resize(DEFAULT_LOCALE + 1);
        FemaleText.resize(DEFAULT_LOCALE + 1);
    }

    uint32 Id;
    uint32 Language;
    StringVector MaleText;
    StringVector FemaleText;
    uint32 EmoteId0;
    uint32 EmoteId1;
    uint32 EmoteId2;
    uint32 EmoteDelay0;
    uint32 EmoteDelay1;
    uint32 EmoteDelay2;
    uint32 SoundId;
    uint32 Unk1;
    uint32 Unk2;
    // uint32 VerifiedBuild;

    std::string const& GetText(LocaleConstant locale = DEFAULT_LOCALE, uint8 gender = GENDER_MALE, bool forceGender = false) const
    {
        if (gender == GENDER_FEMALE && (forceGender || !FemaleText[DEFAULT_LOCALE].empty()))
        {
            if (FemaleText.size() > size_t(locale) && !FemaleText[locale].empty())
                return FemaleText[locale];
            return FemaleText[DEFAULT_LOCALE];
        }
        // else if (gender == GENDER_MALE)
        {
            if (MaleText.size() > size_t(locale) && !MaleText[locale].empty())
                return MaleText[locale];
            return MaleText[DEFAULT_LOCALE];
        }
    }
};

typedef std::unordered_map<uint32, BroadcastText> BroadcastTextContainer;

typedef std::set<uint32> CellGuidSet;
typedef std::map<uint32/*player guid*/, uint32/*instance*/> CellCorpseSet;
struct CellObjectGuids
{
    CellGuidSet creatures;
    CellGuidSet gameobjects;
    CellCorpseSet corpses;
};
typedef std::unordered_map<uint32/*cell_id*/, CellObjectGuids> CellObjectGuidsMap;
typedef std::unordered_map<uint32/*(mapid, spawnMode) pair*/, CellObjectGuidsMap> MapObjectGuids;

// Trinity string ranges
#define MIN_TRINITY_STRING_ID           1                    // 'trinity_string'
#define MAX_TRINITY_STRING_ID           2000000000
#define MIN_DB_SCRIPT_STRING_ID        MAX_TRINITY_STRING_ID // 'db_script_string'
#define MAX_DB_SCRIPT_STRING_ID        2000010000

// Trinity Trainer Reference start range
#define TRINITY_TRAINER_START_REF      200000

struct TrinityStringLocale
{
    StringVector Content;
};

typedef std::map<ObjectGuid, ObjectGuid> LinkedRespawnContainer;
typedef std::unordered_map<uint32, CreatureData> CreatureDataContainer;
typedef std::unordered_map<uint32, GameObjectData> GameObjectDataContainer;
typedef std::map<TempSummonGroupKey, std::vector<TempSummonData> > TempSummonDataContainer;
typedef std::unordered_map<uint32, CreatureLocale> CreatureLocaleContainer;
typedef std::unordered_map<uint32, GameObjectLocale> GameObjectLocaleContainer;
typedef std::unordered_map<uint32, ItemLocale> ItemLocaleContainer;
typedef std::unordered_map<uint32, ItemSetNameLocale> ItemSetNameLocaleContainer;
typedef std::unordered_map<uint32, QuestLocale> QuestLocaleContainer;
typedef std::unordered_map<uint32, NpcTextLocale> NpcTextLocaleContainer;
typedef std::unordered_map<uint32, PageTextLocale> PageTextLocaleContainer;
typedef std::unordered_map<int32, TrinityStringLocale> TrinityStringLocaleContainer;
typedef std::unordered_map<uint32, GossipMenuItemsLocale> GossipMenuItemsLocaleContainer;
typedef std::unordered_map<uint32, PointOfInterestLocale> PointOfInterestLocaleContainer;

typedef std::multimap<uint32, uint32> QuestRelations; // unit/go -> quest
typedef std::multimap<uint32, uint32> QuestRelationsReverse; // quest -> unit/go
typedef std::pair<QuestRelations::const_iterator, QuestRelations::const_iterator> QuestRelationBounds;
typedef std::pair<QuestRelationsReverse::const_iterator, QuestRelationsReverse::const_iterator> QuestRelationReverseBounds;

struct PetLevelInfo
{
    PetLevelInfo() : health(0), mana(0), armor(0) { for (uint8 i=0; i < MAX_STATS; ++i) stats[i] = 0; }

    uint16 stats[MAX_STATS];
    uint16 health;
    uint16 mana;
    uint16 armor;
};

struct MailLevelReward
{
    MailLevelReward() : raceMask(0), mailTemplateId(0), senderEntry(0) { }
    MailLevelReward(uint32 _raceMask, uint32 _mailTemplateId, uint32 _senderEntry) : raceMask(_raceMask), mailTemplateId(_mailTemplateId), senderEntry(_senderEntry) { }

    uint32 raceMask;
    uint32 mailTemplateId;
    uint32 senderEntry;
};

typedef std::list<MailLevelReward> MailLevelRewardList;
typedef std::unordered_map<uint8, MailLevelRewardList> MailLevelRewardContainer;

struct MailLevelRewardExtended
{
    MailLevelRewardExtended() : level(0), raceMask(0), flags(0), senderName(0), subject(0), body(0)  { }
    MailLevelRewardExtended(uint8 _level, uint32 _raceMask, uint32 _flags, std::string _senderName, std::string _subject, std::string _body) : 
        level(_level), raceMask(_raceMask), flags(_flags), senderName(_senderName), subject(_subject), body(_body) { }

    uint8 level;
    uint32 raceMask;
    uint32 flags;
    std::string senderName;
    std::string subject;
    std::string body;
};

typedef std::list<MailLevelRewardExtended> MailLevelRewardExtendedList;
typedef std::unordered_map<uint8, MailLevelRewardExtendedList> MailLevelRewardExtendedContainer;

// We assume the rate is in general the same for all three types below, but chose to keep three for scalability and customization
struct RepRewardRate
{
    float questRate;            // We allow rate = 0.0 in database. For this case, it means that
    float questDailyRate;
    float questWeeklyRate;
    float questMonthlyRate;
    float questRepeatableRate;
    float creatureRate;         // no reputation are given at all for this faction/rate type.
    float spellRate;
};

struct ReputationOnKillEntry
{
    uint32 RepFaction1;
    uint32 RepFaction2;
    uint32 ReputationMaxCap1;
    int32 RepValue1;
    uint32 ReputationMaxCap2;
    int32 RepValue2;
    bool IsTeamAward1;
    bool IsTeamAward2;
    bool TeamDependent;
};

struct RepSpilloverTemplate
{
    uint32 faction[MAX_SPILLOVER_FACTIONS];
    float faction_rate[MAX_SPILLOVER_FACTIONS];
    uint32 faction_rank[MAX_SPILLOVER_FACTIONS];
};

struct PointOfInterest
{
    uint32 entry;
    float x;
    float y;
    uint32 icon;
    uint32 flags;
    uint32 data;
    std::string icon_name;
};

struct GossipMenuItems
{
    uint32          MenuId;
    uint32          OptionIndex;
    uint8           OptionIcon;
    std::string     OptionText;
    uint32          OptionBroadcastTextId;
    uint32          OptionType;
    uint32          OptionNpcflag;
    uint32          ActionMenuId;
    uint32          ActionPoiId;
    bool            BoxCoded;
    uint32          BoxMoney;
    std::string     BoxText;
    uint32          BoxBroadcastTextId;
    ConditionContainer   Conditions;
};

struct GossipMenus
{
    uint32          entry;
    uint32          text_id;
    ConditionContainer   conditions;
};

typedef std::multimap<uint32, GossipMenus> GossipMenusContainer;
typedef std::pair<GossipMenusContainer::const_iterator, GossipMenusContainer::const_iterator> GossipMenusMapBounds;
typedef std::pair<GossipMenusContainer::iterator, GossipMenusContainer::iterator> GossipMenusMapBoundsNonConst;
typedef std::multimap<uint32, GossipMenuItems> GossipMenuItemsContainer;
typedef std::pair<GossipMenuItemsContainer::const_iterator, GossipMenuItemsContainer::const_iterator> GossipMenuItemsMapBounds;
typedef std::pair<GossipMenuItemsContainer::iterator, GossipMenuItemsContainer::iterator> GossipMenuItemsMapBoundsNonConst;

struct QuestPOIPoint
{
    int32 x;
    int32 y;

    QuestPOIPoint() : x(0), y(0) { }
    QuestPOIPoint(int32 _x, int32 _y) : x(_x), y(_y) { }
};

struct QuestPOI
{
    uint32 Id;
    int32 ObjectiveIndex;
    uint32 MapId;
    uint32 AreaId;
    uint32 FloorId;
    uint32 Unk3;
    uint32 Unk4;
    std::vector<QuestPOIPoint> points;

    QuestPOI() : Id(0), ObjectiveIndex(0), MapId(0), AreaId(0), FloorId(0), Unk3(0), Unk4(0) { }
    QuestPOI(uint32 id, int32 objIndex, uint32 mapId, uint32 areaId, uint32 floorId, uint32 unk3, uint32 unk4) : Id(id), ObjectiveIndex(objIndex), MapId(mapId), AreaId(areaId), FloorId(floorId), Unk3(unk3), Unk4(unk4) { }
};

typedef std::vector<QuestPOI> QuestPOIVector;
typedef std::unordered_map<uint32, QuestPOIVector> QuestPOIContainer;

struct TicketQuestState
{
    uint32 quest;
    uint32 ticket;
    std::string state;
    
    TicketQuestState() : quest(0), ticket(0), state("") {}
    TicketQuestState(uint32 questId, uint32 ticketId, std::string State) : quest(questId), ticket(ticketId), state(State) {}
};

typedef std::unordered_map<uint32, TicketQuestState> QuestStateContainer;

struct GraveYardData
{
    uint32 safeLocId;
    uint32 team;
};

typedef std::multimap<uint32, GraveYardData> GraveYardContainer;
typedef std::pair<GraveYardContainer::const_iterator, GraveYardContainer::const_iterator> GraveYardMapBounds;
typedef std::pair<GraveYardContainer::iterator, GraveYardContainer::iterator> GraveYardMapBoundsNonConst;

typedef std::unordered_map<uint32, VendorItemData> CacheVendorItemContainer;
typedef std::unordered_map<uint32, TrainerSpellData> CacheTrainerSpellContainer;

enum SkillRangeType
{
    SKILL_RANGE_LANGUAGE,                                   // 300..300
    SKILL_RANGE_LEVEL,                                      // 1..max skill for level
    SKILL_RANGE_MONO,                                       // 1..1, grey monolite bar
    SKILL_RANGE_RANK,                                       // 1..skill for known rank
    SKILL_RANGE_NONE                                        // 0..0 always
};

SkillRangeType GetSkillRangeType(SkillRaceClassInfoEntry const* rcEntry);

#define MAX_PLAYER_NAME          12                         // max allowed by client name length
#define MAX_INTERNAL_PLAYER_NAME 15                         // max server internal player name length (> MAX_PLAYER_NAME for support declined names)
#define MAX_PET_NAME             12                         // max allowed by client name length
#define MAX_CHARTER_NAME         24                         // max allowed by client name length

GAME_API bool normalizePlayerName(std::string& name);

struct LanguageDesc
{
    Language lang_id = LANG_ADDON;
    uint32   spell_id = 255;
    uint32   skill_id = 255;
    bool valid = true;

    constexpr explicit operator bool() const { return valid; }
};

GAME_API LanguageDesc GetLanguageDescByID(uint32 lang);
GAME_API LanguageDesc GetLanguageDescByIndex(uint32 index);

enum EncounterCreditType
{
    ENCOUNTER_CREDIT_KILL_CREATURE  = 0,
    ENCOUNTER_CREDIT_CAST_SPELL     = 1
};

struct DungeonEncounter
{
    DungeonEncounter(DungeonEncounterEntry const* _dbcEntry, EncounterCreditType _creditType, uint32 _creditEntry, uint32 _lastEncounterDungeon, std::string&& ScriptName)
        : dbcEntry(_dbcEntry), creditType(_creditType), creditEntry(_creditEntry), lastEncounterDungeon(_lastEncounterDungeon), ScriptName(std::move(ScriptName)) { }

    DungeonEncounterEntry const* dbcEntry;
    EncounterCreditType creditType;
    uint32 creditEntry;
    uint32 lastEncounterDungeon;
    std::string ScriptName;
};

constexpr uint32 NO_ENCOUNTER = std::numeric_limits<uint32>::max();

using DungeonEncounterVector = std::vector<DungeonEncounter>;
using DungeonEncounterSpan = span<DungeonEncounter>;
using DungeonEncounterContainer = std::unordered_map<uint32, DungeonEncounterVector>;

class PlayerDumpReader;

#define ANTICHEAT_ACTIONS 2
#define ANTICHEAT_CHECK_PARAMETERS 2

struct AntiCheatConfig
{
    AntiCheatConfig() : checkType(0), alarmsCount(0),disableOperation(false), messageNum(0)
    {
        for (int i=0; i < ANTICHEAT_ACTIONS; ++i)
        {
            actionType[i] = 0;
            actionParam[i] = 0;
        };

        for (int i=0; i < ANTICHEAT_CHECK_PARAMETERS; ++i)
        {
            checkParam[i] = 0;
        }

        for (int i=0; i < ANTICHEAT_CHECK_PARAMETERS; ++i)
        {
            checkFloatParam[i] = 0.0f;
        }
    }
    uint32 checkType;
    uint32 checkPeriod;
    uint32 alarmsCount;
    bool   disableOperation;
    uint32 messageNum;
    uint32 checkParam[ANTICHEAT_CHECK_PARAMETERS];
    float  checkFloatParam[ANTICHEAT_CHECK_PARAMETERS];
    uint32 actionType[ANTICHEAT_ACTIONS];
    uint32 actionParam[ANTICHEAT_ACTIONS];
    std::string description;
};

typedef std::tuple<uint32, int32, int32, uint32> PolyIdentifier;
typedef std::unordered_map<PolyIdentifier, std::vector<uint32>, boost::hash<PolyIdentifier>> PolygonMap;

class GAME_API ObjectMgr
{
    friend class PlayerDumpReader;

    private:
        ObjectMgr();
        ~ObjectMgr();

    public:
        ObjectMgr(ObjectMgr const&) = delete;
        ObjectMgr(ObjectMgr&&) = delete;

        ObjectMgr& operator= (ObjectMgr const&) = delete;
        ObjectMgr& operator= (ObjectMgr&&) = delete;

        static ObjectMgr* instance();

        typedef std::unordered_map<uint32, Item*> ItemMap;

        typedef std::unordered_map<uint32, Quest> QuestMap;

        typedef std::unordered_map<uint32, AreaTrigger> AreaTriggerContainer;

        typedef std::unordered_map<uint32, uint32> AreaTriggerScriptContainer;

        typedef std::unordered_map<uint32, AccessRequirement*> AccessRequirementContainer;

        typedef std::unordered_map<uint32, RepRewardRate > RepRewardRateContainer;
        typedef std::unordered_map<uint32, ReputationOnKillEntry> RepOnKillContainer;
        typedef std::unordered_map<uint32, RepSpilloverTemplate> RepSpilloverTemplateContainer;

        typedef std::unordered_map<uint32, PointOfInterest> PointOfInterestContainer;

        using ScriptNameContainer = std::vector<std::string>;
        using ScriptIndexContainer = std::unordered_map<std::string_view, uint32>;

        typedef std::map<uint32, uint32> CharacterConversionMap;

        typedef std::map<uint32, AntiCheatConfig> AntiCheatConfigMap;
        AntiCheatConfigMap m_AntiCheatConfig;               // [check_type]
        AntiCheatConfig const* GetAntiCheatConfig(uint32 checkType) const;
        void LoadAntiCheatConfig();

        GameObjectTemplate const* GetGameObjectTemplate(uint32 entry);
        GameObjectTemplateContainer const* GetGameObjectTemplates() const { return &_gameObjectTemplateStore; }
        int LoadReferenceVendor(int32 vendor, int32 item_id, std::set<uint32> *skip_vendors);

        void LoadGameObjectTemplate();
        void AddGameobjectInfo(GameObjectTemplate* goinfo);

        CreatureTemplate const* GetCreatureTemplate(uint32 entry);
        CreatureTemplateContainer const* GetCreatureTemplates() const { return &_creatureTemplateStore; }
        CreatureModelInfo const* GetCreatureModelInfo(uint32 modelId);
        CreatureModelInfo const* GetCreatureModelRandomGender(uint32* displayID);
        static uint32 ChooseDisplayId(CreatureTemplate const* cinfo, CreatureData const* data = NULL);
        static void ChooseCreatureFlags(CreatureTemplate const* cinfo, uint32& npcflag, uint32& unit_flags, uint32& dynamicflags, CreatureData const* data = NULL);
        EquipmentInfo const* GetEquipmentInfo(uint32 entry, int8& id);
        CreatureAddon const* GetCreatureAddon(uint32 lowguid);
        GameObjectAddon const* GetGameObjectAddon(uint32 lowguid);
        VendorItemCounts const* GetCreatureVendorItemCounts(uint32 lowguid);
        CreatureAddon const* GetCreatureTemplateAddon(uint32 entry);
        ItemTemplate const* GetItemTemplate(uint32 entry);
        ItemTemplateContainer const* GetItemTemplateStore() const { return &_itemTemplateStore; }

        ItemSetNameEntry const* GetItemSetNameEntry(uint32 itemId)
        {
            ItemSetNameContainer::iterator itr = _itemSetNameStore.find(itemId);
            if (itr != _itemSetNameStore.end())
                return &itr->second;
            return NULL;
        }

        InstanceTemplate const* GetInstanceTemplate(uint32 mapId);

        PetLevelInfo const* GetPetLevelInfo(uint32 creature_id, uint8 level) const;

        PlayerClassInfo const* GetPlayerClassInfo(uint32 class_) const
        {
            if (class_ >= MAX_CLASSES)
                return NULL;
            return _playerClassInfo[class_];
        }
        void GetPlayerClassLevelInfo(uint32 class_, uint8 level, PlayerClassLevelInfo* info) const;

        PlayerInfo const* GetPlayerInfo(uint32 race, uint32 class_) const;

        void GetPlayerLevelInfo(uint32 race, uint32 class_, uint8 level, PlayerLevelInfo* info) const;

        uint32 GetNearestTaxiNode(float x, float y, float z, uint32 mapid, uint32 team);
        void GetTaxiPath(uint32 source, uint32 destination, uint32 &path, uint32 &cost);
        uint32 GetTaxiMountDisplayId(uint32 id, uint32 team, bool allowed_alt_team = false);

        Quest const* GetQuestTemplate(uint32 quest_id) const
        {
            QuestMap::const_iterator itr = _questTemplates.find(quest_id);
            return itr != _questTemplates.end() ? &itr->second : NULL;
        }

        QuestMap const& GetQuestTemplates() const { return _questTemplates; }

        uint32 GetQuestForAreaTrigger(uint32 Trigger_ID) const
        {
            QuestAreaTriggerContainer::const_iterator itr = _questAreaTriggerStore.find(Trigger_ID);
            if (itr != _questAreaTriggerStore.end())
                return itr->second;
            return 0;
        }

        bool IsTavernAreaTrigger(uint32 Trigger_ID) const
        {
            return _tavernAreaTriggerStore.find(Trigger_ID) != _tavernAreaTriggerStore.end();
        }

        // Will return 0 if trigger_Id is not an inn trigger.
        uint32 GetFactionForTavernTrigger(uint32 trigger_Id) const
        {
            if (IsTavernAreaTrigger(trigger_Id))
                return _tavernAreaTriggerStore.at(trigger_Id);
            else
                return 0;
        }

        bool IsGameObjectForQuests(uint32 entry) const
        {
            return _gameObjectForQuestStore.find(entry) != _gameObjectForQuestStore.end();
        }

        bool IsGameObjectStaticTransport(uint32 entry);

        GossipText const* GetGossipText(uint32 Text_ID) const;

        WorldSafeLocsEntry const* GetDefaultGraveYard(uint32 team);
        WorldSafeLocsEntry const* GetClosestGraveYard(float x, float y, float z, uint32 MapId, uint32 team);
        bool AddGraveYardLink(uint32 id, uint32 zoneId, uint32 team, bool persist = true);
        void RemoveGraveYardLink(uint32 id, uint32 zoneId, uint32 team, bool persist = false);
        void LoadGraveyardZones();
        GraveYardData const* FindGraveYardData(uint32 id, uint32 zone);

        AreaTrigger const* GetAreaTrigger(uint32 trigger) const
        {
            AreaTriggerContainer::const_iterator itr = _areaTriggerStore.find(trigger);
            if (itr != _areaTriggerStore.end())
                return &itr->second;
            return NULL;
        }

        AccessRequirement const* GetAccessRequirement(uint32 mapid, Difficulty difficulty) const
        {
            AccessRequirementContainer::const_iterator itr = _accessRequirementStore.find(MAKE_PAIR32(mapid, difficulty));
            if (itr != _accessRequirementStore.end())
                return itr->second;
            return NULL;
        }

        std::string_view GetAreaScript(uint32 ID) const;

        AreaTrigger const* GetGoBackTrigger(uint32 Map) const;
        AreaTrigger const* GetMapEntranceTrigger(uint32 Map) const;

        uint32 GetAreaTriggerScriptId(uint32 trigger_id);
        SpellScriptsBounds GetSpellScriptsBounds(uint32 spellId);

        RepRewardRate const* GetRepRewardRate(uint32 factionId) const
        {
            RepRewardRateContainer::const_iterator itr = _repRewardRateStore.find(factionId);
            if (itr != _repRewardRateStore.end())
                return &itr->second;

            return NULL;
        }

        ReputationOnKillEntry const* GetReputationOnKilEntry(uint32 id) const
        {
            RepOnKillContainer::const_iterator itr = _repOnKillStore.find(id);
            if (itr != _repOnKillStore.end())
                return &itr->second;
            return NULL;
        }

        int32 GetBaseReputationOf(FactionEntry const* factionEntry, uint8 race, uint8 playerClass);

        RepSpilloverTemplate const* GetRepSpilloverTemplate(uint32 factionId) const
        {
            RepSpilloverTemplateContainer::const_iterator itr = _repSpilloverTemplateStore.find(factionId);
            if (itr != _repSpilloverTemplateStore.end())
                return &itr->second;

            return NULL;
        }

        PointOfInterest const* GetPointOfInterest(uint32 id) const
        {
            PointOfInterestContainer::const_iterator itr = _pointsOfInterestStore.find(id);
            if (itr != _pointsOfInterestStore.end())
                return &itr->second;
            return NULL;
        }

        QuestPOIVector const* GetQuestPOIVector(uint32 questId)
        {
            QuestPOIContainer::const_iterator itr = _questPOIStore.find(questId);
            if (itr != _questPOIStore.end())
                return &itr->second;
            return NULL;
        }
        
        TicketQuestState const* GetQuestState(uint32 questId)
        {
            QuestStateContainer::const_iterator itr = _questStateStore.find(questId);
            if (itr != _questStateStore.end())
                return &itr->second;
            return NULL;
        }

        VehicleAccessoryList const* GetVehicleAccessoryList(Vehicle* veh) const;

        DungeonEncounterSpan GetDungeonEncounterList(uint32 mapId, Difficulty difficulty) const;
        uint32 GetEncounterForCreature(uint32 entry, Difficulty difficulty) const;

        void LoadQuests();
        void LoadQuestStartersAndEnders()
        {
            TC_LOG_INFO("server.loading", "Loading GO Start Quest Data...");
            LoadGameobjectQuestStarters();
            TC_LOG_INFO("server.loading", "Loading GO End Quest Data...");
            LoadGameobjectQuestEnders();
            TC_LOG_INFO("server.loading", "Loading Creature Start Quest Data...");
            LoadCreatureQuestStarters();
            TC_LOG_INFO("server.loading", "Loading Creature End Quest Data...");
            LoadCreatureQuestEnders();
        }
        void LoadGameobjectQuestStarters();
        void LoadGameobjectQuestEnders();
        void LoadCreatureQuestStarters();
        void LoadCreatureQuestEnders();

        QuestRelations* GetGOQuestRelationMap()
        {
            return &_goQuestRelations;
        }

        QuestRelationBounds GetGOQuestRelationBounds(uint32 go_entry)
        {
            return _goQuestRelations.equal_range(go_entry);
        }

        QuestRelationBounds GetGOQuestInvolvedRelationBounds(uint32 go_entry)
        {
            return _goQuestInvolvedRelations.equal_range(go_entry);
        }

        QuestRelationReverseBounds GetGOQuestInvolvedRelationReverseBounds(uint32 questId)
        {
            return _goQuestInvolvedRelationsReverse.equal_range(questId);
        }

        QuestRelations* GetCreatureQuestRelationMap()
        {
            return &_creatureQuestRelations;
        }

        QuestRelationBounds GetCreatureQuestRelationBounds(uint32 creature_entry)
        {
            return _creatureQuestRelations.equal_range(creature_entry);
        }

        QuestRelationBounds GetCreatureQuestInvolvedRelationBounds(uint32 creature_entry)
        {
            return _creatureQuestInvolvedRelations.equal_range(creature_entry);
        }

        QuestRelationReverseBounds GetCreatureQuestInvolvedRelationReverseBounds(uint32 questId)
        {
            return _creatureQuestInvolvedRelationsReverse.equal_range(questId);
        }

        void LoadEventScripts();
        void LoadSpellScripts();
        void LoadWaypointScripts();

        void LoadSpellScriptNames();
        void ValidateSpellScripts();

        void LoadBroadcastTexts();
        void LoadBroadcastTextLocales();
        bool LoadTrinityStrings(char const* table, int32 min_value, int32 max_value);
        bool LoadTrinityStrings() { return LoadTrinityStrings("trinity_string", MIN_TRINITY_STRING_ID, MAX_TRINITY_STRING_ID); }
        void LoadDbScriptStrings();
        void LoadCreatureClassLevelStats();
        void LoadCreatureLocales();
        void LoadCreatureTemplates();
        void LoadCreatureTemplateAddons();
        void CheckCreatureTemplate(CreatureTemplate const* cInfo);
        void LoadTempSummons();
        void LoadCreatures();
        void LoadLinkedRespawn();
        bool SetCreatureLinkedRespawn(uint32 guid, uint32 linkedGuid);
        void LoadCreatureAddons();
        void LoadGameObjectAddons();
        void LoadCreatureVendorItemCount();
        void LoadCreatureModelInfo();
        void LoadEquipmentTemplates();
        void LoadGameObjectLocales();
        void LoadGameobjects();
        void LoadItemTemplates();
        void LoadItemLocales();
        void LoadItemSetNames();
        void LoadItemSetNameLocales();
        void LoadQuestLocales();
        void LoadNpcTextLocales();
        void LoadPageTextLocales();
        void LoadGossipMenuItemsLocales();
        void LoadPointOfInterestLocales();
        void LoadInstanceTemplate();
        void LoadInstanceEncounters();
        void LoadMailLevelRewards();
        void LoadMailLevelRewardsExtended();
        void LoadVehicleTemplateAccessories();
        void LoadVehicleAccessories();

        void LoadGossipText();

        void LoadAreaTriggerTeleports();
        void LoadAccessRequirements();
        void LoadQuestAreaTriggers();
        void LoadAreaTriggerScripts();
        void LoadAreaScripts();
        void LoadTavernAreaTriggers();
        void LoadGameObjectForQuests();

        void LoadPageTexts();
        PageText const* GetPageText(uint32 pageEntry);

        void LoadPlayerInfo();
        void LoadPetLevelInfo();
        void LoadExplorationBaseXP();
        void LoadPetNames();
        void LoadPetNumber();
        void LoadCorpses();
        void LoadFishingBaseSkillLevel();

        void LoadReputationRewardRate();
        void LoadReputationOnKill();
        void LoadReputationSpilloverTemplate();

        void LoadPointsOfInterest();
        void LoadQuestPOI();
        
        void LoadQuestStates();

        void LoadNPCSpellClickSpells();

        void LoadGameTele();

        void LoadGossipMenu();
        void LoadGossipMenuItems();

        void LoadVendors();
        void LoadTrainerSpell();
        void AddSpellToTrainer(uint32 entry, uint32 spell, uint32 spellCost, uint32 reqSkill, uint32 reqSkillValue, uint32 reqLevel);

        std::string GeneratePetName(uint32 entry);
        uint32 GetBaseXP(uint8 level);
        uint32 GetXPForLevel(uint8 level) const;

        int32 GetFishingBaseSkillLevel(uint32 entry) const
        {
            FishingBaseSkillContainer::const_iterator itr = _fishingBaseForAreaStore.find(entry);
            return itr != _fishingBaseForAreaStore.end() ? itr->second : 0;
        }

        void ReturnOrDeleteOldMails(bool serverUp);

        CreatureBaseStats const* GetCreatureBaseStats(uint8 level, uint8 unitClass);

        void SetHighestGuids();
        uint32 GenerateLowGuid(HighGuid guidhigh);
        uint32 GenerateAuctionID();
        uint64 GenerateEquipmentSetGuid();
        uint32 GenerateMailID();
        uint32 GeneratePetNumber();

        typedef std::multimap<int32, uint32> ExclusiveQuestGroups;
        typedef std::pair<ExclusiveQuestGroups::const_iterator, ExclusiveQuestGroups::const_iterator> ExclusiveQuestGroupsBounds;

        ExclusiveQuestGroups mExclusiveQuestGroups;

        MailLevelReward const* GetMailLevelReward(uint32 level, uint32 raceMask)
        {
            MailLevelRewardContainer::const_iterator map_itr = _mailLevelRewardStore.find(level);
            if (map_itr == _mailLevelRewardStore.end())
                return NULL;

            for (MailLevelRewardList::const_iterator set_itr = map_itr->second.begin(); set_itr != map_itr->second.end(); ++set_itr)
                if (set_itr->raceMask & raceMask)
                    return &*set_itr;

            return NULL;
        }
        
        MailLevelRewardExtended const* GetMailLevelRewardExtended(uint32 level, uint32 raceMask)
        {
            MailLevelRewardExtendedContainer::const_iterator map_itr = _mailLevelRewardExtendedStore.find(level);
            if (map_itr == _mailLevelRewardExtendedStore.end())
                return NULL;

            for (MailLevelRewardExtendedList::const_iterator set_itr = map_itr->second.begin(); set_itr != map_itr->second.end(); ++set_itr)
                if (set_itr->raceMask & raceMask)
                    return &*set_itr;

            return NULL;
        }

        CellObjectGuids const& GetCellObjectGuids(uint16 mapid, uint8 spawnMode, uint32 cell_id)
        {
            return _mapObjectGuidsStore[MAKE_PAIR32(mapid, spawnMode)][cell_id];
        }

        CellObjectGuidsMap const& GetMapObjectGuids(uint16 mapid, uint8 spawnMode)
        {
            return _mapObjectGuidsStore[MAKE_PAIR32(mapid, spawnMode)];
        }

        /**
         * Gets temp summon data for all creatures of specified group.
         *
         * @param summonerId   Summoner's entry.
         * @param summonerType Summoner's type, see SummonerType for available types.
         * @param group        Id of required group.
         *
         * @return null if group was not found, otherwise reference to the creature group data
         */
        std::vector<TempSummonData> const* GetSummonGroup(uint32 summonerId, SummonerType summonerType, uint8 group) const
        {
            TempSummonDataContainer::const_iterator itr = _tempSummonDataStore.find(TempSummonGroupKey(summonerId, summonerType, group));
            if (itr != _tempSummonDataStore.end())
                return &itr->second;

            return NULL;
        }

        BroadcastText const* GetBroadcastText(uint32 id) const
        {
            BroadcastTextContainer::const_iterator itr = _broadcastTextStore.find(id);
            if (itr != _broadcastTextStore.end())
                return &itr->second;
            return NULL;
        }

        CreatureData const* GetCreatureData(uint32 guid) const
        {
            CreatureDataContainer::const_iterator itr = _creatureDataStore.find(guid);
            if (itr == _creatureDataStore.end()) return NULL;
            return &itr->second;
        }
        CreatureData& NewOrExistCreatureData(uint32 guid) { return _creatureDataStore[guid]; }
        void DeleteCreatureData(uint32 guid);
        ObjectGuid GetLinkedRespawnGuid(ObjectGuid guid) const
        {
            LinkedRespawnContainer::const_iterator itr = _linkedRespawnStore.find(guid);
            if (itr == _linkedRespawnStore.end()) return ObjectGuid::Empty;
            return itr->second;
        }
        CreatureLocale const* GetCreatureLocale(uint32 entry) const
        {
            CreatureLocaleContainer::const_iterator itr = _creatureLocaleStore.find(entry);
            if (itr == _creatureLocaleStore.end()) return NULL;
            return &itr->second;
        }
        GameObjectLocale const* GetGameObjectLocale(uint32 entry) const
        {
            GameObjectLocaleContainer::const_iterator itr = _gameObjectLocaleStore.find(entry);
            if (itr == _gameObjectLocaleStore.end()) return NULL;
            return &itr->second;
        }
        ItemLocale const* GetItemLocale(uint32 entry) const
        {
            ItemLocaleContainer::const_iterator itr = _itemLocaleStore.find(entry);
            if (itr == _itemLocaleStore.end()) return NULL;
            return &itr->second;
        }
        ItemSetNameLocale const* GetItemSetNameLocale(uint32 entry) const
        {
            ItemSetNameLocaleContainer::const_iterator itr = _itemSetNameLocaleStore.find(entry);
            if (itr == _itemSetNameLocaleStore.end())return NULL;
            return &itr->second;
        }
        QuestLocale const* GetQuestLocale(uint32 entry) const
        {
            QuestLocaleContainer::const_iterator itr = _questLocaleStore.find(entry);
            if (itr == _questLocaleStore.end()) return NULL;
            return &itr->second;
        }
        NpcTextLocale const* GetNpcTextLocale(uint32 entry) const
        {
            NpcTextLocaleContainer::const_iterator itr = _npcTextLocaleStore.find(entry);
            if (itr == _npcTextLocaleStore.end()) return NULL;
            return &itr->second;
        }
        PageTextLocale const* GetPageTextLocale(uint32 entry) const
        {
            PageTextLocaleContainer::const_iterator itr = _pageTextLocaleStore.find(entry);
            if (itr == _pageTextLocaleStore.end()) return NULL;
            return &itr->second;
        }
        GossipMenuItemsLocale const* GetGossipMenuItemsLocale(uint32 entry) const
        {
            GossipMenuItemsLocaleContainer::const_iterator itr = _gossipMenuItemsLocaleStore.find(entry);
            if (itr == _gossipMenuItemsLocaleStore.end()) return NULL;
            return &itr->second;
        }
        PointOfInterestLocale const* GetPointOfInterestLocale(uint32 poi_id) const
        {
            PointOfInterestLocaleContainer::const_iterator itr = _pointOfInterestLocaleStore.find(poi_id);
            if (itr == _pointOfInterestLocaleStore.end()) return NULL;
            return &itr->second;
        }

        GameObjectData const* GetGOData(uint32 guid) const
        {
            GameObjectDataContainer::const_iterator itr = _gameObjectDataStore.find(guid);
            if (itr == _gameObjectDataStore.end()) return NULL;
            return &itr->second;
        }
        GameObjectData& NewGOData(uint32 guid) { return _gameObjectDataStore[guid]; }
        void DeleteGOData(uint32 guid);

        TrinityStringLocale const* GetTrinityStringLocale(int32 entry) const
        {
            TrinityStringLocaleContainer::const_iterator itr = _trinityStringLocaleStore.find(entry);
            if (itr == _trinityStringLocaleStore.end()) return NULL;
            return &itr->second;
        }
        const char *GetTrinityString(int32 entry, LocaleConstant locale_idx) const;
        const char *GetTrinityStringForDBCLocale(int32 entry) const { return GetTrinityString(entry, DBCLocaleIndex); }
        LocaleConstant GetDBCLocaleIndex() const { return DBCLocaleIndex; }
        void SetDBCLocaleIndex(LocaleConstant locale) { DBCLocaleIndex = locale; }

        void AddCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid, uint32 instance);
        void DeleteCorpseCellData(uint32 mapid, uint32 cellid, uint32 player_guid);

        // grid objects
        void AddCreatureToGrid(uint32 guid, CreatureData const* data);
        void RemoveCreatureFromGrid(uint32 guid, CreatureData const* data);
        void AddGameobjectToGrid(uint32 guid, GameObjectData const* data);
        void RemoveGameobjectFromGrid(uint32 guid, GameObjectData const* data);
        uint32 AddGOData(uint32 entry, uint32 map, float x, float y, float z, float o, uint32 spawntimedelay = 0, float rotation0 = 0, float rotation1 = 0, float rotation2 = 0, float rotation3 = 0);
        uint32 AddCreData(uint32 entry, uint32 map, float x, float y, float z, float o, uint32 spawntimedelay = 0);
        bool MoveCreData(uint32 guid, uint32 map, const Position& pos);

        // reserved names
        void LoadReservedPlayersNames();
        bool IsReservedName(std::string const& name) const;
        bool AddReservedName(const std::string& name);
        bool DeleteReservedName(const std::string& name);

        // name with valid structure and symbols
        static uint8 CheckPlayerName(std::string const& name, bool create = false);
        static PetNameInvalidReason CheckPetName(std::string const& name);
        static bool IsValidCharterName(std::string const& name);

        static bool CheckDeclinedNames(const std::wstring& w_ownname, DeclinedName const& names);

        GameTele const* GetGameTele(uint32 id) const
        {
            GameTeleContainer::const_iterator itr = _gameTeleStore.find(id);
            if (itr == _gameTeleStore.end()) return NULL;
            return &itr->second;
        }
        GameTele const* GetGameTele(std::string const& name) const;
        GameTele const* GetGameTeleExactName(std::string const& name) const;
        GameTeleContainer const& GetGameTeleMap() const { return _gameTeleStore; }
        bool AddGameTele(GameTele& data);
        bool DeleteGameTele(std::string const& name);

        TrainerSpellData const* GetNpcTrainerSpells(uint32 entry) const
        {
            CacheTrainerSpellContainer::const_iterator  iter = _cacheTrainerSpellStore.find(entry);
            if (iter == _cacheTrainerSpellStore.end())
                return NULL;

            return &iter->second;
        }

        VendorItemData const* GetNpcVendorItemList(uint32 entry) const
        {
            CacheVendorItemContainer::const_iterator iter = _cacheVendorItemStore.find(entry);
            if (iter == _cacheVendorItemStore.end())
                return NULL;

            return &iter->second;
        }
        void AddVendorItem(uint32 entry, uint32 item, int32 maxcount, uint32 incrtime, uint32 extendedCost, bool persist = true); // for event
        bool RemoveVendorItem(uint32 entry, uint32 item, bool persist = true); // for event
        bool IsVendorItemValid(uint32 vendor_entry, uint32 item, int32 maxcount, uint32 ptime, uint32 ExtendedCost, Player* player = NULL, std::set<uint32>* skip_vendors = NULL, uint32 ORnpcflag = 0) const;

        void LoadScriptNames();
        ScriptNameContainer &GetScriptNames() { return _scriptNamesStore; }
        const char * GetScriptName(uint32 id) const { return id < _scriptNamesStore.size() ? _scriptNamesStore[id].c_str() : ""; }
        uint32 GetScriptId(const char *name);

        SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 creature_id) const
        {
            return _spellClickInfoStore.equal_range(creature_id);
        }

        GossipMenusMapBounds GetGossipMenusMapBounds(uint32 uiMenuId) const
        {
            return _gossipMenusStore.equal_range(uiMenuId);
        }

        GossipMenusMapBoundsNonConst GetGossipMenusMapBoundsNonConst(uint32 uiMenuId)
        {
            return _gossipMenusStore.equal_range(uiMenuId);
        }

        GossipMenuItemsMapBounds GetGossipMenuItemsMapBounds(uint32 uiMenuId) const
        {
            return _gossipMenuItemsStore.equal_range(uiMenuId);
        }
        GossipMenuItemsMapBoundsNonConst GetGossipMenuItemsMapBoundsNonConst(uint32 uiMenuId)
        {
            return _gossipMenuItemsStore.equal_range(uiMenuId);
        }

        // for wintergrasp only
        GraveYardContainer GraveYardStore;

        template<class... Data>
        static void FillLocaleContainer(Field*& fields, Data&... data);

        static inline void GetLocaleString(span<std::string> data, int loc_idx, std::string& value)
        {
            if (data.size() > size_t(loc_idx) && !data[loc_idx].empty())
                value = data[loc_idx];
        }

        CharacterConversionMap FactionChangeAchievements;
        CharacterConversionMap FactionChangeItems;
        CharacterConversionMap FactionChangeQuests;
        CharacterConversionMap FactionChangeReputation;
        CharacterConversionMap FactionChangeFullReputationMap;
        CharacterConversionMap FactionChangeSpells;
        CharacterConversionMap FactionChangeCfbgSpells;
        CharacterConversionMap FactionChangeTitles;
        CharacterConversionMap FactionChangeFullTitleMap;

        void LoadFactionChangeAchievements();
        void LoadFactionChangeItems();
        void LoadFactionChangeQuests();
        void LoadFactionChangeReputations();
        void LoadFactionChangeSpells();
        void LoadFactionChangeTitles();

        bool IsTransportMap(uint32 mapId) const { return _transportMaps.count(mapId) != 0; }
        typedef std::unordered_map<uint32, GossipText> GossipTextContainer;
        GossipTextContainer _gossipTextStore;

        ItemIdGenerator& GetItemIdGenerator() { return _itemGuidGenerator; }

        void LoadPolygonAssignments();
        std::vector<uint32> const* GetPolygonAssignments(PolyIdentifier id) const
        {
            PolygonMap::const_iterator itr = polygonAssignments.find(id);
            if (itr != polygonAssignments.cend())
                return &itr->second;
            else
                return NULL;
        }

    private:
        static void PrepareLocaleContainer(StringVector& data);
        static void AddLocaleString(StringVector& data, uint8 index, Field*& field);

        // first free id for selected id type
        uint32 _auctionId;
        uint64 _equipmentSetGuid;
        uint32 _itemTextId;
        uint32 _mailId;
        uint32 _hiPetNumber;

        // first free low guid for selected guid type
        uint32 _hiCharGuid;
        uint32 _hiCreatureGuid;
        uint32 _hiPetGuid;
        uint32 _hiVehicleGuid;
        uint32 _hiGoGuid;
        uint32 _hiDoGuid;
        uint32 _hiCorpseGuid;
        uint32 _hiMoTransGuid;
        ItemIdGenerator _itemGuidGenerator;

        QuestMap _questTemplates;

        typedef std::unordered_map<uint32, uint32> QuestAreaTriggerContainer;
        typedef std::map<uint32, uint32> TavernAreaTriggerContainer; // triggerId + faction
        typedef std::set<uint32> GameObjectForQuestContainer;

        using EncounterCreatureStore = std::unordered_map<std::pair<uint32, Difficulty>, uint32>;
        using AreaScriptStore = std::unordered_map<uint32, std::string>;

        QuestAreaTriggerContainer _questAreaTriggerStore;
        TavernAreaTriggerContainer _tavernAreaTriggerStore;
        GameObjectForQuestContainer _gameObjectForQuestStore;
        AreaTriggerContainer _areaTriggerStore;
        AreaTriggerScriptContainer _areaTriggerScriptStore;
        AccessRequirementContainer _accessRequirementStore;
        DungeonEncounterContainer _dungeonEncounterStore;
        EncounterCreatureStore _encounterCreatureStore;
        AreaScriptStore _areaScriptStore;

        RepRewardRateContainer _repRewardRateStore;
        RepOnKillContainer _repOnKillStore;
        RepSpilloverTemplateContainer _repSpilloverTemplateStore;

        GossipMenusContainer _gossipMenusStore;
        GossipMenuItemsContainer _gossipMenuItemsStore;
        PointOfInterestContainer _pointsOfInterestStore;

        QuestPOIContainer _questPOIStore;
        QuestStateContainer _questStateStore;

        QuestRelations _goQuestRelations;
        QuestRelations _goQuestInvolvedRelations;
        QuestRelationsReverse _goQuestInvolvedRelationsReverse;
        QuestRelations _creatureQuestRelations;
        QuestRelations _creatureQuestInvolvedRelations;
        QuestRelationsReverse _creatureQuestInvolvedRelationsReverse;

        //character reserved names
        typedef std::set<std::wstring> ReservedNamesContainer;
        ReservedNamesContainer _reservedNamesStore;

        GameTeleContainer _gameTeleStore;

        ScriptNameContainer _scriptNamesStore;
        ScriptIndexContainer _scriptIndexStore;

        SpellClickInfoContainer _spellClickInfoStore;

        SpellScriptsContainer _spellScriptsStore;

        VehicleAccessoryContainer _vehicleTemplateAccessoryStore;
        VehicleAccessoryContainer _vehicleAccessoryStore;

        LocaleConstant DBCLocaleIndex;

        PageTextContainer _pageTextStore;
        InstanceTemplateContainer _instanceTemplateStore;

    private:
        void LoadScripts(ScriptsType type);
        void CheckScripts(ScriptsType type, std::unordered_set<int32>& ids);
        void LoadQuestRelationsHelper(QuestRelations& map, QuestRelationsReverse* reverseMap, std::string const& table, bool starter, bool go);
        void PlayerCreateInfoAddItemHelper(uint32 race_, uint32 class_, uint32 itemId, int32 count);

        MailLevelRewardContainer _mailLevelRewardStore;
        MailLevelRewardExtendedContainer _mailLevelRewardExtendedStore;

        CreatureBaseStatsContainer _creatureBaseStatsStore;

        typedef std::map<uint32, PetLevelInfo*> PetLevelInfoContainer;
        // PetLevelInfoContainer[creature_id][level]
        PetLevelInfoContainer _petInfoStore;                            // [creature_id][level]

        PlayerClassInfo* _playerClassInfo[MAX_CLASSES];

        void BuildPlayerLevelInfo(uint8 race, uint8 class_, uint8 level, PlayerLevelInfo* plinfo) const;

        PlayerInfo* _playerInfo[MAX_RACES][MAX_CLASSES];

        typedef std::vector<uint32> PlayerXPperLevel;       // [level]
        PlayerXPperLevel _playerXPperLevel;

        typedef std::map<uint32, uint32> BaseXPContainer;          // [area level][base xp]
        BaseXPContainer _baseXPTable;

        typedef std::map<uint32, int32> FishingBaseSkillContainer; // [areaId][base skill level]
        FishingBaseSkillContainer _fishingBaseForAreaStore;

        typedef std::map<uint32, StringVector> HalfNameContainer;
        HalfNameContainer _petHalfName0;
        HalfNameContainer _petHalfName1;

        typedef std::unordered_map<uint32, ItemSetNameEntry> ItemSetNameContainer;
        ItemSetNameContainer _itemSetNameStore;

        MapObjectGuids _mapObjectGuidsStore;
        CreatureDataContainer _creatureDataStore;
        CreatureTemplateContainer _creatureTemplateStore;
        CreatureModelContainer _creatureModelStore;
        CreatureAddonContainer _creatureAddonStore;
        CreatureAddonContainer _creatureTemplateAddonStore;
        GameObjectAddonContainer _gameObjectAddonStore;
        EquipmentInfoContainer _equipmentInfoStore;
        LinkedRespawnContainer _linkedRespawnStore;
        CreatureLocaleContainer _creatureLocaleStore;
        CreatureVendorItemCountContainer _creatureVendorItemCountContainer;
        GameObjectDataContainer _gameObjectDataStore;
        GameObjectLocaleContainer _gameObjectLocaleStore;
        GameObjectTemplateContainer _gameObjectTemplateStore;
        /// Stores temp summon data grouped by summoner's entry, summoner's type and group id
        TempSummonDataContainer _tempSummonDataStore;

        BroadcastTextContainer _broadcastTextStore;
        ItemTemplateContainer _itemTemplateStore;
        ItemLocaleContainer _itemLocaleStore;
        ItemSetNameLocaleContainer _itemSetNameLocaleStore;
        QuestLocaleContainer _questLocaleStore;
        NpcTextLocaleContainer _npcTextLocaleStore;
        PageTextLocaleContainer _pageTextLocaleStore;
        TrinityStringLocaleContainer _trinityStringLocaleStore;
        GossipMenuItemsLocaleContainer _gossipMenuItemsLocaleStore;
        PointOfInterestLocaleContainer _pointOfInterestLocaleStore;

        CacheVendorItemContainer _cacheVendorItemStore;
        CacheTrainerSpellContainer _cacheTrainerSpellStore;

        std::set<uint32> _difficultyEntries[MAX_DIFFICULTY - 1]; // already loaded difficulty 1 value in creatures, used in CheckCreatureTemplate
        std::set<uint32> _hasDifficultyEntries[MAX_DIFFICULTY - 1]; // already loaded creatures with difficulty 1 values, used in CheckCreatureTemplate

        enum CreatureLinkedRespawnType
        {
            CREATURE_TO_CREATURE,
            CREATURE_TO_GO,         // Creature is dependant on GO
            GO_TO_GO,
            GO_TO_CREATURE          // GO is dependant on creature
        };

        std::set<uint32> _transportMaps; // Helper container storing map ids that are for transports only, loaded from gameobject_template

        PolygonMap polygonAssignments;
};

template<class... Data>
void ObjectMgr::FillLocaleContainer(Field*& fields, Data&... data)
{
    (PrepareLocaleContainer(data), ...);

    for (uint8 i = 1; i < TOTAL_LOCALES; ++i)
    {
        (AddLocaleString(data, i, fields), ...);
    }
}


#define sObjectMgr ObjectMgr::instance()

#endif