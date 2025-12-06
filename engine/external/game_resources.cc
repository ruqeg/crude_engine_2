#include <engine/core/assert.h>

#include <engine/external/game_resources.h>

char const*
crude_game_item_to_string
(
  _In_ crude_game_item                                     game_item
)
{
  switch ( game_item )
  {
  case CRUDE_GAME_ITEM_NONE: return "Item Nonde";
  case CRUDE_GAME_ITEM_SERUM: return "Item Serum";
  case CRUDE_GAME_ITEM_SYRINGE_DRUG: return "Syringe Drug";
  case CRUDE_GAME_ITEM_SYRINGE_HEALTH: return "Syringe Health";
  case CRUDE_GAME_ITEM_AMMUNITION: return "Item Ammuntion";
  }
  CRUDE_ASSERT( false );
  return "Item Unknown";
}