# test reverse sending with non-blocking relay

use strict;
use warnings;

our %args = (
    client => {
	func => \&read_char,
    },
    relay => {
	func => sub { ioflip(@_); relay(@_); },
	nonblocking => 1,
    },
    server => {
	func => \&write_char,
    },
    len => 251,
    md5 => "bc3a3f39af35fe5b1687903da2b00c7f",
);

1;
