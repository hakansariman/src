/*	$OpenBSD: hpux_syscallargs.h,v 1.3 2004/07/11 00:21:29 mickey Exp $	*/

/*
 * System call argument lists.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	OpenBSD: syscalls.master,v 1.3 2004/07/11 00:20:46 mickey Exp 
 */

#ifdef	syscallarg
#undef	syscallarg
#endif

#define	syscallarg(x)							\
	union {								\
		register_t pad;						\
		struct { x datum; } le;					\
		struct {						\
			int8_t pad[ (sizeof (register_t) < sizeof (x))	\
				? 0					\
				: sizeof (register_t) - sizeof (x)];	\
			x datum;					\
		} be;							\
	}

struct hpux_sys_read_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(u_int) nbyte;
};

struct hpux_sys_write_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(u_int) nbyte;
};

struct hpux_sys_open_args {
	syscallarg(char *) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct hpux_sys_wait_args {
	syscallarg(int *) status;
};

struct hpux_sys_creat_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
};

struct hpux_sys_unlink_args {
	syscallarg(char *) path;
};

struct hpux_sys_execv_args {
	syscallarg(char *) path;
	syscallarg(char **) argp;
};

struct hpux_sys_chdir_args {
	syscallarg(char *) path;
};

struct hpux_sys_time_6x_args {
	syscallarg(time_t *) t;
};

struct hpux_sys_mknod_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
	syscallarg(int) dev;
};

struct hpux_sys_chmod_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
};

struct hpux_sys_chown_args {
	syscallarg(char *) path;
	syscallarg(int) uid;
	syscallarg(int) gid;
};

struct hpux_sys_stime_6x_args {
	syscallarg(int) time;
};

struct hpux_sys_ptrace_args {
	syscallarg(int) req;
	syscallarg(int) pid;
	syscallarg(int *) addr;
	syscallarg(int) data;
};

struct hpux_sys_alarm_6x_args {
	syscallarg(int) deltat;
};

struct hpux_sys_utime_6x_args {
	syscallarg(char *) fname;
	syscallarg(time_t *) tptr;
};

struct hpux_sys_stty_6x_args {
	syscallarg(int) fd;
	syscallarg(caddr_t) arg;
};

struct hpux_sys_gtty_6x_args {
	syscallarg(int) fd;
	syscallarg(caddr_t) arg;
};

struct hpux_sys_access_args {
	syscallarg(char *) path;
	syscallarg(int) flags;
};

struct hpux_sys_nice_6x_args {
	syscallarg(int) nval;
};

struct hpux_sys_ftime_6x_args {
	syscallarg(struct hpux_timeb *) tp;
};

struct hpux_sys_kill_args {
	syscallarg(pid_t) pid;
	syscallarg(int) signo;
};

struct hpux_sys_stat_args {
	syscallarg(char *) path;
	syscallarg(struct hpux_stat *) sb;
};

struct hpux_sys_lstat_args {
	syscallarg(char *) path;
	syscallarg(struct hpux_stat *) sb;
};

struct hpux_sys_times_6x_args {
	syscallarg(struct tms *) tms;
};

struct hpux_sys_ioctl_args {
	syscallarg(int) fd;
	syscallarg(int) com;
	syscallarg(caddr_t) data;
};

struct hpux_sys_symlink_args {
	syscallarg(char *) path;
	syscallarg(char *) link;
};

struct hpux_sys_utssys_args {
	syscallarg(struct hpux_utsname *) uts;
	syscallarg(int) dev;
	syscallarg(int) request;
};

struct hpux_sys_readlink_args {
	syscallarg(char *) path;
	syscallarg(char *) buf;
	syscallarg(int) count;
};

struct hpux_sys_execve_args {
	syscallarg(char *) path;
	syscallarg(char **) argp;
	syscallarg(char **) envp;
};

struct hpux_sys_fcntl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(int) arg;
};

struct hpux_sys_ulimit_args {
	syscallarg(int) cmd;
	syscallarg(int) newlimit;
};

struct hpux_sys_mmap_args {
	syscallarg(caddr_t) addr;
	syscallarg(size_t) len;
	syscallarg(int) prot;
	syscallarg(int) flags;
	syscallarg(int) fd;
	syscallarg(long) pos;
};

struct hpux_sys_getpgrp2_args {
	syscallarg(pid_t) pid;
};

struct hpux_sys_setpgrp2_args {
	syscallarg(pid_t) pid;
	syscallarg(pid_t) pgid;
};

struct hpux_sys_wait3_args {
	syscallarg(int *) status;
	syscallarg(int) options;
	syscallarg(int) rusage;
};

struct hpux_sys_fstat_args {
	syscallarg(int) fd;
	syscallarg(struct hpux_stat *) sb;
};

struct hpux_sys_sigvec_args {
	syscallarg(int) signo;
	syscallarg(struct sigvec *) nsv;
	syscallarg(struct sigvec *) osv;
};

struct hpux_sys_sigblock_args {
	syscallarg(int) mask;
};

struct hpux_sys_sigsetmask_args {
	syscallarg(int) mask;
};

struct hpux_sys_sigpause_args {
	syscallarg(int) mask;
};

struct hpux_sys_readv_args {
	syscallarg(int) fd;
	syscallarg(struct iovec *) iovp;
	syscallarg(u_int) iovcnt;
};

struct hpux_sys_writev_args {
	syscallarg(int) fd;
	syscallarg(struct iovec *) iovp;
	syscallarg(u_int) iovcnt;
};

struct hpux_sys_rename_args {
	syscallarg(char *) from;
	syscallarg(char *) to;
};

struct hpux_sys_truncate_args {
	syscallarg(char *) path;
	syscallarg(long) length;
};

struct hpux_sys_sysconf_args {
	syscallarg(int) name;
};

struct hpux_sys_mkdir_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
};

struct hpux_sys_rmdir_args {
	syscallarg(char *) path;
};

struct hpux_sys_getrlimit_args {
	syscallarg(u_int) which;
	syscallarg(struct ogetrlimit *) rlp;
};

struct hpux_sys_setrlimit_args {
	syscallarg(u_int) which;
	syscallarg(struct ogetrlimit *) rlp;
};

struct hpux_sys_rtprio_args {
	syscallarg(pid_t) pid;
	syscallarg(int) prio;
};

struct hpux_sys_lockf_args {
	syscallarg(int) fd;
	syscallarg(int) func;
	syscallarg(long) size;
};

struct hpux_sys_shmctl_args {
	syscallarg(int) shmid;
	syscallarg(int) cmd;
	syscallarg(caddr_t) buf;
};

struct hpux_sys_getcontext_args {
	syscallarg(char *) buf;
	syscallarg(int) len;
};

struct hpux_sys_sigprocmask_args {
	syscallarg(int) how;
	syscallarg(hpux_sigset_t *) set;
	syscallarg(hpux_sigset_t *) oset;
};

struct hpux_sys_sigpending_args {
	syscallarg(hpux_sigset_t *) set;
};

struct hpux_sys_sigsuspend_args {
	syscallarg(hpux_sigset_t *) set;
};

struct hpux_sys_sigaction_args {
	syscallarg(int) signo;
	syscallarg(struct hpux_sigaction *) nsa;
	syscallarg(struct hpux_sigaction *) osa;
};

struct hpux_sys_waitpid_args {
	syscallarg(pid_t) pid;
	syscallarg(int *) status;
	syscallarg(int) options;
	syscallarg(struct rusage *) rusage;
};

struct hpux_sigsetreturn_args {
	syscallarg(int) cookie;
};

struct hpux_sys_nshmctl_args {
	syscallarg(int) shmid;
	syscallarg(int) cmd;
	syscallarg(caddr_t) buf;
};

struct hpux_sys_sigaltstack_args {
	syscallarg(const struct hpux_sigaltstack *) nss;
	syscallarg(struct hpux_sigaltstack *) oss;
};

/*
 * System call prototypes.
 */

int	sys_nosys(struct proc *, void *, register_t *);
int	sys_exit(struct proc *, void *, register_t *);
int	hpux_sys_fork(struct proc *, void *, register_t *);
int	hpux_sys_read(struct proc *, void *, register_t *);
int	hpux_sys_write(struct proc *, void *, register_t *);
int	hpux_sys_open(struct proc *, void *, register_t *);
int	sys_close(struct proc *, void *, register_t *);
int	hpux_sys_wait(struct proc *, void *, register_t *);
int	hpux_sys_creat(struct proc *, void *, register_t *);
int	sys_link(struct proc *, void *, register_t *);
int	hpux_sys_unlink(struct proc *, void *, register_t *);
int	hpux_sys_execv(struct proc *, void *, register_t *);
int	hpux_sys_chdir(struct proc *, void *, register_t *);
int	hpux_sys_time_6x(struct proc *, void *, register_t *);
int	hpux_sys_mknod(struct proc *, void *, register_t *);
int	hpux_sys_chmod(struct proc *, void *, register_t *);
int	hpux_sys_chown(struct proc *, void *, register_t *);
int	sys_obreak(struct proc *, void *, register_t *);
int	compat_43_sys_lseek(struct proc *, void *, register_t *);
int	sys_getpid(struct proc *, void *, register_t *);
int	sys_setuid(struct proc *, void *, register_t *);
int	sys_getuid(struct proc *, void *, register_t *);
int	hpux_sys_stime_6x(struct proc *, void *, register_t *);
#ifdef PTRACE
int	hpux_sys_ptrace(struct proc *, void *, register_t *);
#else
#endif
int	hpux_sys_alarm_6x(struct proc *, void *, register_t *);
int	hpux_sys_pause_6x(struct proc *, void *, register_t *);
int	hpux_sys_utime_6x(struct proc *, void *, register_t *);
int	hpux_sys_stty_6x(struct proc *, void *, register_t *);
int	hpux_sys_gtty_6x(struct proc *, void *, register_t *);
int	hpux_sys_access(struct proc *, void *, register_t *);
int	hpux_sys_nice_6x(struct proc *, void *, register_t *);
int	hpux_sys_ftime_6x(struct proc *, void *, register_t *);
int	sys_sync(struct proc *, void *, register_t *);
int	hpux_sys_kill(struct proc *, void *, register_t *);
int	hpux_sys_stat(struct proc *, void *, register_t *);
int	hpux_sys_setpgrp_6x(struct proc *, void *, register_t *);
int	hpux_sys_lstat(struct proc *, void *, register_t *);
int	sys_dup(struct proc *, void *, register_t *);
int	sys_opipe(struct proc *, void *, register_t *);
int	hpux_sys_times_6x(struct proc *, void *, register_t *);
int	sys_profil(struct proc *, void *, register_t *);
int	sys_setgid(struct proc *, void *, register_t *);
int	sys_getgid(struct proc *, void *, register_t *);
int	hpux_sys_ioctl(struct proc *, void *, register_t *);
int	hpux_sys_symlink(struct proc *, void *, register_t *);
int	hpux_sys_utssys(struct proc *, void *, register_t *);
int	hpux_sys_readlink(struct proc *, void *, register_t *);
int	hpux_sys_execve(struct proc *, void *, register_t *);
int	sys_umask(struct proc *, void *, register_t *);
int	sys_chroot(struct proc *, void *, register_t *);
int	hpux_sys_fcntl(struct proc *, void *, register_t *);
int	hpux_sys_ulimit(struct proc *, void *, register_t *);
int	hpux_sys_vfork(struct proc *, void *, register_t *);
int	hpux_sys_mmap(struct proc *, void *, register_t *);
int	sys_munmap(struct proc *, void *, register_t *);
int	sys_mprotect(struct proc *, void *, register_t *);
int	sys_getgroups(struct proc *, void *, register_t *);
int	sys_setgroups(struct proc *, void *, register_t *);
int	hpux_sys_getpgrp2(struct proc *, void *, register_t *);
int	hpux_sys_setpgrp2(struct proc *, void *, register_t *);
int	sys_setitimer(struct proc *, void *, register_t *);
int	hpux_sys_wait3(struct proc *, void *, register_t *);
int	sys_getitimer(struct proc *, void *, register_t *);
int	sys_dup2(struct proc *, void *, register_t *);
int	hpux_sys_fstat(struct proc *, void *, register_t *);
int	sys_select(struct proc *, void *, register_t *);
int	sys_fsync(struct proc *, void *, register_t *);
int	hpux_sys_sigvec(struct proc *, void *, register_t *);
int	hpux_sys_sigblock(struct proc *, void *, register_t *);
int	hpux_sys_sigsetmask(struct proc *, void *, register_t *);
int	hpux_sys_sigpause(struct proc *, void *, register_t *);
int	compat_43_sys_sigstack(struct proc *, void *, register_t *);
int	sys_gettimeofday(struct proc *, void *, register_t *);
int	hpux_sys_readv(struct proc *, void *, register_t *);
int	hpux_sys_writev(struct proc *, void *, register_t *);
int	sys_settimeofday(struct proc *, void *, register_t *);
int	sys_fchown(struct proc *, void *, register_t *);
int	sys_fchmod(struct proc *, void *, register_t *);
int	sys_setresuid(struct proc *, void *, register_t *);
int	sys_setresgid(struct proc *, void *, register_t *);
int	hpux_sys_rename(struct proc *, void *, register_t *);
int	hpux_sys_truncate(struct proc *, void *, register_t *);
int	compat_43_sys_ftruncate(struct proc *, void *, register_t *);
int	hpux_sys_sysconf(struct proc *, void *, register_t *);
int	hpux_sys_mkdir(struct proc *, void *, register_t *);
int	hpux_sys_rmdir(struct proc *, void *, register_t *);
int	hpux_sys_getrlimit(struct proc *, void *, register_t *);
int	hpux_sys_setrlimit(struct proc *, void *, register_t *);
int	hpux_sys_rtprio(struct proc *, void *, register_t *);
int	hpux_sys_lockf(struct proc *, void *, register_t *);
#ifdef SYSVSEM
int	sys_semget(struct proc *, void *, register_t *);
int	sys___semctl(struct proc *, void *, register_t *);
int	sys_semop(struct proc *, void *, register_t *);
#else
#endif
#ifdef SYSVMSG
int	sys_msgget(struct proc *, void *, register_t *);
int	sys_msgctl(struct proc *, void *, register_t *);
int	sys_msgsnd(struct proc *, void *, register_t *);
int	sys_msgrcv(struct proc *, void *, register_t *);
#else
#endif
#ifdef SYSVSHM
int	sys_shmget(struct proc *, void *, register_t *);
int	hpux_sys_shmctl(struct proc *, void *, register_t *);
int	sys_shmat(struct proc *, void *, register_t *);
int	sys_shmdt(struct proc *, void *, register_t *);
#else
#endif
int	hpux_sys_getcontext(struct proc *, void *, register_t *);
int	hpux_sys_sigprocmask(struct proc *, void *, register_t *);
int	hpux_sys_sigpending(struct proc *, void *, register_t *);
int	hpux_sys_sigsuspend(struct proc *, void *, register_t *);
int	hpux_sys_sigaction(struct proc *, void *, register_t *);
int	hpux_sys_waitpid(struct proc *, void *, register_t *);
int	hpux_sigsetreturn(struct proc *, void *, register_t *);
int	sys_poll(struct proc *, void *, register_t *);
int	sys_fchdir(struct proc *, void *, register_t *);
int	compat_43_sys_accept(struct proc *, void *, register_t *);
int	sys_bind(struct proc *, void *, register_t *);
int	sys_connect(struct proc *, void *, register_t *);
int	sys_getpeername(struct proc *, void *, register_t *);
int	sys_getsockname(struct proc *, void *, register_t *);
int	sys_getsockopt(struct proc *, void *, register_t *);
int	sys_listen(struct proc *, void *, register_t *);
int	compat_43_sys_recv(struct proc *, void *, register_t *);
int	compat_43_sys_recvfrom(struct proc *, void *, register_t *);
int	compat_43_sys_recvmsg(struct proc *, void *, register_t *);
int	compat_43_sys_send(struct proc *, void *, register_t *);
int	compat_43_sys_sendmsg(struct proc *, void *, register_t *);
int	sys_sendto(struct proc *, void *, register_t *);
int	sys_setsockopt(struct proc *, void *, register_t *);
int	sys_shutdown(struct proc *, void *, register_t *);
int	sys_socket(struct proc *, void *, register_t *);
int	sys_socketpair(struct proc *, void *, register_t *);
#ifdef SYSVSEM
int	sys___semctl(struct proc *, void *, register_t *);
#else
#endif
#ifdef SYSVMSG
int	sys_msgctl(struct proc *, void *, register_t *);
#else
#endif
#ifdef SYSVSHM
int	hpux_sys_nshmctl(struct proc *, void *, register_t *);
#else
#endif
int	sys_lchown(struct proc *, void *, register_t *);
int	sys_nanosleep(struct proc *, void *, register_t *);
int	hpux_sys_sigaltstack(struct proc *, void *, register_t *);
