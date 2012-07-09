#!/usr/bin/php
<?php
/***************************************************************
 *
 * OpenPCD.org - ISO15693 decoding of sniffed 13.56MHz
 *               HID iClass tags
 *
 * Copyright 2007-2012 Milosch Meriac <meriac@openpcd.de>
 *
 * please visit http://www.OpenPCD.org for furter information
 *
 ***************************************************************

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

define('PICC_BIT_PERIOD', 37.76);   // full PICC bit period in microseconds
define('PICC_SOF_PERIOD', 56.64);
define('PCD_BIT_PERIOD', 9.44);
define('TOLERANCE', 0.75);

if($argc==2)
	demodulate_file($argv[1]);
else
	exit('usage: '.$argv[0]." filename.csv\n");

function decode_PCD($delta_t=false,$level=0)
{
	global $time;

	static $slot_prev = 0;
	static $last_edge = 0;
	static $sof = FALSE;
	static $byte = 0;
	static $bit_pos  = 0;

	if(!$delta_t || $delta_t>(4*PICC_BIT_PERIOD))
	{
		$sof = false;
		$last_edge = $slot_prev = $byte = $bit_pos = 0;
		decode_PCD_byte(-1);
		return;
	}

	// count time since start of bit period
	$last_edge+=$delta_t;

	// maintain current signal level
	if($level)
	{
		$slot = intval(($last_edge / ((PICC_BIT_PERIOD*2)/8))+0.5) - $slot_prev;

		if($sof)
		{
			if($slot&1)
			{
				$data=($slot-1)/2;
				$byte=($byte>>2) | ($data<<6);

				$bit_pos++;
				if($bit_pos==4)
				{
					decode_PCD_byte($byte);
					$bit_pos=$byte=0;
				}
			}
			else
			{
				if($slot!=2)
					echo "PCD:  invalid slot number=$slot\n";
				decode_PCD_byte(-1);
				$sof=FALSE;
			}

			$slot_prev = 8-$slot;
		}
		else
		{
			if($slot==5)
			{
//				printf("\nPCD:  SoF %9.3fms\n",$time/1000);;
				$sof=true;
				$slot_prev = 8-$slot;
			}
//			else
//				echo "PCD:  invalid SoF slot=$slot\n";
		}
		$last_edge=0;
	}
}

function decode_PCD_byte($data)
{
	static $packet, $data_r;
	global $time;
	global $state;
	global $cmd;

	if($data<0)
	{
		if($packet)
		{
			echo "PCD : CMD ";
			$cmd=ord(substr($packet,0,1));
			$data=substr($packet,1);

			if(strlen($data)>=3)
			{
				$crc=false;

				switch($cmd)
				{
					case 0x05:
						$data=unpack('N*',$data);
						printf("CHECK CHALLENGE=0x%08X SIGNATURE=0x%08X\n",$data[1],$data[2]);
						break;
					case 0x0C:
						$data=substr($data,0,1);
						$crc=true;
						printf("READ ADDRESS=0x%02X",ord($data));
						break;
					case 0x81:
						printf("SELECT UID=0x%s\n",strtoupper(bin2hex($data)));
						break;
					default:
						printf("UNKNOWN CMD=0x%02X PACKET=0x%s\n",$cmd,strtoupper(bin2hex($data)));
				}

				if($crc)
				{
					$crc = unpack('v',substr($packet,-2));
					printf(" CRC=%s\n",($crc[1]==crc16($data))?'OK':'INVALID');
				}
			}
			else
				switch($cmd)
				{
					case 0x0A:
						printf("ACTALL\n");
						break;
					case 0x0C:
						printf("IDENTIFY\n");
						break;
					case 0x88:
						$data=substr($data,0,1);
						printf("READCHECK ADDRESS=0x%02X\n",ord($data));
						break;
					default:
						printf("UNKNOWN CMD=0x%02X PACKET=0x%s\n",$cmd,strtoupper(bin2hex($data)));
				}
		}
		$packet='';
		$state=0;
	}
	else
		$packet.=pack('C',$data);
}

function decode_PICC($delta_t=false,$level=0)
{
	global $cmd;
	global $time;

	static $duration = 0;
	static $last_bit = FALSE;
	static $sof = FALSE;

	if(!$delta_t || $delta_t>(4*PICC_BIT_PERIOD))
	{
		$sof = FALSE;
		$duration = 0;
		decode_PICC_bit(-1);
		return;
	}

	switch($sof)
	{
		case 1:
			if(!$level && ($delta_t>=((PICC_BIT_PERIOD/2)*TOLERANCE)) && ($delta_t<=((PICC_BIT_PERIOD/2)/TOLERANCE)))
			{
				$duration = $delta_t;
				$last_bit = TRUE;
				$sof = 2;
			}
			else
				$sof = FALSE;
			break;

		case 2:
			if($level)
			{
				$pause = round($duration/(PICC_BIT_PERIOD/2));
				$high = round($delta_t/(PICC_BIT_PERIOD/2));

				// check for EoF
				if(($pause==1) && ($high==3))
					decode_PICC_bit(-1);
				else
					// check for invalid codes
					if(($pause>2)||($pause<1)||($high>2)||($high<1))
						printf("PICC: Coding Error (pause=$pause high=$high)\n");
					else
					{
						$bit = ($pause==1)?$last_bit:!$last_bit;

						if($high==2)
						{
							decode_PICC_bit($bit);
							$bit = !$bit;
						}
						decode_PICC_bit($bit);
						$last_bit = $bit;

//						echo "PICC: pause=$pause high=$high\n";
					}
			}
			else
				$duration = $delta_t;
			break;

		default:
			if($level && ($delta_t>=(PICC_SOF_PERIOD*TOLERANCE)) && ($delta_t<=(PICC_SOF_PERIOD/TOLERANCE)))
			{
				if($cmd==0x0A)
					printf("PICC: SoF@%9.3fms\n",$time/1000);
				$sof = 1;
			}
	}
}

function decode_PICC_bit($bit)
{
	global $state;
	global $cmd;

	static $packet = '';
	static $frame = TRUE;
	static $byte = 0;
	static $bit_pos = 0;

	if($bit<0)
	{
		if($packet)
		{
			echo "PICC: ";
			switch($cmd)
			{
				case 0x88:
					printf("READCHECK RESPONSE=0x%s\n",strtoupper(bin2hex($packet)));
					break;
				case 0x05:
					printf("CHECK RESPONSE=0x%s\n",strtoupper(bin2hex($packet)));
					break;
				default:
					$data = substr($packet,0,-2);
					$crc = unpack('v',substr($packet,-2));
					printf("RESPONSE=0x%s CRC=%s\n",strtoupper(bin2hex($data)),($crc[1]==crc16($data))?'OK':'INVALID');
			}
			$packet='';
			echo "\n";
		}
		$byte = $bit_pos = $state = 0;
		$frame = TRUE;
	}
	else
	{
		if($frame)
			$frame=FALSE;
		else
		{
//			printf("PICC: bit=%u\n",$bit?1:0);
			$byte>>=1;
			if($bit)
				$byte|=0x80;

			$bit_pos ++;
			if($bit_pos == 8)
			{
//				printf("PICC: byte=0x%02X ####\n",$byte);
				$packet.=pack('C',$byte);
				$bit_pos = $byte = 0;
			}
		}
	}
}

// calculate CRC16 according to ISO/IEC 13239
function crc16($packet)
{
	$crc=0xE012;
	foreach(str_split($packet) as $data)
	{
		$crc ^= ord($data);
		for($i=0;$i<8;$i++)
			$crc = ($crc & 1) ? ($crc >> 1) ^ 0x8408 : ($crc >> 1);
	}
	return $crc;
}

function demodulate_file($file)
{
	global $time;
	global $state;

	// reset time
	$time = $time_abs = 0;

	if(($handle = @fopen($file,'r'))===FALSE)
	exit("error: can't open '$file'\n\n");

	printf("Decoding file '%s'\n\n",$file);

	$header = NULL;

	$state = 0;
	$delta_prev = 0;
	while (!feof($handle))
	{
		$line = trim(fgets($handle));
		if($line)
			$values = explode(',',$line);
		else
			continue;

		if(count($values)!=2)
			exit('Two comma seperated values needed ('.$i.'): "DeltaTime[in ns since last change],SignalEnvelope[logical 0 or 1]"');

		if($header)
		{
			// read new line of values
			list($delta_t,$level) = $values;

			// convert time to microseconds
			$delta_t = intval($delta_t) / 1000;
			// maintain global absolute us counter
			$time_abs += $delta_t;
			$time = intval($time_abs);

			// find pause
			if($delta_t>=(PICC_SOF_PERIOD*4))
			{
				$state=0;
				if($delta_t>100000)
					echo "\n\n\n";
			}
			else
			{
				if(!$state && $level && $delta_prev>=(PICC_SOF_PERIOD*4))
				{
					if($delta_t>=(PICC_SOF_PERIOD*TOLERANCE))
					{
						decode_PICC();
						$state=2;
					}
					else
						if($delta_t>=(PCD_BIT_PERIOD*TOLERANCE))
						{
							decode_PCD();
							$state=1;
						}
				}

				switch($state)
				{
					// PCD mode
					case 1:
						decode_PCD($delta_t, $level);
						break;

					// PICC mode
					case 2:
//						printf("RX: %6u\tstate=%u\t%u\n",round($delta_t),$state,$level);
						decode_PICC($delta_t, $level);
						break;
				}
			}

			$delta_prev = $delta_t;
		}
		else
			$header = $values;
	}
	fclose($handle);
}
?>
