#!/usr/bin/php
<?php
define('STORY_FILE','story.txt');
define('FREQUENCY',15625);

if(($file = fopen(STORY_FILE,'r'))===FALSE)
	exit('Can\'t open '.STORY_FILE);

$index = array();
while($line = fgets($file))
{
	list($start,$end,$text) = explode("\t",strtr($line,',','.'));

	$obj = new stdClass;
	$obj->start=(int)(floatval($start)*FREQUENCY);
	$obj->end=(int)(floatval($end)*FREQUENCY);
	$obj->text=trim($text);
	$index[]=$obj;

	printf("%8u ->%8u ='%s'\n",$obj->start,$obj->end,$obj->text);
}

$count = count($index);

echo("\n INDEX=$count objects\n");

?>