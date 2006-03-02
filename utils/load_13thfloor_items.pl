#! /usr/bin/perl

use DBI;
use Getopt::Std;

getopts('d:h');
$dbini = "db.ini";
if($opt_h) {
	die "Usage: serialize_items.pl [-d path/to/db.ini]\n";
}
if($opt_d) {
	$dbini = $opt_d;
}

$db = "";
$user = "";
$pass = "";
$host = "";
open(F, "<$dbini") or die "Unable to open database config $dbini\n";
while(<F>) {
	s/\r//g;
	if(/host\s*=\s*(.*)/) {
		$host = $1;
	} elsif(/user\s*=\s*(.*)/) {
		$user = $1;
	} elsif(/password\s*=\s*(.*)/) {
		$pass = $1;
	} elsif(/database\s*=\s*(.*)/) {
		$db = $1;
	}
}
if(!$db || !$user || !$pass || !$host) {
	die "Invalid db.ini, missing one of: host, user, password, database\n";
}

$source="DBI:mysql:database=$db;host=$host";

my $dbh = DBI->connect($source, $user, $pass) || die "Could not create db handle\n";



$_=<STDIN>;
chomp();
s/'/\\'/g;
@fields=split("(?<!\\\\)\\|", $_);

%conversions = (
	"itemtype" => "itemuse"
);

$insert="insert into items (".join(",",@fields).",source,updated,serialization,serialized) values ('";

#select(STDOUT); $|=1;
while(<STDIN>) {
	chomp();
	s/'/\\'/g;
	@f=split("(?<!\\\\)\\|", $_);
	$insert2=join("','",@f);
	$#f--;
	grep(s/\\\|/\\\\\|/g,@f);
	grep(s/"/\\\\"/g,@f);
	$statement=sprintf("%s%s','13THFLOOR',now(),'%s',now())",$insert,$insert2,join('|',@f));
	$dbh->do($statement);
	printf("Processing: %d %s                        \r",$f[4],$f[1]);
	++$count;
}
printf("Processed: %d items(s)                     \n",$count);

