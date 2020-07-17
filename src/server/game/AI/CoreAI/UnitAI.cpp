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

#include "UnitAI.h"
#include "Player.h"
#include "Creature.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Spell.h"
#include "CreatureAIImpl.h"

void UnitAI::AttackStart(Unit* victim)
{
    if (victim && me->Attack(victim, true))
    {
        // Clear distracted state on attacking
        if (me->HasUnitState(UNIT_STATE_DISTRACTED))
        {
            me->ClearUnitState(UNIT_STATE_DISTRACTED);
            me->GetMotionMaster()->Clear();
        }
        me->GetMotionMaster()->MoveChase(victim);
    }
}

void UnitAI::AttackStartCaster(Unit* victim, float dist)
{
    if (victim && me->Attack(victim, false))
        me->GetMotionMaster()->MoveChase(victim, dist);
}

void UnitAI::DoMeleeAttackIfReady(bool ignoreLOS)
{
    if (me->HasUnitState(UNIT_STATE_CASTING))
        return;

    Unit* victim = me->GetVictim();

    if (!me->IsWithinMeleeRange(victim))
        return;

    //Make sure our attack is ready and we aren't currently casting before checking distance
    if (me->isAttackReady())
    {
        me->AttackerStateUpdate(victim, BASE_ATTACK, false, ignoreLOS);
        me->resetAttackTimer();
    }

    if (me->haveOffhandWeapon() && me->isAttackReady(OFF_ATTACK))
    {
        me->AttackerStateUpdate(victim, OFF_ATTACK, false, ignoreLOS);
        me->resetAttackTimer(OFF_ATTACK);
    }
}

bool UnitAI::DoSpellAttackIfReady(uint32 spell)
{
    if (me->HasUnitState(UNIT_STATE_CASTING) || !me->isAttackReady())
        return true;

    if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell))
    {
        if (me->IsWithinCombatRange(me->GetVictim(), spellInfo->GetMaxRange(false)))
        {
            me->CastSpell(me->GetVictim(), spell, false);
            me->resetAttackTimer();
            return true;
        }
    }

    return false;
}

Unit* UnitAI::SelectTarget(SelectAggroTarget targetType, uint32 offset, float dist, bool playerOnly, bool withTank, int32 aura)
{
    return SelectTarget(targetType, offset, DefaultTargetSelector(me, dist, playerOnly, withTank, aura));
}

void UnitAI::SelectTargetList(std::list<Unit*>& targetList, uint32 num, SelectAggroTarget targetType, uint32 offset, float dist, bool playerOnly, bool withTank, int32 aura)
{
    SelectTargetList(targetList, num, targetType, offset, DefaultTargetSelector(me, dist, playerOnly, withTank, aura));
}

float UnitAI::DoGetSpellMaxRange(uint32 spellId, bool positive)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    return spellInfo ? spellInfo->GetMaxRange(positive) : 0;
}

void UnitAI::DoCast(uint32 spellId)
{
    Unit* target = NULL;

    switch (AISpellInfo[spellId].target)
    {
        default:
        case AITARGET_SELF:
            target = me;
            break;
        case AITARGET_VICTIM:
            target = me->GetVictim();
            break;
        case AITARGET_ENEMY:
        {
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
            {
                bool playerOnly = spellInfo->HasAttribute(SPELL_ATTR3_ONLY_TARGET_PLAYERS);
                target = SelectTarget(SELECT_TARGET_RANDOM, 0, spellInfo->GetMaxRange(false), playerOnly);
            }
            break;
        }
        case AITARGET_ALLY:
            target = me;
            break;
        case AITARGET_BUFF:
            target = me;
            break;
        case AITARGET_DEBUFF:
        {
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
            {
                bool playerOnly = spellInfo->HasAttribute(SPELL_ATTR3_ONLY_TARGET_PLAYERS);
                float range = spellInfo->GetMaxRange(false);

                DefaultTargetSelector targetSelector(me, range, playerOnly, true, -(int32)spellId);
                if (!(spellInfo->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_VICTIM)
                    && targetSelector(me->GetVictim()))
                    target = me->GetVictim();
                else
                    target = SelectTarget(SELECT_TARGET_RANDOM, 0, targetSelector);
            }
            break;
        }
    }

    if (target)
        me->CastSpell(target, spellId, false);
}

void UnitAI::DoCast(Unit* victim, uint32 spellId, bool triggered)
{
    if (!victim || (me->HasUnitState(UNIT_STATE_CASTING) && !triggered))
        return;

    me->CastSpell(victim, spellId, triggered);
}

void UnitAI::DoCastVictim(uint32 spellId, bool triggered)
{
    if (!me->GetVictim() || (me->HasUnitState(UNIT_STATE_CASTING) && !triggered))
        return;

    me->CastSpell(me->GetVictim(), spellId, triggered);
}

void UnitAI::DoCastAOE(uint32 spellId, bool triggered)
{
    if (!triggered && me->HasUnitState(UNIT_STATE_CASTING))
        return;

    me->CastSpell((Unit*)NULL, spellId, triggered);
}

BasicEvent* UnitAI::DoCastDelayed(Unit* victim, uint32 spellId, uint32 delay, bool triggered, bool aliveCheck)
{
    struct DelayedCast : public BasicEvent
    {
        DelayedCast(Unit *caster, Unit *target, uint32 spellId, bool triggered, bool aliveCheck) : BasicEvent(),
            _caster(caster), _target(target), _spellId(spellId), _triggered(triggered), _aliveCheck(aliveCheck) { }

        bool Execute(uint64, uint32)
        {
            if ((_aliveCheck && _caster->IsAlive()) || !_aliveCheck)
                _caster->CastSpell(_target, _spellId, _triggered);
            return true;
        }

        Unit *_caster;
        Unit *_target;
        uint32 _spellId;
        bool _triggered;
        bool _aliveCheck;
    };

    BasicEvent* castEvent = new DelayedCast(me, victim, spellId, triggered, aliveCheck);
    me->m_Events.AddEvent(castEvent, me->m_Events.CalculateTime(delay));
    return castEvent;
}

#define UPDATE_TARGET(a) {if (AIInfo->target<a) AIInfo->target=a;}

void UnitAI::FillAISpellInfo()
{
    AISpellInfo = new AISpellInfoType[sSpellMgr->GetSpellInfoStoreSize()];

    AISpellInfoType* AIInfo = AISpellInfo;
    const SpellInfo* spellInfo;

    for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i, ++AIInfo)
    {
        spellInfo = sSpellMgr->GetSpellInfo(i);
        if (!spellInfo)
            continue;

        if (spellInfo->HasAttribute(SPELL_ATTR0_CASTABLE_WHILE_DEAD))
            AIInfo->condition = AICOND_DIE;
        else if (spellInfo->IsPassive() || spellInfo->GetDuration() == -1)
            AIInfo->condition = AICOND_AGGRO;
        else
            AIInfo->condition = AICOND_COMBAT;

        if (AIInfo->cooldown < spellInfo->RecoveryTime)
            AIInfo->cooldown = spellInfo->RecoveryTime;

        if (!spellInfo->GetMaxRange(false))
            UPDATE_TARGET(AITARGET_SELF)
        else
        {
            for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
            {
                uint32 targetType = spellInfo->Effects[j].TargetA.GetTarget();

                if (targetType == TARGET_UNIT_TARGET_ENEMY
                    || targetType == TARGET_DEST_TARGET_ENEMY)
                    UPDATE_TARGET(AITARGET_VICTIM)
                else if (targetType == TARGET_UNIT_DEST_AREA_ENEMY)
                    UPDATE_TARGET(AITARGET_ENEMY)

                if (spellInfo->Effects[j].Effect == SPELL_EFFECT_APPLY_AURA)
                {
                    if (targetType == TARGET_UNIT_TARGET_ENEMY)
                        UPDATE_TARGET(AITARGET_DEBUFF)
                    else if (spellInfo->IsPositive())
                        UPDATE_TARGET(AITARGET_BUFF)
                }
            }
        }
        AIInfo->realCooldown = spellInfo->RecoveryTime + spellInfo->StartRecoveryTime;
        AIInfo->maxRange = spellInfo->GetMaxRange(false) * 3 / 4;
    }
}

ThreatManager& UnitAI::GetThreatManager()
{
    return me->GetThreatManager();
}

void UnitAI::SortByDistance(std::list<Unit*> list, bool ascending)
{
    list.sort(Trinity::ObjectDistanceOrderPred(me, ascending));
}

DefaultTargetSelector::DefaultTargetSelector(Unit const* unit, float dist, bool playerOnly, bool withTank, int32 aura)
    : me(unit), m_dist(dist), m_playerOnly(playerOnly), except(!withTank ? unit->GetThreatManager().GetCurrentVictim() : nullptr), m_aura(aura)
{
}

bool DefaultTargetSelector::operator()(Unit const* target) const
{
    if (!me)
        return false;

    if (!target)
        return false;

    if (except && target == except)
        return false;

    if (m_playerOnly && (target->GetTypeId() != TYPEID_PLAYER))
        return false;

    if (m_dist > 0.0f && !me->IsWithinCombatRange(target, m_dist))
        return false;

    if (m_dist < 0.0f && me->IsWithinCombatRange(target, -m_dist))
        return false;

    if (m_aura)
    {
        if (m_aura > 0)
        {
            if (!target->HasAura(m_aura))
                return false;
        }
        else
        {
            if (target->HasAura(-m_aura))
                return false;
        }
    }

    return true;
}

SpellTargetSelector::SpellTargetSelector(Unit* caster, uint32 spellId) :
    _caster(caster), _spellInfo(sSpellMgr->GetSpellForDifficultyFromSpell(sSpellMgr->GetSpellInfo(spellId), caster))
{
    ASSERT(_spellInfo);
}

bool SpellTargetSelector::operator()(Unit const* target) const
{
    if (!target)
        return false;

    if (_spellInfo->CheckTarget(_caster, target) != SPELL_CAST_OK)
        return false;

    // copypasta from Spell::CheckRange
    uint32 range_type = _spellInfo->RangeEntry ? _spellInfo->RangeEntry->type : 0;
    float max_range = _caster->GetSpellMaxRangeForTarget(target, _spellInfo);
    float min_range = _caster->GetSpellMinRangeForTarget(target, _spellInfo);


    if (target && target != _caster)
    {
        if (range_type == SPELL_RANGE_MELEE)
        {
            // Because of lag, we can not check too strictly here.
            if (!_caster->IsWithinMeleeRange(target))
                return false;
        }
        else if (!_caster->IsWithinCombatRange(target, max_range))
            return false;

        if (range_type == SPELL_RANGE_RANGED)
        {
            if (_caster->IsWithinMeleeRange(target))
                return false;
        }
        else if (min_range && _caster->IsWithinCombatRange(target, min_range)) // skip this check if min_range = 0
            return false;
    }

    return true;
}

bool NonTankTargetSelector::operator()(Unit const* target) const
{
    if (!target)
        return false;

    if (_playerOnly && target->GetTypeId() != TYPEID_PLAYER)
        return false;

    return target != _source->GetVictim();
}
