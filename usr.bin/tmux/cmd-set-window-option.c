/* $OpenBSD: cmd-set-window-option.c,v 1.17 2011/01/04 00:42:47 nicm Exp $ */

/*
 * Copyright (c) 2008 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include "tmux.h"

/*
 * Set a window option. This is just an alias for set-option -w.
 */

int	cmd_set_window_option_exec(struct cmd *, struct cmd_ctx *);

const struct cmd_entry cmd_set_window_option_entry = {
	"set-window-option", "setw",
	"agt:u", 1, 2,
	"[-agu] " CMD_TARGET_WINDOW_USAGE " option [value]",
	0,
	NULL,
	NULL,
	cmd_set_window_option_exec
};

int
cmd_set_window_option_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct args	*args = self->args;

	args_set(args, 'w', NULL);
	return (cmd_set_option_entry.exec(self, ctx));
}
