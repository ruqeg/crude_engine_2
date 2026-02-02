#include <engine/gui/hardcoded_icon.h>

void
crude_gui_queue_render_hardcoded_icon_im
(
  _In_ ImDrawList                                         *draw_list,
  _In_ ImVec2 const                                       *a,
  _In_ ImVec2 const                                       *b,
  _In_ crude_gui_hardcoded_icon_type                       type,
  _In_ bool                                                filled,
  _In_ ImU32                                               color,
  _In_ ImU32                                               inner_color
)
{
  ImRect                                                   rect;
  ImVec2                                                   rect_center;
  float32                                                  outline_scale, rect_center_x, rect_center_y, rect_x, rect_y, rect_w, rect_h;
  int32                                                    extra_segments;

  rect = CRUDE_COMPOUNT( ImRect, { *a, *b } );
  rect_x = rect.Min.x;
  rect_y = rect.Min.y;
  rect_w = rect.Max.x - rect.Min.x;
  rect_h = rect.Max.y - rect.Min.y;
  rect_center_x = (rect.Min.x + rect.Max.x) * 0.5f;
  rect_center_y = (rect.Min.y + rect.Max.y) * 0.5f;
  rect_center = CRUDE_COMPOUNT( ImVec2, { rect_center_x, rect_center_y } );
  outline_scale  = rect_w / 24.0f;
  extra_segments = CRUDE_CAST( int32, 2.f * outline_scale ); // for full circle

  if ( type == CRUDE_GUI_HARDCODED_ICON_TYPE_FLOW )
  {
    ImRect                                                 canvas;
    ImVec2                                                 tip_top, tip_right, tip_bottom;
    float32                                                canvas_x,canvas_y, canvas_w, canvas_h;
    float32                                                offset_x, offset_y, margin, rounding, origin_scale, tip_round;
    float32                                                left, right, top, bottom, center_y;

    origin_scale = rect_w / 24.0f;
  
    offset_x = 1.0f * origin_scale;
    offset_y = 0.0f * origin_scale;
    margin = (filled ? 2.0f : 2.0f) * origin_scale;
    rounding = 0.1f * origin_scale;
    tip_round = 0.7f;
     
    canvas = ImRect(
      rect.Min.x + margin + offset_x,
      rect.Min.y + margin + offset_y,
      rect.Max.x - margin + offset_x,
      rect.Max.y - margin + offset_y);

    canvas_x = canvas.Min.x;
    canvas_w = canvas.Max.x - canvas.Min.x;
    canvas_y = canvas.Min.y;
    canvas_h = canvas.Max.y - canvas.Min.y;
  
    left = canvas_x + canvas_w            * 0.5f * 0.3f;
    right = canvas_x + canvas_w - canvas_w * 0.5f * 0.3f;
    top = canvas_y + canvas_h            * 0.5f * 0.2f;
    bottom = canvas_y + canvas_h - canvas_h * 0.5f * 0.2f;
    center_y = (top + bottom) * 0.5f;
  
    tip_top = CRUDE_COMPOUNT( ImVec2, { canvas_x + canvas_w * 0.5f, top } );
    tip_right  = CRUDE_COMPOUNT( ImVec2, { right, center_y } );
    tip_bottom = CRUDE_COMPOUNT( ImVec2, {canvas_x + canvas_w * 0.5f, bottom } );
  
    draw_list->PathLineTo(ImVec2(left, top) + ImVec2(0, rounding));
    draw_list->PathBezierCubicCurveTo( ImVec2(left, top), ImVec2(left, top), ImVec2(left, top) + ImVec2(rounding, 0));
    draw_list->PathLineTo(tip_top);
    draw_list->PathLineTo(tip_top + (tip_right - tip_top) * tip_round);
    draw_list->PathBezierCubicCurveTo( tip_right, tip_right, tip_bottom + (tip_right - tip_bottom) * tip_round );
    draw_list->PathLineTo(tip_bottom);
    draw_list->PathLineTo(ImVec2(left, bottom) + ImVec2(rounding, 0));
    draw_list->PathBezierCubicCurveTo( ImVec2(left, bottom), ImVec2(left, bottom), ImVec2(left, bottom) - ImVec2(0, rounding));
  
    if ( !filled )
    {
      if ( inner_color & 0xFF000000 )
      {
        draw_list->AddConvexPolyFilled( draw_list->_Path.Data, draw_list->_Path.Size, inner_color );
      }
      
      draw_list->PathStroke( color, true, 2.0f * outline_scale );
    }
    else
    {
      draw_list->PathFillConvex( color );
    }
  }
  else
  {
    float32                                                triangle_start;
    int32                                                  rect_offset;

    triangle_start = rect_center_x + 0.32f * rect_w;
    rect_offset = -CRUDE_CAST( int32, rect_w * 0.25f * 0.25f );

    rect.Min.x += rect_offset;
    rect.Max.x += rect_offset;
    rect_x += rect_offset;
    rect_center_x += rect_offset * 0.5f;
    rect_center.x += rect_offset * 0.5f;

    if ( type == CRUDE_GUI_HARDCODED_ICON_TYPE_CIRCLE )
    {
      ImVec2 c = rect_center;

      if ( !filled )
      {
        float32 r = 0.5f * rect_w / 2.0f - 0.5f;
        if ( inner_color & 0xFF000000 )
        {
          draw_list->AddCircleFilled( c, r, inner_color, 12 + extra_segments );
        }
        draw_list->AddCircle( c, r, color, 12 + extra_segments, 2.0f * outline_scale );
      }
      else
      {
        draw_list->AddCircleFilled( c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments );
      }
    }

    if ( type == CRUDE_GUI_HARDCODED_ICON_TYPE_SQUARE )
    {
      if ( filled )
      {
        float32 r  = 0.5f * rect_w / 2.0f;
        ImVec2 p0 = rect_center - ImVec2(r, r);
        ImVec2 p1 = rect_center + ImVec2(r, r);

        draw_list->AddRectFilled( p0, p1, color, 0, ImDrawFlags_RoundCornersAll );
      }
      else
      {
        float32 r = 0.5f * rect_w / 2.0f - 0.5f;
        ImVec2 p0 = rect_center - ImVec2(r, r);
        ImVec2 p1 = rect_center + ImVec2(r, r);

        if ( inner_color & 0xFF000000 )
        {
          draw_list->AddRectFilled( p0, p1, inner_color, 0, ImDrawFlags_RoundCornersAll );
        }

        draw_list->AddRect(p0, p1, color, 0, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale);
      }
    }

    if ( type == CRUDE_GUI_HARDCODED_ICON_TYPE_GRID )
    {
      float32 r = 0.5f * rect_w / 2.0f;
      float32 w = ceilf(r / 3.0f);

      ImVec2 base_tl = ImVec2( floorf( rect_center_x - w * 2.5f ), floorf( rect_center_y - w * 2.5f ) );
      ImVec2 base_br = ImVec2( floorf( base_tl.x + w ), floorf( base_tl.y + w ) );

      ImVec2 tl = base_tl;
      ImVec2 br = base_br;
      for ( int32 i = 0; i < 3; ++i )
      {
        tl.x = base_tl.x;
        br.x = base_br.x;
        draw_list->AddRectFilled( tl, br, color );
        tl.x += w * 2;
        br.x += w * 2;
        if ( i != 1 || filled )
        {
          draw_list->AddRectFilled( tl, br, color );
        }
        tl.x += w * 2;
        br.x += w * 2;
        draw_list->AddRectFilled( tl, br, color );

        tl.y += w * 2;
        br.y += w * 2;
      }

      triangle_start = br.x + w + 1.0f / 24.0f * rect_w;
    }

    if ( type == CRUDE_GUI_HARDCODED_ICON_TYPE_ROUND_SQUARE )
    {
      if ( filled )
      {
        float32 r  = 0.5f * rect_w / 2.0f;
        float32 cr = r * 0.5f;
        ImVec2 p0 = rect_center - ImVec2( r, r );
        ImVec2 p1 = rect_center + ImVec2( r, r );
        
        draw_list->AddRectFilled( p0, p1, color, cr, ImDrawFlags_RoundCornersAll );
      }
      else
      {
        float32 r = 0.5f * rect_w / 2.0f - 0.5f;
        float32 cr = r * 0.5f;
        ImVec2 p0 = rect_center - ImVec2(r, r);
        ImVec2 p1 = rect_center + ImVec2(r, r);

        if ( inner_color & 0xFF000000 )
        {
          draw_list->AddRectFilled( p0, p1, inner_color, cr, ImDrawFlags_RoundCornersAll );
        }

        draw_list->AddRect( p0, p1, color, cr, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale );
      }
    }
    else if ( type == CRUDE_GUI_HARDCODED_ICON_TYPE_DIAMOND )
    {
      if ( filled )
      {
        float32 r = 0.607f * rect_w / 2.0f;
        ImVec2 c = rect_center;

        draw_list->PathLineTo(c + ImVec2( 0, -r));
        draw_list->PathLineTo(c + ImVec2( r,  0));
        draw_list->PathLineTo(c + ImVec2( 0,  r));
        draw_list->PathLineTo(c + ImVec2(-r,  0));
        draw_list->PathFillConvex(color);
      }
      else
      {
        float32 r = 0.607f * rect_w / 2.0f - 0.5f;
        ImVec2 c = rect_center;

        draw_list->PathLineTo(c + ImVec2( 0, -r));
        draw_list->PathLineTo(c + ImVec2( r,  0));
        draw_list->PathLineTo(c + ImVec2( 0,  r));
        draw_list->PathLineTo(c + ImVec2(-r,  0));

        if ( inner_color & 0xFF000000 )
        {
          draw_list->AddConvexPolyFilled( draw_list->_Path.Data, draw_list->_Path.Size, inner_color );
        }

        draw_list->PathStroke( color, true, 2.0f * outline_scale );
      }
    }
    else
    {
      float32 triangle_tip = triangle_start + rect_w * (0.45f - 0.32f);

      draw_list->AddTriangleFilled(
        ImVec2( ceilf( triangle_tip ), rect_y + rect_h * 0.5f ),
        ImVec2( triangle_start, rect_center_y + 0.15f * rect_h ),
        ImVec2( triangle_start, rect_center_y - 0.15f * rect_h ),
        color);
    }
  }
}


void
crude_gui_queue_render_icon
(
  _In_ XMFLOAT2                                            size,
  _In_ crude_gui_hardcoded_icon_type                       type,
  _In_ bool                                                filled,
  _In_ XMFLOAT4                                            color,
  _In_ XMFLOAT4                                            inner_color
)
{
  ImVec2 im_size = CRUDE_COMPOUNT( ImVec2, { size.x, size.y } );

  if ( ImGui::IsRectVisible( im_size ) )
  {
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    ImVec2 cursor_end = cursor_pos + im_size;
    ImDrawList *draw_list  = ImGui::GetWindowDrawList();
    crude_gui_queue_render_hardcoded_icon_im( draw_list, &cursor_pos, &cursor_end, type, filled,
      CRUDE_COMPOUNT( ImColor, { color.x, color.y, color.z, color.w } ),
      CRUDE_COMPOUNT( ImColor, { inner_color.x, inner_color.y, inner_color.z, inner_color.w } ) );
  }

  ImGui::Dummy( im_size);
}

