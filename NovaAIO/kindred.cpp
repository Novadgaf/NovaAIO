#include "../plugin_sdk/plugin_sdk.hpp"
#include "kindred.h"
#include "iostream"

namespace kindred
{
    // define colors in on_draw() using RGBA (102,255,255)
    #define COLOR_1 (MAKE_COLOR ( 178, 58, 238, 100 )) 
    #define COLOR_11 (MAKE_COLOR ( 178, 58, 238, 255 )) 
    #define COLOR_2 (MAKE_COLOR ( 255, 0, 0, 100 ))  
    #define COLOR_22 (MAKE_COLOR ( 255, 0, 0, 255 ))  
    #define GREEN (MAKE_COLOR (0,255,0,255))
    #define RED (MAKE_COLOR (255,0,0,255))


    glow_data glow(0.02f, 3.0f, 0.0f, 0.0f);
    

    #define W_EFFECT_RADIUS 800
    #define KINDRED_MARK_HASH 1680409346
    #define KINDRED_PASSIVE_HASH 1854341764
    #define KINDRED_PASSIVE_ON_CAMP_HASH 983279781
    #define VALID_E_MONSTER_MODELS {"SRU_Blue", "SRU_Red", "SRU_Gromp", "SRU_Krug", "SRU_Murkwolf", "SRU_Razorbeak", "Sru_Crab"}
    #define BASIC_SMITE_HASH 106858133
    #define UNLEASHED_SMITE_HASH 43626882
    #define PRIMAL_SMITE_HASH 210032469

    script_spell* q = nullptr;
    script_spell* w = nullptr;
    script_spell* e = nullptr;
    script_spell* r = nullptr;

    script_spell* smite = nullptr;

    TreeTab* main_tab = nullptr;

    namespace draw_settings
    {
        TreeEntry* draw_range_q = nullptr;
        TreeEntry* draw_range_w = nullptr;
        TreeEntry* draw_range_e = nullptr;
        TreeEntry* draw_range_r = nullptr;
    }

    namespace combo
    {
        TreeEntry* q_seperator = nullptr;
        TreeEntry* q_smart = nullptr;
        TreeEntry* q_gapclose = nullptr;
        TreeEntry* q_gapclose_range = nullptr;
        TreeEntry* q_dive = nullptr;

        TreeEntry* w_seperator = nullptr;
        TreeEntry* w_logic = nullptr;

        TreeEntry* e_seperator = nullptr;

        TreeEntry* r_seperator = nullptr;
        TreeEntry* use_r = nullptr;
        std::map<std::uint32_t, TreeEntry*> use_r_on;
    }

    namespace harass
    {
        TreeEntry* q_seperator = nullptr;
        TreeEntry* use_q = nullptr;
        TreeEntry* w_seperator = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* e_seperator = nullptr;
        TreeEntry* use_e = nullptr;
    }

    namespace laneclear
    {
        TreeEntry* q_seperator = nullptr;
        TreeEntry* use_q = nullptr;
        TreeEntry* w_seperator = nullptr;
        TreeEntry* use_w = nullptr;
    }

    namespace jungleclear
    {
        TreeEntry* q_seperator = nullptr;
        TreeEntry* use_q = nullptr;
        TreeEntry* w_seperator = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* e_seperator = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* misc_seperator = nullptr;
        TreeEntry* auto_smite_marked = nullptr;
    }

    namespace fleemode
    {
        TreeEntry* q_seperator = nullptr;
        TreeEntry* use_q = nullptr;
        TreeEntry* w_seperator = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* e_seperator = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* r_seperator = nullptr;
        TreeEntry* use_r = nullptr;
    }

    namespace misc
    {
        TreeEntry* use_q_animation_cancel = nullptr;
    }

    // Event handler functions
    void on_update();
    void on_draw();
    void on_buff_gain(game_object_script sender, buff_instance_script buff);
    void on_process_spell_cast(game_object_script sender, spell_instance_script spell);

    //combo
    bool q_logic_combo();
    bool q_gapclose_combo();
    bool is_q_end_position_under_enemy_turret(vector cast_pos);
    bool w_logic_combo();
    bool e_logic_combo();
    bool r_logic_combo();
    bool can_use_r_on(game_object_script ally);
    bool use_r_on_ally(game_object_script ally);

    //harass
    bool q_logic_harass();
    bool is_valid_monster(game_object_script monster);
    
    //farm
    bool smite_if_marked(game_object_script monster);
    uint32_t get_smite_damage();

    //flee
    bool q_logic_flee();
    bool w_logic_flee();
    bool e_logic_flee();
    bool r_logic_flee();

    //misc
    uint32_t get_mark_bonus_range();
    uint32_t get_e_range();

    enum Position
    {
        Line,
        Jungle
    };

    Position my_hero_region;

    void load()
    {
        // Registering a spells
        //
        q = plugin_sdk->register_spell(spellslot::q, 340);
        q->set_skillshot(0.25f, 60.0f, FLT_MAX, { collisionable_objects::yasuo_wall}, skillshot_type::skillshot_line);
        w = plugin_sdk->register_spell(spellslot::w, 500);
        e = plugin_sdk->register_spell(spellslot::e, get_e_range());
        r = plugin_sdk->register_spell(spellslot::r, 500);

        // Checking which slot the Summoner Smite is on
        //
        if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerSmite"))
        {
            smite = plugin_sdk->register_spell(spellslot::summoner1, 500);
        }
        else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerSmite"))
        {
            smite = plugin_sdk->register_spell(spellslot::summoner2, 500);
        }

        // Create a menu according to the description in the "Menu Section"
        //
        main_tab = menu->create_tab("kindred", "NovaAIO Kindred");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        {
            auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo");
            {
                combo::q_seperator = combo->add_separator("settingsQ", " Q Settings ");
                combo::q_smart = combo->add_combobox("comboQLogic", "Q Logic", { { "Smart Logic" , nullptr }, {"Mouse Position", nullptr} }, 0);
                combo::q_gapclose = combo->add_checkbox(myhero->get_model() + ".comboUseQGapclose", "Use Q gapclose", true);
                combo::q_gapclose_range = combo->add_slider(".comboQGapcloseRange", "Q gapclose range", 2000, 1000, 3000);
                combo::q_dive = combo->add_hotkey("comboQDive", "Allow Q Dive", TreeHotkeyMode::Toggle, 0x41 /*A key*/, false);

                combo::w_seperator = combo->add_separator("settingsW", " W Settings ");
                combo::w_logic = combo->add_combobox("comboWLogic", "W Logic", { { "On Target" , nullptr }, {"Mouse Position", nullptr} }, 0);

                combo::e_seperator = combo->add_separator("settingsE", " E Settings ");

                combo::r_seperator = combo->add_separator("settingsR", " R Settings ");
                combo::use_r = combo->add_checkbox(myhero->get_model() + ".comboUseR", "Use R", true);
                combo::use_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
                auto use_r_on_tab = combo->add_tab("use_r", "Use R On");
                {
                    for (auto&& ally : entitylist->get_ally_heroes())
                    {
                        // In this case you HAVE to set should save to false since key contains network id which is unique per game
                        //
                        combo::use_r_on[ally->get_network_id()] = use_r_on_tab->add_checkbox(std::to_string(ally->get_network_id()), ally->get_model(), true, false);

                        // Set texture to enemy square icon
                        //
                        combo::use_r_on[ally->get_network_id()]->set_texture(ally->get_square_icon_portrait());
                    }
                }
            }

            auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Harass");
            {
                harass::q_seperator = harass->add_separator("settingsQ", " Q Settings ");
                harass::use_q = harass->add_checkbox(myhero->get_model() + ".harassUseQ", "Use Q", true);
                harass::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());

                harass::w_seperator = harass->add_separator("settingsW", " W Settings ");
                harass::use_w = harass->add_checkbox(myhero->get_model() + ".harassUseW", "Use W", false);
                harass::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());

                harass::e_seperator = harass->add_separator("settingsE", " E Settings ");
                harass::use_e = harass->add_checkbox(myhero->get_model() + ".harassUseE", "Use E", false);
                harass::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
            }

            auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "LaneClear");
            {
                harass::q_seperator = laneclear->add_separator("settingsQ", " Q Settings ");
                laneclear::use_q = laneclear->add_checkbox(myhero->get_model() + ".laneclearUseQ", "Use Q", true);
                laneclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());

                harass::w_seperator = laneclear->add_separator("settingsW", " W Settings ");
                laneclear::use_w = laneclear->add_checkbox(myhero->get_model() + ".laneclearUseW", "Use W", false);
                laneclear::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
            }

            auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleclear", "JungleClear");
            {
                jungleclear::q_seperator = jungleclear->add_separator("settingsQ", " Q Settings ");
                jungleclear::use_q = jungleclear->add_checkbox(myhero->get_model() + ".jungleclearUseQ", "Use Q", true);
                jungleclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());

                jungleclear::w_seperator = jungleclear->add_separator("settingsW", " W Settings ");
                jungleclear::use_w = jungleclear->add_checkbox(myhero->get_model() + ".jungleclearUseW", "Use W", true);
                jungleclear::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());

                jungleclear::e_seperator = jungleclear->add_separator("settingsE", " E Settings ");
                jungleclear::use_e = jungleclear->add_checkbox(myhero->get_model() + ".jungleclearUseE", "Use E", true);
                jungleclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());

                jungleclear::misc_seperator = jungleclear->add_separator("settingsMisc", " Misc Settings ");
                jungleclear::auto_smite_marked = jungleclear->add_checkbox(myhero->get_model() + ".jungleclearUseAutoSmite", "Smite marked camps", true);
                jungleclear::auto_smite_marked->set_texture(myhero->get_spell(smite->get_slot())->get_icon_texture());
            }

            auto fleemode = main_tab->add_tab(myhero->get_model() + ".fleemode", "Flee");
            {
                fleemode::q_seperator = fleemode->add_separator("settingsQ", " Q Settings ");
                fleemode::use_q = fleemode->add_checkbox(myhero->get_model() + ".fleemoderUseQ", "Use Q", true);
                fleemode::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());

                fleemode::w_seperator = fleemode->add_separator("settingsW", " W Settings ");
                fleemode::use_w = fleemode->add_checkbox(myhero->get_model() + ".fleemodeUseW", "Use W", true);
                fleemode::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());

                fleemode::e_seperator = fleemode->add_separator("settingsE", " E Settings ");
                fleemode::use_e = fleemode->add_checkbox(myhero->get_model() + ".fleemodeUseE", "Use E", true);
                fleemode::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());

                fleemode::r_seperator = fleemode->add_separator("settingsR", " R Settings ");
                fleemode::use_r = fleemode->add_checkbox(myhero->get_model() + ".fleemodeUseR", "Use R", true);
                fleemode::use_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
            }

            auto misc = main_tab->add_tab(myhero->get_model() + ".misc", "Misc");
            {
                misc::use_q_animation_cancel = misc->add_checkbox(myhero->get_model() + ".miscUseQCancel", "Use Q animation cancel", true);
            }

            auto draw_settings = main_tab->add_tab(myhero->get_model() + ".drawings", "Drawings");
            {
                draw_settings::draw_range_q = draw_settings->add_checkbox(myhero->get_model() + ".drawingQ", "Draw Q range", true);
                draw_settings::draw_range_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                draw_settings::draw_range_w = draw_settings->add_checkbox(myhero->get_model() + ".drawingW", "Draw W range", false);
                draw_settings::draw_range_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                draw_settings::draw_range_e = draw_settings->add_checkbox(myhero->get_model() + ".drawingE", "Draw E range", true);
                draw_settings::draw_range_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                draw_settings::draw_range_r = draw_settings->add_checkbox(myhero->get_model() + ".drawingR", "Draw R range", false);
                draw_settings::draw_range_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
            }
        }

        // To add a new event you need to define a function and call add_calback
        //
        event_handler<events::on_update>::add_callback(on_update);
        event_handler<events::on_draw>::add_callback(on_draw);
        event_handler<events::on_buff_gain>::add_callback(on_buff_gain);
        event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);

    }

    void unload()
    {
        // Always remove all declared spells
        //
        plugin_sdk->remove_spell(q);
        plugin_sdk->remove_spell(w);
        plugin_sdk->remove_spell(e);
        plugin_sdk->remove_spell(r);

        if (smite)
            plugin_sdk->remove_spell(smite);


        // VERY important to remove always ALL events
        //
        event_handler<events::on_update>::remove_handler(on_update);
        event_handler<events::on_draw>::remove_handler(on_draw);
        event_handler<events::on_buff_gain>::remove_handler(on_buff_gain);
        event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
    }


    void on_update()
    {
        if (myhero->is_dead())
        {
            return;
        }

        

        if (orbwalker->combo_mode())
        {
            if (r->is_ready())
            {
                if (r_logic_combo())
                    return;
            }

            if (w->is_ready())
            {
                if (w_logic_combo())
                    return;
            }

            if (e->is_ready())
            {
                if (e_logic_combo())
                    return;
            }

            if (q->is_ready())
            {
                if (q_logic_combo())
                    return;
            }
        }

        if (orbwalker->harass())
        {
            if (w->is_ready() && harass::use_w->get_bool())
            {
                w->cast();
                return;
            }

            if (e->is_ready() && harass::use_e->get_bool())
            {
                if (e_logic_combo())
                    return;
            }

            if (q->is_ready() && harass::use_q->get_bool())
            {
                if (q_logic_harass())
                    return;
            }
        }

        // Checking if the user has selected flee_mode() (Default Z)
        if (orbwalker->flee_mode())
        {
            if (r->is_ready() && fleemode::use_r->get_bool())
            {
                if (r_logic_flee())
                    return;
            }

            if (w->is_ready() && fleemode::use_w->get_bool())
            {
                if (w_logic_flee())
                    return;
            }

            if (e->is_ready() && fleemode::use_e->get_bool())
            {
                if (e_logic_flee())
                    return;
            }

            if (q->is_ready() && fleemode::use_q->get_bool())
            {
                if (q_logic_flee())
                    return;
            }
        }

        // Checking if the user has selected lane_clear_mode() (Default V)
        if (orbwalker->lane_clear_mode())
        {
            if (myhero->is_winding_up())
            {
                return;
            }

            auto lane_minions = entitylist->get_enemy_minions();
            lane_minions.erase(std::remove_if(lane_minions.begin(), lane_minions.end(), [](game_object_script x)
            {
                return !x->is_valid_target(1000);
            }), lane_minions.end());


            auto monsters = entitylist->get_jugnle_mobs_minions();
            monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
            {
                return !x->is_valid_target(800);
            }), monsters.end());

            //std::sort -> sort monsters by max helth
            std::sort(monsters.begin(), monsters.end(), [](game_object_script a, game_object_script b)
            {
                return a->get_max_health() > b->get_max_health();
            });

            if (!lane_minions.empty())
            {
                // Logic responsible for lane minions
                //
                if (q->is_ready() && laneclear::use_q->get_bool())
                {
                    if (q->cast(hud->get_hud_input_logic()->get_game_cursor_position()))
                        return;
                }
                if (w->is_ready() && laneclear::use_w->get_bool())
                {
                    if (w->cast(myhero->get_position()))
                    {
                        return;
                    }
                }
            }

            if (!monsters.empty())
            {
                if (monsters[0]->has_buff(KINDRED_PASSIVE_ON_CAMP_HASH))
                {
                    if (smite_if_marked(monsters[0]))
                        return;
                }
                if (w->is_ready() && jungleclear::use_w->get_bool())
                {
                    if (is_valid_monster(monsters[0]))
                    {
                        if (w->cast(myhero->get_position()))
                            return;
                    }
                }

                if (e->is_ready() && jungleclear::use_e->get_bool())
                {
                    if (is_valid_monster(monsters[0]))
                    {
                        if (e->cast(monsters[0]))
                            return;
                    }
                }

                if (q->is_ready() && jungleclear::use_q->get_bool())
                {
                    if (q->cast(hud->get_hud_input_logic()->get_game_cursor_position()))
                        return;
                }
            }
        }
    }

    void on_buff_gain(game_object_script sender, buff_instance_script buff)
    {
        if (buff->get_hash_name() == KINDRED_PASSIVE_HASH)
            e->set_range(get_e_range());
    }

    void on_draw()
    {
        if (myhero->is_dead())
        {
            return;
        }

        if (q->is_ready() && draw_settings::draw_range_q->get_bool())
        {
            draw_manager->add_circle_with_glow_gradient(myhero->get_position(), COLOR_1, COLOR_2, q->range(), 1.0f, glow);
            draw_manager->add_circle_gradient(myhero->get_position(), q->range(), COLOR_11, COLOR_22);
        }

        if (w->is_ready() && draw_settings::draw_range_w->get_bool())
        {
            draw_manager->add_circle_with_glow_gradient(myhero->get_position(), COLOR_1, COLOR_2, w->range(), 1.0f, glow);
            draw_manager->add_circle_gradient(myhero->get_position(), w->range(), COLOR_11, COLOR_22);
        }

        if (e->is_ready() && draw_settings::draw_range_e->get_bool())
        {
            draw_manager->add_circle_with_glow_gradient(myhero->get_position(), COLOR_1, COLOR_2, e->range(), 1.0f, glow);
            draw_manager->add_circle_gradient(myhero->get_position(), e->range(), COLOR_11, COLOR_22);
        }


        if (r->is_ready() && draw_settings::draw_range_r->get_bool())
        {
            draw_manager->add_circle_with_glow_gradient(myhero->get_position(), COLOR_1, COLOR_2, r->range(), 3.0f, glow);
            draw_manager->add_circle_gradient(myhero->get_position(), r->range(), COLOR_11, COLOR_22);
        }


        auto pos = myhero->get_position();

        renderer->world_to_screen(pos, pos);
        if (combo::q_dive->get_bool())
        {
            draw_manager->add_text_on_screen(pos + vector(-45, 30), GREEN, 15, "DIVE [ON]");
        }
        else
        {
            draw_manager->add_text_on_screen(pos + vector(-45, 30), RED, 15, "DIVE [OFF]");
        }

        draw_manager->overwrite_shader_settings(true, false);
    }

    #pragma region combo_functions
    bool q_logic_combo()
    {
        if (myhero->is_winding_up())
            return false;

        auto target = target_selector->get_target(myhero->get_attack_range()+q->range(), damage_type::physical, false, true);

        if (target == nullptr)
            return q_gapclose_combo();

        vector cast_pos;
        if (target->is_facing(myhero) || combo::q_smart->get_int() == 1)
        {
            cast_pos = hud->get_hud_input_logic()->get_game_cursor_position();
        }
        else
        {
            cast_pos = target->get_position();
        }

        if (!combo::q_dive->get_bool())
        {
            if (is_q_end_position_under_enemy_turret(cast_pos))
            {
                if (myhero->is_under_enemy_turret())
                {
                    return false;
                }
                else
                    cast_pos = myhero->get_position();
            }
        }

        q->cast(cast_pos);
        return true;
    }

    bool q_gapclose_combo()
    {
        if (!combo::q_gapclose->get_bool())
            return false;

        auto target = target_selector->get_target(combo::q_gapclose_range->get_int(), damage_type::physical, false, true);

        if (target == nullptr)
            return false;
        
        vector cast_pos = hud->get_hud_input_logic()->get_game_cursor_position();
        q->cast(cast_pos);
        return true;
    }

    bool is_q_end_position_under_enemy_turret  (vector cast_pos)
    {
        // Getting current position of the hero
        auto my_pos = myhero->get_position();

        // Calculating the direction from current position to cast position
        vector direction = (cast_pos - my_pos).normalized();

        // Calculating the distance between the two points
        float distance = my_pos.distance(cast_pos);

        // Getting the range of the spell
        float spell_range = q->range();

        vector end_pos;

        // Checking if the distance is greater than the spell range
        if (distance > spell_range) {
            // If so, calculate the new cast position
            end_pos = my_pos + direction * spell_range;
        }
        else {
            // Otherwise, use the original cast position
            end_pos = cast_pos;
        }
        
        return end_pos.is_under_enemy_turret();
    }

    bool w_logic_combo()
    {
        auto target = target_selector->get_target(w->range() + W_EFFECT_RADIUS, damage_type::magical);

        if (target == nullptr)
            return false;

        vector w_pos;

        if (combo::w_logic->get_int() == 0)
            w_pos = target->get_position();
        else
            w_pos = hud->get_hud_input_logic()->get_game_cursor_position();
            
        w->cast(w_pos);

        return true;
    }

    bool e_logic_combo()
    {
        if (myhero->is_winding_up())
            return false;

        auto target = target_selector->get_target(e->range(), damage_type::physical, false, true);

        if (target == nullptr)
            return false;

        e->cast(target);

        return true;
    }

    bool r_logic_combo()
    {
        if (!combo::use_r->get_bool())
            return false;

        auto allies = entitylist->get_ally_heroes();

        for (auto ally : allies)
        {
            if (use_r_on_ally(ally))
            {
                r->cast();
                return true;
            }
        }
        return false;
    }

    bool can_use_r_on(game_object_script ally)
    {
        auto it = combo::use_r_on.find(ally->get_network_id());
        if (it == combo::use_r_on.end())
            return false;

        return it->second->get_bool();
    }

    bool use_r_on_ally(game_object_script ally)
    {
        vector ally_pos = ally->get_position();
        float distance = myhero->get_distance(ally_pos);
        float r_range = r->range();

        if (distance > r_range)
            return false;

        if (!can_use_r_on(ally))
            return false;

        float ally_hp = ally->get_health();
        float incoming_damage = health_prediction->get_incoming_damage(ally, 0.25, true);

        return incoming_damage > ally_hp;
    }

    #pragma endregion

    #pragma region harass
    bool q_logic_harass()
    {
        if (myhero->is_winding_up())
            return false;

        auto target = target_selector->get_target(myhero->get_attack_range(), damage_type::physical, false, true);

        if (target == nullptr)
            return false;

        q->cast(target->get_position());

        return true;
    }


    #pragma endregion

    #pragma region farm
    bool is_valid_monster(game_object_script monster)
    {
        if (monster == nullptr)
            return false;

        if (monster->is_epic_monster())
            return true;

        for (const std::string& monster_model : VALID_E_MONSTER_MODELS)
        {
            if (monster->get_model() == monster_model)
                return true;
        }

        return false;
    }

    bool smite_if_marked(game_object_script monster)
    {

        if (!jungleclear::auto_smite_marked->get_bool())
            return false;

        if (monster == nullptr)
            return false;

        if (!smite)
            return false;

        float monster_hp = monster->get_health();
        int smite_damage = get_smite_damage();

        if (smite_damage >= monster_hp)
        {

            console->print("SMITE NOW");
            smite->cast(monster);
        }

        return false;
    }

    uint32_t get_smite_damage()
    {
         auto smite_hash = smite->name_hash();

        if (smite_hash == BASIC_SMITE_HASH)
            return 600;
        else if(smite_hash == UNLEASHED_SMITE_HASH)
            return 900;
        else if(smite_hash == PRIMAL_SMITE_HASH)
            return 1200;

        return 0;
    }

    #pragma endregion

    #pragma region flee
    bool q_logic_flee()
    {
        vector cast_pos = hud->get_hud_input_logic()->get_game_cursor_position();

        q->cast(cast_pos);

        return true;
    }

    bool w_logic_flee()
    {
        vector w_pos = hud->get_hud_input_logic()->get_game_cursor_position();

        w->cast(w_pos);

        return true;
    }

    bool e_logic_flee()
    {
        auto target = target_selector->get_target(e->range(), damage_type::physical, false, true);

        if (target == nullptr)
            return false;

        e->cast(target);

        return true;
    }

    bool r_logic_flee()
    {
        auto allies = entitylist->get_ally_heroes();

        for (auto ally : allies)
        {
            console->print("Testing r for %s", ally->get_name_cstr());
            if (use_r_on_ally(ally))
            {
                r->cast();
                return true;
            }
        }

        return false;
    }
    #pragma endregion

    #pragma region misc
    void on_process_spell_cast(game_object_script sender, spell_instance_script spell)
    {
        // Q animation cancel
        if (spell->get_spell_data()->get_name_hash() == q->name_hash())
            if (misc::use_q_animation_cancel->get_bool())
            {
                myhero->send_emote(emote_type::EMOTE_DANCE);
                return;
            }
    }

    uint32_t get_e_range()
    {
        return get_mark_bonus_range() + 500 + myhero->get_bounding_radius();
    }

    uint32_t get_mark_bonus_range()
    {
        uint32_t marks = myhero->get_buff_count(KINDRED_MARK_HASH);

        if (marks < 4)
            return 0;

        uint32_t bonus_range = 75;
        marks -= 4;
        bonus_range += int(marks / 3) * 25;

        return bonus_range;
    }
    #pragma endregion
   

}