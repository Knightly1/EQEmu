#! /usr/bin/perl

use DBI;
use Getopt::Std;

getopts('u:h:p:d:');
if (!$opt_d || !$opt_p || !$opt_u) {
	die "Usage:\n\tload_13thfloor_items.pl -d db -u user -p pass [-h host]\n";
}
$source="DBI:mysql:database=$opt_d";
$source.=";host=$opt_h" if ($opt_h);

my $dbh = DBI->connect($source, $opt_u, $opt_p) || die "Could not create db handle\n";


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

