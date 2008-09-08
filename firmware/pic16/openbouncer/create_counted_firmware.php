#!/usr/bin/php
<?php

/***************************************************************
 *
 * OpenBeacon.org - firmware serialization
 *
 * Copyright 2006 Milosch Meriac <meriac@openbeacon.de>
 *
 * This PHP shell scrip takes precompiled intel hex file, increments
 * id, stores random seed and XXTEA key and creates output
 * hexfile for PicKit2 programmer software.
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

define('PIC16_DATA_OPCODE',0x34);
define('INPUT_FILE','obj/openbeacontag.hex');
define('OUTPUT_FILE','openbeacontag.hex');
define('COUNT_FILE','firmware_counter');
define('SYMBOLS_FILE','obj/openbeacontag.sym');
define('SYMBOLS_SEG','CONST');

//
// Actual TEA encryption key of the tag
//
$tea_key = array(0x00112233,0x44556677,0x8899AABB,0xCCDDEEFF);

// Patches list: symbol, relative offset, data
$patch_list = array(
	'_oid' => (is_readable(COUNT_FILE))?intval(trim(file_get_contents(COUNT_FILE))):2000,
	'_seed' => hexdec(substr(md5(microtime().implode('-',$tea_key).rand()),0,8)),
	'_tea_key' => $tea_key,
    );

// Lookup actual symbol offsets from symbols file 
$patches=patch_lookup_patches(SYMBOLS_FILE,$patch_list);
// Read HEX file from file
patch_hexread(INPUT_FILE);
// apply patch first time
patch_apply($patches,TRUE);
// keep spinning while incrementing counter
while(TRUE)
{
    echo "\nOpenBeacon tag ID set to '$patch_list[_oid]'\n";
    patch_hexwrite(OUTPUT_FILE);
    
    // Increment tag ID
    $patch_list['_oid']++;
    file_put_contents(COUNT_FILE,$patch_list['_oid']);
    
    // Lookup actual symbol offsets from symbols file ...
    $patches=patch_lookup_patches(SYMBOLS_FILE,$patch_list);    
    // ... and apply them
    patch_apply($patches,FALSE);

    echo 'press [ENTER] to continue';
    fgets(STDIN);
}



//
// Helper functions
//

function patch_lookup_patches($file,$patch_list)
{
	$patches = array();
		
	if(!is_readable($file))
		exit("Can't read symbol lookup file '$file'\n");
	else
		foreach(file($file) as $symbol)
		{
			$symbol = explode(' ',trim($symbol));
		
			$name = $symbol[0];
		
			if($name && array_key_exists($name,$patch_list))
			{
			if($symbol[3]!=SYMBOLS_SEG)
				exit($name.' must be in '.SYMBOLS_SEG." segment - invalid segment $symbol[3] sepecified\n");
		
			$offset = hexdec($symbol[1])*2;		
			if($offset)	
			{
				$patch = $patch_list[$name];
				if(is_array($patch))
				foreach($patch as $content)
				{
					$patches[$offset]=$content;
					$offset+=8;
				}
				else
				$patches[$offset]=$patch;		
			}
			}
		}
	return $patches;
}

function patch_hexread($file)
{
	global $hexfile,$memory;
	
	$hexfile=array();
	$memory=array();

	if(!is_readable($file))
		exit("Can't read HEX input file '$file'\n");
	else
		foreach(file($file) as $row=>$line)
		{
			if(!preg_match('/^:(..)(....)(..)(.*)(..)$/',$line,$matches))
			exit("error at line($row)\n");
			
			$rec=array();
			foreach(array( 1=>'count', 2=>'address', 3=>'type', 5=>'checksum') as $offset=>$name )
				$rec[$name]=hexdec($matches[$offset]);
			
			$hexfile[]=$rec;
			
			if($rec['type']>0)
				break;
				
			$address=$rec['address'];
			foreach(explode(',',trim(chunk_split($matches[4],2,','),',')) as $byte)
				$memory[$address++]=hexdec($byte);
		}
}

function patch_apply($patches,$first=FALSE)
{
	global $memory;
	
	foreach($patches as $address=>$data)
	{
		for($i=0;$i<4;$i++)
		{
			if($first && (!isset($memory[$address]) || ($memory[$address]!=0xFF)))
				exit(sprintf("expecting data set to 0xFF at 0x%04X\n",$address));
			
			if($memory[$address+1]!=PIC16_DATA_OPCODE)
				exit(sprintf("expecting PIC16 data opcode at 0x%04X\n",$address+1));
			
			$memory[$address]=$data&0xFF;	
			$data>>=8;
			$address+=2;
		}
	}
}

function patch_hexwrite($file)
{
	global $hexfile,$memory;
		
	$handle=fopen($file,'w');
	if(!$handle)
		exit("Can't open HEX output file '$file'\n");
	else
	{
		foreach($hexfile as $rec)
		{
			$address=$rec['address'];    
			$line=sprintf('%02X%04X%02X',$rec['count'],$address,$rec['type']);
		
			for($i=0;$i<$rec['count'];$i++)
			$line.=sprintf('%02X',$memory[$address++]);
		
			$crc=0;
			foreach(explode(',',chunk_split($line,2,',')) as $byte)
			$crc+=hexdec($byte);
		
			fwrite($handle,sprintf(":%s%02X\n",$line,(0x100-$crc)&0xFF));
		}
		fclose($handle);
	}
}

?>