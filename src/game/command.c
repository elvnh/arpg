#include "command.h"

#include "base/linear_arena.h"
#include "base/list.h"
#include "base/utils.h"
#include "components/equipment.h"
#include "components/inventory.h"
#include "entity/entity_id.h"
#include "entity/entity_system.h"
#include "ui/game_ui.h"
#include "world/world.h"

void push_command(CommandQueue *commands, Command command, LinearArena *arena)
{
    Command *command_node = la_allocate_item(arena, Command);
    *command_node = command;

    sl_list_push_back(commands, command_node);
}

void execute_command_queue(CommandQueue *commands, World *world, GameUI *game_ui)
{
    Command *current = list_head(commands);

    // TODO: perform more validation in this function
    while (current) {
        BEGIN_EXHAUSTIVE_SWITCH;
        switch (current->kind) {
            case COMMAND_WALK: {
                MoveCommand *cmd = &current->as.move;

                Entity *player = world_get_player_entity(world);
                PhysicsComponent *physics = es_get_component(player, PhysicsComponent);
                entity_try_transition_to_state(world, player, physics,
                    state_walking(cmd->direction));
            } break;

            case COMMAND_ATTACK: {
                AttackCommand *cmd = &current->as.attack;

                Entity *player = world_get_player_entity(world);
                SpellCasterComponent *spellcaster =
                    es_get_component(player, SpellCasterComponent);
                SpellID selected_spell = get_spell_at_spellbook_index(spellcaster,
                    game_ui->selected_spellbook_index);

                try_cast_spell(world, selected_spell, player, cmd->target);
            } break;

            case COMMAND_ITEM_TRANSACTION: {
                ItemTransactionCommand *cmd = &current->as.item_transaction;

                execute_item_transaction(cmd->transaction, world, game_ui);
            } break;
        }
        END_EXHAUSTIVE_SWITCH;

        sl_list_pop(commands);
        current = list_head(commands);
    }
}
