<?php
/***************************************************************
 *
 * BlinkenBike.org - Cycle Simulation
 *
 * Copyright 2011 Milosch Meriac <meriac@blinkenbike.org>
 *
 * This PHP shell scrip renders a set of PNG files that
 * are turned into a movie file for accurate cycle simulation.
 *
/***************************************************************

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

define('WIDTH', 1920);
define('HEIGHT',350);

define('RADIUS', 160);
define('INNER_SPACE', RADIUS/5);
define('CROSSING', 3);
define('ANIMATION_STEPS', 100);
define('ANIMATION_ANGLE', 10);
define('CIRCUMFERENCE', 2 * M_PI * RADIUS);

function imagecrossing ($img, $x, $y, $alpha, $color)
{
    $dy1 = $y + sin($alpha * M_PI/180)*INNER_SPACE;
    $dx1 = $x + cos($alpha * M_PI/180)*INNER_SPACE;

    $dy2 = $y + sin($alpha * M_PI/180)*RADIUS;
    $dx2 = $x + cos($alpha * M_PI/180)*RADIUS;

    imageline($img,$dx1,$dy1,$dx2,$dy2,$color);
}

function imagewheel ($img, $x ,$y, $alpha, $color)
{
    for($i=0;$i<CROSSING;$i++)
	imagecrossing ($img, $x, $y, $alpha+=360/CROSSING, $color);
}

function imagewheelmove ($img, $x ,$y, $speed)
{
    $color = 0x00;
    $start_x = $x - ($speed*ANIMATION_STEPS);
    $alpha = 360*($x/CIRCUMFERENCE);

    $steps = ANIMATION_STEPS;

    while($steps--)
    {
	$col = imagecolorallocate($img,$color, $color, $color);
	imagewheel($img, $start_x, $y, $alpha, $col);

	$color += 0x100/ANIMATION_STEPS;
	$start_x += $speed;
	$alpha += 360*($speed/CIRCUMFERENCE);
    }
}

function imagewheelanim ($x, $y, $speed, $steps)
{
    for($i=0;$i<$steps;$i++)
    {
	$img = imagecreatetruecolor(WIDTH, HEIGHT);
	imageantialias($img, true);
	$col_back = imagecolorallocate($img,0x00,0x00,0x00);
        imagefilledrectangle($img,0,0,WIDTH,HEIGHT,$col_back);

	imagewheelmove($img,$x, $y, $speed);
	$x+=$speed;

	imagepng($img,sprintf('anim%04u.png',$i));
	imagedestroy($img);
    }

}

imagewheelanim (-RADIUS, HEIGHT/2, 5, (WIDTH+(4*RADIUS))/5);
?>
