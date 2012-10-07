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

define('DATA_FILE','../get/brucon.json');
define('MAX_PROX_COLORS',16);
error_reporting(E_ALL);

function draw_tag($im, $x, $y, $id, $button)
{
    global $tag_color;
    global $tag_color_high;

    $avatar = @imagecreatefrompng('avatars/'.intval($id).'.png');
    if($avatar)
    {
	/* get image size */
	$av_width = imagesx($avatar);
	$av_height = imagesy($avatar);
	imagecopyresampled($im, $avatar, $x-6, $y-6, 0, 0, 12, 12, $av_width, $av_height);
    }
    else
	imagefilledellipse($im, $x, $y, 12, 12, $button?$tag_color_high:$tag_color);
    imagestring($im, 4, $x,$y+8, $id, $button?$tag_color_high:$tag_color);
}


function main()
{
    global $tag_color;
    global $tag_color_high;

    $time_start = microtime(TRUE);

    /* only display tags on this particular floor */
    $floor=isset($_GET['floor'])?intval($_GET['floor']):0;
    if($floor<1 || $floor>3)
	$floor=1;

    /* open floor specific image */
    $imgfile = '../images/brucon.png';
    $im_png = imagecreatefrompng($imgfile)
	or die('Cannot Initialize new GD image stream with file '.$imgfile);

    /* get image size */
    $im_width = imagesx($im_png);
    $im_height = imagesy($im_png);

    $im = imagecreatetruecolor($im_width, $im_height)
	or die('Cannot initialize new GD image');
    /* enable antialiasing */
    imageantialias ($im , true );

    /* specify colors */
    $bg_color       = imagecolorallocate($im,0x10,0x10,0x10);
    $inactive_color = imagecolorallocate($im, 150, 150, 150);
    $reader_color   = imagecolorallocate($im, 255,  64,  32);
    $prox_color     = imagecolorallocate($im, 128,  32,  16);
    $tag_color      = imagecolorallocate($im,  32,  64, 128);
    $tag_color_high = imagecolorallocate($im, 255, 128,  64);

    $prox_color = array();
    for($i=1;$i<=MAX_PROX_COLORS;$i++)
	$prox_color[$i] = imagecolorallocate($im, (255/MAX_PROX_COLORS)*$i, (64/MAX_PROX_COLORS)*$i, (32/MAX_PROX_COLORS)*$i);

    imagecopy($im, $im_png, 0, 0, 0, 0, $im_width, $im_height);

    if(($data=@file_get_contents(DATA_FILE))===FALSE)
	die('can\'t open data file');

    if(!($track=json_decode($data)))
	die('can\'t decode JSON tracking state');

    /* create associative list for reader id for current floor */
    $reader_list = array();
    foreach($track->reader as $reader)
	if($reader->floor==$floor && isset($reader->loc))
	{
	    $reader_list[intval($reader->id)] = true;
	    /* for BCC reader ID/IP is derieved from switch port number */
	    $reader_id=sprintf("%s%03u",chr(ord('A')+(($reader->id>>8)&0x1F)),$reader->id&0xFF);

	    $px = $reader->loc[0];
	    $py = $reader->loc[1];
	    imagefilledrectangle($im, $px-3, $py-3, $px+3, $py+3, $reader_color);
	    imagestring($im, 4, $px, $py-18, $reader_id, $inactive_color);
	}

    $tag_list = array();
    foreach($track->tag as $tag)
	$tag_list[intval($tag->id)]=$tag;

    /* draw all edges */
    foreach($track->edge as $edge)
    {
	$tag_id1 = intval($edge->tag[0]);
	$tag_id2 = intval($edge->tag[1]);
	if(isset($tag_list[$tag_id1])&&isset($tag_list[$tag_id2]))
	{
	    $tag1 = $tag_list[$tag_id1];
	    $tag2 = $tag_list[$tag_id2];
	    $power = ($edge->power<=MAX_PROX_COLORS) ? $edge->power : MAX_PROX_COLORS;
	    if(isset($tag1->loc) && isset($tag2->loc))
	      imageline($im, $tag1->loc[0], $tag1->loc[1], $tag2->loc[0], $tag2->loc[1], $prox_color[$power]);
	}
    }

    /* draw all non-button-pressed tags */
    foreach($track->tag as $tag)
	if(!isset($tag->button) && isset($tag->reader) && isset($reader_list[$tag->reader]) && isset($tag->loc))
	    draw_tag($im, $tag->loc[0], $tag->loc[1], $tag->id, false);

    /* draw all button-pressed tags */
    foreach($track->tag as $tag)
	if(isset($tag->button) && isset($tag->reader) && isset($reader_list[$tag->reader]) && isset($tag->loc))
	    draw_tag($im, $tag->loc[0], $tag->loc[1], $tag->id, true);

    // timestamp
    $duration = sprintf(" [rate=%u/s, render=%ums]",$track->packets->rate,round((microtime(TRUE)-$time_start)*1000,1));
    imagestring($im, 4, $im_width-470, $im_height-30, date(DATE_RSS,$track->time).$duration, $inactive_color);

    header('Content-type: image/png');
    header('Refresh: 1');
    imagepng($im);
    imagedestroy($im);
}

main();
exit;
?>
