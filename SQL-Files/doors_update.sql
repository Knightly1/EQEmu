ALTER TABLE `doors` ADD UNIQUE DoorIndex (doorid,zone);
ALTER TABLE `doors` ADD `size` SMALLINT(5)  UNSIGNED DEFAULT "100" NOT NULL;
ALTER TABLE `doors` CHANGE `liftheight` `door_param` INT(4)  DEFAULT "0" NOT NULL;

# Make SURE all three queries dont produce any errors!!!