<?php
/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position tracker
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

    define('DATA_FILE','../logs/sightings.json');

    error_reporting(E_ALL);

    $time_start = microtime(TRUE);

    /* only display tags on this particular floor */
    $floor=isset($_GET['floor'])?intval($_GET['floor']):0;
    if($floor<1 || $floor>3)
	$floor=2;

    /* open floor specific image */
    $imgfile = sprintf('bcc_map_level%c.png',ord('A')-1+$floor);
    $im = imagecreatefrompng($imgfile)
	or die('Cannot Initialize new GD image stream with file '.$imgfile);

    /* get image size */
    $im_width = imagesx($im);
    $im_height = imagesy($im);

    /* specify colors */
    $inactive_color = imagecolorallocate($im, 150, 150, 150);
    $reader_color   = imagecolorallocate($im, 255,  64,  32);
    $tag_color      = imagecolorallocate($im,  64, 128, 255);
    $text_color     = imagecolorallocate($im,  32,  64, 128);

    if(($data=@file_get_contents(DATA_FILE))===FALSE)
	die('can\'t open data file');

    if(!($track=json_decode($data)))
	die('can\'t decode JSON tracking state');

    foreach($track->reader as $reader)
	if($reader->group==2 && $reader->floor==$floor)
	{
	    /* for BCC reader ID/IP is derieved from switch port number */
	    $reader_id=sprintf("%s%03u",chr(ord('A')+(($reader->id>>8)&0x1F)),$reader->id&0xFF);

	    imagefilledrectangle($im, $reader->px-3, $reader->py-3, $reader->px+3, $reader->py+3, $reader_color);
	    imagestring($im, 4, $reader->px, $reader->py-18, $reader_id, $inactive_color);
	}

    foreach($track->tag as $tag)
	{
	    imagefilledellipse($im, $tag->px, $tag->py, 6, 6, $tag_color);
	    imagestring($im, 4, $tag->px,$tag->py+4, $tag->id, $text_color);
	}

    // timestamp
    $duration = sprintf(" [rate=%u/s, %ums]",$track->packets->rate,round((microtime(TRUE)-$time_start)*1000,1));
    imagestring($im, 4, $im_width-420, $im_height-30, date(DATE_RSS,$track->time).$duration, $inactive_color);

    header('Content-type: image/png');
    header('Refresh: 1');
    imagepng($im);
    imagedestroy($im);
    exit;
?>
