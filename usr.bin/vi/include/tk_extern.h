/*	$OpenBSD: tk_extern.h,v 1.2 2001/01/29 01:58:47 niklas Exp $	*/

int tk_addstr __P((SCR *, const char *, size_t));
int tk_attr __P((SCR *, scr_attr_t, int));
int tk_baud __P((SCR *, u_long *));
int tk_bell __P((SCR *));
int tk_clrtoeol __P((SCR *));
int tk_cursor __P((SCR *, size_t *, size_t *));
int tk_deleteln __P((SCR *));
int tk_ex_adjust __P((SCR *, exadj_t));
int tk_insertln __P((SCR *));
int tk_keyval __P((SCR *, scr_keyval_t, CHAR_T *, int *));
int tk_move __P((SCR *, size_t, size_t));
int tk_refresh __P((SCR *, int));
int tk_rename __P((SCR *));
int tk_suspend __P((SCR *, int *));
void tk_usage __P((void));
int tk_event __P((SCR *, EVENT *, u_int32_t, int));
int tk_key __P((ClientData, Tcl_Interp *, int, char *[]));
int tk_screen __P((SCR *, u_int32_t));
int tk_quit __P((GS *));
int tk_term_init __P((SCR *));
int tk_term_end __P((GS *));
int tk_fmap __P((SCR *, seq_t, CHAR_T *, size_t, CHAR_T *, size_t));
int tk_optchange __P((SCR *, int, char *, u_long *));
int tk_ssize __P((SCR *, int, size_t *, size_t *, int *));
int tk_op __P((ClientData, Tcl_Interp *, int, char *[]));
int tk_opt_init __P((ClientData, Tcl_Interp *, int, char *[]));
int tk_opt_set __P((ClientData, Tcl_Interp *, int, char *[]));
int tk_version __P((ClientData, Tcl_Interp *, int, char *[]));
void tk_msg __P((SCR *, mtype_t, char *, size_t));
