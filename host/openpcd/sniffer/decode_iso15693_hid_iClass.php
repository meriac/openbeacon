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

if($argc==2)
	demodulate_file($argv[1]);
else
	exit('usage: '.$argv[0]." filename.csv\n");

function decode_PCD($delta_t)
{
	global $time;

	static $slot_prev = 0;
	static $last_edge = 0;
	static $sof = FALSE;
	static $level = FALSE;
	static $byte = 0;
	static $bit_pos = 0;

	if($delta_t>(4*PICC_BIT_PERIOD))
	{
		$level = $frame = $sof = false;
		$pulses = $last_edge = $slot_prev = $byte = $bit_pos = 0;
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
				if($slot==2)
				{
					$sof=FALSE;
					printf("PCD:  EoF %9.3fms\n",$time/1000);
					decode_PCD_byte(-1);
				}
				else
					echo "PCD:  invalid slot number=$slot\n";

			$slot_prev = 8-$slot;
		}
		else
		{
			if($slot==5)
			{
				printf("\nPCD:  SoF %9.3fms\n",$time/1000);;
				$sof=true;
				$slot_prev = 8-$slot;
			}
			else
				echo "PCD:  invalid SoF slot=$slot\n";
		}
		$last_edge=0;
	}

	// maintain current signal level
	$level = !$level;
}

function decode_PCD_byte($data)
{
	static $crc, $packet, $data_r;
	global $time;

	if($data<0)
	{
		if($packet)
			printf("PCD:  packet=%s (%u) CRC=%s:[0x%04X]\n\n",strtoupper(bin2hex($packet)),strlen($packet),(strlen($packet)>2)?(($crc==0x4DA1)?'OK':'FAILED'):'NONE',$crc);
		$packet='';
		$crc=0xFFFF;
	}
	else
	{
		$packet.=pack('C',$data);
		$crc = crc16($data,$crc);
//		printf("PCD:  data=0x%02X crc=0x%04X\n",$data,$crc);
	}
}

function decode_PICC($delta_t)
{
	global $time;

	static $pulses = 0;
	static $last_edge = 0;
	static $new_bit = FALSE;

	static $frame = FALSE;
	static $level = FALSE;
	static $sof = FALSE;
	static $byte = 0;
	static $bit_pos  = 0;

	if($delta_t>(4*PICC_BIT_PERIOD))
	{
		$level = $frame = $sof = false;
		$pulses = $last_edge = $byte = $bit_pos = 0;
		decode_PICC_byte(-1);
		echo "\n";
		return;
	}

	// maintain current signal level
	$level = !$level;

	// count subcarrier pulses <2us
	if(($delta_t < 2)&&$level)
		$pulses++;

	// count time since start of bit period
	$last_edge+=$delta_t;

	if($sof)
	{
		if($pulses==8 && (intval($last_edge)>1))
		{
			$bit=($last_edge > (PICC_BIT_PERIOD*(2/3)));

			// skip first bit in frame - part of SoF
			if($frame)
			{
				$byte>>=1;
				if($bit)
					$byte|=0x80;

				$bit_pos ++;
				if($bit_pos == 8)
				{
					decode_PICC_byte($byte);
					$bit_pos = $byte = 0;
				}
			}
			else
				$frame=TRUE;

			$last_edge = $bit ? 0:-(PICC_BIT_PERIOD/2);
			$pulses = 0;
		}
		else
			if($pulses==16)
			{
				$sof=FALSE;
				printf("PICC: EoF %9.3fms\n",$time/1000);
				decode_PICC_byte(-1);
			}
	}
	else
	{
		if($pulses==24)
		{
			printf("PICC: SoF %9.3fms\n",$time/1000);
			$sof = TRUE;
			$pulses = 0;
			$last_edge = 0;
			decode_PICC_byte(-1);
		}
	}
}

function decode_PICC_byte($data)
{
	static $crc, $packet;
	global $time;

	if($data<0)
	{
		if($packet)
			printf("PICC: packet=%s (%u) CRC=%s:[0x%04X]\n\n",strtoupper(bin2hex($packet)),strlen($packet),(strlen($packet)>2)?(($crc==0xC316)?'OK':'FAILED'):'NONE',$crc);
		$packet='';
		$crc=0xFFFF;
	}
	else
	{
		$packet.=pack('C',$data);
		$crc=crc16($data,$crc);
//		printf("PICC: data=0x%02X crc=0x%04X\n",$data,$crc);
	}
}

// calculate CRC16 according to ISO/IEC 13239
function crc16($data,$crc)
{
	$crc^= $data;
	for($i=0;$i<8;$i++)
		$crc = ($crc & 1) ? ($crc >> 1) ^ 0x8408 : ($crc >> 1);
	return $crc;
}


function demodulate_file($file)
{
	global $time;

	// reset time
	$time = $time_abs = 0;

	if(($handle = @fopen($file,'r'))===FALSE)
	exit("error: can't open '$file'\n\n");

	printf("Decoding file '%s'\n\n",$file);

	$header = NULL;
	$PCD_time = $PICC_time = 0;
	$PCD_state = $PICC_state = 0;

	 while (!feof($handle))
	{
		$line = trim(fgets($handle));
		if($line)
			$values = explode(',',$line);
		else
			continue;

		if(count($values)!=2)
			exit('Three comma seperated values needed ('.$i.'): "DeltaTime[in ns since last change],SignalEnvelope[logical 0 or 1]"');

		if($header)
		{
			// read new line of values
			list($delta_t,$PCD) = $values;

			// convert time to microseconds
			$delta_t = intval($delta_t) / 1000;
			$PCD_time += $delta_t;

			// maintain global absolute us counter
			$time_abs += $delta_t;
			$time = intval($time_abs);

			// decode PCD->PICC with individual delta time stamp
			if($PCD!=$PCD_state)
			{
				decode_PCD($PCD_time);
				$PCD_state = $PCD;
				$PCD_time = 0;
			}
		}
		else
			$header = $values;
	}
	fclose($handle);
}
?>
