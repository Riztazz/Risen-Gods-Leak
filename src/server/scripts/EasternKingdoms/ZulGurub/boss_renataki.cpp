/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Boss_Renataki
SD%Complete: 100
SDComment:
SDCategory: Zul'Gurub
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "zulgurub.h"

enum Spells
{
    SPELL_THOUSAND_BLADES       = 24767,
    SPELL_SINISTRE_STRIKE       = 19472,
    SPELL_SNAP_KICK             = 24671,
    SPELL_AXE_FLURRY            = 24020
};

enum Misc
{
    EQUIP_ID_MAIN_HAND          = 0,     //was item display id 31818, but this id does not exist
    MODEL_INVISIBLE             = 11686,
    MODEL_VISIBLE               = 15268
};

class boss_renataki : public CreatureScript
{
    public:
        boss_renataki() : CreatureScript("boss_renataki") { }

        struct boss_renatakiAI : public BossAI
        {
            boss_renatakiAI(Creature* creature) : BossAI(creature, DATA_EDGE_OF_MADNESS) { }

            uint32 Invisible_Timer;
            uint32 ThousandBlades_Timer;
            uint32 Visible_Timer;
            uint32 Aggro_Timer;
            uint32 SinistreStrike_Timer;
            uint32 SnapKick_Timer;
            uint32 AxeFlurry_Timer;

            bool Invisible;
            bool ThousandBlades;

            void Reset() override
            {
                BossAI::Reset();
                Invisible_Timer      =  urand(8, 18)*IN_MILLISECONDS;
                ThousandBlades_Timer =             3*IN_MILLISECONDS;
                Visible_Timer        =             4*IN_MILLISECONDS;
                Aggro_Timer          = urand(15, 25)*IN_MILLISECONDS;
                SinistreStrike_Timer =   urand(4, 8)*IN_MILLISECONDS;
                SnapKick_Timer       = urand(10, 20)*IN_MILLISECONDS;
                AxeFlurry_Timer      =             9*IN_MILLISECONDS;

                Invisible      = false;
                ThousandBlades = false;
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                //Invisible_Timer
                if (Invisible_Timer <= diff)
                {
                    me->InterruptSpell(CURRENT_GENERIC_SPELL);

                    SetEquipmentSlots(false, EQUIP_UNEQUIP, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
                    me->SetDisplayId(MODEL_INVISIBLE);

                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    Invisible = true;

                    Invisible_Timer = urand(15, 30)*IN_MILLISECONDS;
                } else Invisible_Timer -= diff;

                if (Invisible)
                {
                    if (ThousandBlades_Timer <= diff)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        {
                            DoTeleportTo(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
                            DoCast(target, SPELL_THOUSAND_BLADES);
                        }

                        ThousandBlades = true;
                        ThousandBlades_Timer = 3000;
                    } else ThousandBlades_Timer -= diff; 
                }

                if (ThousandBlades)
                {
                    if (Visible_Timer <= diff)
                    {
                        me->InterruptSpell(CURRENT_GENERIC_SPELL);

                        me->SetDisplayId(MODEL_VISIBLE);
                        SetEquipmentSlots(false, EQUIP_ID_MAIN_HAND, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);

                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        Invisible = false;

                        Visible_Timer = 4000;
                    } else Visible_Timer -= diff;
                }

                //Resetting some aggro so he attacks other gamers
                if (!Invisible)
                {
                    if (Aggro_Timer <= diff)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        {
                            if (GetThreat(me->GetVictim()))
                                ModifyThreatByPercent(me->GetVictim(), -50);
                            AttackStart(target);
                        }

                        Aggro_Timer = urand(7, 20)*IN_MILLISECONDS;
                    } else Aggro_Timer -= diff;

                    if (SinistreStrike_Timer <= diff)
                    {
                        DoCastVictim(SPELL_SINISTRE_STRIKE);
                        SinistreStrike_Timer = urand(7, 12)*IN_MILLISECONDS;
                    } else SinistreStrike_Timer -= diff; 

                    if (SnapKick_Timer <= diff)
                    {
                        DoCastVictim(SPELL_SNAP_KICK);
                        SnapKick_Timer = urand(10, 16)*IN_MILLISECONDS;
                    } else SnapKick_Timer -= diff; 

                    if (AxeFlurry_Timer <= diff)
                    {
                        DoCastVictim(SPELL_AXE_FLURRY);
                        AxeFlurry_Timer = urand(9, 15)*IN_MILLISECONDS;
                    } else AxeFlurry_Timer -= diff;                     
                   
                    DoMeleeAttackIfReady();
                }
               
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_renatakiAI(creature);
        }
};

void AddSC_boss_renataki()
{
    new boss_renataki();
}

