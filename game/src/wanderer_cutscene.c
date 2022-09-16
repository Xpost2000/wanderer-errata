#include "wanderer.h"

#include "wanderer_cutscene.h"

/*Cutscene system*/
void cutscene_info_set_sequence_time( cutscene_info* info, f32 time_set ){
    info->timed = (time_set > 0.f);
    info->scene_time = time_set;
}

void cutscene_info_push_floating_message( cutscene_info* info, 
                                          f32 message_lifetime, 
                                          f32 message_x,
                                          f32 message_y,
                                          char* message_string ){
    cutscene_action* current_action = &info->actions[info->action_count++];
    current_action->type = CUTSCENE_ACTION_SHOW_FLOATING_MESSAGE;

    cutscene_action_show_floating_message* floating_message_action =
        &current_action->show_floating_message;

    floating_message_action->message_x = message_x;
    floating_message_action->message_y = message_y;
    floating_message_action->message_lifetime = message_lifetime;
    floating_message_action->message_string = message_string;
}

void cutscene_info_push_camera_transition( cutscene_info* info, f32 offset_x, f32 offset_y ){
    cutscene_action* current_action = &info->actions[info->action_count++];
    current_action->type = CUTSCENE_ACTION_CAMERA_TRANSITION;

    cutscene_action_camera_transition* camera_transition = 
        &current_action->camera_transition;

    camera_transition->to.x = -offset_x;
    camera_transition->to.y = -offset_y;

    camera_transition->relative_end_position = true;
}

void cutscene_info_push_move_actor( cutscene_info* info, 
                                    actor* target, 
                                    f32 offset_x,
                                    f32 offset_y ){
    cutscene_action* current_action = &info->actions[info->action_count++];
    current_action->type = CUTSCENE_ACTION_ACTOR_ACTION;

    current_action->actor_action.type = CUTSCENE_ACTOR_ACTION_MOVE;

    cutscene_action_actor_move* actor_move =
        &current_action->actor_action.move;

    current_action->actor_action.target = target;

    actor_move->end_x = offset_x;
    actor_move->end_y = offset_y;

    actor_move->relative_end_position = true;
}

void cutscene_info_wait_for_current_event_to_finish( cutscene_info* info ){
    info->actions[ info->action_count-1 ].wait_for_finish = true;
    info->actions[ info->action_count-1 ].wait_time = 0.f;
}

void cutscene_info_wait_time( cutscene_info* info, f32 time_to_wait ){
    info->actions[ info->action_count-1 ].wait_for_finish = false;
    info->actions[ info->action_count-1 ].wait_time = time_to_wait;
}

void cutscene_info_run( cutscene_info* info, game_state* state, f32 delta_time ){
    /*do cutscene setup.*/
    if(!info->running){
        fprintf(stderr, "cutscene setup.\n");
        info->running = true;
        info->return_to_original_camera_pos = false;

        info->before.x = state->camera.x;
        info->before.y = state->camera.y;
        info->before.scale = state->camera.scale;

        info->current_action = 0;
        info->scene_time = 0.f;

        for(unsigned action_index = 0;
            action_index < info->action_count;
            ++action_index){
            info->actions[action_index].first_frame = true;
            fprintf(stderr, "toggled first frame flag.\n");
        }
    }

    // update and actually do cutscene stuff here
    {
        cutscene_action* current_action = &info->actions[ info->current_action ];

        switch( current_action->type ){
            case CUTSCENE_ACTION_NONE:
            {
                info->current_action++;
                fprintf(stderr, "null action, advancing\n");
            }
            break;
            // All actor actions 
            // call an invocation of actor_think to determine
            // when they should stop.
            //
            // This will end up overriding the previous think state,
            // so be wary!
            case CUTSCENE_ACTION_ACTOR_ACTION:
            {
                cutscene_action_actor_action* actor_action = &current_action->actor_action;
                actor* target = actor_action->target;

                switch( actor_action->type ){
                    case CUTSCENE_ACTOR_ACTION_MOVE:
                    {
                        cutscene_action_actor_move* actor_move =
                            &actor_action->move;

                        if( current_action->first_frame ){
                            fprintf(stderr, "actor movement init\n");

                            current_action->first_frame = false;

                            actor_move->start_x = target->position.x;
                            actor_move->start_y = target->position.y;

                            if( actor_move->relative_end_position ){
                                actor_move->end_x += actor_move->start_x;
                                actor_move->end_y += actor_move->start_y;

                                fprintf(stderr, "Relative actor movement:\n");

                                fprintf(stderr, "\tactor_move->start_x : %3.3f\n", actor_move->start_x);
                                fprintf(stderr, "\tactor_move->start_y : %3.3f\n", actor_move->start_y);
                                fprintf(stderr, "\tactor_move->end_x : %3.3f\n", actor_move->end_x);
                                fprintf(stderr, "\tactor_move->end_y : %3.3f\n", actor_move->end_y);
                            }

                            /* actor_move_to( target, actor_move->end_x, actor_move->end_y ); */
                        }
                    }
                    break;
                }

                // I suppose the thing is I don't really want the
                // actor to "think" do I?
                b32 finished = true;//actor_run_action( target, state, delta_time );

                if( current_action->wait_for_finish ){
                    if( finished ){
                        info->current_action++;
                        fprintf(stderr, "finished actor thinking, waited finished.\n");
                    }else{
                        fprintf(stderr, "waiting for actor think to finish.\n");
                    }
                }else{
                    if( current_action->wait_time > 0.f ){
                        current_action->wait_time -= delta_time;
                    }else{
                        info->current_action++;
                    }
                }
            }
            break;
            case CUTSCENE_ACTION_CAMERA_TRANSITION:
            {
                cutscene_action_camera_transition* camera_transition =
                    &current_action->camera_transition;

                if( current_action->first_frame ){
                    current_action->first_frame = false;
                    camera_transition->from = state->camera;

                    if( camera_transition->relative_end_position ){
                        camera_transition->to.x += camera_transition->from.x;
                        camera_transition->to.y += camera_transition->from.y;
                        camera_transition->to.scale += camera_transition->from.scale;
                    }

                    camera_transition_move_to( &state->transition, 
                            (vec2){camera_transition->from.x, camera_transition->from.y}, 
                            (vec2){camera_transition->to.x, camera_transition->to.y} );
                }

                // actual frame work.
                {
                    // because my camera movement is
                    // lerping I just query the game state's
                    // transition timer for finishing.
                    b32 finished_transition =
                        (state->transition.time >= 1.f);

                    if( current_action->wait_for_finish ){
                        if( finished_transition ){
                            info->current_action++;
                            fprintf(stderr, "wait for camera_transition finished\n");
                        }
                    }else{
                        if( current_action->wait_time > 0.f ){
                            current_action->wait_time -= delta_time;
                        }else{
                            info->current_action++;
                            fprintf(stderr, "wait time for camera_transition finished\n");
                        }
                    }
                }
            }
            break;
            case CUTSCENE_ACTION_SHOW_FLOATING_MESSAGE:
            {
                // TODO(jerry): Track the message I'm showing!!!
                if( current_action->first_frame ){
                    current_action->first_frame = false;

                    cutscene_action_show_floating_message* floating_message =
                        &current_action->show_floating_message;

                    floating_messages_push_message( 
                            &state->floating_messages, 
                            floating_message->message_string,
                            v2(floating_message->message_x, floating_message->message_y),
                            (colorf){1, 1, 1, 1},
                            floating_message->message_lifetime);
                }
                
                if( current_action->wait_for_finish ){
                    info->current_action++;
                }else{
                    if( current_action->wait_time > 0.f ){
                        current_action->wait_time -= delta_time;
                    }else{
                        info->current_action++;
                    }
                }
            }
            break;
            default:
            {
                // should probably not do anything?
            }
            break;
        }

        info->scene_time -= delta_time;
    }

    bool finished_cutscene = false;

    if( !info->timed ){
        if( info->current_action >= info->action_count ){
            finished_cutscene = true;
        }
    }else{
        if( info->scene_time <= 0.f ){
            finished_cutscene = true;
        }
    }

    if(finished_cutscene){
        info->running = false;
        fprintf(stderr, "\tinfo->current_action = %d\n", info->current_action);
        fprintf(stderr, "\tinfo->action_count = %d\n", info->action_count);
        fprintf(stderr, "cutscene done!\n");
        //return to original camera state prior to cutscene start.
        {
            if( !info->return_to_original_camera_pos ){
                info->return_to_original_camera_pos = true;
                camera_transition_move_to( &state->transition,  
                        (vec2){ state->camera.x, state->camera.y },
                        (vec2){ info->before.x, info->before.y } );
            }
        }
    }
}
/*Cutscene System*/
