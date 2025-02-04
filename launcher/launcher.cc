#define CR_HOST
#include <cr.h>
#include <Windows.h>
#include <engine.h>

int APIENTRY wWinMain(_In_ HINSTANCE      hInstance,
                      _In_opt_ HINSTANCE  hPrevInstance,
                      _In_ LPWSTR         lpCmdLine,
                      _In_ int            nCmdShow)
{
  struct cr_plugin editor;
  
  crude_engine engine = crude_engine_initialize( 1u );

  cr_plugin_open( editor, CR_PLUGIN( "crude_editor" ) );
  
  while ( engine.running )
  {
    cr_plat_init();
  }

  cr_plugin_close(editor);
  return EXIT_SUCCESS;
}
