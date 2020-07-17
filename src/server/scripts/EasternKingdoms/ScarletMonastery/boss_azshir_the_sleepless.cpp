/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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
SDName: Boss_Azshir_the_Sleepless
SD%Complete: 80
SDComment:
SDCategory: Scarlet Monastery
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "scarlet_monastery.h"

enum Spells
{
    SPELL_CALL_OF_THE_GRAVE         = 17831,
    SPELL_TERRIFY                   = 7399,
    SPELL_SOUL_SIPHON               = 7290
};

enum Events
{
    EVENT_CALL_OF_GRAVE             = 1,
    EVENT_TERRIFY,
    EVENT_SOUL_SIPHON 
};

class boss_azshir_the_sleepless : public CreatureScript
{
    public:
        boss_azshir_the_sleepless() : CreatureScript("boss_azshir_the_sleepless") { }

        struct boss_azshir_the_sleeplessAI : public BossAI
        {
            boss_azshir_the_sleeplessAI(Creature* creature) : BossAI(creature, DATA_AZSHIR) { }

            void Reset() override
            {
                BossAI::Reset();
                _siphon = false;
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(EVENT_CALL_OF_GRAVE, 30000);
                events.ScheduleEvent(EVENT_TERRIFY, 20000);
            }

            void DamageTaken(Unit* /*done_by*/, uint32& /*damage*/) override
            {
                if (HealthBelowPct(50) && !_siphon)
                {
                    DoCastVictim(SPELL_SOUL_SIPHON);
                    events.ScheduleEvent(EVENT_SOUL_SIPHON, 20000);
                    _siphon = true;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;
                    
                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CALL_OF_GRAVE:
                            DoCastVictim(SPELL_CALL_OF_THE_GRAVE);
                            events.ScheduleEvent(EVENT_CALL_OF_GRAVE, 30000);
                            break;
                        case EVENT_TERRIFY:
                            DoCastVictim(SPELL_TERRIFY);
                            events.ScheduleEvent(EVENT_TERRIFY, 20000);
                            break;
                        case EVENT_SOUL_SIPHON:
                            DoCastVictim(SPELL_SOUL_SIPHON);
                            events.ScheduleEvent(EVENT_SOUL_SIPHON, 20000);
                            break;
                        default:
                            break;
                    }

                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool _siphon;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_azshir_the_sleeplessAI>(creature);
        }
};

void AddSC_boss_azshir_the_sleepless()
{
    new boss_azshir_the_sleepless();
}
