#!/usr/bin/php
<?php
define ('CHARACTER_PT', 91);
define ('CHARACTER_SIZE', 130);
define ('CHARACTER_COUNT', 26);
define ('CHARACTER_START', 'A');
define ('CHARACTER_FONT', 'verdanab.ttf');
define ('CHARACTER_FILE', 'font');

$start_char = ord(CHARACTER_START);
$dir = '';

function icrc16($data)
{
	$crc = 0xFFFF;

	if($data && strlen($data))
	{
		$buffer = unpack('C*',$data);
		foreach($buffer as $data)
		{
			$crc = (($crc >> 8) | ($crc << 8))&0xFFFF;
			$crc^= $data;
			$crc^= ($crc & 0xFF) >> 4;
			$crc^= ($crc << 12)&0xFFFF;
			$crc^= ($crc & 0xFF) << 5;
		}
	}
	return $crc^0xFFFF;
}

function directory_entry($type, $id, $next_id, $pos, $string='', $flags = 0)
{
	return pack('CCnnNNn',$type,$flags,$id,$next_id,$pos,strlen($string),icrc16($string));
}

$directory = '';
$output = '';

$dir_entry_size = strlen(directory_entry(0x00,0,0,0,''));
printf("directory entry size = 0x%02X\n",$dir_entry_size);
$total = (((CHARACTER_COUNT)*2)+1)*$dir_entry_size;
printf("data start = 0x%04X\n",$total);

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
	$rle = '';
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
						$rle.=chr($byte_pre).chr($count);

					$count=1;
					$byte_pre = $byte;
				}
				break;

			default:
				if($count)
				{
					$rle.=chr($byte_pre).chr($count);
					$count = 0;
				}
				$rle.=chr($byte);
				$byte_pre = -1;
		}

		if($count==0xFF)
		{
			$rle.=chr($byte_pre).chr($count);
			$byte_pre = -1;
			$count=0;
		}
	}

	if($count);
	{
		$rle.=chr($byte_pre).chr($count);
		$count=0;
	}

	/* store character */
	printf("Video '%s' @0x%08X (%u bytes)\n",$char,$total,strlen($rle));
	$directory.=directory_entry(0x02,ord($char),0,$total,$rle);
	$total += strlen($rle);
	$output.= $rle;

	imagedestroy ($im);

	/* store audio file */
	$aud_file = sprintf("alphabet%03u.raw",$i+1);
	if(($data = file_get_contents($aud_file))==FALSE)
		echo "failed to open '$audio_file'\n";

	printf("Audio '%s' @0x%08X (%u bytes)\n",$char,$total,strlen($data));
	$directory.=directory_entry(0x03,ord($char),0,$total,$data);
	$total += strlen($data);
	$output.= $data;
}

/* create root entry */
printf("\n\nROOT\n\n");
$directory = directory_entry(0x01,0,0,$dir_entry_size,$directory).$directory;
printf("\n");
printf("directory size = 0x%08X\n",strlen($directory));
printf("output size    = 0x%08X\n",strlen($output));
$output = $directory.$output;
$length = strlen($output);
printf("total size     = 0x%08X (%u)\n",$length,$length);

file_put_contents('database.raw',$output);