#!/usr/bin/perl

#use strict;
use DBI;
use Getopt::Std;

@fieldlist = (
	"itemclass",
	"Name",
	"lore",
	"idfile",
	"id",
	"weight",
	"norent",
	"nodrop",
	"size",
	"slots",
	"price",
	"icon",
	"UNK012",
	"UNK013",
	"benefitflag",
	"tradeskills",
	"cr",
	"dr",
	"pr",
	"mr",
	"fr",
	"astr",
	"asta",
	"aagi",
	"adex",
	"acha",
	"aint",
	"awis",
	"hp",
	"mana",
	"ac",
	"deity",
	"skillmodvalue",
	"skillmodtype",
	"banedmgrace",
	"banedmgamt",
	"banedmgbody",
	"magic",
	"casttime_",
	"reqlevel",
	"bardtype",
	"bardvalue",
	"light",
	"delay",
	"reclevel",
	"recskill",
	"elemdmgtype",
	"elemdmgamt",
	"range",
	"damage",
	"color",
	"classes",
	"races",
	"UNK053",
	"maxcharges",
	"itemtype",
	"material",
	"sellrate",
	"UNK058",
	"casttime",
	"UNK060",
	"procrate",
	"combateffects",
	"shielding",
	"stunresist",
	"strikethrough",
	"extradmgskill",
	"extradmgamt",
	"spellshield",
	"avoidance",
	"accuracy",
	"charmfileid",
	"factionmod1",
	"factionmod2",
	"factionmod3",
	"factionmod4",
	"factionamt1",
	"factionamt2",
	"factionamt3",
	"factionamt4",
	"charmfile",
	"augtype",
	"augslot1type",
	"augslot2type",
	"augslot3type",
	"augslot4type",
	"augslot5type",
	"ldontheme",
	"ldonprice",
	"ldonsold",
	"bagtype",
	"bagslots",
	"bagsize",
	"bagwr",
	"book",
	"booktype",
	"filename",
	"banedmgraceamt",
	"augrestrict",
	"loreflag",
	"pendingloreflag",
	"artifactflag",
	"summonedflag",
	"favor",
	"fvnodrop",
	"endur",
	"dotshielding",
	"attack",
	"regen",
	"manaregen",
	"haste",
	"damageshield",
	"recastdelay",
	"recasttype",
	"guildfavor",
	"augdistiller",
	"UNK116",
	"UNK117",
	"attuneable",
	"UNK119",
	"UNK120",
	"UNK121",
	"UNK122",
	"UNK123",
	"clickeffect",
	"clicktype",
	"clicklevel",
	"proceffect",
	"proctype",
	"proclevel",
	"worneffect",
	"worntype",
	"wornlevel",
	"focuseffect",
	"focustype",
	"focuslevel",
	"scrolleffect",
	"scrolltype",
	"scrolllevel"
);

getopts('u:h:p:d:');
if (!$opt_d || !$opt_p || !$opt_u) {
	die "Usage:\n\tserialize_items.pl -d db -u user -p pass [-h host]\n";
}
$source="DBI:mysql:database=$opt_d";
$source.=";host=$opt_h" if ($opt_h);

my $dbh = DBI->connect($source, $opt_u, $opt_p) || die "Could not create db handle\n";

select(STDOUT); $|=1;

$sth = $dbh->prepare("select * from items");
$sth->execute();
while (my $data = $sth->fetchrow_hashref) {
	my @fields;
	$data->{sellrate}=sprintf("%.6f",$data->{sellrate});
	grep(push(@fields,$data->{$_}),@fieldlist);

	$serialized=join('|',@fields);

	printf("Processing: %d %s                            \r",$data->{id},$data->{Name});
	if ($serialized ne $data->{serialization}) {
		printf(" (UPDATED)\n");
		$dbh->do("update items set serialized=now(),serialization=".$dbh->quote($serialized)." where id=".$data->{id});
	}
}

