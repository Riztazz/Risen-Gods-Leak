/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2010-2015 Rising Gods <http://www.rising-gods.de/>
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

/* ScriptData
SDName: Boss_Baron_Geddon
SD%Complete: 100
SDComment:
SDCategory: Molten Core
EndScriptData */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "molten_core.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"

enum Emotes
{
    EMOTE_SERVICE       = 0
};

enum Spells
{
    SPELL_INFERNO       = 19695,
    SPELL_INFERNO_DMG   = 19698,
    SPELL_IGNITE_MANA   = 19659,
    SPELL_LIVING_BOMB   = 20475,
    SPELL_ARMAGEDDON    = 20478,
};

enum Events
{
    EVENT_INFERNO       = 1,
    EVENT_IGNITE_MANA   = 2,
    EVENT_LIVING_BOMB   = 3,
};

class boss_baron_geddon : public CreatureScript
{
    public:
        boss_baron_geddon() : CreatureScript("boss_baron_geddon") { }

        struct boss_baron_geddonAI : public BossAI
        {
            boss_baron_geddonAI(Creature* creature) : BossAI(creature, BOSS_BARON_GEDDON) { }

            void Reset() override
            {
                BossAI::Reset();
                hasYelled = false;
            }

            void EnterCombat(Unit* victim) override
            {
                BossAI::EnterCombat(victim);
                events.ScheduleEvent(EVENT_INFERNO,     45 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_IGNITE_MANA, 30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_LIVING_BOMB, 35 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                // If we are <2% hp cast Armageddon
                if (HealthBelowPct(2) && !hasYelled)
                {
                    // TEMP: remove armageddon
                    //me->InterruptNonMeleeSpells(true);
                    //DoCastSelf(SPELL_ARMAGEDDON);
                    hasYelled = true;
                    Talk(EMOTE_SERVICE);
                    return;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INFERNO:
                            DoCastSelf(SPELL_INFERNO);
                            events.ScheduleEvent(EVENT_INFERNO, 45 * IN_MILLISECONDS);
                            break;
                        case EVENT_IGNITE_MANA:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, true, -SPELL_IGNITE_MANA))
                                DoCast(target, SPELL_IGNITE_MANA);
                            events.ScheduleEvent(EVENT_IGNITE_MANA, 30 * IN_MILLISECONDS);
                            break;
                        case EVENT_LIVING_BOMB:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_LIVING_BOMB);
                            events.ScheduleEvent(EVENT_LIVING_BOMB, 35 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool hasYelled;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_baron_geddonAI(creature);
        }
};

class spell_baron_geddon_inferno : public SpellScriptLoader
{
    public:
        spell_baron_geddon_inferno() : SpellScriptLoader("spell_baron_geddon_inferno") { }

        class spell_baron_geddon_inferno_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_baron_geddon_inferno_AuraScript);

            void OnPeriodic(AuraEffect const* aurEff)
            {
                PreventDefaultAction();
                int32 damageForTick[8] = { 500, 500, 1000, 1000, 2000, 2000, 3000, 5000 };
                GetTarget()->CastCustomSpell(SPELL_INFERNO_DMG, SPELLVALUE_BASE_POINT0, damageForTick[aurEff->GetTickNumber() - 1], (Unit*)nullptr, TRIGGERED_FULL_MASK, nullptr, aurEff);
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_baron_geddon_inferno_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_baron_geddon_inferno_AuraScript();
        }
};

void AddSC_boss_baron_geddon()
{
    new boss_baron_geddon();
    new spell_baron_geddon_inferno();
}
