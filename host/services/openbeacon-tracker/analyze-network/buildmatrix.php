#!/usr/bin/php
<?php
/***************************************************************
 *
 * OpenBeacon.org - JSON-to-MatrixGraph converter shell script
 *
 * uses a physical model and statistical analysis to calculate
 * positions of tags
 *
 * Copyright 2009-2011 Milosch Meriac <meriac@bitmanufaktur.de>
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published
 by the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

define('DATA_FILE','example.json');
define('MAX_SIZE',49);

if(($data=@file_get_contents(DATA_FILE))===FALSE)
    die('can\'t open data file '.DATA_FILE);

if(!($track=json_decode($data)))
    die('can\'t decode JSON tracking state');

$matrix_row=array();
$matrix_col=array();
$matrix_unique=array();
$matrix_row_total=array();
foreach($track->total_edges as $edge)
{
    $tag1=intval($edge->tag[0]);
    $tag2=intval($edge->tag[1]);

    $matrix_unique[$tag1]=true;
    $matrix_unique[$tag2]=true;

    $matrix_col[$tag2]=$edge->power;

    if(!isset($matrix_row_total[$tag1]))
	$matrix_row_total[$tag1]=$edge->power;
    else
	$matrix_row_total[$tag1]+=$edge->power;

    if(!isset($matrix_row[$tag1]))
	$matrix_row[$tag1]=array();
    $matrix_row[$tag1][$tag2]=$edge->power;
}

ksort($matrix_unique);
foreach($matrix_unique as $file => $active)
    if(file_exists("png/$file.png"))
	echo "# $file.png\n";

// sort for tag IDs
ksort($matrix_row);
foreach($matrix_row as &$row)
    ksort($row);
arsort($matrix_col);
arsort($matrix_row_total);

$matrix_row_total = array_slice(array_keys($matrix_row_total),0,MAX_SIZE);
$matrix_col = array_slice(array_keys($matrix_col),0,MAX_SIZE);

echo "labels";
foreach($matrix_col as $id)
    echo" T$id";
foreach($matrix_row_total as $row_id)
{
    echo "\nT$row_id";
    foreach($matrix_col as $col_id)
    {
	echo " ";
	if(isset($matrix_row[$row_id][$col_id]))
	    echo $matrix_row[$row_id][$col_id];
	else
	    echo '-';
    }
}
?>
