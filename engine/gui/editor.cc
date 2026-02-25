#include <nfd.h>

#include <engine/engine.h>

#include <engine/gui/editor.h>

nfdu8filteritem_t                                          crude_gui_editor_node_file_filters_[ ] = 
{ 
  CRUDE_COMPOUNT( nfdu8filteritem_t, { "Crude Node", "crude_node" } )
};


static void
crude_gui_editor_push_style_
(
);

static void
crude_gui_editor_pop_style_
(
);

void
crude_gui_editor_initialize
(
  _In_ crude_gui_editor                                   *editor,
  _In_ crude_engine                                       *engine
)
{
  editor->engine = engine;
  editor->selected_node = engine->main_node;
  crude_gui_viewport_initialize( &editor->viewport, &engine->gpu, crude_gfx_access_texture( &engine->gpu, crude_gfx_render_graph_builder_access_resource_by_name( engine->scene_renderer.render_graph->builder, "game_final" )->resource_info.texture.handle )->handle, 0 );
  crude_gui_node_inspector_initialize( &editor->node_inspector, &engine->components_serialization_manager, &engine->node_manager );
  crude_gui_node_tree_initialize( &editor->node_tree );
  crude_gui_log_viewer_initialize( &editor->log_viewer );
  crude_gui_content_browser_initialize( &editor->content_browser, engine->environment.directories.resources_absolute_directory, &engine->develop_temporary_allocator );
}

void
crude_gui_editor_deinitialize
(
  _In_ crude_gui_editor                                   *editor
)
{
  crude_gui_node_tree_deinitialize( &editor->node_tree );
  crude_gui_node_inspector_deinitialize( &editor->node_inspector );
  crude_gui_log_viewer_deinitialize( &editor->log_viewer );
  crude_gui_content_browser_deinitialize( &editor->content_browser );
}

void
crude_gui_editor_queue_draw
(
  _In_ crude_gui_editor                                   *editor
)
{
  ImGuiViewport                                           *im_viewport;
  ImGuiID                                                  im_dockspace_id;
  
  crude_gui_editor_push_style_( );

  im_dockspace_id = ImGui::GetID( "Crude Editor" );
  im_viewport = ImGui::GetMainViewport( );
  
  if ( ImGui::DockBuilderGetNode( im_dockspace_id ) == NULL )
  {
    ImGuiID                                                im_dock_id_node_tree, im_dock_id_viewport, im_dock_id_inspector, im_dock_id_logger;

    ImGui::DockBuilderAddNode( im_dockspace_id, ImGuiDockNodeFlags_None );
    ImGui::DockBuilderSetNodeSize( im_dockspace_id, im_viewport->Size );
    
    ImGui::DockBuilderSplitNode( im_dockspace_id, ImGuiDir_Up, 0.75f, &im_dock_id_viewport, &im_dock_id_logger );

    ImGui::DockBuilderSplitNode( im_dock_id_viewport, ImGuiDir_Left, 0.20f, &im_dock_id_node_tree, &im_dock_id_viewport );
    ImGui::DockBuilderSplitNode( im_dock_id_node_tree, ImGuiDir_Up, 0.50f, &im_dock_id_node_tree, &im_dock_id_inspector );
    
    ImGui::DockBuilderDockWindow( "Viewport", im_dock_id_viewport );
    ImGui::DockBuilderDockWindow( "Inspector", im_dock_id_inspector );
    ImGui::DockBuilderDockWindow( "Node Tree", im_dock_id_node_tree );
    ImGui::DockBuilderDockWindow( "Content Browser", im_dock_id_logger );
    ImGui::DockBuilderDockWindow( "Log Viewer", im_dock_id_logger );
    ImGui::DockBuilderFinish( im_dockspace_id );
  }
  
  ImGui::DockSpaceOverViewport( im_dockspace_id, im_viewport, ImGuiDockNodeFlags_PassthruCentralNode );
  
  if ( ImGui::BeginMainMenuBar( ) )
  {
    bool                                                   file_filepath_error;

    file_filepath_error = false;

    if ( ImGui::BeginMenu( "File" ) )
    {
      if ( ImGui::MenuItem( "Open Node" ) )
      {
        nfdu8char_t                                       *nfd_out_path;
        nfdopendialogu8args_t                              nfd_args;
        nfdresult_t                                        nfd_result;

        nfd_args = CRUDE_COMPOUNT_EMPTY( nfdopendialogu8args_t );
        nfd_args.filterList = crude_gui_editor_node_file_filters_;
        nfd_args.filterCount = CRUDE_COUNTOF( crude_gui_editor_node_file_filters_ );
        
        nfd_result = NFD_OpenDialogU8_With( &nfd_out_path, &nfd_args );
        if ( nfd_result == NFD_OKAY )
        {
          char                                            *relative_filepath_candidate;
          char                                             selected_relative_filepath[ CRUDE_NODE_RELATIVE_FILEPATH_LENGTH_MAX ];

          relative_filepath_candidate = strstr( nfd_out_path, "\\resources\\" );
          if ( relative_filepath_candidate )
          {
            crude_snprintf( selected_relative_filepath, sizeof( selected_relative_filepath ), "%s", relative_filepath_candidate + crude_string_length( "\\resources\\" ) );
            NFD_FreePathU8( nfd_out_path );
            
            crude_engine_commands_manager_push_load_node_command( &editor->engine->commands_manager, selected_relative_filepath );
          }
          else
          {
            file_filepath_error = true;
          }
        }
      }
      if ( ImGui::MenuItem( "Save Node" ) )
      {
        nfdu8char_t                                       *nfd_out_path;
        nfdsavedialogu8args_t                              nfd_args;
        nfdresult_t                                        nfd_result;

        nfd_args = CRUDE_COMPOUNT_EMPTY( nfdsavedialogu8args_t );
        nfd_args.filterList = crude_gui_editor_node_file_filters_;
        nfd_args.filterCount = CRUDE_COUNTOF( crude_gui_editor_node_file_filters_ );
        
        nfd_result = NFD_SaveDialogU8_With( &nfd_out_path, &nfd_args );
        if ( nfd_result == NFD_OKAY )
        {
          char                                            *relative_filepath_candidate;
          char                                             selected_relative_filepath[ CRUDE_NODE_RELATIVE_FILEPATH_LENGTH_MAX ];

          relative_filepath_candidate = strstr( nfd_out_path, "\\resources\\" );
          if ( relative_filepath_candidate )
          {
            crude_snprintf( selected_relative_filepath, sizeof( selected_relative_filepath ), "%s", relative_filepath_candidate + crude_string_length( "\\resources\\" ) );
            NFD_FreePathU8( nfd_out_path );

            crude_node_manager_save_node_to_file( &editor->engine->node_manager, editor->engine->world, editor->engine->main_node, selected_relative_filepath );
          }
          else
          {
            file_filepath_error = true;
          }
        }
      }
      ImGui::EndMenu( );
    }

    if ( file_filepath_error )
    {
      ImGui::OpenPopup( "Node Filepath Error" );
    }

    if ( ImGui::BeginPopup( "Node Filepath Error" ) )
    {
      ImGui::Text( "Fuckind idiot, node filepath has to be inside resources directory!" );
      ImGui::Text( "Resources Directory \"%s\"!", editor->engine->environment.directories.resources_absolute_directory );
      ImGui::EndPopup();
    }

    ImGui::EndMainMenuBar( );
  }

  ImGui::PushStyleColor( ImGuiCol_WindowBg, CRUDE_COMPOUNT( ImVec4, { 0.25f * 30 / 255.f, 0.25f *23 / 255.f, 0.25f *12 / 255.f, 1.00f } ) );
  ImGui::Begin( "Viewport" );
  crude_gui_viewport_queue_draw( &editor->viewport, editor->engine->world, editor->selected_node, editor->engine->camera_node );
  ImGui::End( );
  ImGui::PopStyleColor( );
  
  ImGui::Begin( "Inspector" );
  crude_gui_node_inspector_queue_draw( &editor->node_inspector, editor->engine->world, editor->selected_node );
  ImGui::End( );
  
  ImGui::Begin( "Node Tree" );
  crude_gui_node_tree_queue_draw( &editor->node_tree, editor->engine->world, editor->engine->main_node, &editor->selected_node );
  ImGui::End( );
  
  ImGui::Begin( "Log Viewer" );
  crude_gui_log_viewer_queue_draw( &editor->log_viewer );
  ImGui::End( );
  
  ImGui::Begin( "Content Browser" );
  crude_gui_content_browser_queue_draw( &editor->content_browser );
  ImGui::End( );
  
  crude_gui_editor_pop_style_( );
    ImGui::ShowDemoWindow();
}

void
crude_gui_editor_update
(
  _In_ crude_gui_editor                                   *editor
)
{
}

void
crude_gui_editor_handle_input
(
  _In_ crude_gui_editor                                   *editor
)
{
}

void
crude_gui_editor_push_style_
(
)
{
  ImVec4                                                   dark_brown, lighter_brown, accent_brown;
  ImVec4                                                   selection_brown, hover_brown, active_brown;
  ImVec4                                                   title_brown, menu_brown, button_brown;
  ImVec4                                                   badge_brown;
  ImVec4                                                   text_primary, text_comment, text_keyword;
  ImVec4                                                   text_string, text_number, text_function;
  ImVec4                                                   text_class, text_variable;
  ImVec4                                                   error_bg, error_border, warning_bg;
  ImVec4                                                   info_border;
  
  dark_brown = CRUDE_COMPOUNT( ImVec4, { 30 / 255.f, 23 / 255.f, 12 / 255.f, 1.00f } );
  lighter_brown = CRUDE_COMPOUNT( ImVec4, { 0.32f, 0.28f, 0.24f, 1.00f } );
  accent_brown = CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 1.00f } );
  selection_brown = CRUDE_COMPOUNT( ImVec4, { 0.52f, 0.38f, 0.24f, 0.67f } );
  hover_brown = CRUDE_COMPOUNT( ImVec4, { 0.49f, 0.31f, 0.13f, 0.40f } );
  active_brown = CRUDE_COMPOUNT( ImVec4, { 0.49f, 0.31f, 0.13f, 1.00f } );
  title_brown = CRUDE_COMPOUNT( ImVec4, { 0.26f, 0.22f, 0.14f, 1.00f } );
  menu_brown = CRUDE_COMPOUNT( ImVec4, { 0.21f, 0.15f, 0.07f, 1.00f } );
  button_brown = CRUDE_COMPOUNT( ImVec4, { 0.43f, 0.35f, 0.23f, 1.00f } );
  badge_brown = CRUDE_COMPOUNT( ImVec4, { 0.50f, 0.36f, 0.22f, 1.00f } );
  
  text_primary = CRUDE_COMPOUNT( ImVec4, { 204 / 255.f, 204 / 255.f, 204 / 255.f, 1.00f } );
  text_comment = CRUDE_COMPOUNT( ImVec4, { 0.65f, 0.48f, 0.30f, 1.00f } );
  text_keyword = CRUDE_COMPOUNT( ImVec4, { 0.60f, 0.40f, 0.42f, 1.00f } );
  text_string = CRUDE_COMPOUNT( ImVec4, { 0.54f, 0.61f, 0.29f, 1.00f } );
  text_number = CRUDE_COMPOUNT( ImVec4, { 0.97f, 0.60f, 0.20f, 1.00f } );
  text_function = CRUDE_COMPOUNT( ImVec4, { 0.54f, 0.69f, 0.69f, 1.00f } );
  text_class = CRUDE_COMPOUNT( ImVec4, { 0.94f, 0.39f, 0.19f, 1.00f } );
  text_variable = CRUDE_COMPOUNT( ImVec4, { 0.86f, 0.22f, 0.35f, 1.00f } );
  
  error_bg = CRUDE_COMPOUNT( ImVec4, { 0.37f, 0.05f, 0.05f, 1.00f } );
  error_border = CRUDE_COMPOUNT( ImVec4, { 0.62f, 0.18f, 0.14f, 1.00f } );
  warning_bg = CRUDE_COMPOUNT( ImVec4, { 0.32f, 0.28f, 0.24f, 1.00f } );
  info_border = CRUDE_COMPOUNT( ImVec4, { 0.11f, 0.38f, 0.65f, 1.00f } );
  
  ImGui::PushStyleColor( ImGuiCol_Text, text_primary );
  ImGui::PushStyleColor( ImGuiCol_TextDisabled, CRUDE_COMPOUNT( ImVec4, { 0.65f, 0.65f, 0.65f, 1.00f } ) );
  
  ImGui::PushStyleColor( ImGuiCol_WindowBg, dark_brown );
  ImGui::PushStyleColor( ImGuiCol_ChildBg, dark_brown );
  ImGui::PushStyleColor( ImGuiCol_PopupBg, CRUDE_COMPOUNT( ImVec4, { 0.13f, 0.10f, 0.06f, 0.94f } ) );
  
  ImGui::PushStyleColor( ImGuiCol_Border, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.50f } ) );
  ImGui::PushStyleColor( ImGuiCol_BorderShadow, CRUDE_COMPOUNT( ImVec4, { 0.00f, 0.00f, 0.00f, 0.00f } ) );
  
  ImGui::PushStyleColor( ImGuiCol_FrameBg, lighter_brown );
  ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, hover_brown );
  ImGui::PushStyleColor( ImGuiCol_FrameBgActive, active_brown );
  
  ImGui::PushStyleColor( ImGuiCol_TitleBg, title_brown );
  ImGui::PushStyleColor( ImGuiCol_TitleBgActive, title_brown );
  ImGui::PushStyleColor( ImGuiCol_TitleBgCollapsed, CRUDE_COMPOUNT( ImVec4, { 0.13f, 0.10f, 0.06f, 0.75f } ) );
  
  ImGui::PushStyleColor( ImGuiCol_MenuBarBg, menu_brown );
  
  ImGui::PushStyleColor( ImGuiCol_ScrollbarBg, CRUDE_COMPOUNT( ImVec4, { 0.13f, 0.10f, 0.06f, 0.60f } ) );
  ImGui::PushStyleColor( ImGuiCol_ScrollbarGrab, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.50f } ) );
  ImGui::PushStyleColor( ImGuiCol_ScrollbarGrabHovered, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.70f } ) );
  ImGui::PushStyleColor( ImGuiCol_ScrollbarGrabActive, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.90f } ) );
  
  ImGui::PushStyleColor( ImGuiCol_CheckMark, text_function );
  ImGui::PushStyleColor( ImGuiCol_SliderGrab, accent_brown );
  ImGui::PushStyleColor( ImGuiCol_SliderGrabActive, text_function );
  
  ImGui::PushStyleColor( ImGuiCol_Button, button_brown );
  ImGui::PushStyleColor( ImGuiCol_ButtonHovered, badge_brown );
  ImGui::PushStyleColor( ImGuiCol_ButtonActive, CRUDE_COMPOUNT( ImVec4, { 0.55f, 0.41f, 0.26f, 1.00f } ) );
  
  ImGui::PushStyleColor( ImGuiCol_Header, selection_brown );
  ImGui::PushStyleColor( ImGuiCol_HeaderHovered, hover_brown );
  ImGui::PushStyleColor( ImGuiCol_HeaderActive, active_brown );
  
  ImGui::PushStyleColor( ImGuiCol_Separator, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.50f } ) );
  ImGui::PushStyleColor( ImGuiCol_SeparatorHovered, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.70f } ) );
  ImGui::PushStyleColor( ImGuiCol_SeparatorActive, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.90f } ) );
  
  ImGui::PushStyleColor( ImGuiCol_ResizeGrip, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.30f } ) );
  ImGui::PushStyleColor( ImGuiCol_ResizeGripHovered, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.50f } ) );
  ImGui::PushStyleColor( ImGuiCol_ResizeGripActive, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.70f } ) );
  
  ImGui::PushStyleColor( ImGuiCol_Tab, title_brown );
  ImGui::PushStyleColor( ImGuiCol_TabHovered, hover_brown );
  ImGui::PushStyleColor( ImGuiCol_TabActive, title_brown );
  ImGui::PushStyleColor( ImGuiCol_TabUnfocused, title_brown );
  ImGui::PushStyleColor( ImGuiCol_TabUnfocusedActive, title_brown );
  ImGui::PushStyleColor( ImGuiCol_TabSelected, title_brown );
  ImGui::PushStyleColor( ImGuiCol_TabSelectedOverline, title_brown );
  
  ImGui::PushStyleColor( ImGuiCol_DockingPreview, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.70f } ) );
  ImGui::PushStyleColor( ImGuiCol_DockingEmptyBg, dark_brown );
  
  ImGui::PushStyleColor( ImGuiCol_PlotLines, text_comment );
  ImGui::PushStyleColor( ImGuiCol_PlotLinesHovered, text_variable );
  ImGui::PushStyleColor( ImGuiCol_PlotHistogram, text_string );
  ImGui::PushStyleColor( ImGuiCol_PlotHistogramHovered, text_function );
  
  ImGui::PushStyleColor( ImGuiCol_TableHeaderBg, menu_brown );
  ImGui::PushStyleColor( ImGuiCol_TableBorderStrong, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.50f } ) );
  ImGui::PushStyleColor( ImGuiCol_TableBorderLight, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.30f } ) );
  ImGui::PushStyleColor( ImGuiCol_TableRowBg, dark_brown );
  ImGui::PushStyleColor( ImGuiCol_TableRowBgAlt, menu_brown );
  
  ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, selection_brown );
  ImGui::PushStyleColor( ImGuiCol_DragDropTarget, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 0.90f } ) );
  
  ImGui::PushStyleColor( ImGuiCol_NavHighlight, CRUDE_COMPOUNT( ImVec4, { 0.44f, 0.37f, 0.31f, 1.00f } ) );
  ImGui::PushStyleColor( ImGuiCol_NavWindowingHighlight, CRUDE_COMPOUNT( ImVec4, { 1.00f, 1.00f, 1.00f, 0.70f } ) );
  ImGui::PushStyleColor( ImGuiCol_NavWindowingDimBg, CRUDE_COMPOUNT( ImVec4, { 0.80f, 0.80f, 0.80f, 0.20f } ) );
  ImGui::PushStyleColor( ImGuiCol_ModalWindowDimBg, CRUDE_COMPOUNT( ImVec4, { 0.13f, 0.10f, 0.06f, 0.75f } ) );
  
  ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 4.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 4.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 3.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_PopupRounding, 4.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_ScrollbarRounding, 9.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_GrabRounding, 3.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_TabRounding, 4.0f );
  
  ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8.0f, 8.0f ) );
  ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 5.0f, 3.0f ) );
  ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4.0f, 2.0f ) );
  ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 8.0f, 4.0f ) );
  ImGui::PushStyleVar( ImGuiStyleVar_ItemInnerSpacing, ImVec2( 4.0f, 4.0f ) );
  ImGui::PushStyleVar( ImGuiStyleVar_IndentSpacing, 21.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_ScrollbarSize, 14.0f );
  ImGui::PushStyleVar( ImGuiStyleVar_GrabMinSize, 10.0f );
}

void
crude_gui_editor_pop_style_
(
)
{
  ImGui::PopStyleVar( 15 );
  ImGui::PopStyleColor( 57 );
}