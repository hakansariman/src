#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/mkfifo/01.t,v 1.1 2007/01/17 01:42:09 pjd Exp $

desc="mkfifo returns ENOTDIR if a component of the path prefix is not a directory"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
expect 0 create ${n0}/${n1} 0644
expect ENOTDIR mkfifo ${n0}/${n1}/test 0644
expect 0 unlink ${n0}/${n1}
expect 0 rmdir ${n0}
