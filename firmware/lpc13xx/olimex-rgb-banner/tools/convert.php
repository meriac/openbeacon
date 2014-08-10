#!/usr/bin/env php
<?php

define('IMAGE_MULTIPLIER', 18);
define('IMAGE_NAME', 'image.png');

$img = imagecreatefrompng(IMAGE_NAME);

$sx = imagesx($img);
$sy = imagesy($img);

echo "#ifndef __IMAGE_H__\n";
echo "#define __IMAGE_H__\n";

printf("\n/* auto generated with %s\n   using image %s */\n", __FILE__, IMAGE_NAME);

echo "\nconst uint8_t g_img_lines[] = {\n";

$dist = array();
$lookup = array();
$table_offset = 0;

for($x=0; $x<$sx; $x++)
{
	$line = array();

	for($y=0; $y<$sy; $y++)
	{
		$rgb = imagecolorat ($img, $x, $y);

		$r = ($rgb >> 16) & 0xFF;
		$g = ($rgb >>  8) & 0xFF;
		$b = ($rgb >>  0) & 0xFF;
		$gray = intval((($r+$g+$b)/3)/IMAGE_MULTIPLIER);

		$line[] = $gray;
	}

	/* convert to 4 bit packed */
	$compressed = array();
	$first = true;
	for($y=0; $y<($sy/2); $y++)
	{
		$data = $line[($y*2)+0] | ($line[($y*2)+1]<<4);
		if($first)
		{
			$first = false;
			$prev = $data;
			$count = 1;
		}
		else
		{
			if($data!=$prev)
			{
				if($count)
				{
					if($count==1)
						$compressed[] = $data;
					else
					{
						$compressed[] = 0xF0 | $count;
						$compressed[] = $data;
					}
				}
				$count = 1;
			}
			else
			{
				$count++;
				if($count>=15)
				{
					$compressed[] = 0xF0 | $count;
					$compressed[] = $data;
					$count = 0;
				}
			}

			$prev = $data;
		}
	}
	$compressed[] = 0xF0;

	$s = implode(',',$compressed);
	if(isset($dist[$s]))
		$lookup[] = $dist[$s];
	else
	{
		printf("\t/*0x%04X*/ ",$table_offset);
		foreach($compressed as $value)
			printf("0x%02X,",$value);
		echo "\n";

		$lookup[] = $table_offset;
		$dist[$s] = $table_offset;
		$table_offset += count($compressed);
	}
}
printf("\t/*0x%04X*/ 0xF0};\n", $table_offset);

echo "\nconst uint16_t g_img_lookup_line[] = {";

foreach($lookup as $index => $offset)
{
	if(($index % 8) == 0)
		echo "\n\t";
	printf("0x%04X,", $offset);
}
echo "\n\t0xFFFF};\n";

echo "\n";
echo "#define IMAGE_WIDTH  $sx\n";
echo "#define IMAGE_HEIGHT $sy\n";
printf("#define IMAGE_SIZE   %u\n",$table_offset + count($lookup)*2);
printf("#define IMAGE_VALUE_MULTIPLIER %u\n", IMAGE_MULTIPLIER);

echo "\n#endif/*__IMAGE_H__*/\n";
