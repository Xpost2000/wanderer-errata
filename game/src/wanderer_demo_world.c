/*
 * This is a bad source file meant to just
 * build the demo scene because it was faster
 * than making an editor and the asset system no less.
 *
 * meant to be included as part of the wanderer.c translation unit!
 */

static void game_initialize_cutscenes( game_state* state, renderer* renderer ){
#if 0
    // cutscene init
    {
        {
            global_test_cutscene.name = "OldManYoungManInteractionScene";

            //cutscene_info_set_sequence_time( &global_test_cutscene, 13.f );
            cutscene_info_push_camera_transition(
                &global_test_cutscene,
                500,  /*offset x*/
                105   /*offset y*/
            );

            cutscene_info_push_move_actor( 
                &global_test_cutscene, 
                game_get_player_actor( state ),
                3,  /*offset x*/
                0     /*offset y*/ 
            );

            cutscene_info_wait_for_current_event_to_finish( &global_test_cutscene );

            {
                actor* young_man_actor = actors_list_get_actor( &state->actors, 1 );
                cutscene_info_push_floating_message( 
                    &global_test_cutscene, 
                    1.35f,
                    young_man_actor->position.x * tile_px_size,
                    young_man_actor->position.y * tile_px_size,
                    "Bah... I've had enough of your noise old man!"
                );
            }
            cutscene_info_wait_time( &global_test_cutscene, 3.f );
            {
                actor* young_man_actor = actors_list_get_actor( &state->actors, 1 );
                cutscene_info_push_floating_message( 
                    &global_test_cutscene, 
                    1.35f,
                    young_man_actor->position.x * tile_px_size,
                    young_man_actor->position.y * tile_px_size,
                    "Let someone else deal with you."
                );
            }
            cutscene_info_wait_time( &global_test_cutscene, 2.f );
            {
                actor* old_man_actor = actors_list_get_actor( &state->actors, 2 );
                cutscene_info_push_floating_message( 
                    &global_test_cutscene, 
                    1.35f,
                    old_man_actor->position.x * tile_px_size,
                    old_man_actor->position.y * tile_px_size,
                    "You wouldn't leave me here!"
                );
            }
            cutscene_info_wait_time( &global_test_cutscene, 2.f );
            {
                actor* young_man_actor = actors_list_get_actor( &state->actors, 1 );
                cutscene_info_push_floating_message( 
                    &global_test_cutscene, 
                    1.75f,
                    young_man_actor->position.x * tile_px_size,
                    young_man_actor->position.y * tile_px_size,
                    "No... I believe I will, I'm tired of this crap."
                );
                cutscene_info_push_move_actor( 
                    &global_test_cutscene, 
                    young_man_actor,
                    -2.5,  /*offset x*/
                    3     /*offset y*/ 
                );
            }
            cutscene_info_wait_time( &global_test_cutscene, 2.f );
        }
    }
#endif
}

static void game_initialize_entities( game_state* state, renderer* renderer ){
    tilemap* first_map = tilemap_list_get_tilemap( &state->tilemaps, 0 );
    tilemap* second_map = tilemap_list_get_tilemap( &state->tilemaps, 1 );

    // entities initialization.
    {
        {
            state->player_index = actors_list_push_actor( &state->actors,
                    actor_create_default("The Wanderer",
                                         32,
                                         ACTOR_MINIMUM_ACCEPTABLE_ACTION_POINTS) );
            actor* player = game_get_player_actor( state );

            player->think_type = ACTOR_THINK_TYPE_PLAYER;

            actor_set_resistance(player, DAMAGE_TYPE_PHYSICAL, 2);
            actor_set_resistance(player, DAMAGE_TYPE_MAGIC, -10);
            player->armor_class = 13;

            // crappy inventory initialization
            {
                for( int i = 0; i < 69; ++i ){
                    actor_add_item( player,
                            item_dictionary_make_instance_of( &state->items, "Rubbish" ));
                }

                actor_add_item( player,
                                item_dictionary_make_instance_of( &state->items, "Pendant" ));

                actor_add_item( player,
                                item_dictionary_make_instance_of( &state->items, "Iron Sword" ));
            }

            player->visual_info.model_id = state->models[ACTOR_MODEL_HUMAN],
            player->position.x = 7;
            player->position.y = 9;

            player->size.w = 0.95;
            player->size.h = 0.95;

            player->ability_scores = 
                (ability_score_info){
                    18,
                    17,
                    9,
                    10,
                    17,
                    6
                };

            player->current_map = first_map;

            {
                actor other_man_actor = actor_create_default("Dumb man Man", 32, ACTOR_MINIMUM_ACCEPTABLE_ACTION_POINTS);
                other_man_actor.position.x = 4; 
                other_man_actor.position.y = 6;
                other_man_actor.visual_info.model_id = state->models[ACTOR_MODEL_HUMAN];
                other_man_actor.current_map = first_map;

                actor_set_dialogue_file_reference(&other_man_actor, "data/old_man_dialogue.dialogue");
                u64 old_man_index = actors_list_push_actor( &state->actors, other_man_actor );    
            }

            {
                actor other_man_actor = actor_create_default("MAN MAN", 32, ACTOR_MINIMUM_ACCEPTABLE_ACTION_POINTS);
                other_man_actor.position.x = 4; 
                other_man_actor.position.y = 9;
                other_man_actor.visual_info.model_id = state->models[ACTOR_MODEL_HUMAN];
                other_man_actor.current_map = first_map;

                actor_set_dialogue_file_reference(&other_man_actor, "data/old_man_dialogue.dialogue");
                u64 old_man_index = actors_list_push_actor( &state->actors, other_man_actor );    
            }
        }
    }
}

static void game_initialize_world( game_state* state, renderer* renderer ){
    u64 first_map_index = tilemap_list_new_tilemap( &state->tilemaps );
    u64 second_map_index = tilemap_list_new_tilemap( &state->tilemaps );

    tilemap* first_map = tilemap_list_get_tilemap( &state->tilemaps, first_map_index );
    tilemap* second_map = tilemap_list_get_tilemap( &state->tilemaps, second_map_index );

    char first_map_ascii[17][24] = {
        {"########################"},
        {"#......#...............#"},
        {"#.......#..............#"},
        {"#........#.............#"},
        {"#......................#"},
        {"#..................#####"},
        {"#......................#"},
        {"#......................."},
        {"#......................#"},
        {"#..................#####"},
        {"#......................#"},
        {"#......................#"},
        {"#.....#..#.............#"},
        {"#.....#..#.............#"},
        {"#.....####.............#"},
        {"#......................#"},
        {"########################"},
    };

    char second_map_ascii[17][24] = {
        {"########################"},
        {"#......................#"},
        {"#........#######.......#"},
        {"#........#.....#.......#"},
        {"#.......#.......#......#"},
        {"#......................#"},
        {".......................#"},
        {".......................#"},
        {".......................#"},
        {".......................#"},
        {".......................#"},
        {"#......................#"},
        {"#.............##.......#"},
        {"#.............#........#"},
        {"#......########........#"},
        {"#......########........#"},
        {"########################"},
    };

    first_map->width = 24;
    first_map->height = 17;

    second_map->width = 24;
    second_map->height = 17;

    for( unsigned i = 0; i < 17; ++i ){
        for( unsigned j = 0; j < 24; ++j ){
            switch( first_map_ascii[i][j] ){
                case '#':
                {
                    first_map->tiles[i][j].type = 1;
                    first_map->tiles[i][j].solid = true;
                    first_map->tiles[i][j].x = j;
                    first_map->tiles[i][j].y = i;
                }
                break;
            }
        }
    }

    for( unsigned i = 0; i < 17; ++i ){
        for( unsigned j = 0; j < 24; ++j ){
            switch( second_map_ascii[i][j] ){
                case '#':
                {
                    second_map->tiles[i][j].type = 1;
                    second_map->tiles[i][j].solid = true;
                    second_map->tiles[i][j].x = j;
                    second_map->tiles[i][j].y = i;
                }
                break;
            }
        }
    }

#if 1
    {
        u32 test_container_index =
            tilemap_new_container( first_map );
        container* test_container = 
            tilemap_get_container( first_map, test_container_index );

        test_container->position.x = 6;
        test_container->position.y = 6;
        test_container->size.w = 2;
        test_container->size.h = 1;

        for (unsigned i = 0; i < 30; ++i) {
            container_add_item(test_container, item_dictionary_make_instance_of( &state->items, "Rubbish" ));
        }
    }
#endif

    {
        tilemap_push_trigger( first_map, 
                (map_trigger)
                {
                .trigger_name = "transition_trigger",
                .max_use_times = -1,

                .trigger_type = MAP_TRIGGER_TRANSITION,
                .trigger_shape = MAP_TRIGGER_SHAPE_RECTANGLE,

                .shape.rect.x = first_map->width,
                .shape.rect.y = 7,

                .shape.rect.w = 0.1,
                .shape.rect.h = 1,

                .trigger_info.transition.at_x = 1.1f,
                .trigger_info.transition.at_y = 7.0f,

                .trigger_info.transition.to_map_index = 1
                }
                );

        tilemap_push_trigger( second_map, 
                (map_trigger)
                {
                .trigger_name = "transition_trigger",
                .max_use_times = -1,

                .trigger_type = MAP_TRIGGER_TRANSITION,
                .trigger_shape = MAP_TRIGGER_SHAPE_RECTANGLE,

                .shape.rect.x = 0,
                .shape.rect.y = 5,

                .shape.rect.w = 0.2,
                .shape.rect.h = 7,

                .trigger_info.transition.at_x = 22.f,
                .trigger_info.transition.at_y = 7.0f,

                .trigger_info.transition.to_map_index = 0
                }
                );
    }
#if 0
    {
        tilemap_push_trigger( second_map, 
        (map_trigger)
        {
            .trigger_name = "cutscene_trigger",
            .max_use_times = 1,

            .trigger_type = MAP_TRIGGER_PLAY_CUTSCENE,
            .trigger_shape = MAP_TRIGGER_SHAPE_RECTANGLE,
            
            .trigger_info.cutscene.scene_ptr = &global_test_cutscene,

            .shape.rect.x = 6,
            .shape.rect.y = 0,

            .shape.rect.w = 1,
            .shape.rect.h = 17,
        }
        );
    }
#endif

    game_initialize_entities( state, renderer );
#if 0
    game_initialize_cutscenes( state, renderer );
#endif
}
