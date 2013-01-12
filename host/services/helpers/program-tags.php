#!/usr/bin/php
<?php
/***************************************************************
 *
 * OpenBeacon.org - automated QR code based wireless tag ID
 *                  assignment
 *
 * uses cu and zbarcam+webcam
 *
 * Copyright 2012 Milosch Meriac <meriac@bitmanufaktur.de>
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

error_reporting(E_ALL);

define('PROGRAM_CU','/usr/bin/cu -l /dev/ttyACM0 -s 115200');
define('PROGRAM_ZBARCAM','/usr/bin/zbarcam');
define('QR_PREFIX','QR-Code:http://qr.tk/');
define('PROGRAM_TAG_ID',151);
define('TIMEOUT', 5);

$qr_seen = NULL;
$tag_seen = array();
$tag_wait = NULL;

function process_usb_line ($line)
{
	global $tag_seen, $tag_wait;

	if(($line = trim($line))=='')
		return 0;

	if(preg_match('|^[PR]: (?<tag_id>[0-9]+)={(?<strength>[0-3]),0x[0-9A-F]+}$|',$line,$matches))
	{
		$strength = intval($matches['strength']);
		$tag_id = intval($matches['tag_id']);

		/* remember lowest strength level */
		if(!isset($tag_seen[$tag_id]))
		{
			$tag_seen[$tag_id] = $strength;

			if( $tag_id == $tag_wait )
			{
				$tag_wait = NULL;
				echo chr(7)."USB: Updated tag ID to $tag_id\n";
			}
			else
				if($tag_id!=PROGRAM_TAG_ID)
					echo "USB: new tag ID $tag_id\n";
		}
		else
			if($strength<$tag_seen[$tag_id])
				$tag_seen[$tag_id] = $strength;
	}
	else
		if(preg_match('|^BUTTON:|',$line))
			echo("USB: $line\n");
}

$spec = array(
	0 => array('pipe','r'),
	1 => array('pipe','w'),
	2 => array('pipe','w')
);

if(($usb = @proc_open(PROGRAM_CU,$spec,$cu))===FALSE)
{
	exit('Can\'t run USB sniffer progran at \''.PROGRAM_CU."'\n");
}
/* non-blocking read from OpenBeacon USB */
stream_set_blocking($cu[1],0);
/* empty write */
fwrite($cu[0], "\n\n");
/* flush read buffers */
sleep(1);
while(fread($cu[1], 4096));

/* set target tag id */
fwrite($cu[0],'R '.PROGRAM_TAG_ID."\n");

$buffer = '';
$zbarcam = NULL;
$cam = array();
$timeout = time();

while (proc_get_status($usb)['running'])
{
	if(((time()-$timeout)>TIMEOUT) || !$zbarcam || !proc_get_status($zbarcam)['running'])
	{
		foreach($cam as $handle)
			fclose($handle);

		if($zbarcam)
		{
			proc_terminate($zbarcam);
			proc_close($zbarcam);
		}

		if(($zbarcam = @proc_open(PROGRAM_ZBARCAM,$spec,$cam))===FALSE)
		{
			echo 'Can\'t find QR code scanner program at \''.PROGRAM_ZBARCAM."'\n";
			break;
		}
		/* non-blocking read from camera QR code scanner */
		stream_set_blocking($cam[1],0);

		/* re-trigger timeout */
		$timeout = time();
	}

	$buffer .= fread($cu[1], 4096);
	$lines = explode("\n",$buffer);
	switch($count = count($lines))
	{
		case 0:
			break;

		case 1:
			process_usb_line($lines[0]);
			$buffer = '';
			break;

		default:
			$count--;
			for($i=0;$i<$count;$i++)
				process_usb_line($lines[$i]);
			$buffer = $lines[$count];
	}

	if($line = fgets($cam[1]))
	{
		if(preg_match('|^'.QR_PREFIX.'(?<QR>[0-9]+)$|',$line,$matches))
		{
			$qr_seen = intval($matches['QR']);
			echo "QR: seen tag ID $qr_seen\n";

			/* re-trigger timeout */
			$timeout = time();
		}
	}

	if(isset($tag_seen[PROGRAM_TAG_ID]))
	{
		unset($tag_seen[PROGRAM_TAG_ID]);
		if($qr_seen)
		{
			if(isset($tag_seen[$qr_seen]))
				echo "QR: scanned old tag ID $qr_seen\n";
			else
			{
				echo "QR: writing new tag ID $qr_seen\n";
				$tag_wait = $qr_seen;
				fwrite($cu[0],"TAG $qr_seen\n");
			}

			$qr_seen = NULL;
		}
	}

	usleep(200*1000);
}

@proc_close($zbarcam);
@proc_close($usb);

echo "\nTerminated gracefully.\n\n";
?>
