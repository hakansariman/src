#	$Sendmail: IRIX.5.x,v 8.11 1999/07/24 23:37:45 gshapiro Exp $
define(`confCC', `cc -mips2')
define(`confMAPDEF', `-DNDBM -DNIS')
define(`confENVDEF', `-DIRIX5')
define(`confLIBS', `-lmld -lmalloc')
define(`confMBINDIR', `/usr/lib')
define(`confSBINDIR', `/usr/etc')
define(`confUBINDIR', `/usr/bsd')
define(`confEBINDIR', `/usr/lib')
define(`confSBINGRP', `sys')
define(`confSTDIR', `/var')
define(`confINSTALL', `${BUILDBIN}/install.sh')
define(`confDEPEND_TYPE', `CC-M')
