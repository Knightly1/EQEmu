
CREATE TABLE aa_swarmpets (
	spell_id mediumint unsigned not null,
	count tinyint unsigned not null,
	npc_id int not null,
	duration mediumint unsigned not null,
	PRIMARY KEY(spell_id)
);


