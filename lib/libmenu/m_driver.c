/*	$OpenBSD: m_driver.c,v 1.3 1997/12/03 05:31:17 millert Exp $	*/

/*-----------------------------------------------------------------------------+
|           The ncurses menu library is  Copyright (C) 1995-1997               |
|             by Juergen Pfeifer <Juergen.Pfeifer@T-Online.de>                 |
|                          All Rights Reserved.                                |
|                                                                              |
| Permission to use, copy, modify, and distribute this software and its        |
| documentation for any purpose and without fee is hereby granted, provided    |
| that the above copyright notice appear in all copies and that both that      |
| copyright notice and this permission notice appear in supporting             |
| documentation, and that the name of the above listed copyright holder(s) not |
| be used in advertising or publicity pertaining to distribution of the        |
| software without specific, written prior permission.                         | 
|                                                                              |
| THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD TO  |
| THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-  |
| NESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR   |
| ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RE- |
| SULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, |
| NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH    |
| THE USE OR PERFORMANCE OF THIS SOFTWARE.                                     |
+-----------------------------------------------------------------------------*/

/***************************************************************************
* Module m_driver                                                          *
* Central dispatching routine                                              *
***************************************************************************/

#include "menu.priv.h"

MODULE_ID("Id: m_driver.c,v 1.9 1997/10/21 08:44:31 juergen Exp $")

/* Macros */

/* Remove the last character from the match pattern buffer */
#define Remove_Character_From_Pattern(menu) \
  (menu)->pattern[--((menu)->pindex)] = '\0'

/* Add a new character to the match pattern buffer */
#define Add_Character_To_Pattern(menu,ch) \
  { (menu)->pattern[((menu)->pindex)++] = (ch);\
    (menu)->pattern[(menu)->pindex] = '\0'; }

/*---------------------------------------------------------------------------
|   Facility      :  libnmenu  
|   Function      :  static bool Is_Sub_String( 
|                           bool IgnoreCaseFlag,
|                           const char *part,
|                           const char *string)
|   
|   Description   :  Checks whether or not part is a substring of string.
|
|   Return Values :  TRUE   - if it is a substring
|                    FALSE  - if it is not a substring
+--------------------------------------------------------------------------*/
static bool Is_Sub_String(
			  bool  IgnoreCaseFlag,
			  const char *part,
			  const char *string
			 )
{
  assert( part && string );
  if ( IgnoreCaseFlag )
    {
      while(*string && *part)
	{
	  if (toupper(*string++)!=toupper(*part)) break;
	  part++;
	}
    }
  else
    {
      while( *string && *part )
	if (*part != *string++) break;
      part++;
    }
  return ( (*part) ? FALSE : TRUE );
}

/*---------------------------------------------------------------------------
|   Facility      :  libnmenu  
|   Function      :  int _nc_Match_Next_Character_In_Item_Name(
|                           MENU *menu,
|                           int  ch,
|                           ITEM **item)
|   
|   Description   :  This internal routine is called for a menu positioned
|                    at an item with three different classes of characters:
|                       - a printable character; the character is added to
|                         the current pattern and the next item matching
|                         this pattern is searched.
|                       - NUL; the pattern stays as it is and the next item
|                         matching the pattern is searched
|                       - BS; the pattern stays as it is and the previous
|                         item matching the pattern is searched
|
|                       The item parameter contains on call a pointer to
|                       the item where the search starts. On return - if
|                       a match was found - it contains a pointer to the
|                       matching item.
|  
|   Return Values :  E_OK        - an item matching the pattern was found
|                    E_NO_MATCH  - nothing found
+--------------------------------------------------------------------------*/
int _nc_Match_Next_Character_In_Item_Name(MENU *menu, int ch, ITEM **item)
{
  bool found = FALSE, passed = FALSE;
  int  idx, last;
  
  assert( menu && item && *item);
  idx = (*item)->index;
  
  if (ch && ch!=BS)
    {
      /* if we become to long, we need no further checking : there can't be
	 a match ! */
      if ((menu->pindex+1) > menu->namelen) 
	RETURN(E_NO_MATCH);
      
      Add_Character_To_Pattern(menu,ch);
      /* we artificially position one item back, because in the do...while
	 loop we start with the next item. This means, that with a new
	 pattern search we always start the scan with the actual item. If
	 we do a NEXT_PATTERN oder PREV_PATTERN search, we start with the
	 one after or before the actual item. */
      if (--idx < 0) 
	idx = menu->nitems-1;
    }
  
  last = idx;			/* this closes the cycle */
  
  do{
    if (ch==BS)
      {			/* we have to go backward */
	if (--idx < 0) 
	  idx = menu->nitems-1;
      }
    else
      {			/* otherwise we always go forward */
	if (++idx >= menu->nitems) 
	  idx = 0;
      }
    if (Is_Sub_String((menu->opt & O_IGNORECASE) != 0,
		      menu->pattern,
		      menu->items[idx]->name.str)
	)
      found = TRUE;
    else
      passed = TRUE;    
  } while (!found && (idx != last));
  
  if (found)
    {
      if (!((idx==(*item)->index) && passed))
	{
	  *item = menu->items[idx];
	  RETURN(E_OK);
	}
      /* This point is reached, if we fully cycled through the item list
	 and the only match we found is the starting item. With a NEXT_PATTERN
	 or PREV_PATTERN scan this means, that there was no additional match.
	 If we searched with an expanded new pattern, we should never reach
	 this point, because if the expanded pattern matches also the actual
	 item we will find it in the first attempt (passed==FALSE) and we
	 will never cycle through the whole item array.   
	 */
      assert( ch==0 || ch==BS );
    }
  else
    {
      if (ch && ch!=BS && menu->pindex>0)
	{
	  /* if we had no match with a new pattern, we have to restore it */
	  Remove_Character_From_Pattern(menu);
	}
    }		
  RETURN(E_NO_MATCH);
}	

/*---------------------------------------------------------------------------
|   Facility      :  libnmenu  
|   Function      :  int menu_driver(MENU *menu, int c)
|   
|   Description   :  Central dispatcher for the menu. Translates the logical
|                    request 'c' into a menu action.
|
|   Return Values :  E_OK            - success
|                    E_BAD_ARGUMENT  - invalid menu pointer
|                    E_BAD_STATE     - menu is in user hook routine
|                    E_NOT_POSTED    - menu is not posted
+--------------------------------------------------------------------------*/
int menu_driver(MENU * menu, int   c)
{
#define NAVIGATE(dir) \
  if (!item->dir)\
     result = E_REQUEST_DENIED;\
  else\
     item = item->dir

  int result = E_OK;
  ITEM *item;
  int my_top_row, rdiff;
  
  if (!menu)
    RETURN(E_BAD_ARGUMENT);
  
  if ( menu->status & _IN_DRIVER )
    RETURN(E_BAD_STATE);
  if ( !( menu->status & _POSTED ) )
    RETURN(E_NOT_POSTED);
  
  my_top_row = menu->toprow;
  item    = menu->curitem;
  assert(item);
  
  if ((c > KEY_MAX) && (c<=MAX_MENU_COMMAND))
    {  
      if (!((c==REQ_BACK_PATTERN)
	    || (c==REQ_NEXT_MATCH) || (c==REQ_PREV_MATCH)))
	{
	  assert( menu->pattern );
	  Reset_Pattern(menu);
	}
      
      switch(c)
	{
	case REQ_LEFT_ITEM:
	  /*=================*/  
	  NAVIGATE(left);
	  break;
	  
	case REQ_RIGHT_ITEM:
	  /*==================*/  
	  NAVIGATE(right);
	  break;
	  
	case REQ_UP_ITEM:
	  /*===============*/  
	  NAVIGATE(up);
	  break;
	  
	case REQ_DOWN_ITEM:
	  /*=================*/  
	  NAVIGATE(down);
	  break;
	  
	case REQ_SCR_ULINE:
	  /*=================*/  
	  if (my_top_row == 0)
	    result = E_REQUEST_DENIED;
	  else
	    {
	      --my_top_row;
	      item = item->up;
	    }  
	  break;
	  
	case REQ_SCR_DLINE:
	  /*=================*/  
	  my_top_row++;
	  if ((menu->rows - menu->arows)>0)
	    {
	      /* only if the menu has less items than rows, we can deny the
		 request. Otherwise the epilogue of this routine adjusts the
		 top row if necessary */
	      my_top_row--;
	      result = E_REQUEST_DENIED;
	    }
	  else
	    item = item->down;
	  break;
	  
	case REQ_SCR_DPAGE:
	  /*=================*/  
	  rdiff = menu->rows - menu->arows - my_top_row;
	  if (rdiff > menu->arows) 
	    rdiff = menu->arows;
	  if (rdiff==0)
	    result = E_REQUEST_DENIED;
	  else
	    {
	      my_top_row += rdiff;
	      while(rdiff-- > 0)
		item = item->down;
	    }
	  break;
	  
	case REQ_SCR_UPAGE:
	  /*=================*/  
	  rdiff = (menu->arows < my_top_row) ?
	    menu->arows : my_top_row;
	  if (rdiff==0)
	    result = E_REQUEST_DENIED;
	  else
	    {
	      my_top_row -= rdiff;
	      while(rdiff--)
		item = item->up;
	    }
	  break;
	  
	case REQ_FIRST_ITEM:
	  /*==================*/  
	  item = menu->items[0];
	  break;
	  
	case REQ_LAST_ITEM:
	  /*=================*/  
	  item = menu->items[menu->nitems-1];
	  break;

	case REQ_NEXT_ITEM:
	  /*=================*/  
	  if ((item->index+1)>=menu->nitems)
	    {
	      if (menu->opt & O_NONCYCLIC)
		result = E_REQUEST_DENIED;
	      else
		item = menu->items[0];
	    }
	  else
	    item = menu->items[item->index + 1];
	  break;
	  
	case REQ_PREV_ITEM:
	  /*=================*/  
	  if (item->index<=0)
	    {
	      if (menu->opt & O_NONCYCLIC)
		result = E_REQUEST_DENIED;
	      else
		item = menu->items[menu->nitems-1];
	    }
	  else
	    item = menu->items[item->index - 1];
	  break;
	  
	case REQ_TOGGLE_ITEM:
	  /*===================*/  
	  if (menu->opt & O_ONEVALUE)
	    {
	      result = E_REQUEST_DENIED;
	    }
	  else
	    {
	      if (menu->curitem->opt & O_SELECTABLE)
		{
		  menu->curitem->value = !menu->curitem->value;
		  Move_And_Post_Item(menu,menu->curitem);
		  _nc_Show_Menu(menu);
		}
	      else
		result = E_NOT_SELECTABLE;
	    }
	  break;
	  
	case REQ_CLEAR_PATTERN:
	  /*=====================*/  
	  /* already cleared in prologue */
	  break;
	  
	case REQ_BACK_PATTERN:
	  /*====================*/  
	  if (menu->pindex>0)
	    {
	      assert(menu->pattern);
	      Remove_Character_From_Pattern(menu);
	      pos_menu_cursor( menu );
	    }
	  else
	    result = E_REQUEST_DENIED;
	  break;
	  
	case REQ_NEXT_MATCH:
	  /*==================*/  
	  assert(menu->pattern);
	  if (menu->pattern[0])
	    result = _nc_Match_Next_Character_In_Item_Name(menu,0,&item);
	  else
	    {
	      if ((item->index+1)<menu->nitems)
		item=menu->items[item->index+1];
	      else
		{
		  if (menu->opt & O_NONCYCLIC)
		    result = E_REQUEST_DENIED;
		  else
		    item = menu->items[0];
		}
	    }
	  break;	
	  
	case REQ_PREV_MATCH:
	  /*==================*/  
	  assert(menu->pattern);
	  if (menu->pattern[0])
	    result = _nc_Match_Next_Character_In_Item_Name(menu,BS,&item);
	  else
	    {
	      if (item->index)
		item = menu->items[item->index-1];
	      else
		{
		  if (menu->opt & O_NONCYCLIC)
		    result = E_REQUEST_DENIED;
		  else
		    item = menu->items[menu->nitems-1];
		}
	    }
	  break;
	  
	default:
	  /*======*/  
	  result = E_UNKNOWN_COMMAND;
	  break;
	}
    }
  else
    {				/* not a command */
      if ( !(c & ~((int)MAX_REGULAR_CHARACTER)) && isprint(c) )
	result = _nc_Match_Next_Character_In_Item_Name( menu, c, &item );
      else
	result = E_UNKNOWN_COMMAND;
    }
  
  /* Adjust the top row if it turns out that the current item unfortunately
     doesn't appear in the menu window */
  if ( item->y < my_top_row )
    my_top_row = item->y;
  else if ( item->y >= (my_top_row + menu->arows) )
    my_top_row = item->y - menu->arows + 1;
  
  _nc_New_TopRow_and_CurrentItem( menu, my_top_row, item );
  
  RETURN(result);
}

/* m_driver.c ends here */
