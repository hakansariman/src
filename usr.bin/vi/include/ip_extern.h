/*	$OpenBSD: ip_extern.h,v 1.2 2001/01/29 01:58:47 niklas Exp $	*/

int ip_addstr __P((SCR *, const char *, size_t));
int ip_attr __P((SCR *, scr_attr_t, int));
int ip_baud __P((SCR *, u_long *));
int ip_bell __P((SCR *));
void ip_busy __P((SCR *, const char *, busy_t));
int ip_clrtoeol __P((SCR *));
int ip_cursor __P((SCR *, size_t *, size_t *));
int ip_deleteln __P((SCR *));
int ip_ex_adjust __P((SCR *, exadj_t));
int ip_insertln __P((SCR *));
int ip_keyval __P((SCR *, scr_keyval_t, CHAR_T *, int *));
int ip_move __P((SCR *, size_t, size_t));
int ip_refresh __P((SCR *, int));
int ip_rename __P((SCR *));
int ip_suspend __P((SCR *, int *));
void ip_usage __P((void));
int ip_event __P((SCR *, EVENT *, u_int32_t, int));
int ip_screen __P((SCR *, u_int32_t));
int ip_quit __P((GS *));
int ip_term_init __P((SCR *));
int ip_term_end __P((GS *));
int ip_fmap __P((SCR *, seq_t, CHAR_T *, size_t, CHAR_T *, size_t));
int ip_optchange __P((SCR *, int, char *, u_long *));
