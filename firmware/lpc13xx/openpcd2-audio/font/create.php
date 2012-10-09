#!/usr/bin/php
<?php
define ('CHARACTER_PT', 91);
define ('CHARACTER_SIZE', 130);
define ('CHARACTER_COUNT', 26);
define ('CHARACTER_START', 'A');
define ('CHARACTER_FONT', 'verdanab.ttf');
define ('CHARACTER_FILE', 'font');

if(($file = fopen(CHARACTER_FILE.'.c','w'))==NULL)
	exit("Can't open '".CHARACTER_FILE.".c'\n");

fprintf($file,"#include <openbeacon.h>\n");
fprintf($file,"#include <font.h>\n");

$start_char = ord(CHARACTER_START);

$total = 0;

$font = array();

for($i=0;$i<CHARACTER_COUNT;$i++)
{
	$char = chr($start_char+$i);
	$img_file = "$char.png";

	$im = imagecreatetruecolor (CHARACTER_SIZE, CHARACTER_SIZE);
	$color_back = imagecolorallocate ($im, 0, 0, 0);
	$color_text = imagecolorallocate ($im, 255, 255, 255);
	$box = imagettfbbox (CHARACTER_PT, 0, CHARACTER_FONT, $char);

	$px = (CHARACTER_SIZE-($box[2]-$box[0]))/2 - 4;
	$py = (CHARACTER_SIZE-($box[1]-$box[7]))/2 + $box[1];
	imagettftext ($im, CHARACTER_PT, 0, $px, CHARACTER_SIZE-$py, $color_text, CHARACTER_FONT, $char);
	imagepng ($im, $img_file);

	/* read out image */
	$data = array();
	for($y=0;$y<CHARACTER_SIZE;$y++)
		for($x=0;$x<CHARACTER_SIZE;$x++)
			$data[] = round((imagecolorat ($im, $x, $y) & 0xFF)/17);

	/* adjust array size to an even count */
	$size = count($data);
	if($size&1)
		$data[] = 0;
	$bytes = $size/2;

	/* create byte array from 4 bit nibbles */
	$rle = array();
	$byte_pre = -1;
	$count = 0;
	for($t=0;$t<$bytes;$t++)
	{
		$byte = ($data[$t*2]<<4) | $data[($t*2)+1];
		switch($byte)
		{
			case 0x00:
			case 0xFF:
				if($byte==$byte_pre)
					$count++;
				else
				{
					if($count)
					{
						$rle[] = $byte_pre;
						$rle[] = $count;
					}

					$count=1;
					$byte_pre = $byte;
				}
				break;

			default:
				if($count)
				{
					$rle[] = $byte_pre;
					$rle[] = $count;
					$count = 0;
				}
				$rle[] = $byte;
				$byte_pre = -1;
		}

		if($count==0xFF)
		{
			$rle[] = $byte_pre;
			$rle[] = $count;
			$byte_pre = -1;
			$count=0;
		}

	}

	if($count);
	{
		$rle[] = $byte_pre;
		$rle[] = $count;
		$count=0;
	}

	/* store character */
	$font[$char] = $rle;

	/* maintain statistics */
	$total += count($rle);

	imagedestroy ($im);
}

fprintf($file,"\n/* total data %u bytes */\n",$total);

foreach ($font as $char => $rle)
{
	fprintf($file,"\n/* character '$char' with ".sizeof($rle)." bytes of data */\n");
	fprintf($file,"static const uint8_t _$char"."[] = {");

	foreach ($rle as $index => $byte)
	{
		if(($index%8)==0)
			fprintf($file,"\n\t");
		fprintf($file,$index?',':' ');
		fprintf($file,"0x%02X",$byte);
	}

	fprintf($file,"};\n");
}

fprintf($file,"\nconst uint8_t* font[]={");
foreach(array_keys($font) as $index => $char)
{
	if(($index%8)==0)
		fprintf($file,"\n\t");
	fprintf($file,$index?', ':'  ');
	fprintf($file,"_$char");
}
fprintf($file,"};\n\n");

/* close *.c - create *.h */
fclose($file);
if(($file = fopen(CHARACTER_FILE.'.h','w'))==NULL)
	exit("Can't open '".CHARACTER_FILE.".h'\n");

fprintf($file,"#ifndef __FONT_H__\n#define __FONT_H__\n\n");
fprintf($file,"#define CHARACTER_SIZE ".CHARACTER_SIZE."\n");
fprintf($file,"#define CHARACTER_COUNT ".CHARACTER_COUNT."\n");
fprintf($file,"#define CHARACTER_START '".CHARACTER_START."'\n");
fprintf($file,"\nextern const uint8_t* font[];\n");
fprintf($file,"\n#endif/*__FONT_H__*/\n");

fclose($file);
