#! /usr/bin/perl
# ex:ts=8 sw=4:
# $OpenBSD: PkgCreate.pm,v 1.5 2010/06/06 11:00:40 espie Exp $
#
# Copyright (c) 2003-2010 Marc Espie <espie@openbsd.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use strict;
use warnings;

use OpenBSD::AddCreateDelete;

package OpenBSD::PkgCreate::State;
our @ISA = qw(OpenBSD::AddCreateDelete::State);

sub init
{
	my $self = shift;

	$self->{stash} = {};
	$self->SUPER::init(@_);
	$self->{simple_status} = 0;
}

sub stash
{
	my ($self, $key) = @_;
	return $self->{stash}{$key};
}

sub error
{
	my $self = shift;
	$self->{bad}++;
	$self->errsay(@_);
}

sub set_status
{
	my ($self, $status) = @_;
	if ($self->{simple_status}) {
		print "\n$status";
	} else {
		if ($self->progress->set_header($status)) {
			$self->progress->message;
		} else {
			$| = 1;
			print "$status...";
			$self->{simple_status} = 1;
		}
	}
}

sub end_status
{
	my $self = shift;

	if ($self->{simple_status}) {
		print "\n";
	} else {
		$self->progress->clear;
	}
}

package OpenBSD::PkgCreate;

use OpenBSD::PackingList;
use OpenBSD::PackageInfo;
use OpenBSD::Getopt;
use OpenBSD::Temp;
use OpenBSD::Error;
use OpenBSD::Ustar;
use OpenBSD::ArcCheck;
use OpenBSD::Paths;
use File::Basename;

# Extra stuff needed to archive files
package OpenBSD::PackingElement;
sub create_package
{
	my ($self, $state) = @_;

	$self->archive($state);
	if ($state->verbose) {
		$self->comment_create_package;
	}
}

sub pretend_to_archive
{
	my ($self, $state) = @_;
	$self->comment_create_package;
}

sub archive {}
sub comment_create_package {}

sub print_file {}

sub avert_duplicates_and_other_checks
{
	my ($self, $state) = @_;
	return unless $self->NoDuplicateNames;
	my $n = $self->fullname;
	if (defined $state->stash($n)) {
		$state->error("Error in packing-list: duplicate item $n");
	}
	$state->{stash}{$n} = 1;
}

sub makesum_plist
{
	my ($self, $plist, $state) = @_;
	$self->add_object($plist);
}

sub verify_checksum
{
}

sub compute_checksum
{
	my ($self, $result, $state, $base) = @_;
	my $name = $self->fullname;
	my $fname = $name;
	if (defined $base) {
		$fname = $base.$fname;
	}

	if (-l $fname) {
		my $value = readlink $fname;
		$result->make_symlink($value);
	} elsif (-f _) {
		my ($dev, $ino, $size) = (stat _)[0,1,7];
		if (defined $state->stash("$dev/$ino")) {
			$result->make_hardlink($state->stash("$dev/$ino"));
		} else {
			$state->{stash}{"$dev/$ino"} = $name;
			$result->add_digest($self->compute_digest($fname));
			$result->add_size($size);
		}
	} else {
		$state->error("Error in package: $fname does not exist");
	}
}

sub makesum_plist_with_base
{
	my ($self, $plist, $state, $base) = @_;
	$self->compute_checksum($self, $state, $base);
	$self->add_object($plist);
}

sub verify_checksum_with_base
{
	my ($self, $state, $base) = @_;
	my $check = ref($self)->new($self->name);
	$self->compute_checksum($check, $state, $base);

	for my $field (qw(symlink link size)) {  # md5
		if ((defined $check->{$field} && defined $self->{$field} &&
		    $check->{$field} ne $self->{$field}) ||
		    (defined $check->{$field} xor defined $self->{$field})) {
		    	$state->error("Error: $field inconsistency for ",
			    $self->fullname);
		}
	}
	if ((defined $check->{d} && defined $self->{d} &&
	    !$check->{d}->equals($self->{d})) ||
	    (defined $check->{d} xor defined $self->{d})) {
	    	$state->error("Error: checksum inconsistency for ",
		    $self->fullname);
	}
}


sub prepare_for_archival
{
	my ($self, $state) = @_;

	my $o = $state->{archive}->prepare_long($self);
	if (!$o->verify_modes($self)) {
		$state->error("Modes don't match");
	}
	return $o;
}

sub copy_over
{
}
package OpenBSD::PackingElement::SpecialFile;
sub archive
{
	&OpenBSD::PackingElement::FileBase::archive;
}

sub pretend_to_archive
{
	&OpenBSD::PackingElement::FileBase::pretend_to_archive;
}

sub comment_create_package
{
	my ($self) = @_;
	print "Adding ", $self->name, "\n";
}

sub makesum_plist
{
	my ($self, $plist, $state) = @_;
	$self->makesum_plist_with_base($plist, $state, undef);
}

sub verify_checksum
{
	my ($self, $state) = @_;
	$self->verify_checksum_with_base($state, undef);
}

sub prepare_for_archival
{
	my ($self, $state) = @_;

	my $o = $state->{archive}->prepare_long($self);
	$o->{uname} = 'root';
	$o->{gname} = 'wheel';
	$o->{uid} = 0;
	$o->{gid} = 0;
	$o->{mode} &= 0555; # zap all write and suid modes
	return $o;
}

sub copy_over
{
	my ($self, $wrarc, $rdarc) = @_;
	$wrarc->destdir($rdarc->info);
	my $e = $wrarc->prepare($self->{name});
	$e->write;
}

# override for CONTENTS: we cannot checksum this.
package OpenBSD::PackingElement::FCONTENTS;
sub makesum_plist
{
}

sub verify_checksum
{
}


package OpenBSD::PackingElement::Cwd;
sub archive
{
	my ($self, $state) = @_;
	$state->{archive}->destdir($state->{base}."/".$self->name);
}

sub pretend_to_archive
{
	my ($self, $state) = @_;
	$state->{archive}->destdir($state->{base}."/".$self->name);
	$self->comment_create_package;
}

sub comment_create_package
{
	my ($self) = @_;
	print "Cwd: ", $self->name, "\n";
}

package OpenBSD::PackingElement::FileBase;

sub archive
{
	my ($self, $state) = @_;

	my $o = $self->prepare_for_archival($state);

	$o->write unless $state->{bad};
}

sub pretend_to_archive
{
	my ($self, $state) = @_;

	$self->prepare_for_archival($state);
	$self->comment_create_package;
}

sub comment_create_package
{
	my ($self) = @_;
	print "Adding ", $self->name, "\n";
}

sub print_file
{
	my ($item) = @_;
	print '@', $item->keyword, " ", $item->fullname, "\n";
}

sub makesum_plist
{
	my ($self, $plist, $state) = @_;
	$self->makesum_plist_with_base($plist, $state, $state->{base});
}

sub verify_checksum
{
	my ($self, $state) = @_;
	$self->verify_checksum_with_base($state, $state->{base});
}

sub copy_over
{
	my ($self, $wrarc, $rdarc) = @_;
	my $e = $rdarc->next;
	if (!$e->check_name($self)) {
		die "Names don't match: ", $e->{name}, " ", $self->{name};
	}
	$e->copy_long($wrarc);
}

package OpenBSD::PackingElement::InfoFile;
sub makesum_plist
{
	my ($self, $plist, $state) = @_;
	$self->SUPER::makesum_plist($plist, $state);
	my $fname = $self->fullname;
	for (my $i = 1; ; $i++) {
		if (-e "$state->{base}/$fname-$i") {
			my $e = OpenBSD::PackingElement::File->add($plist, $self->name."-".$i);
			$e->compute_checksum($e, $state, $state->{base});
		} else {
			last;
		}
	}
}

package OpenBSD::PackingElement::Manpage;
sub makesum_plist
{
	my ($self, $plist, $state) = @_;
	if ($state->{subst}->empty("USE_GROFF") || !$self->is_source) {
		return $self->SUPER::makesum_plist($plist, $state);
	}
	my $dest = $self->source_to_dest;
	$self->format($state->{base}, $self->cwd."/".$dest);
	my $e = OpenBSD::PackingElement::Manpage->add($plist, $dest);
	$e->compute_checksum($e, $state, $state->{base});
}

package OpenBSD::PackingElement::Depend;
sub avert_duplicates_and_other_checks
{
	my ($self, $state) = @_;
	if (!$self->spec->is_valid) {
		$state->error("Error in packing-list: invalid \@",
		    $self->keyword, " ", $self->stringize);
	}
	$self->SUPER::avert_duplicates_and_other_checks($state);
}

package OpenBSD::PackingElement::Conflict;
sub avert_duplicates_and_other_checks
{
	&OpenBSD::PackingElement::Depend::avert_duplicates_and_other_checks;
}

package OpenBSD::PackingElement::AskUpdate;
sub avert_duplicates_and_other_checks
{
	&OpenBSD::PackingElement::Depend::avert_duplicates_and_other_checks;
}

package OpenBSD::PackingElement::Dependency;
sub avert_duplicates_and_other_checks
{
	my ($self, $state) = @_;

	my @issues = OpenBSD::PackageName->from_string($self->{def})->has_issues;
	if (@issues > 0) {
		$state->error("Error in packing-list: invalid \@",
		    $self->keyword, " ", $self->stringize, "\n",
		    "$self->{def}: , ", join(' ', @issues));
	}

	$self->SUPER::avert_duplicates_and_other_checks($state);
}

package OpenBSD::PackingElement::Name;
sub avert_duplicates_and_other_checks
{
	my ($self, $state) = @_;

	my @issues = OpenBSD::PackageName->from_string($self->name)->has_issues;
	if (@issues > 0) {
		$state->error("Bad packagename ", $self->name, ":",
		    join(' ', @issues));
	}
	$self->SUPER::avert_duplicates_and_other_checks($state);
}

# put together file and filename, in order to handle fragments simply
package MyFile;
sub new
{
	my ($class, $filename) = @_;

	open(my $fh, '<', $filename) or die "Missing file $filename";

	bless { fh => $fh, name => $filename }, $class;
}

sub readline
{
	my $self = shift;
	return readline $self->{fh};
}

sub name
{
	my $self = shift;
	return $self->{name};
}

sub close
{
	my $self = shift;
	close($self->{fh});
}

package OpenBSD::PkgCreate;
our @ISA = qw(OpenBSD::AddCreateDelete);

sub deduce_name
{
	my ($state, $o, $frag, $not) = @_;

	my $noto = $o;
	my $nofrag = "no-$frag";

	$o =~ s/PFRAG\./PFRAG.$frag-/o or
	    $o =~ s/PLIST/PFRAG.$frag/o;

	$noto =~ s/PFRAG\./PFRAG.no-$frag-/o or
	    $noto =~ s/PLIST/PFRAG.no-$frag/o;
	unless (-e $o or -e $noto) {
		die "Missing fragments for $frag: $o and $noto don't exist";
	}
	if ($not) {
		print "Switching to $noto\n" if !defined $state->opt('q');
		return $noto if -e $noto;
    	} else {
		print "Switching to $o\n" if !defined $state->opt('q');
		return $o if -e $o;
	}
	return;
}

sub read_fragments
{
	my ($state, $plist, $filename) = @_;

	my $stack = [];
	my $subst = $state->{subst};
	push(@$stack, MyFile->new($filename));

	return $plist->read($stack,
	    sub {
		my ($stack, $cont) = @_;
		local $_;
		while(my $file = pop @$stack) {
			GETLINE:
			while ($_ = $file->readline) {
				$state->progress->working(2048);
				if (my ($not, $frag) = m/^(\!)?\%\%(.*)\%\%$/) {
					my $def = $frag;
					if ($frag eq 'SHARED') {
						$def = 'SHARED_LIBS';
						$frag = 'shared';
					}
					if ($subst->has_fragment($def, $frag)) {
						next GETLINE if defined $not;
					} else {
						next GETLINE unless defined $not;
					}
					my $newname = deduce_name($state, $file->name, $frag, $not);
					if (defined $newname) {
						push(@$stack, $file);
						$file = MyFile->new($newname);
					}
					next GETLINE;
				}
				if (m/^(\@comment\s+\$(?:Open)BSD\$)$/o) {
					$_ = '@comment $'.'OpenBSD: '.basename($file->name).',v$';
				}
				if (m/^\@lib\s+(.*)$/o &&
				    OpenBSD::PackingElement::Lib->parse($1)) {
				    	$state->error("Shared library without SHARED_LIBS: $_");
				}
				&$cont($subst->do($_));
			}
		}
	    }
	);
}

sub add_special_file
{
	my ($subst, $plist, $name, $opt) = @_;
	if (defined $opt) {
	    my $o = OpenBSD::PackingElement::File->add($plist, $name);
	    $subst->copy($opt, $o->fullname) if defined $o->fullname;
	}
}

sub add_description
{
	my ($subst, $plist, $name, $opt_d) = @_;
	my $o = OpenBSD::PackingElement::FDESC->add($plist, $name);
	my $comment = $subst->value('COMMENT');
	if (defined $comment) {
		if (length $comment > 60) {
			print STDERR "Error: comment is too long\n";
			print STDERR $comment, "\n";
			print STDERR ' 'x60, "^"x (length($comment)-60), "\n";
			exit 1;
		}
	} else {
		Usage "Comment required";
	}
	if (!defined $opt_d) {
		Usage "Description required";
	}
	if (defined $o->fullname) {
	    open(my $fh, '>', $o->fullname) or die "Can't write to DESC: $!";
	    if (defined $comment) {
	    	print $fh $subst->do($comment), "\n";
	    }
	    if ($opt_d =~ /^\-(.*)$/o) {
		print $fh $1, "\n";
	    } else {
		$subst->copy_fh($opt_d, $fh);
	    }
	    if (defined $comment) {
		if ($subst->empty('MAINTAINER')) {
			Warn "no MAINTAINER\n";
		} else {
			print $fh "\n", $subst->do('Maintainer: ${MAINTAINER}'), "\n";
		}
		if (!$subst->empty('HOMEPAGE')) {
			print $fh "\n", $subst->do('WWW: ${HOMEPAGE}'), "\n";
		}
	    }
	    close($fh);
	}
}

sub add_signature
{
	my ($self, $plist, $cert, $privkey) = @_;
		
	require OpenBSD::x509;

	my $sig = OpenBSD::PackingElement::DigitalSignature->new_x509;
	$sig->add_object($plist);
	$sig->{b64sig} = OpenBSD::x509::compute_signature($plist,
	    $cert, $privkey);
}

sub create_archive
{
	my ($self, $filename, $dir) = @_;
	open(my $fh, "|-", OpenBSD::Paths->gzip, "-f", "-o", $filename);
	return  OpenBSD::Ustar->new($fh, $dir);
}

sub sign_existing_package
{
	my ($self, $pkgname, $cert, $privkey) = @_;

	require OpenBSD::PackageLocator;

	my $true_package = OpenBSD::PackageLocator->find($pkgname);
	die "No such package $pkgname" unless $true_package;
	my $dir = $true_package->info;
	my $plist = OpenBSD::PackingList->fromfile($dir.CONTENTS);
	$plist->set_infodir($dir);
	$self->add_signature($plist, $cert, $privkey);
	$plist->save;
	my $tmp = OpenBSD::Temp::permanent_file(".", "pkg");
	my $wrarc = $self->create_archive($tmp, ".");
	$plist->copy_over($wrarc, $true_package);
	$wrarc->close;
	$true_package->wipe_info;
	unlink($plist->pkgname.".tgz");
	rename($tmp, $plist->pkgname.".tgz") or
	    die "Can't create final signed package $!\n";
}

sub add_extra_info
{
	my ($self, $plist, $subst) = @_;

	my $fullpkgpath = $subst->value('FULLPKGPATH');
	my $cdrom = $subst->value('PERMIT_PACKAGE_CDROM') ||
	    $subst->value('CDROM');;
	my $ftp = $subst->value('PERMIT_PACKAGE_FTP') ||
	    $subst->value('FTP');
	if (defined $fullpkgpath || defined $cdrom || defined $ftp) {
		$fullpkgpath //= '';
		$cdrom //= 'no';
		$ftp //= 'no';
		$cdrom = 'yes' if $cdrom =~ m/^yes$/io;
		$ftp = 'yes' if $ftp =~ m/^yes$/io;

		OpenBSD::PackingElement::ExtraInfo->add($plist,
		    $fullpkgpath, $cdrom, $ftp);
	} else {
		Warn "Package without FULLPKGPATH\n";
	}
}

sub add_elements
{
	my ($self, $plist, $state, $dep, $want) = @_;

	my $subst = $state->{subst};
	add_description($subst, $plist, DESC, $state->opt('d'));
	add_special_file($subst, $plist, DISPLAY, $state->opt('M'));
	add_special_file($subst, $plist, UNDISPLAY, $state->opt('U'));
	if (defined $state->opt('p')) {
		OpenBSD::PackingElement::Cwd->add($plist, $state->opt('p'));
	} else {
		Usage "Prefix required";
	}
	for my $d (sort keys %$dep) {
		OpenBSD::PackingElement::Dependency->add($plist, $d);
	}

	for my $w (sort keys %$want) {
		OpenBSD::PackingElement::Wantlib->add($plist, $w);
	}

	if (defined $state->opt('A')) {
		OpenBSD::PackingElement::Arch->add($plist, $state->opt('A'));
	}

	if (defined $state->opt('L')) {
		OpenBSD::PackingElement::LocalBase->add($plist, $state->opt('L'));
	}
	$self->add_extra_info($plist, $subst);
}

sub create_plist
{
	my ($self, $state, $pkgname, $frags, $dep, $want) = @_;

	my $plist = OpenBSD::PackingList->new;

	if ($pkgname =~ m|([^/]+)$|o) {
		$pkgname = $1;
		$pkgname =~ s/\.tgz$//o;
	}
	$plist->set_pkgname($pkgname);
	print "Creating package $pkgname\n" 
	    if !(defined $state->opt('q')) && $state->opt('v');
	if (!$state->opt('q')) {
		$plist->set_infodir(OpenBSD::Temp->dir);
	}

	$self->add_elements($plist, $state, $dep, $want);
	unless (defined $state->opt('q') && defined $state->opt('n')) {
		$state->set_status("reading plist");
	}
	for my $contentsfile (@$frags) {
		read_fragments($state, $plist, $contentsfile) or
		    Fatal "Can't read packing list $contentsfile";
	}
	return $plist;
}

sub make_plist_with_sum
{
	my ($self, $state, $plist) = @_;
	my $p2 = OpenBSD::PackingList->new;
	$state->progress->visit_with_count($plist, 'makesum_plist', $p2, $state);
	$p2->set_infodir($plist->infodir);
	return $p2;
}

sub read_existing_plist
{
	my ($self, $contents) = @_;

	my $plist = OpenBSD::PackingList->new;
	if (-d $contents && -f $contents.'/'.CONTENTS) {
		$plist->set_infodir($contents);
		$contents .= '/'.CONTENTS;
	} else {
		$plist->set_infodir(dirname($contents));
	}
	$plist->fromfile($contents) or
	    Fatal "Can't read packing list $contents";
	return $plist;
}

sub create_package
{
	my ($self, $state, $plist, $wname) = @_;

	print "Creating gzip'd tar ball in '$wname'\n" if $state->opt('v');
	my $h = sub {
		unlink $wname;
		my $caught = shift;
		$SIG{$caught} = 'DEFAULT';
		kill $caught, $$;
	};

	local $SIG{'INT'} = $h;
	local $SIG{'QUIT'} = $h;
	local $SIG{'HUP'} = $h;
	local $SIG{'KILL'} = $h;
	local $SIG{'TERM'} = $h;
	$state->{archive} = $self->create_archive($wname, $plist->infodir);
	$state->set_status("archiving");
	$state->progress->visit_with_size($plist, 'create_package', $state);
	$state->end_status;
	$state->{archive}->close;
	if ($state->{bad}) {
		unlink($wname);
		exit(1);
	}
}

sub parse_and_run
{
	my $self = shift;

	my ($cert, $privkey);
	my $regen_package = 0;
	my $sign_only = 0;
	my (@contents, %dependencies, %wantlib, @signature_params);


	my $state = OpenBSD::PkgCreate::State->new;

	$state->{opt} = {
	    'f' =>
		    sub {
			    push(@contents, shift);
		    },
	    'P' => sub {
			    my $d = shift;
			    $dependencies{$d} = 1;
		    },
	    'W' => sub {
			    my $w = shift;
			    $wantlib{$w} = 1;
		    },
	    's' => sub {
			    push(@signature_params, shift);
		    }
	};
	$self->handle_options('p:f:d:M:U:s:A:L:B:P:W:qQ', $state,
	    'pkg_create [-nQqvx] [-A arches] [-B pkg-destdir] [-D name[=value]]',
	    '[-L localbase] [-M displayfile] [-P pkg-dependency]',
	    '[-s x509 -s cert -s priv] [-U undisplayfile] [-W wantedlib]',
	    '-d desc -D COMMENT=value -f packinglist -p prefix pkg-name');

	if (@ARGV == 0) {
		$regen_package = 1;
	} elsif (@ARGV != 1) {
		if (@contents || @signature_params == 0) {
			Usage "Exactly one single package name is required: ",
			    join(' ', @ARGV);
		}
	}

	try {
	if (@signature_params > 0) {
		if (@signature_params != 3 || $signature_params[0] ne 'x509' ||
		    !-f $signature_params[1] || !-f $signature_params[2]) {
			Usage "Signature only works as -s x509 -s cert -s privkey";
		}
		$cert = $signature_params[1];
		$privkey = $signature_params[2];
	}

	if (defined $state->opt('Q')) {
		$state->{opt}{q} = 1;
	}

	if (!@contents) {
		if (@signature_params > 0) {
			$sign_only = 1;
		} else {
			Usage "Packing list required";
		}
	}

	my $plist;
	if ($regen_package) {
		if (@contents != 1) {
			Usage "Exactly one single packing list is required";
		}
		$plist = $self->read_existing_plist($contents[0]);
	} elsif ($sign_only) {
		if ($state->not) {
			Fatal "Can't pretend to sign existing packages";
		}
		for my $pkgname (@ARGV) {
			$self->sign_existing($pkgname, $cert, $privkey);
		}
		exit(0);
	} else {
		$plist = $self->create_plist($state, $ARGV[0], \@contents, 
		    \%dependencies, \%wantlib);
	}


	my $base = '/';
	if (defined $state->opt('B')) {
		$base = $state->opt('B');
	} elsif (defined $ENV{'PKG_PREFIX'}) {
		$base = $ENV{'PKG_PREFIX'};
	}

	$state->{base} = $base;

	unless (defined $state->opt('q') && defined $state->opt('n')) {
		$state->set_status("checksumming");
		if ($regen_package) {
			$state->progress->visit_with_count($plist, 'verify_checksum', $state);
		} else {
			$plist = $self->make_plist_with_sum($state, $plist);
		}
		$state->end_status;
	}

	if (!defined $plist->pkgname) {
		$state->error("Can't write unnamed packing list");
		exit 1;
	}

	if (defined $state->opt('q')) {
		if (defined $state->opt('Q')) {
			$plist->print_file;
		} else {
			$plist->write(\*STDOUT);
		}
		exit 0 if defined $state->opt('n');
	}

	if ($plist->{deprecated}) {
		$state->error("Error: found obsolete constructs");
		exit 1;
	}

	$plist->avert_duplicates_and_other_checks($state);
	$state->{stash} = {};

	if ($state->{bad} && $state->{subst}->empty('REGRESSION_TESTING')) {
		exit 1;
	}
	$state->{bad} = 0;

	if (defined $cert) {
		$self->add_signature($plist, $cert, $privkey);
		$plist->save if $regen_package;
	}

	my $wname;
	if ($regen_package) {
		$wname = $plist->pkgname.".tgz";
	} else {
		$plist->save or Fatal "Can't write packing list";
		$wname = $ARGV[0];
	}

	if ($state->opt('n')) {
		$state->{archive} = OpenBSD::Ustar->new(undef, $plist->infodir);
		$plist->pretend_to_archive($state);
	} else {
		$self->create_package($state, $plist, $wname);
	} 
	}catch {
		print STDERR "$0: $_\n";
		exit(1);
	};
}

1;