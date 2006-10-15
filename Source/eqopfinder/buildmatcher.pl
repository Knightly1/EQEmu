#!/usr/bin/perl -W

my %opstructs = ();

open(F, "<../common/opcode_dispatch.h") || die "Unable to open opcode to struct map (../common/opcode_dispatch.h)\n";
while(<F>) {
	chomp;
	my $mode;
	my $op;
	my $struct;
	if(/^([a-zA-Z_]+)\(\s*(OP_[a-zA-Z0-9_]+)\s*\);/) {
		$mode = $1;
		$op = $2;
		$struct = "0";
	} elsif(/^([a-zA-Z_]+)\(\s*(OP_[a-zA-Z0-9_]+)\s*,\s*([a-zA-Z0-9_]+)\s*\);/) {
		$mode = $1;
		$op = $2;
		$struct = $3;
	} else {
		next;
	}
	
	#skip raw packets, cant do anything with them
	next if($mode =~ /r$/);
	
	if(defined($structs{$op})) {
		my $a = $structs{$op};
		#skip the struct if we allready have it
		my $f = 0;
		foreach my $e(@{$a}) {
			if($e->[1] eq $struct) {
				$f = 1;
				last;
			}
		}
		if($f == 0)
			push(@{$a}, [$mode, $struct]);
	} else {
		$structs{$op} = [ [$mode, $struct] ];
	}
}
close(F);

foreach my $op(keys(%structs)) {
	my @a = @{$structs{$s}};
	my $code;
	if(@a == 1) {
		#single entry
		my $e = $a[0];
		my $cc = GenerateMatch($e, "");
		$code = "bool Match_$op(const SMPacket *p) {\n$cc\n\treturn(false);\n}\n\n\n";
	} else {
		#multiple structs
	}
}

sub GenerateMatch {
	my $entry = shift;
	my $i = shift;
	my $mode = $entry->[0];
	my $struct = $entry->[1];
	my $r = "";
	
	if($mode =~ /v$/) {
		$r .= "${i}if(p->size < sizeof($struct))\n${i}\treturn(false);\n";
	} elsif($mode =~ /z$/) {
		$r .= "${i}if(p->size != 0)\n${i}\treturn(false);\n";
	} else {
		$r .= "${i}if(p->size != sizeof($struct))\n${i}\treturn(false);\n";
	}
	
	
	
	return($r);
}
