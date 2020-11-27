// termmouse.c (part of mintty)
// Copyright 2008-12 Andy Koppe, 2017-20 Thomas Wolff
// Based on code from PuTTY-0.60 by Simon Tatham and team.
// Licensed under the terms of the GNU General Public License v3 or later.

#include "termpriv.h"
#include "win.h"
#include "child.h"
#include "charset.h"  // cs__utftowcs
#include "tek.h"


/*
 * Fetch the character at a particular position in a line array.
 * The reason this isn't just a simple array reference is that if the
 * character we find is UCSWIDE, then we must look one space further
 * to the left.
 */
static wchar
get_char(termline *line, int x)
{
  wchar c = line->chars[x].chr;
  if (c == UCSWIDE && x > 0)
    c = line->chars[x - 1].chr;
  return c;
}

static pos
sel_spread_word(pos p, bool forward)
{
  pos ret_p = p;
  termline *line = fetch_line(p.y);

  for (;;) {
    wchar c = get_char(line, p.x);
    if (iswalnum(c)) ret_p = p;
    else if (cterm->mouse_state != MS_OPENING && *cfg.word_chars_excl){
      if (strchr(cfg.word_chars_excl, c))
        break;
    }
    else if (cterm->mouse_state != MS_OPENING && *cfg.word_chars) {
      if (!strchr(cfg.word_chars, c))
        break;
      ret_p = p;
    }
    else if (strchr("_#%~+-", c))
      ret_p = p;
    else if (strchr(".$@/\\", c)) {
      if (!forward)
        ret_p = p;
    }
    else if (c == ' ' && p.x > 0 && get_char(line, p.x - 1) == '\\')
      ret_p = p;
    else if (!(strchr("&,;?!", c) || c == (forward ? '=' : ':')))
      break;

    if (forward) {
      p.x++;
      if (p.x >= cterm->cols - ((line->lattr & LATTR_WRAPPED2) != 0)) {
        if (!(line->lattr & LATTR_WRAPPED))
          break;
        p.x = 0;
        release_line(line);
        line = fetch_line(++p.y);
      }
    }
    else {
      if (p.x <= 0) {
        if (p.y <= -sblines())
          break;
        release_line(line);
        line = fetch_line(--p.y);
        if (!(line->lattr & LATTR_WRAPPED))
          break;
        p.x = cterm->cols - ((line->lattr & LATTR_WRAPPED2) != 0);
      }
      p.x--;
    }
  }

  release_line(line);
  return ret_p;
}

/*
 * Spread the selection outwards according to the selection mode.
 */
static pos
sel_spread_half(pos p, bool forward)
{
  switch (cterm->mouse_state) {
    when MS_SEL_CHAR: {
     /*
      * In this mode, every character is a separate unit, except
      * for runs of spaces at the end of a non-wrapping line.
      */
      termline *line = fetch_line(p.y);
      if (!(line->lattr & LATTR_WRAPPED)) {
        termchar *q = line->chars + cterm->cols;
        while (q > line->chars && q[-1].chr == ' ' && !q[-1].cc_next)
          q--;
        if (q == line->chars + cterm->cols)
          q--;
        if (p.x >= q - line->chars)
          p.x = forward ? cterm->cols - 1 : q - line->chars;
      }
      release_line(line);
    }
    when MS_SEL_WORD or MS_OPENING:
      p = sel_spread_word(p, forward);
    when MS_SEL_LINE:
      if (forward) {
        termline *line = fetch_line(p.y);
        while (line->lattr & LATTR_WRAPPED) {
          release_line(line);
          line = fetch_line(++p.y);
          p.x = 0;
        }
        int x = p.x;
        p.x = cterm->cols - 1;
        do {
          if (get_char(line, x) != ' ')
            p.x = x;
        } while (++x < line->cols);
        release_line(line);
      }
      else {
        p.x = 0;
        while (p.y > -sblines()) {
          termline *line = fetch_line(p.y - 1);
          bool wrapped = line->lattr & LATTR_WRAPPED;
          release_line(line);
          if (!wrapped)
            break;
          p.y--;
        }
      }
    otherwise:
     /* Shouldn't happen. */
      break;
  }
  return p;
}

static void
sel_spread(void)
{
  cterm->sel_start = sel_spread_half(cterm->sel_start, false);
  cterm->sel_end = sel_spread_half(cterm->sel_end, true);
  incpos(cterm->sel_end);
}

static bool
hover_spread_empty(void)
{
  cterm->hover_start = sel_spread_word(cterm->hover_start, false);
  cterm->hover_end = sel_spread_word(cterm->hover_end, true);
  bool eq = cterm->hover_start.y == cterm->hover_end.y && cterm->hover_start.x == cterm->hover_end.x;
  incpos(cterm->hover_end);
  return eq;
}

static void
sel_drag(pos selpoint)
{
  //printf("sel_drag %d+%d/2 (anchor %d+%d/2)\n", selpoint.x, selpoint.r, cterm->sel_anchor.x, cterm->sel_anchor.r);
  cterm->selected = true;
  if (!cterm->sel_rect) {
   /*
    * For normal selection, we set (sel_start,sel_end) to
    * (selpoint,sel_anchor) in some order.
    */
    if (poslt(selpoint, cterm->sel_anchor)) {
      cterm->sel_start = selpoint;
      cterm->sel_end = cterm->sel_anchor;
      if (cfg.elastic_mouse && !cterm->mouse_mode) {
        if (selpoint.r) {
          incpos(cterm->sel_start);
        }
        if (!cterm->sel_anchor.r) {
          decpos(cterm->sel_end);
        }
      }
    }
    else {
      cterm->sel_start = cterm->sel_anchor;
      cterm->sel_end = selpoint;
      if (cfg.elastic_mouse && !cterm->mouse_mode) {
        if (cterm->sel_anchor.r) {
          incpos(cterm->sel_start);
        }
        if (!selpoint.r) {
          decpos(cterm->sel_end);
        }
      }
    }
    sel_spread();
  }
  else {
   /*
    * For rectangular selection, we may need to
    * interchange x and y coordinates (if the user has
    * dragged in the -x and +y directions, or vice versa).
    */
    cterm->sel_start.x = min(cterm->sel_anchor.x, selpoint.x);
    cterm->sel_end.x = 1 + max(cterm->sel_anchor.x, selpoint.x);
    cterm->sel_start.y = min(cterm->sel_anchor.y, selpoint.y);
    cterm->sel_end.y = max(cterm->sel_anchor.y, selpoint.y);
  }
}

static void
sel_extend(pos selpoint)
{
  //printf("sel_extend %d+%d/2 (anchor %d+%d/2)\n", selpoint.x, selpoint.r, cterm->sel_anchor.x, cterm->sel_anchor.r);
  if (cterm->selected) {
    if (!cterm->sel_rect) {
     /*
      * For normal selection, we extend by moving
      * whichever end of the current selection is closer
      * to the mouse.
      */
      if (posdiff(selpoint, cterm->sel_start) <
          posdiff(cterm->sel_end, cterm->sel_start) / 2) {
        cterm->sel_anchor = cterm->sel_end;
        decpos(cterm->sel_anchor);
      }
      else
        cterm->sel_anchor = cterm->sel_start;
    }
    else {
     /*
      * For rectangular selection, we have a choice of
      * _four_ places to put sel_anchor and selpoint: the
      * four corners of the selection.
      */
      cterm->sel_anchor.x =
        selpoint.x * 2 < cterm->sel_start.x + cterm->sel_end.x
        ? cterm->sel_end.x - 1
        : cterm->sel_start.x;
      cterm->sel_anchor.y =
        selpoint.y * 2 < cterm->sel_start.y + cterm->sel_end.y
        ? cterm->sel_end.y
        : cterm->sel_start.y;
    }
  }
  else
    cterm->sel_anchor = selpoint;
  sel_drag(selpoint);
}

typedef enum {
  MA_CLICK = 0,
  MA_MOVE = 1,
  MA_WHEEL = 2,
  MA_RELEASE = 3
} mouse_action;  // values are significant, used for calculation!

static void
send_mouse_event(mouse_action a, mouse_button b, mod_keys mods, pos p)
{
  if (cterm->mouse_mode == MM_LOCATOR) {
    // handle DECSLE: select locator events
    if ((a == MA_CLICK && cterm->locator_report_up)
     || (a == MA_RELEASE && cterm->locator_report_dn)) {
      int pe = 0;
      switch (b) {
        when MBT_LEFT:
          pe = a == MA_CLICK ? 2 : 3;
        when MBT_MIDDLE:
          pe = a == MA_CLICK ? 4 : 5;
        when MBT_RIGHT:
          pe = a == MA_CLICK ? 6 : 7;
        when MBT_4:
          pe = a == MA_CLICK ? 8 : 9;
        otherwise:;
      }
      if (pe) {
        int x, y, buttons;
        win_get_locator_info(&x, &y, &buttons, cterm->locator_by_pixels);
        child_printf(cterm,"\e[%d;%d;%d;%d;0&w", pe, buttons, y, x);
        cterm->locator_rectangle = false;
      }
    }
    // handle DECEFR: enable filter rectangle
    else if (a == MA_MOVE && cterm->locator_rectangle) {
      /* Anytime the locator is detected outside of the filter
         rectangle, an outside rectangle event is generated and the
         rectangle is disabled.
      */
      int x, y, buttons;
      win_get_locator_info(&x, &y, &buttons, cterm->locator_by_pixels);
      if (x < cterm->locator_left || x > cterm->locator_right
          || y < cterm->locator_top || y > cterm->locator_bottom) {
        child_printf(cterm,"\e[10;%d;%d;%d;0&w", buttons, y, x);
        cterm->locator_rectangle = false;
      }
    }
    return;
  }

  uint x = p.x + 1, y = p.y + 1;

  if (a != MA_WHEEL) {
    if (cfg.old_xbuttons)
      switch (b) {
        when MBT_4:
          b = MBT_LEFT; mods |= MDK_ALT;
        when MBT_5:
          b = MBT_RIGHT; mods |= MDK_ALT;
        otherwise:;
      }
    else
      switch (b) {
        when MBT_4:
          b = 129;
        when MBT_5:
          b = 130;
        otherwise:;
      }
  }

  uint code = b ? b - 1 : 0x3;

  if (a != MA_RELEASE)
    code |= a * 0x20;
  else if (cterm->mouse_enc != ME_XTERM_CSI && cterm->mouse_enc != ME_PIXEL_CSI)
    code = 0x3;

  code |= (mods & ~cfg.click_target_mod) * 0x4;

  if (cterm->mouse_enc == ME_XTERM_CSI)
    child_printf(cterm,"\e[<%u;%u;%u%c", code, x, y, (a == MA_RELEASE ? 'm' : 'M'));
  else if (cterm->mouse_enc == ME_PIXEL_CSI)
    child_printf(cterm,"\e[<%u;%u;%u%c", code, p.pix + 1, p.piy + 1, (a == MA_RELEASE ? 'm' : 'M'));
  else if (cterm->mouse_enc == ME_URXVT_CSI)
    child_printf(cterm,"\e[%u;%u;%uM", code + 0x20, x, y);
  else {
    // Xterm's hacky but traditional character offset approach.
    char buf[8] = "\e[M";
    uint len = 3;

    void encode_coord(uint c) {
      c += 0x20;
      if (cterm->mouse_enc != ME_UTF8)
        buf[len++] = c < 0x100 ? c : 0;
      else if (c < 0x80)
        buf[len++] = c;
      else if (c < 0x800) {
        // In extended mouse mode, positions from 96 to 2015 are encoded as a
        // two-byte UTF-8 sequence (as introduced in xterm #262.)
        buf[len++] = 0xC0 + (c >> 6);
        buf[len++] = 0x80 + (c & 0x3F);
      }
      else {
        // Xterm reports out-of-range positions as a NUL byte.
        buf[len++] = 0;
      }
    }

    buf[len++] = code + 0x20;
    encode_coord(x);
    encode_coord(y);

    child_write(cterm,buf, len);
  }
}

static pos
box_pos(pos p)
{
  p.y = min(max(0, p.y), cterm->rows - 1);
  p.x = min(max(0, p.x), cterm->cols - 1);
  // p.piy and p.pix already clipped in translate_pos()
  return p;
}

static pos
get_selpoint(const pos p)
{
  pos sp = { .y = p.y + cterm->disptop, .x = p.x, .r = p.r };
  termline *line = fetch_line(sp.y);

  // Adjust to presentational direction.
  if (line->lattr & LATTR_PRESRTL) {
    sp.x = cterm->cols - 1 - sp.x;
    sp.r = !sp.r;
  }

  // Adjust to double-width line display.
  if ((line->lattr & LATTR_MODE) != LATTR_NORM)
    sp.x /= 2;

 /*
  * Transform x through the bidi algorithm to find the _logical_
  * click point from the physical one.
  */
  if (term_bidi_line(line, p.y) != null) {
#ifdef debug_bidi_cache
    printf("mouse @ log %d -> vis %d\n", sp.x, cterm->post_bidi_cache[p.y].backward[sp.x]);
#endif
    sp.x = cterm->post_bidi_cache[p.y].backward[sp.x];
  }

  // Back to previous cell if current one is second half of a wide char
  if (line->chars[sp.x].chr == UCSWIDE)
    sp.x--;

  release_line(line);
  return sp;
}

static void
send_keys(char *code, uint len, uint count)
{
  if (count) {
    uint size = len * count;
    char buf[size];
    char *p = buf;
    while (count--) { memcpy(p, code, len); p += len; }
    child_write(cterm,buf, size);
  }
}
static bool
check_app_mouse(mod_keys *mods_p)
{
  if (cterm->locator_1_enabled)
    return true;
  if(cterm->selection_pending==2)return 0;
  if (!cterm->mouse_mode || cterm->show_other_screen)
    return false;
  bool override = *mods_p & cfg.click_target_mod;
  *mods_p &= ~cfg.click_target_mod;
  return cfg.clicks_target_app ^ override;
}

bool
term_mouse_click(mouse_button b, mod_keys mods, pos p, int count)
{
  compose_clear();

  if (cterm->hovering) {
    cterm->hovering = false;
    win_update(true);
  }

  bool res = true;
  if (tek_mode == TEKMODE_GIN) {
    char c = '`';
    switch (b) {
      when MBT_LEFT: c = 'l';
      when MBT_MIDDLE: c = 'm';
      when MBT_RIGHT: c = 'r';
      when MBT_4: c = 'p';
      when MBT_5: c = 'q';
    }
    if (mods & MDK_SHIFT)
      c ^= ' ';
    child_send(cterm,&c, 1);
    tek_send_address();
  }
  else if (check_app_mouse(&mods)) {
    if (cterm->mouse_mode == MM_X10)
      mods = 0;
    send_mouse_event(MA_CLICK, b, mods, box_pos(p));
    cterm->mouse_state = (int)b;
  }
  else {
    // generic transformation M4/M5 -> Alt+left/right;
    // if any specific handling is designed for M4/M5, this needs to be tweaked
    bool fake_alt = false;
    switch (b) {
      when MBT_4:
        b = MBT_LEFT; mods |= MDK_ALT; fake_alt = true;
      when MBT_5:
        b = MBT_RIGHT; mods |= MDK_ALT; fake_alt = true;
      otherwise:;
    }

    bool alt = mods & MDK_ALT;
    bool shift_or_ctrl = mods & (MDK_SHIFT | MDK_CTRL);
    int mca = cfg.middle_click_action;
    int rca = cfg.right_click_action;
    cterm->mouse_state = 0;
    if (b == MBT_RIGHT && (rca == RC_MENU || shift_or_ctrl)) {
      // disable Alt+mouse menu opening;
      // the menu would often be closed soon by auto-repeat Alt, sending
      // WM_CAPTURECHANGED, WM_UNINITMENUPOPUP, WM_MENUSELECT, WM_EXITMENULOOP
      // trying to ignore WM_CAPTURECHANGED does not help
      if (!alt || fake_alt)
        win_popup_menu(0,mods);
      else
        res = false;
    }
    else if (b == MBT_MIDDLE && (mods & ~MDK_SHIFT) == MDK_CTRL) {
      if (cfg.zoom_mouse)
        win_zoom_font(0, mods & MDK_SHIFT);
      else
        res = false;
    }
    else if ((b == MBT_RIGHT && rca == RC_PASTE) ||
             (b == MBT_MIDDLE && mca == MC_PASTE))
    {
      if (!alt)
        cterm->mouse_state = shift_or_ctrl ? MS_COPYING : MS_PASTING;
      else
        res = false;
    }
    else if ((b == MBT_RIGHT && rca == RC_ENTER) ||
             (b == MBT_MIDDLE && mca == MC_ENTER)) {
      child_send(cterm,"\r", 1);
    }
    else if (b == MBT_LEFT && mods == MDK_SHIFT && rca == RC_EXTEND) {
      cterm->mouse_state = MS_PASTING;
    }
    else if (b == MBT_LEFT && (mods & ~cfg.click_target_mod) == MDK_CTRL) {
      if (count == cfg.opening_clicks) {
        // Open word under cursor
        p = get_selpoint(box_pos(p));
        cterm->mouse_state = MS_OPENING;
        cterm->selected = true;
        cterm->sel_rect = false;
        cterm->sel_start = cterm->sel_end = cterm->sel_anchor = p;
        sel_spread();
        win_update(true);
      }
      else
        res = false;
    }
    else if (b == MBT_MIDDLE && mca == MC_VOID) {
      // res = true; // MC_VOID explicitly ignores the click
    }
    else if ((mods & (MDK_CTRL | MDK_ALT)) != (MDK_CTRL | MDK_ALT)) {
      // Only clicks for selecting and extending should get here.
      p = get_selpoint(box_pos(p));
      //MS_SEL_CHAR = -1, 
      //MS_SEL_WORD = -2, 
      //MS_SEL_LINE = -3,
      cterm->mouse_state = -count;
      cterm->sel_rect = alt;
      if (b != MBT_LEFT || shift_or_ctrl)
        sel_extend(p);
      else if (count == 1) {
        cterm->selected = false;
        cterm->sel_anchor = p;
      }
      else {
        // Double or triple-click: select whole word or line
        cterm->selected = true;
        cterm->sel_rect = false;
        cterm->sel_start = cterm->sel_end = cterm->sel_anchor = p;
        sel_spread();
      }
      win_capture_mouse();
      win_update(true);
    }
    else {
      res = false;
    }
  }

  return res;
}

void
term_mouse_release(mouse_button b, mod_keys mods, pos p)
{
  compose_clear();

  int state = cterm->mouse_state;
  cterm->mouse_state = 0;
  switch (state) {
    when MS_COPYING: term_copy();
    when MS_PASTING: win_paste();
    when MS_OPENING: {
      termline *line = fetch_line(p.y + cterm->disptop);
      int urli = line->chars[p.x].attr.link;
      release_line(line);
      char * url = geturl(urli);
      if (url)
        win_open(cs__utftowcs(url), true);  // win_open frees its argument
      else
        term_open();
      cterm->selected = false;
      cterm->hovering = false;
      win_update(true);
    }
    when MS_SEL_CHAR or MS_SEL_WORD or MS_SEL_LINE: {
      // Finish selection.
      if (cterm->selected && cfg.copy_on_select)
        term_copy();

      // Flush any output held back during selection.
      term_flush();

      // "Clicks place cursor" implementation.
      if (!cfg.clicks_place_cursor || cterm->on_alt_screen || cterm->app_cursor_keys)
        return;

      pos dest = cterm->selected ? cterm->sel_end : get_selpoint(box_pos(p));

      static bool moved_previously;
      static pos last_dest;

      pos orig;
      if (state == MS_SEL_CHAR)
        orig = (pos){.y = cterm->curs.y, .x = cterm->curs.x};
      else if (moved_previously)
        orig = last_dest;
      else
        return;

      bool forward = posle(orig, dest);
      pos end = forward ? dest : orig;
      p = forward ? orig : dest;

      uint count = 0;
      while (p.y != end.y) {
        termline *line = fetch_line(p.y);
        if (!(line->lattr & LATTR_WRAPPED)) {
          release_line(line);
          moved_previously = false;
          return;
        }
        int cols = cterm->cols - ((line->lattr & LATTR_WRAPPED2) != 0);
        for (int x = p.x; x < cols; x++) {
          if (line->chars[x].chr != UCSWIDE)
            count++;
        }
        p.y++;
        p.x = 0;
        release_line(line);
      }
      termline *line = fetch_line(p.y);
      for (int x = p.x; x < end.x; x++) {
        if (line->chars[x].chr != UCSWIDE)
          count++;
      }
      release_line(line);

      char code[3] =
        {'\e', cterm->app_cursor_keys ? 'O' : '[', forward ? 'C' : 'D'};

      send_keys(code, 3, count);

      moved_previously = true;
      last_dest = dest;
    }
    otherwise:
      if (check_app_mouse(&mods)) {
        if (cterm->mouse_mode >= MM_VT200)
          send_mouse_event(MA_RELEASE, b, mods, box_pos(p));
      }
  }
}

static void
sel_scroll_cb(void)
{
  if (term_selecting() && cterm->sel_scroll) {
    term_scroll(0, cterm->sel_scroll);
    sel_drag(get_selpoint(cterm->sel_pos));
    win_update(true);
    win_set_timer(sel_scroll_cb, 125);
  }
}

void
term_mouse_move(mod_keys mods, pos p)
{
  compose_clear();

  //printf("mouse_move %d+%d/2\n", p.x, p.r);
  pos bp = box_pos(p);

  if (term_selecting()) {
    if (p.y < 0 || p.y >= cterm->rows) {
      if (!cterm->sel_scroll)
        win_set_timer(sel_scroll_cb, 200);
      cterm->sel_scroll = p.y < 0 ? p.y : p.y - cterm->rows + 1;
      cterm->sel_pos = bp;
    }
    else {
      cterm->sel_scroll = 0;
      if (p.x < 0 && p.y + cterm->disptop > cterm->sel_anchor.y)
        bp = (pos){.y = p.y - 1, .x = cterm->cols - 1, .r = p.r};
    }

    bool alt = mods & MDK_ALT;
    cterm->sel_rect = alt;
    sel_drag(get_selpoint(bp));

    win_update(true);
  }
  else if (cterm->mouse_state == MS_OPENING) {
    // let's not clear link opening state when just moving the mouse (#1039)
    // but only after hovering out of the link area (below)
#ifdef link_opening_only_if_unmoved
    cterm->mouse_state = 0;
    cterm->selected = false;
    win_update(true);
#endif
  }
  else if (cterm->mouse_state > 0) {
    if (cterm->mouse_mode >= MM_BTN_EVENT)
      send_mouse_event(MA_MOVE, (mouse_button)cterm->mouse_state, mods, bp);
  }
  else {
    if (cterm->mouse_mode == MM_ANY_EVENT)
      send_mouse_event(MA_MOVE, 0, mods, bp);
  }

  if (!check_app_mouse(&mods) && (mods & ~cfg.click_target_mod) == MDK_CTRL && cterm->has_focus) {
    p = get_selpoint(box_pos(p));
    cterm->hover_start = cterm->hover_end = p;
    if (!hover_spread_empty()) {
      cterm->hovering = true;
      termline *line = fetch_line(p.y);
      cterm->hoverlink = line->chars[p.x].attr.link;
      release_line(line);
      win_update(true);
    }
    else if (cterm->hovering) {
      cterm->hovering = false;
      win_update(true);
    }
    //printf("->hovering %d (opening %d)\n", cterm->hovering, cterm->mouse_state == MS_OPENING);
    // clear link opening state after hovering out of link area
    if (!cterm->hovering && cterm->mouse_state == MS_OPENING)
      cterm->mouse_state = 0;
  }
}

void
term_mouse_wheel(bool horizontal, int delta, int lines_per_notch, mod_keys mods, pos p)
{
  compose_clear();

  if (cterm->hovering) {
    cterm->hovering = false;
    win_update(true);
  }

  enum { NOTCH_DELTA = 120 };

  static int accu = 0;
  accu += delta;

  if (tek_mode == TEKMODE_GIN) {
    int step = (mods & MDK_SHIFT) ? 40 : (mods & MDK_CTRL) ? 1 : 4;
    if (horizontal ^ (mods & MDK_CTRL))
      tek_move_by(0, step * delta / NOTCH_DELTA);
    else
      tek_move_by(step * delta / NOTCH_DELTA, 0);
  }
  else if (check_app_mouse(&mods)) {
    if (strstr(cfg.suppress_wheel, "report"))
      return;
    // Send as mouse events, with one event per notch.
    int notches = accu / NOTCH_DELTA;
    if (notches) {
      accu -= NOTCH_DELTA * notches;
      mouse_button b = (notches < 0) + 1;
      if (horizontal)
        b = 5 - b;
      notches = abs(notches);
      do
        send_mouse_event(MA_WHEEL, b, mods, box_pos(p));
      while (--notches);
    }
  }
  else if (horizontal) {
  }
  else if (cfg.zoom_mouse && (mods & ~MDK_SHIFT) == MDK_CTRL) {
    if (strstr(cfg.suppress_wheel, "zoom"))
      return;
    if (cfg.zoom_mouse) {
      int zoom = accu / NOTCH_DELTA;
      if (zoom) {
        accu -= NOTCH_DELTA * zoom;
        win_zoom_font(zoom, mods & MDK_SHIFT);
      }
    }
  }
  else if (!(mods & ~(MDK_SHIFT | MDK_CTRL | MDK_ALT))) {
    if (mods & MDK_CTRL)
      lines_per_notch = 1;
    else if (cfg.lines_per_notch > 0)
      lines_per_notch = min(cfg.lines_per_notch, cterm->rows - 1);

    // Scroll, taking the lines_per_notch setting into account.
    // Scroll by a page per notch if setting is -1 or Shift is pressed.
    int lines_per_page = max(1, cterm->rows);
    if (lines_per_notch == -1 || mods & MDK_SHIFT)
      lines_per_notch = lines_per_page;
    int lines = lines_per_notch * accu / NOTCH_DELTA;
    //printf("mouse lines %d per notch %d accu %d\n", lines, lines_per_notch, accu);
    if (lines) {
      accu -= lines * NOTCH_DELTA / lines_per_notch;
      if ((!cterm->on_alt_screen || cterm->show_other_screen) && !(mods & MDK_ALT)) {
        if (strstr(cfg.suppress_wheel, "scrollwin"))
          return;
        term_scroll(0, -lines);
      }
      else if (cterm->wheel_reporting || cterm->wheel_reporting_xterm) {
        if (strstr(cfg.suppress_wheel, "scrollapp") && !cterm->wheel_reporting_xterm)
          return;
        // Send scroll distance as CSI a/b events
        bool up = lines > 0;
        lines = abs(lines);
        int pages = lines / lines_per_page;
        lines -= pages * lines_per_page;
        if (cterm->app_wheel && !cterm->wheel_reporting_xterm) {
          send_keys(up ? "\e[1;2a" : "\e[1;2b", 6, pages);
          send_keys(up ? "\eOa" : "\eOb", 3, lines);
        }
        else if (cterm->vt52_mode) {
          send_keys(up ? "\eA" : "\eB", 2, lines);
        }
        else {
          send_keys(up ? "\e[5~" : "\e[6~", 4, pages);
          char code[3] =
            {'\e', cterm->app_cursor_keys ? 'O' : '[', up ? 'A' : 'B'};
          send_keys(code, 3, lines);
        }
      }
    }
  }
}
