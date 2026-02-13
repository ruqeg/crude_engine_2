#include <engine/core/file.h>

#include <engine/gui/content_browser.h>

static void
crude_gui_content_browser_queue_draw_internal_
(
  _In_ crude_gui_content_browser                          *browser,
  _In_ char const                                         *relative_directory,
  _In_ char const                                         *absolute_directory,
  _In_ uint32                                             *current_node_index
);

void
crude_gui_content_browser_initialize
(
  _In_ crude_gui_content_browser                          *browser,
  _In_ char const                                         *resources_absolute_directory,
  _In_ crude_stack_allocator                              *temporary_allocator
)
{
  browser->resources_absolute_directory = resources_absolute_directory;
  browser->temporary_allocator = temporary_allocator;
}

void
crude_gui_content_browser_deinitialize
(
  _In_ crude_gui_content_browser                          *browser
)
{
}

void
crude_gui_content_browser_update
(
  _In_ crude_gui_content_browser                          *browser
)
{
}

void
crude_gui_content_browser_queue_draw
(
  _In_ crude_gui_content_browser                          *browser
)
{
  uint32                                                   current_node_index;

  current_node_index = 0;
  crude_gui_content_browser_queue_draw_internal_( browser, browser->resources_absolute_directory, browser->resources_absolute_directory, &current_node_index );
}

void
crude_gui_content_browser_queue_draw_internal_
(
  _In_ crude_gui_content_browser                          *browser,
  _In_ char const                                         *relative_directory,
  _In_ char const                                         *absolute_directory,
  _In_ uint32                                             *current_node_index
)
{
  char const                                              *filter;
  crude_string_buffer                                      temporary_string_buffer;
  crude_file_iterator                                      file_iterator;
  ImGuiTreeNodeFlags                                       tree_node_flags;
  uint64                                                   temporary_allocator_marker;
  
  temporary_allocator_marker = crude_stack_allocator_get_marker( browser->temporary_allocator );

  crude_string_buffer_initialize( &temporary_string_buffer, crude_string_length( absolute_directory ) + 16, crude_stack_allocator_pack( browser->temporary_allocator ) );
  filter = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s\\*.*", absolute_directory );

  tree_node_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DrawLinesFull;
  
  if ( ImGui::TreeNodeEx( ( void* )( intptr_t )*current_node_index, tree_node_flags, relative_directory ) )
  {
    if ( crude_file_iterator_initialize( &file_iterator, filter ) )
    {
      while ( true )
      {
        if ( crude_file_iterator_is_directory( &file_iterator ) )
        {
          char const                                        *child_relative_directory;
          char const                                        *child_absolute_directory;
          uint64                                             second_temporary_allocator_marker;
          crude_string_buffer                                directory_temporary_string_buffer;
  
          second_temporary_allocator_marker = crude_stack_allocator_get_marker( browser->temporary_allocator );

          child_relative_directory = crude_file_iterator_name( &file_iterator );
          
          if ( child_relative_directory != NULL && child_relative_directory[ 0 ] != 0 && child_relative_directory[ 0 ] != '.' )
          {
            crude_string_buffer_initialize( &temporary_string_buffer, crude_string_length( absolute_directory ) + crude_string_length( child_relative_directory ) + 16, crude_stack_allocator_pack( browser->temporary_allocator ) );
            child_absolute_directory = crude_string_buffer_append_use_f( &temporary_string_buffer, "%s\\%s", absolute_directory, child_relative_directory );

            crude_gui_content_browser_queue_draw_internal_( browser, child_relative_directory, child_absolute_directory, current_node_index );
          }
          crude_stack_allocator_free_marker( browser->temporary_allocator, second_temporary_allocator_marker  );
        }
        else
        {
          tree_node_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DrawLinesFull | ImGuiTreeNodeFlags_Leaf;

          if ( ImGui::TreeNodeEx( ( void* )( intptr_t )*current_node_index, tree_node_flags, crude_file_iterator_name( &file_iterator ) ) )
          {
            ImGui::TreePop( );
          }
        }
        
        ++( *current_node_index );

        if ( !crude_file_iterator_next( &file_iterator ) )
        {
          break;
        }
      }

      crude_file_iterator_deinitialize( &file_iterator );
    }
  
    ImGui::TreePop( );
  }
  
  *current_node_index += 1000;

cleanup_stack_allocator:
  crude_stack_allocator_free_marker( browser->temporary_allocator,temporary_allocator_marker  );
}