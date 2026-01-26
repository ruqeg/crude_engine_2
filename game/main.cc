#define CGLTF_IMPLEMENTATION
#include <thirdparty/cgltf/cgltf.h>

#define STB_SPRINTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <thirdparty/stb/stb_sprintf.h>
#include <thirdparty/stb/stb_image.h>

#define VMA_IMPLEMENTATION
#include <thirdparty/vma/include/vk_mem_alloc.h>

#include <engine/core/file.h>

#include <game/game.h>

int
main
(
)
{
  crude_engine                                             engine;

  /* Initialization */
  {
    char                                                   working_directory[ 4096 ];
    
    crude_get_current_working_directory( working_directory, sizeof( working_directory ) );
    crude_engine_initialize( &engine, working_directory );

    crude_editor_instance_intialize( );

    crude_editor_initialize( crude_editor_instance( ), &engine, working_directory );
  }

   while ( engine.running )
   {
     crude_engine_update( &engine );
     crude_editor_update( crude_editor_instance( ) );
   }
   
  /* Deinitialization */
  {
    crude_editor_deinitialize( crude_editor_instance( ) );
    crude_engine_deinitialize( &engine );
    crude_editor_instance_deintialize( );
  }

  return 0;
}
