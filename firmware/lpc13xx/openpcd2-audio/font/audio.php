#!/usr/bin/php
<?php
define('STORY_FILE','story.txt');
define('FREQUENCY',15625);

if(($file = fopen(STORY_FILE,'r'))===FALSE)
	exit('Can\'t open '.STORY_FILE);

while($line = fgets($file))
{
	list($start,$end,$text) = explode("\t",strtr($line,',','.'));
	$start=(int)(floatval($start)*FREQUENCY);
	$end=(int)(floatval($end)*FREQUENCY);
	$text=trim($text);

	printf("%8u ->%8u ='%s'\n",$start,$end,$text);
}

?>