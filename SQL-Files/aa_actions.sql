
CREATE TABLE aa_actions (
	aaid mediumint unsigned not null,
	rank tinyint unsigned not null,
	reuse_time mediumint unsigned not null,
	spell_id mediumint unsigned not null,
	target tinyint unsigned not null,
	nonspell_action tinyint unsigned not null,
	nonspell_mana mediumint unsigned not null,
	nonspell_duration mediumint unsigned not null,
	redux_aa mediumint unsigned not null,
	redux_rate tinyint not null,
	
	PRIMARY KEY(aaid, rank)
);



