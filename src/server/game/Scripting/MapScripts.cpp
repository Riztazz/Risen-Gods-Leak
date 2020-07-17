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

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "GossipDef.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "Transport.h"
#include "WaypointManager.h"
#include "World.h"
#include "RG/Logging/LogManager.hpp"
#include "GameTime.h"

/// Put scripts in the execution queue
void Map::ScriptsStart(ScriptMapMap const& scripts, uint32 id, Object* source, Object* target)
{
    ///- Find the script map
    ScriptMapMap::const_iterator s = scripts.find(id);
    if (s == scripts.end())
        return;

    // prepare static data
    ObjectGuid sourceGUID = source ? source->GetGUID() : ObjectGuid::Empty; //some script commands doesn't have source
    ObjectGuid targetGUID = target ? target->GetGUID() : ObjectGuid::Empty;
    ObjectGuid ownerGUID = (source && source->GetTypeId() == TYPEID_ITEM) ? ((Item*)source)->GetOwnerGUID() : ObjectGuid::Empty;

    ///- Schedule script execution for all scripts in the script map
    ScriptMap const* s2 = &(s->second);
    bool immedScript = false;
    for (ScriptMap::const_iterator iter = s2->begin(); iter != s2->end(); ++iter)
    {
        ScriptAction sa;
        sa.sourceGUID = sourceGUID;
        sa.targetGUID = targetGUID;
        sa.ownerGUID  = ownerGUID;

        sa.script = &iter->second;
        m_scriptSchedule.insert(ScriptScheduleMap::value_type(time_t(game::time::GetGameTime() + iter->first), sa));
        if (iter->first == 0)
            immedScript = true;

        sScriptMgr->IncreaseScheduledScriptsCount();
    }
    ///- If one of the effects should be immediate, launch the script execution
    if (/*start &&*/ immedScript && !i_scriptLock)
    {
        i_scriptLock = true;
        ScriptsProcess();
        i_scriptLock = false;
    }
}

void Map::ScriptCommandStart(ScriptInfo const& script, uint32 delay, Object* source, Object* target)
{
    // NOTE: script record _must_ exist until command executed

    // prepare static data
    ObjectGuid sourceGUID = source ? source->GetGUID() : ObjectGuid::Empty;
    ObjectGuid targetGUID = target ? target->GetGUID() : ObjectGuid::Empty;
    ObjectGuid ownerGUID = (source && source->GetTypeId() == TYPEID_ITEM) ? ((Item*)source)->GetOwnerGUID() : ObjectGuid::Empty;

    ScriptAction sa;
    sa.sourceGUID = sourceGUID;
    sa.targetGUID = targetGUID;
    sa.ownerGUID  = ownerGUID;

    sa.script = &script;
    m_scriptSchedule.insert(ScriptScheduleMap::value_type(time_t(game::time::GetGameTime() + delay), sa));

    sScriptMgr->IncreaseScheduledScriptsCount();

    ///- If effects should be immediate, launch the script execution
    if (delay == 0 && !i_scriptLock)
    {
        i_scriptLock = true;
        ScriptsProcess();
        i_scriptLock = false;
    }
}

// Helpers for ScriptProcess method.
inline Player* Map::_GetScriptPlayerSourceOrTarget(Object* source, Object* target, const ScriptInfo* scriptInfo) const
{
    Player* player = NULL;
    if (!source && !target)
        TC_LOG_ERROR("scripts", "%s source and target objects are NULL.", scriptInfo->GetDebugInfo().c_str());
    else
    {
        // Check target first, then source.
        if (target)
            player = target->ToPlayer();
        if (!player && source)
            player = source->ToPlayer();

        if (!player)
            TC_LOG_ERROR("scripts", "%s neither source nor target object is player (source: TypeId: %hhu, Entry: %u, GUID: %u; target: TypeId: %hhu, Entry: %u, GUID: %u), skipping.",
                scriptInfo->GetDebugInfo().c_str(),
                source ? source->GetTypeId() : TypeID::INVALID, source ? source->GetEntry() : 0, source ? source->GetGUID().GetCounter() : 0,
                target ? target->GetTypeId() : TypeID::INVALID, target ? target->GetEntry() : 0, target ? target->GetGUID().GetCounter() : 0);
    }
    return player;
}

inline Creature* Map::_GetScriptCreatureSourceOrTarget(Object* source, Object* target, const ScriptInfo* scriptInfo, bool bReverse) const
{
    Creature* creature = NULL;
    if (!source && !target)
        TC_LOG_ERROR("scripts", "%s source and target objects are NULL.", scriptInfo->GetDebugInfo().c_str());
    else
    {
        if (bReverse)
        {
            // Check target first, then source.
            if (target)
                creature = target->ToCreature();
            if (!creature && source)
                creature = source->ToCreature();
        }
        else
        {
            // Check source first, then target.
            if (source)
                creature = source->ToCreature();
            if (!creature && target)
                creature = target->ToCreature();
        }

        if (!creature)
            TC_LOG_ERROR("scripts", "%s neither source nor target are creatures (source: TypeId: %hhu, Entry: %u, GUID: %u; target: TypeId: %hhu, Entry: %u, GUID: %u), skipping.",
                scriptInfo->GetDebugInfo().c_str(),
                source ? source->GetTypeId() : TypeID::INVALID, source ? source->GetEntry() : 0, source ? source->GetGUID().GetCounter() : 0,
                target ? target->GetTypeId() : TypeID::INVALID, target ? target->GetEntry() : 0, target ? target->GetGUID().GetCounter() : 0);
    }
    return creature;
}

inline Unit* Map::_GetScriptUnit(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const
{
    Unit* unit = NULL;
    if (!obj)
        TC_LOG_ERROR("scripts", "%s %s object is NULL.", scriptInfo->GetDebugInfo().c_str(), isSource ? "source" : "target");
    else if (!obj->isType(TYPEMASK_UNIT))
        TC_LOG_ERROR("scripts", "%s %s object is not unit (TypeId: %hhu, Entry: %u, GUID: %u), skipping.",
            scriptInfo->GetDebugInfo().c_str(), isSource ? "source" : "target", obj->GetTypeId(), obj->GetEntry(), obj->GetGUID().GetCounter());
    else
    {
        unit = obj->ToUnit();
        if (!unit)
            TC_LOG_ERROR("scripts", "%s %s object could not be cast to unit.",
                scriptInfo->GetDebugInfo().c_str(), isSource ? "source" : "target");
    }
    return unit;
}

inline Player* Map::_GetScriptPlayer(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const
{
    Player* player = NULL;
    if (!obj)
        TC_LOG_ERROR("scripts", "%s %s object is NULL.", scriptInfo->GetDebugInfo().c_str(), isSource ? "source" : "target");
    else
    {
        player = obj->ToPlayer();
        if (!player)
            TC_LOG_ERROR("scripts", "%s %s object is not a player (TypeId: %hhu, Entry: %u, GUID: %u).",
                scriptInfo->GetDebugInfo().c_str(), isSource ? "source" : "target", obj->GetTypeId(), obj->GetEntry(), obj->GetGUID().GetCounter());
    }
    return player;
}

inline Creature* Map::_GetScriptCreature(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const
{
    Creature* creature = NULL;
    if (!obj)
        TC_LOG_ERROR("scripts", "%s %s object is NULL.", scriptInfo->GetDebugInfo().c_str(), isSource ? "source" : "target");
    else
    {
        creature = obj->ToCreature();
        if (!creature)
            TC_LOG_ERROR("scripts", "%s %s object is not a creature (TypeId: %hhu, Entry: %u, GUID: %u).", scriptInfo->GetDebugInfo().c_str(),
                isSource ? "source" : "target", obj->GetTypeId(), obj->GetEntry(), obj->GetGUID().GetCounter());
    }
    return creature;
}

inline WorldObject* Map::_GetScriptWorldObject(Object* obj, bool isSource, const ScriptInfo* scriptInfo) const
{
    WorldObject* pWorldObject = NULL;
    if (!obj)
        TC_LOG_ERROR("scripts", "%s %s object is NULL.",
            scriptInfo->GetDebugInfo().c_str(), isSource ? "source" : "target");
    else
    {
        pWorldObject = dynamic_cast<WorldObject*>(obj);
        if (!pWorldObject)
            TC_LOG_ERROR("scripts", "%s %s object is not a world object (TypeId: %hhu, Entry: %u, GUID: %u).",
                scriptInfo->GetDebugInfo().c_str(), isSource ? "source" : "target", obj->GetTypeId(), obj->GetEntry(), obj->GetGUID().GetCounter());
    }
    return pWorldObject;
}

inline void Map::_ScriptProcessDoor(Object* source, Object* target, const ScriptInfo* scriptInfo) const
{
    bool bOpen = false;
    uint32 guid = scriptInfo->ToggleDoor.GOGuid;
    int32 nTimeToToggle = std::max(15, int32(scriptInfo->ToggleDoor.ResetDelay));
    switch (scriptInfo->command)
    {
        case SCRIPT_COMMAND_OPEN_DOOR: bOpen = true; break;
        case SCRIPT_COMMAND_CLOSE_DOOR: break;
        default:
            TC_LOG_ERROR("scripts", "%s unknown command for _ScriptProcessDoor.", scriptInfo->GetDebugInfo().c_str());
            return;
    }
    if (!guid)
        TC_LOG_ERROR("scripts", "%s door guid is not specified.", scriptInfo->GetDebugInfo().c_str());
    else if (!source)
        TC_LOG_ERROR("scripts", "%s source object is NULL.", scriptInfo->GetDebugInfo().c_str());
    else if (!source->isType(TYPEMASK_UNIT))
        TC_LOG_ERROR("scripts", "%s source object is not unit (TypeId: %hhu, Entry: %u, GUID: %u), skipping.", scriptInfo->GetDebugInfo().c_str(),
            source->GetTypeId(), source->GetEntry(), source->GetGUID().GetCounter());
    else
    {
        WorldObject* wSource = dynamic_cast <WorldObject*> (source);
        if (!wSource)
            TC_LOG_ERROR("scripts", "%s source object could not be cast to world object (TypeId: %hhu, Entry: %u, GUID: %u), skipping.",
                scriptInfo->GetDebugInfo().c_str(), source->GetTypeId(), source->GetEntry(), source->GetGUID().GetCounter());
        else
        {
            GameObject* pDoor = _FindGameObject(wSource, guid);
            if (!pDoor)
                TC_LOG_ERROR("scripts", "%s gameobject was not found (guid: %u).", scriptInfo->GetDebugInfo().c_str(), guid);
            else if (pDoor->GetGoType() != GAMEOBJECT_TYPE_DOOR)
                TC_LOG_ERROR("scripts", "%s gameobject is not a door (GoType: %u, Entry: %u, GUID: %u).",
                    scriptInfo->GetDebugInfo().c_str(), pDoor->GetGoType(), pDoor->GetEntry(), pDoor->GetGUID().GetCounter());
            else if (bOpen == (pDoor->GetGoState() == GO_STATE_READY))
            {
                pDoor->UseDoorOrButton(nTimeToToggle);

                if (target && target->isType(TYPEMASK_GAMEOBJECT))
                {
                    GameObject* goTarget = target->ToGameObject();
                    if (goTarget && goTarget->GetGoType() == GAMEOBJECT_TYPE_BUTTON)
                        goTarget->UseDoorOrButton(nTimeToToggle);
                }
            }
        }
    }
}

inline GameObject* Map::_FindGameObject(WorldObject* searchObject, uint32 guid) const
{
    Trinity::GameObjectWithDbGUIDCheck goCheck(*searchObject, guid);

    return searchObject->VisitSingleNearbyObject<GameObject, true>(searchObject->GetGridActivationRange(), goCheck);
}

/// Process queued scripts
void Map::ScriptsProcess()
{
    if (m_scriptSchedule.empty())
        return;

    ///- Process overdue queued scripts
    ScriptScheduleMap::iterator iter = m_scriptSchedule.begin();
    // ok as multimap is a *sorted* associative container
    while (!m_scriptSchedule.empty() && (iter->first <= game::time::GetGameTime()))
    {
        ScriptAction const& step = iter->second;

        Object* source = nullptr;
        if (step.sourceGUID)
        {
            switch (step.sourceGUID.GetHigh())
            {
                case HighGuid::Item: // as well as HighGuid::Container
                    if (Player* player = ObjectAccessor::GetPlayer(*this, step.ownerGUID))
                        source = player->GetItemByGuid(step.sourceGUID);
                    break;
                case HighGuid::Unit:
                case HighGuid::Vehicle:
                    source = ObjectAccessor::GetCreature(*this, step.sourceGUID);
                    break;
                case HighGuid::Pet:
                    source = ObjectAccessor::GetPet(*this, step.sourceGUID);
                    break;
                case HighGuid::Player:
                    source = ObjectAccessor::GetPlayer(*this, step.sourceGUID);
                    break;
                case HighGuid::Transport:
                case HighGuid::Gameobject:
                    source = ObjectAccessor::GetGameObject(*this, step.sourceGUID);
                    break;
                case HighGuid::Corpse:
                    source = ObjectAccessor::GetCorpse(*this, step.sourceGUID);
                    break;
                case HighGuid::Mo_Transport:
                {
                    source = ObjectAccessor::GetMotionTransport(*this, step.sourceGUID);
                    break;
                }
                default:
                    TC_LOG_ERROR("scripts", "%s source with unsupported high guid %s.",
                        step.script->GetDebugInfo().c_str(), step.sourceGUID.ToString().c_str());
                    break;
            }
        }

        WorldObject* target = NULL;
        if (step.targetGUID)
        {
            switch (step.targetGUID.GetHigh())
            {
                case HighGuid::Unit:
                case HighGuid::Vehicle:
                    target = ObjectAccessor::GetCreature(*this, step.targetGUID);
                    break;
                case HighGuid::Pet:
                    target = ObjectAccessor::GetPet(*this, step.targetGUID);
                    break;
                case HighGuid::Player:                       // empty GUID case also
                    target = ObjectAccessor::GetPlayer(*this, step.targetGUID);
                    break;
                case HighGuid::Transport:
                case HighGuid::Gameobject:
                    target = ObjectAccessor::GetGameObject(*this, step.targetGUID);
                    break;
                case HighGuid::Corpse:
                    target = ObjectAccessor::GetCorpse(*this, step.targetGUID);
                    break;
                case HighGuid::Mo_Transport:
                {
                    target = ObjectAccessor::GetMotionTransport(*this, step.targetGUID);
                    break;
                }
                default:
                    TC_LOG_ERROR("scripts", "%s target with unsupported high guid %s.",
                        step.script->GetDebugInfo().c_str(), step.targetGUID.ToString().c_str());
                    break;
            }
        }

        switch (step.script->command)
        {
            case SCRIPT_COMMAND_TALK:
                if (step.script->Talk.ChatType > CHAT_TYPE_WHISPER && step.script->Talk.ChatType != CHAT_MSG_RAID_BOSS_WHISPER)
                {
                    TC_LOG_ERROR("scripts", "%s invalid chat type (%u) specified, skipping.", step.script->GetDebugInfo().c_str(), step.script->Talk.ChatType);
                    break;
                }
                if (step.script->Talk.Flags & SF_TALK_USE_PLAYER)
                {
                    if (Player* player = _GetScriptPlayerSourceOrTarget(source, target, step.script))
                    {
                        LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
                        std::string text(sObjectMgr->GetTrinityString(step.script->Talk.TextID, loc_idx));

                        switch (step.script->Talk.ChatType)
                        {
                            case CHAT_TYPE_SAY:
                                player->Say(text, LANG_UNIVERSAL);
                                break;
                            case CHAT_TYPE_YELL:
                                player->Yell(text, LANG_UNIVERSAL);
                                break;
                            case CHAT_TYPE_TEXT_EMOTE:
                            case CHAT_TYPE_BOSS_EMOTE:
                                player->TextEmote(text);
                                break;
                            case CHAT_TYPE_WHISPER:
                            case CHAT_MSG_RAID_BOSS_WHISPER:
                            {
                                if (!target || !target->ToPlayer())
                                    TC_LOG_ERROR("scripts", "%s attempt to whisper to non-player unit, skipping.", step.script->GetDebugInfo().c_str());
                                else
                                    player->Whisper(text, LANG_UNIVERSAL, target->ToPlayer());
                                break;
                            }
                            default:
                                break;                              // must be already checked at load
                        }
                    }
                }
                else
                {
                    // Source or target must be Creature.
                    if (Creature* cSource = _GetScriptCreatureSourceOrTarget(source, target, step.script))
                    {
                        ObjectGuid targetGUID = target ? target->GetGUID() : ObjectGuid::Empty;
                        switch (step.script->Talk.ChatType)
                        {
                            case CHAT_TYPE_SAY:
                                cSource->MonsterSay(step.script->Talk.TextID, LANG_UNIVERSAL, target);
                                break;
                            case CHAT_TYPE_YELL:
                                cSource->MonsterYell(step.script->Talk.TextID, LANG_UNIVERSAL, target);
                                break;
                            case CHAT_TYPE_TEXT_EMOTE:
                                cSource->MonsterTextEmote(step.script->Talk.TextID, target);
                                break;
                            case CHAT_TYPE_BOSS_EMOTE:
                                cSource->MonsterTextEmote(step.script->Talk.TextID, target, true);
                                break;
                            case CHAT_TYPE_WHISPER:
                                if (!targetGUID || !targetGUID.IsPlayer())
                                    TC_LOG_ERROR("scripts", "%s attempt to whisper to non-player unit, skipping.", step.script->GetDebugInfo().c_str());
                                else
                                    cSource->MonsterWhisper(step.script->Talk.TextID, target->ToPlayer());
                                break;
                            case CHAT_MSG_RAID_BOSS_WHISPER:
                                if (!targetGUID || !targetGUID.IsPlayer())
                                    TC_LOG_ERROR("scripts", "%s attempt to raidbosswhisper to non-player unit, skipping.", step.script->GetDebugInfo().c_str());
                                else
                                    cSource->MonsterWhisper(step.script->Talk.TextID, target->ToPlayer(), true);
                                break;
                            default:
                                break;                              // must be already checked at load
                        }
                    }
                }
                break;

            case SCRIPT_COMMAND_EMOTE:
                // Source or target must be Creature.
                if (Creature* cSource = _GetScriptCreatureSourceOrTarget(source, target, step.script))
                {
                    if (step.script->Emote.Flags & SF_EMOTE_USE_STATE)
                        cSource->SetUInt32Value(UNIT_NPC_EMOTESTATE, step.script->Emote.EmoteID);
                    else
                        cSource->HandleEmoteCommand(step.script->Emote.EmoteID);
                }
                break;

            case SCRIPT_COMMAND_FIELD_SET:
                // Source or target must be Creature.
                if (Creature* cSource = _GetScriptCreatureSourceOrTarget(source, target, step.script))
                {
                    // Validate field number.
                    if (step.script->FieldSet.FieldID <= OBJECT_FIELD_ENTRY || step.script->FieldSet.FieldID >= cSource->GetValuesCount())
                        TC_LOG_ERROR("scripts", "%s wrong field %u (max count: %u) in object (TypeId: %hhu, %s) specified, skipping.",
                            step.script->GetDebugInfo().c_str(), step.script->FieldSet.FieldID,
                            cSource->GetValuesCount(), cSource->GetTypeId(), cSource->GetGUID().ToString().c_str());
                    else
                        cSource->SetUInt32Value(step.script->FieldSet.FieldID, step.script->FieldSet.FieldValue);
                }
                break;

            case SCRIPT_COMMAND_MOVE_TO:
                // Source or target must be Creature.
                if (Creature* cSource = _GetScriptCreatureSourceOrTarget(source, target, step.script))
                {
                    Unit* unit = (Unit*)cSource;
                    if (step.script->MoveTo.TravelTime != 0)
                    {
                        unit->GetMotionMaster()->MovePoint(10000, step.script->MoveTo.DestX, step.script->MoveTo.DestY, step.script->MoveTo.DestZ);
                    }
                    else
                        unit->NearTeleportTo(step.script->MoveTo.DestX, step.script->MoveTo.DestY, step.script->MoveTo.DestZ, unit->GetOrientation());
                }
                break;

            case SCRIPT_COMMAND_FLAG_SET:
                // Source or target must be Creature.
                if (Creature* cSource = _GetScriptCreatureSourceOrTarget(source, target, step.script))
                {
                    // Validate field number.
                    if (step.script->FlagToggle.FieldID <= OBJECT_FIELD_ENTRY || step.script->FlagToggle.FieldID >= cSource->GetValuesCount())
                        TC_LOG_ERROR("scripts", "%s wrong field %u (max count: %u) in object (TypeId: %hhu, Entry: %u, GUID: %u) specified, skipping.",
                            step.script->GetDebugInfo().c_str(), step.script->FlagToggle.FieldID,
                            cSource->GetValuesCount(), cSource->GetTypeId(), cSource->GetEntry(), cSource->GetGUID().GetCounter());
                    else
                        cSource->SetFlag(step.script->FlagToggle.FieldID, step.script->FlagToggle.FieldValue);
                }
                break;

            case SCRIPT_COMMAND_FLAG_REMOVE:
                // Source or target must be Creature.
                if (Creature* cSource = _GetScriptCreatureSourceOrTarget(source, target, step.script))
                {
                    // Validate field number.
                    if (step.script->FlagToggle.FieldID <= OBJECT_FIELD_ENTRY || step.script->FlagToggle.FieldID >= cSource->GetValuesCount())
                        TC_LOG_ERROR("scripts", "%s wrong field %u (max count: %u) in object (TypeId: %hhu, Entry: %u, GUID: %u) specified, skipping.",
                            step.script->GetDebugInfo().c_str(), step.script->FlagToggle.FieldID,
                            cSource->GetValuesCount(), cSource->GetTypeId(), cSource->GetEntry(), cSource->GetGUID().GetCounter());
                    else
                        cSource->RemoveFlag(step.script->FlagToggle.FieldID, step.script->FlagToggle.FieldValue);
                }
                break;

            case SCRIPT_COMMAND_TELEPORT_TO:
                if (step.script->TeleportTo.Flags & SF_TELEPORT_USE_CREATURE)
                {
                    // Source or target must be Creature.
                    if (Creature* cSource = _GetScriptCreatureSourceOrTarget(source, target, step.script, true))
                        cSource->NearTeleportTo(step.script->TeleportTo.DestX, step.script->TeleportTo.DestY, step.script->TeleportTo.DestZ, step.script->TeleportTo.Orientation);
                }
                else
                {
                    // Source or target must be Player.
                    if (Player* player = _GetScriptPlayerSourceOrTarget(source, target, step.script))
                        player->TeleportTo(step.script->TeleportTo.MapID, step.script->TeleportTo.DestX, step.script->TeleportTo.DestY, step.script->TeleportTo.DestZ, step.script->TeleportTo.Orientation);
                }
                break;

            case SCRIPT_COMMAND_QUEST_EXPLORED:
            {
                if (!source)
                {
                    TC_LOG_ERROR("scripts", "%s source object is NULL.", step.script->GetDebugInfo().c_str());
                    break;
                }
                if (!target)
                {
                    TC_LOG_ERROR("scripts", "%s target object is NULL.", step.script->GetDebugInfo().c_str());
                    break;
                }

                // when script called for item spell casting then target == (unit or GO) and source is player
                WorldObject* worldObject;
                Player* player = target->ToPlayer();
                if (player)
                {
                    if (source->GetTypeId() != TYPEID_UNIT && source->GetTypeId() != TYPEID_GAMEOBJECT && source->GetTypeId() != TYPEID_PLAYER)
                    {
                        TC_LOG_ERROR("scripts", "%s source is not unit, gameobject or player (TypeId: %hhu, Entry: %u, GUID: %u), skipping.",
                            step.script->GetDebugInfo().c_str(), source->GetTypeId(), source->GetEntry(), source->GetGUID().GetCounter());
                        break;
                    }
                    worldObject = dynamic_cast<WorldObject*>(source);
                }
                else
                {
                    player = source->ToPlayer();
                    if (player)
                    {
                        if (target->GetTypeId() != TYPEID_UNIT && target->GetTypeId() != TYPEID_GAMEOBJECT && target->GetTypeId() != TYPEID_PLAYER)
                        {
                            TC_LOG_ERROR("scripts", "%s target is not unit, gameobject or player (TypeId: %hhu, Entry: %u, GUID: %u), skipping.",
                                step.script->GetDebugInfo().c_str(), target->GetTypeId(), target->GetEntry(), target->GetGUID().GetCounter());
                            break;
                        }
                        worldObject = dynamic_cast<WorldObject*>(target);
                    }
                    else
                    {
                        TC_LOG_ERROR("scripts", "%s neither source nor target is player (source: TypeId: %hhu, Entry: %u, GUID: %u; target: TypeId: %hhu, Entry: %u, GUID: %u), skipping.",
                            step.script->GetDebugInfo().c_str(), source->GetTypeId(), source->GetEntry(), source->GetGUID().GetCounter(),
                            target->GetTypeId(), target->GetEntry(), target->GetGUID().GetCounter());
                        break;
                    }
                }

                // quest id and flags checked at script loading
                if ((worldObject->GetTypeId() != TYPEID_UNIT || ((Unit*)worldObject)->IsAlive()) &&
                    (step.script->QuestExplored.Distance == 0 || worldObject->IsWithinDistInMap(player, float(step.script->QuestExplored.Distance))))
                    player->AreaExploredOrEventHappens(step.script->QuestExplored.QuestID);
                else
                    player->FailQuest(step.script->QuestExplored.QuestID);

                break;
            }

            case SCRIPT_COMMAND_KILL_CREDIT:
                // Source or target must be Player.
                if (Player* player = _GetScriptPlayerSourceOrTarget(source, target, step.script))
                {
                    if (step.script->KillCredit.Flags & SF_KILLCREDIT_REWARD_GROUP)
                        player->RewardPlayerAndGroupAtEvent(step.script->KillCredit.CreatureEntry, player);
                    else
                        player->KilledMonsterCredit(step.script->KillCredit.CreatureEntry);
                }
                break;

            case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:
                if (!step.script->RespawnGameobject.GOGuid)
                {
                    TC_LOG_ERROR("scripts", "%s gameobject guid (datalong) is not specified.", step.script->GetDebugInfo().c_str());
                    break;
                }

                // Source or target must be WorldObject.
                if (WorldObject* pSummoner = _GetScriptWorldObject(source, true, step.script))
                {
                    GameObject* pGO = _FindGameObject(pSummoner, step.script->RespawnGameobject.GOGuid);
                    if (!pGO)
                    {
                        TC_LOG_ERROR("scripts", "%s gameobject was not found (guid: %u).", step.script->GetDebugInfo().c_str(), step.script->RespawnGameobject.GOGuid);
                        break;
                    }

                    if (pGO->GetGoType() == GAMEOBJECT_TYPE_FISHINGNODE ||
                        pGO->GetGoType() == GAMEOBJECT_TYPE_DOOR        ||
                        pGO->GetGoType() == GAMEOBJECT_TYPE_BUTTON      ||
                        pGO->GetGoType() == GAMEOBJECT_TYPE_TRAP)
                    {
                        TC_LOG_ERROR("scripts", "%s can not be used with gameobject of type %u (guid: %u).",
                            step.script->GetDebugInfo().c_str(), uint32(pGO->GetGoType()), step.script->RespawnGameobject.GOGuid);
                        break;
                    }

                    // Check that GO is not spawned
                    if (!pGO->isSpawned())
                    {
                        int32 nTimeToDespawn = std::max(5, int32(step.script->RespawnGameobject.DespawnDelay));
                        pGO->SetLootState(GO_READY);
                        pGO->SetRespawnTime(nTimeToDespawn);

                        pGO->GetMap()->AddToMap(pGO);
                    }
                }
                break;

            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
            {
                // Source must be WorldObject.
                if (WorldObject* pSummoner = _GetScriptWorldObject(source, true, step.script))
                {
                    if (!step.script->TempSummonCreature.CreatureEntry)
                        TC_LOG_ERROR("scripts", "%s creature entry (datalong) is not specified.", step.script->GetDebugInfo().c_str());
                    else
                    {
                        float x = step.script->TempSummonCreature.PosX;
                        float y = step.script->TempSummonCreature.PosY;
                        float z = step.script->TempSummonCreature.PosZ;
                        float o = step.script->TempSummonCreature.Orientation;

                        if (!pSummoner->SummonCreature(step.script->TempSummonCreature.CreatureEntry, x, y, z, o, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, step.script->TempSummonCreature.DespawnDelay))
                            TC_LOG_ERROR("scripts", "%s creature was not spawned (entry: %u).", step.script->GetDebugInfo().c_str(), step.script->TempSummonCreature.CreatureEntry);
                    }
                }
                break;
            }

            case SCRIPT_COMMAND_OPEN_DOOR:
            case SCRIPT_COMMAND_CLOSE_DOOR:
                _ScriptProcessDoor(source, target, step.script);
                break;

            case SCRIPT_COMMAND_ACTIVATE_OBJECT:
                // Source must be Unit.
                if (Unit* unit = _GetScriptUnit(source, true, step.script))
                {
                    // Target must be GameObject.
                    if (!target)
                    {
                        TC_LOG_ERROR("scripts", "%s target object is NULL.", step.script->GetDebugInfo().c_str());
                        break;
                    }

                    if (target->GetTypeId() != TYPEID_GAMEOBJECT)
                    {
                        TC_LOG_ERROR("scripts", "%s target object is not gameobject (TypeId: %hhu, Entry: %u, GUID: %u), skipping.",
                            step.script->GetDebugInfo().c_str(), target->GetTypeId(), target->GetEntry(), target->GetGUID().GetCounter());
                        break;
                    }

                    if (GameObject* pGO = target->ToGameObject())
                        pGO->Use(unit);
                }
                break;

            case SCRIPT_COMMAND_REMOVE_AURA:
            {
                // Source (datalong2 != 0) or target (datalong2 == 0) must be Unit.
                bool bReverse = step.script->RemoveAura.Flags & SF_REMOVEAURA_REVERSE;
                if (Unit* unit = _GetScriptUnit(bReverse ? source : target, bReverse, step.script))
                    unit->RemoveAurasDueToSpell(step.script->RemoveAura.SpellID);
                break;
            }

            case SCRIPT_COMMAND_CAST_SPELL:
            {
                /// @todo Allow gameobjects to be targets and casters
                if (!source && !target)
                {
                    TC_LOG_ERROR("scripts", "%s source and target objects are NULL.", step.script->GetDebugInfo().c_str());
                    break;
                }

                Unit* uSource = NULL;
                Unit* uTarget = NULL;
                // source/target cast spell at target/source (script->datalong2: 0: s->t 1: s->s 2: t->t 3: t->s
                switch (step.script->CastSpell.Flags)
                {
                    case SF_CASTSPELL_SOURCE_TO_TARGET: // source -> target
                        uSource = source ? source->ToUnit() : NULL;
                        uTarget = target ? target->ToUnit() : NULL;
                        break;
                    case SF_CASTSPELL_SOURCE_TO_SOURCE: // source -> source
                        uSource = source ? source->ToUnit() : NULL;
                        uTarget = uSource;
                        break;
                    case SF_CASTSPELL_TARGET_TO_TARGET: // target -> target
                        uSource = target ? target->ToUnit() : NULL;
                        uTarget = uSource;
                        break;
                    case SF_CASTSPELL_TARGET_TO_SOURCE: // target -> source
                        uSource = target ? target->ToUnit() : NULL;
                        uTarget = source ? source->ToUnit() : NULL;
                        break;
                    case SF_CASTSPELL_SEARCH_CREATURE: // source -> creature with entry
                        uSource = source ? source->ToUnit() : NULL;
                        uTarget = uSource ? uSource->FindNearestCreature(abs(step.script->CastSpell.CreatureEntry), step.script->CastSpell.SearchRadius) : NULL;
                        break;
                }

                if (!uSource || !uSource->isType(TYPEMASK_UNIT))
                {
                    TC_LOG_ERROR("scripts", "%s no source unit found for spell %u", step.script->GetDebugInfo().c_str(), step.script->CastSpell.SpellID);
                    break;
                }

                if (!uTarget || !uTarget->isType(TYPEMASK_UNIT))
                {
                    TC_LOG_ERROR("scripts", "%s no target unit found for spell %u", step.script->GetDebugInfo().c_str(), step.script->CastSpell.SpellID);
                    break;
                }

                bool triggered = (step.script->CastSpell.Flags != 4) ?
                    step.script->CastSpell.CreatureEntry & SF_CASTSPELL_TRIGGERED :
                    step.script->CastSpell.CreatureEntry < 0;
                uSource->CastSpell(uTarget, step.script->CastSpell.SpellID, triggered);
                break;
            }

            case SCRIPT_COMMAND_PLAY_SOUND:
                // Source must be WorldObject.
                if (WorldObject* object = _GetScriptWorldObject(source, true, step.script))
                {
                    // PlaySound.Flags bitmask: 0/1=anyone/target
                    Player* player = NULL;
                    if (step.script->PlaySound.Flags & SF_PLAYSOUND_TARGET_PLAYER)
                    {
                        // Target must be Player.
                        player = _GetScriptPlayer(target, false, step.script);
                        if (!target)
                            break;
                    }

                    // PlaySound.Flags bitmask: 0/2=without/with distance dependent
                    if (step.script->PlaySound.Flags & SF_PLAYSOUND_DISTANCE_SOUND)
                        object->PlayDistanceSound(step.script->PlaySound.SoundID, player);
                    else
                        object->PlayDirectSound(step.script->PlaySound.SoundID, player);
                }
                break;

            case SCRIPT_COMMAND_CREATE_ITEM:
                // Target or source must be Player.
                if (Player* pReceiver = _GetScriptPlayerSourceOrTarget(source, target, step.script))
                {
                    ItemPosCountVec dest;
                    InventoryResult msg = pReceiver->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, step.script->CreateItem.ItemEntry, step.script->CreateItem.Amount);
                    if (msg == EQUIP_ERR_OK)
                    {
                        if (Item* item = pReceiver->StoreNewItem(dest, step.script->CreateItem.ItemEntry, true))
                        {
                            // RG-Custom ItemLogging
                            RG_LOG<ItemLogModule>(item, RG::Logging::ItemLogType::ADDED, step.script->CreateItem.Amount);
                            pReceiver->SendNewItem(item, step.script->CreateItem.Amount, false, true);
                        }
                    }
                    else
                        pReceiver->SendEquipError(msg, NULL, NULL, step.script->CreateItem.ItemEntry);
                }
                break;

            case SCRIPT_COMMAND_DESPAWN_SELF:
                // Target or source must be Creature.
                if (Creature* cSource = _GetScriptCreatureSourceOrTarget(source, target, step.script, true))
                    cSource->DespawnOrUnsummon(step.script->DespawnSelf.DespawnDelay);
                break;

            case SCRIPT_COMMAND_LOAD_PATH:
                // Source must be Unit.
                if (Unit* unit = _GetScriptUnit(source, true, step.script))
                {
                    if (!sWaypointMgr->GetPath(step.script->LoadPath.PathID))
                        TC_LOG_ERROR("scripts", "%s source object has an invalid path (%u), skipping.", step.script->GetDebugInfo().c_str(), step.script->LoadPath.PathID);
                    else
                        unit->GetMotionMaster()->MovePath(step.script->LoadPath.PathID, step.script->LoadPath.IsRepeatable != 0);
                }
                break;

            case SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT:
            {
                if (!step.script->CallScript.CreatureEntry)
                {
                    TC_LOG_ERROR("scripts", "%s creature entry is not specified, skipping.", step.script->GetDebugInfo().c_str());
                    break;
                }
                if (!step.script->CallScript.ScriptID)
                {
                    TC_LOG_ERROR("scripts", "%s script id is not specified, skipping.", step.script->GetDebugInfo().c_str());
                    break;
                }

                Creature* cTarget = NULL;
                WorldObject* wSource = dynamic_cast <WorldObject*> (source);
                if (wSource) //using grid searcher
                {
                    Trinity::CreatureWithDbGUIDCheck target_check(wSource, step.script->CallScript.CreatureEntry);

                    wSource->VisitSingleNearbyObject<Creature, true>(wSource->GetGridActivationRange(), target_check);
                }
                else //check hashmap holders
                {
                    if (CreatureData const* data = sObjectMgr->GetCreatureData(step.script->CallScript.CreatureEntry))
                        cTarget = ObjectAccessor::GetCreature(data->mapid, ObjectGuid(HighGuid::Unit, data->id, step.script->CallScript.CreatureEntry));
                }

                if (!cTarget)
                {
                    TC_LOG_ERROR("scripts", "%s target was not found (entry: %u)", step.script->GetDebugInfo().c_str(), step.script->CallScript.CreatureEntry);
                    break;
                }

                //Lets choose our ScriptMap map
                ScriptMapMap* datamap = GetScriptsMapByType(ScriptsType(step.script->CallScript.ScriptType));
                //if no scriptmap present...
                if (!datamap)
                {
                    TC_LOG_ERROR("scripts", "%s unknown scriptmap (%u) specified, skipping.", step.script->GetDebugInfo().c_str(), step.script->CallScript.ScriptType);
                    break;
                }

                // Insert script into schedule but do not start it
                ScriptsStart(*datamap, step.script->CallScript.ScriptID, cTarget, NULL);
                break;
            }

            case SCRIPT_COMMAND_KILL:
                // Source or target must be Creature.
                if (Creature* cSource = _GetScriptCreatureSourceOrTarget(source, target, step.script))
                {
                    if (cSource->isDead())
                        TC_LOG_ERROR("scripts", "%s creature is already dead (Entry: %u, GUID: %u)",
                            step.script->GetDebugInfo().c_str(), cSource->GetEntry(), cSource->GetGUID().GetCounter());
                    else
                    {
                        cSource->setDeathState(JUST_DIED);
                        if (step.script->Kill.RemoveCorpse == 1)
                            cSource->RemoveCorpse();
                    }
                }
                break;

            case SCRIPT_COMMAND_ORIENTATION:
                // Source must be Unit.
                if (Unit* sourceUnit = _GetScriptUnit(source, true, step.script))
                {
                    if (step.script->Orientation.Flags & SF_ORIENTATION_FACE_TARGET)
                    {
                        // Target must be Unit.
                        Unit* targetUnit = _GetScriptUnit(target, false, step.script);
                        if (!targetUnit)
                            break;

                        sourceUnit->SetFacingToObject(targetUnit);
                    }
                    else
                        sourceUnit->SetFacingTo(step.script->Orientation.Orientation);
                }
                break;

            case SCRIPT_COMMAND_EQUIP:
                // Source must be Creature.
                if (Creature* cSource = _GetScriptCreature(source, true, step.script))
                    cSource->LoadEquipment(step.script->Equip.EquipmentID);
                break;

            case SCRIPT_COMMAND_MODEL:
                // Source must be Creature.
                if (Creature* cSource = _GetScriptCreature(source, true, step.script))
                    cSource->SetDisplayId(step.script->Model.ModelID);
                break;

            case SCRIPT_COMMAND_CLOSE_GOSSIP:
                // Source must be Player.
                if (Player* player = _GetScriptPlayer(source, true, step.script))
                    player->PlayerTalkClass->SendCloseGossip();
                break;

            case SCRIPT_COMMAND_PLAYMOVIE:
                // Source must be Player.
                if (Player* player = _GetScriptPlayer(source, true, step.script))
                    player->SendMovieStart(step.script->PlayMovie.MovieID);
                break;

            case SCRIPT_COMMAND_UNITSTATE:
                // Source must be Creature.
                if (Creature* cSource = _GetScriptCreature(source, true, step.script))
                    cSource->SetStandState(step.script->UnitStandState.UnitstandID);
                break;

            default:
                TC_LOG_ERROR("scripts", "Unknown script command %s.", step.script->GetDebugInfo().c_str());
                break;
        }

        m_scriptSchedule.erase(iter);
        iter = m_scriptSchedule.begin();
        sScriptMgr->DecreaseScheduledScriptCount();
    }
}
