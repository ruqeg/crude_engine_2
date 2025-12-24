#include <engine/platform/platform.h>

void
crude_platform_key_down
(
  _In_ crude_key_state *key
)
{
  if (key->state)
  {
    key->pressed = false;
  }
  else
  {
    key->pressed = true;
  }
  
  key->state   = true;
  key->current = true;
}

void
crude_platform_key_up
(
  _In_ crude_key_state *key
)
{
  key->current = false;
}

void
crude_platform_key_reset
(
  _In_ crude_key_state *key
)
{
  if ( !key->current )
  {
    key->state   = 0;
    key->pressed = 0;
  }
  else if ( key->state )
  {
    key->pressed = 0;
  }
}

void
crude_platform_mouse_reset
(
  _In_ crude_mouse_state *state
)
{
  state->rel.x = 0;
  state->rel.y = 0;
  
  state->scroll.x = 0;
  state->scroll.y = 0;
  
  state->view.x = 0;
  state->view.y = 0;
}