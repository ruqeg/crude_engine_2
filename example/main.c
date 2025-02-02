#include <Windows.h>
#include <core/memory.h>

int APIENTRY wWinMain(_In_ HINSTANCE      hInstance,
                      _In_opt_ HINSTANCE  hPrevInstance,
                      _In_ LPWSTR         lpCmdLine,
                      _In_ int            nCmdShow)
{
  crude_heap heap;
  crude_heap_initialize( &heap, 32 * 1024 * 1024 );
  crude_heap_deinitialize( &heap );
  return EXIT_SUCCESS;
}