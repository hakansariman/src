#	$Sendmail: NCR.MP-RAS.2.x,v 8.13 1999/06/02 22:53:39 gshapiro Exp $
define(`confMAPDEF', `-DNDBM')
define(`confENVDEF', `-DNCR_MP_RAS2')
define(`confOPTIMIZE', `-O2')
APPENDDEF(`confINCDIRS', `-I/usr/include -I/usr/ucbinclude')
define(`confLIBDIRS', `-L/usr/ucblib')
define(`confLIBS', `-lnsl -lnet -lsocket -lelf -lc -lucb')
define(`confMBINDIR', `/usr/ucblib')
define(`confSBINDIR', `/usr/ucbetc')
define(`confUBINDIR', `/usr/ucb')
define(`confEBINDIR', `/usr/ucblib')
define(`confSTDIR', `/var/ucblib')
define(`confINSTALL', `/usr/ucb/install')
define(`confDEPEND_TYPE', `NCR')
