#!/usr/bin/perl -W

%consts = (
);
%lengths = (
	bool => 1,
	char => 1,
	int8 => 1,
	sint8 => 1,
	uint8 => 1,
	int16 => 2,
	sint16 => 2,
	uint16 => 2,
	int32 => 4,
	sint32 => 4,
	uint32 => 4,
	float => 4,
	int64 => 8,
	sint64 => 8,
	uint64 => 8,
	double => 8,

	time_t => 4,
	ItemPacketType => 4
);
require "lengths.pl";
%ignore = ("DyeStruct" => 1, "Color_Struct" => 1);

my %code = ();
my %opstructs = ();
my $curstruct = "";
my $curcode = [];

open(F, "<../common/eq_packet_structs.h");
while(<F>) {
	chomp;
	
	if(/^\s*#define\s+([a-zA-Z0-9_]+)\s+(-?[0-9]+)/) {
		$consts{$1} = $2;
		next;
	}
	if(/^struct ([a-zA-Z0-9_]+)/) {
		if(!defined($lengths{$1})) {
			print "Unable to find length for $1\n";
			next;
		}
		if($curstruct) {
			print "Starting new struct inside another struct ($1 inside $curstruct)...\n";
		}
		$curstruct = $1;
		$curcode = [];
		next;
	}
	
	if(!$curstruct) {
		next;
	}
	
	#strip trailing comments to make matching better
	s/\/\/.*//g;

	if(/}\s*;/) {
		#end of struct
		$len = @{$curcode};
		if($len == 0) {
			print "Zero length struct for $curstruct\n";
		} else {
			$code{$curstruct} = $curcode;
		}
		$curstruct = "";
		$curcode = [];
	#double arrays
	} elsif(/[a-zA-Z0-9_]+\s+[a-zA-Z0-9_]+\s*\[[a-zA-Z0-9_]+\]\[[a-zA-Z0-9_]+\];/) {
		push(@{$curcode}, $_);
	# : seperated crap, first element
	} elsif(/[a-zA-Z0-9_]+\s+[a-zA-Z0-9_]+\s*:\s*[0-9]+\s*,/) {
		push(@{$curcode}, $_);
	# : seperated crap, not first element, skip
	} elsif(/[a-zA-Z0-9_]+\s*:\s*[0-9]+\s*,/) {
		next;
	} elsif(/[a-zA-Z0-9_]+\s+[a-zA-Z0-9_]+\s*\[?[a-zA-Z0-9_]*\]?;/) {
		push(@{$curcode}, $_);
	}
}
close(F);
open(F, "<../common/eq_constants.h");
while(<F>) {
	chomp;
	
	if(/^\s*#define\s+([a-zA-Z0-9_]+)\s+([-0-9])/) {
		$consts{$1} = $2;
		next;
	}
}
close(F);

open(F, "<../common/opcode_dispatch.h") || die "Unable to open opcode to struct map (../common/opcode_dispatch.h)\n";
while(<F>) {
	chomp;
	s#^//alt:##g;
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
	
	if(defined($opstructs{$op})) {
		my $a = $opstructs{$op};
		#skip the struct if we allready have it
		my $f = 0;
		foreach my $e(@{$a}) {
			if($e->[1] eq $struct) {
				$f = 1;
				last;
			}
		}
		if($f == 0) {
			push(@{$a}, [$mode, $struct]);
		}
	} else {
		$opstructs{$op} = [ [$mode, $struct] ];
	}
}
close(F);

my %structs = ();
foreach my $sn (keys(%code)) {
	next if(defined($ignore{$sn}));
	
	my @members;	#must be 'my'
#	print "Dissection of $sn:\n";
	$expect = $lengths{$sn};
	$found = 0;
	my $s = "";
	foreach my $l (@{$code{$sn}}) {
		#strip leading comments and space
		$l =~ s/^\s*\/\*.*\*\/\s*//g;
		#strip raw space
		$l =~ s/^\s+//g;
		#strip tailing crap and ;
		$l =~ s/;.*$//g;
		#strip leading 'struct'
		$l =~ s/struct\s+//g;
		
		#split it into components
		my $type = "";
		my $name = "";
		my $count = 1;
		my $count2 = 1;
		my $countv = 1;
		my $count2v = 1;
		if($l =~ /([a-zA-Z0-9_]+)\s*([a-zA-Z0-9_]+)\[([a-zA-Z0-9_]+)\]\[([a-zA-Z0-9_]+)\]/) {
			$type = $1;
			$name = $2;
			$count = $3;
			$count2 = $4;
		} elsif($l =~ /([a-zA-Z0-9_]+)\s*([a-zA-Z0-9_]+)\[([a-zA-Z0-9_]+)\]/) {
			$type = $1;
			$name = $2;
			$count = $3;
		} elsif($l =~ /([a-zA-Z0-9_]+)\s*([a-zA-Z0-9_]+)/) {
			$type = $1;
			$name = $2;
		} else {
			print "Unable to understand line '$l'\n";
		}
		if(!defined($lengths{$type})) {
			print "Unable to find length of type '$type' in line '$l'\n";
		}

		#try to understand the array size if any
		if($count =~ /^-?[0-9]+$/) {
			$countv = $count;
		} elsif($count =~ /^[a-zA-Z0-9_]+$/) {
			if(!defined($consts{$count})) {
				print "Unable to find constant '$count' for array size.\n";
				$countv = 1;	#hack.
			} else {
				$countv = $consts{$count};
			}
		} else {
			print "Unable to understand array size '$count'.\n";
			$countv = 1;	#hack.
		}
		if($count2 =~ /^-?[0-9]+$/) {
			$count2v = $count2;
		} elsif($count2 =~ /^[a-zA-Z0-9_]+$/) {
			if(!defined($consts{$count2})) {
				print "Unable to find constant '$count2' for array size.\n";
				$count2v = 1;	#hack.
			} else {
				$count2v = $consts{$count2};
			}
		} else {
			print "Unable to understand array size '$count2'.\n";
			$count2v = 1;	#hack.
		}
		
		
		my $size = $lengths{$type};
		$s .= "\tt[$type]$size, n[$name], c[$count*$count2]\n";
		$found += $size * $countv * $count2v;
		push(@members, [$type, $name, $count, $count2, $countv, $count2v]);
	}
	if($expect != $found) {
		print "$sn: expected len $expect, but calculated $found.\n";
		print "$s\n";
	}

	$structs{$sn} = \@members;
}

#first of all look for unique lengths
my %lens = ();
my %notunique = ();
foreach my $sn (keys(%structs)) {
	my $len = $lengths{$sn};
	if(defined($lens{$len})) {
		#not the first struct of this length
		$notunique{$len} = 1;
		my $list = $lens{$len};
		push(@{$list}, $sn);
	} else {
		#first struct of this length
		$lens{$len} = [ $sn, 0 ];
	}
}
#record a quick lookup hash of unique-length struct names
%unique_structs = ();
foreach my $len (keys(%lens)) {
	if(!defined($notunique{$len})) {
		#unique length
		my $a = $lens{$len};
		my $sn = $a->[0];
		$unique_structs{$sn} = 1;
		print "// $sn is of unique length $len\n";
		next;
	}
	my @list = @{$lens{$len}};
#	foreach my $sn(@list) {
#		print "$sn is NOT of unique length $len\n";
#	}
	my $count = @list;
	print "// There are $count structs with length $len\n";
}


#start generating matchers
foreach my $op(keys(%opstructs)) {
	my @a = @{$opstructs{$op}};
	my $code;
	$code = "bool StructMatcher::Match_$op(const SMPacket *p) {\n";
	my $first = 1;
	foreach my $e(@a) {
	my $mode = $e->[0];
		my $struct = $e->[1];
		if($first == 1) {
			$first = 0;
			$code .= "\t";
		} else {
			$code .= " else ";
		}
		if($mode =~ /v$/) {
			$code .= "if(p->size >= sizeof($struct)) {\n";
		} elsif($mode =~ /z$/) {
			$code .= "if(p->size == 0) {\n";
		} else {
			$code .= "if(p->size == sizeof($struct)) {\n";
		}

		$code .= GenerateMatch($e, "\t\t");
		$code .= "\t\treturn(true);\n\t}";
	}
	$code .= "\n\treturn(false);\n}\n\n";

	print $code;
}

sub GenerateMatch {
	my $entry = shift;
	my $i = shift;
	my $clen = shift;
	my $mode = $entry->[0];
	my $struct = $entry->[1];
	my $r = "";
	
	if($struct eq "0") {
		$r .= "${i}//this struct is empty, need to add history checking for it to be useful.\n";
		$r .= "${i}//CheckRecent(OP_.., 5_);\n\n${i}return(false);\n\n";
		return($r);
	}
	
	my $uniq = defined($unique_structs{$struct});
	if(defined($unique_structs{$struct})) {
#bad idea:
#		#a length check is adequate
#		$r .= "${i}//this struct's length is unique, a length check is adequate\n\n";
#		return($r);
	}

	#otherwise, generate a checker for each known field
	$r .= "${i}$struct *s = ($struct *) p->pBuffer;\n";
	
	my $members = $structs{$struct};
	foreach my $ma(@{$members}) {
		my $type = $ma->[0];
		my $name = $ma->[1];
		my $count = $ma->[2];
		my $count2 = $ma->[3];
		my $countv = $ma->[4];
		my $count2v = $ma->[5];
		
		#skip unknowns... since they are... unknown
		if($name =~ /unknown/i) {
			$r .= "${i}//$name skipped\n";
			next;
		}
		
		#figure out which checker to apply
		my $check = "//NO_MATCHER(";
		my $checkargs = "";
		if($name eq "entity_id") {
			$check = "CheckMobExists(";
		} elsif($name eq "item_id") {
			$check = "CheckItemID(";
		} elsif($type eq "float") {
			$check = "CheckFloat(";
			$checkargs = ", -1e7, 1e7, -5, 7";
		} elsif($type eq "char") {
			if($count2 > 1) {
				#array of strings
				$check = "CheckString(";
				$checkargs = ", $count2";
				$count2 = 1;
				$count2v = 1;
			} elsif($count > 1) {
				#single string
				$check = "CheckString(";
				$checkargs = ", $count";
				$count = 1;
				$countv = 1;
			} else {
				#char used for data
			}
		} elsif($type eq "bool") {
			$check = "CheckBool(";
		} elsif($type =~ /^sint[813][62]?/) {
			$check = "//CheckSigned(";
			$checkargs = ", LONG_MIN, LONG_MAX";
		} elsif($type =~ /^uint[813][62]?/) {
			$check = "//CheckUnsigned(";
			$checkargs = ", 0, ULONG_MAX";
		} elsif($type =~ /^int[813][62]?/) {
			$check = "//CheckUnsigned(";
			$checkargs = ", 0, ULONG_MAX";
		}
		
		#generate the check code
		if($countv == 1) {
			#single field
			$r .= "${i}${check}s->${name}${checkargs});\n";
		} elsif($count2v == 1) {
			#single loop
			my $iv = "idx_$name";
			$r .= "${i}uint32 $iv;\n";
			$r .= "${i}for($iv = 0; $iv < $count; $iv++) {\n";
			$r .= "${i}\t${check}s->${name}[$iv]${checkargs});\n";
			$r .= "${i}}\n";
		} else {
			#double loop
			my $iv = "idx_$name";
			$r .= "${i}uint32 $iv, _$iv;\n";
			$r .= "${i}for($iv = 0; $iv < $count; $iv++) {\n";
			$r .= "${i}\tfor(_$iv = 0; _$iv < $count2; _$iv++) {\n";
			$r .= "${i}\t\t${check}s->${name}[$iv][_$iv]${checkargs});\n";
			$r .= "${i}\t}\n";
			$r .= "${i}}\n";
		}
	}
	
	return($r);
}


