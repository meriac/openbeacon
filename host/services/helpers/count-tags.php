#!/usr/bin/php
<?php
/***************************************************************
 *
 * OpenBeacon.org - count QR code equipped tags + create lists
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

declare(ticks = 1);
error_reporting(E_ALL);

define('PROGRAM_ZBARCAM','/usr/bin/zbarcam');
define('PROGRAM_SPEECH','/usr/bin/espeak -s 200 -v male3 ');
define('QR_URL','http://qr.tk/');
define('QR_PREFIX','QR-Code:'.QR_URL);
define('TIMEOUT', 5);

$tag = array();
$zbarcam = NULL;

pcntl_signal(SIGINT, 'signal_handler');

function signal_handler($signal)
{
	global $tag;
	global $zbarcam;

	if($signal!=SIGINT)
		return;

	proc_close($zbarcam);

	fprintf(STDERR, "\nTerminated gracefully.\n\n");

	if(ksort($tag))
		foreach($tag as $id => $enabled)
		{
			$id = sprintf('%04u',$id);
			$id_bin = hexdec(bin2hex(base64_decode($id)));
			echo $id_bin.','.QR_URL."$id\n";
		}

	exit();
}

$spec = array(
	0 => array('pipe','r'),
	1 => array('pipe','w'),
	2 => array('pipe','w')
);
$cam = array();
$timeout = time();

while (TRUE)
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
			fprintf(STDERR, 'Can\'t find QR code scanner program at \''.PROGRAM_ZBARCAM."'\n");
			break;
		}
		/* non-blocking read from camera QR code scanner */
		stream_set_blocking($cam[1],0);

		/* re-trigger timeout */
		$timeout = time();
	}

	if($line = fgets($cam[1]))
	{
		if(preg_match('|^'.QR_PREFIX.'(?<QR>....)$|',$line,$matches))
		{
			$qr_seen = intval($matches['QR']);

			if(!isset($tag[$qr_seen]))
			{
				/* remember tag */
				$tag[$qr_seen] = TRUE;
				$count = count($tag);
				fprintf(STDERR, "QR: seen tag[%04u]=%04u\n",$count,$qr_seen);

				exec(PROGRAM_SPEECH.$count);
			}

			/* re-trigger timeout */
			$timeout = time();
		}
	}

	usleep(100*1000);
}

?>
