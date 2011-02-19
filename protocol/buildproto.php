#!/usr/bin/php
<?php

define('WIRETYPE_VARIANT',     0);
define('WIRETYPE_64BIT',       1);
define('WIRETYPE_LENGTH',      2);
define('WIRETYPE_GROUP_START', 3);
define('WIRETYPE_GROUP_END',   4);
define('WIRETYPE_32BIT',       5);

$wire_type = array (
    // Variant
    'int32'    => WIRETYPE_VARIANT,
    'int64'    => WIRETYPE_VARIANT,
    'uint32'   => WIRETYPE_VARIANT,
    'uint64'   => WIRETYPE_VARIANT,
    'sint32'   => WIRETYPE_VARIANT,
    'sint64'   => WIRETYPE_VARIANT,
    'bool'     => WIRETYPE_VARIANT,
    'enum'     => WIRETYPE_VARIANT,

    // 64-bit
    'fixed64'  => WIRETYPE_64BIT,
    'sfixed64' => WIRETYPE_64BIT,
    'double'   => WIRETYPE_64BIT,

    // Length
    'string'   => WIRETYPE_LENGTH,
    'bytes'    => WIRETYPE_LENGTH,

    // 32-bit
    'fixed32'  => WIRETYPE_32BIT,
    'sfixed32' => WIRETYPE_32BIT,
    'float'    => WIRETYPE_32BIT
);

function read_proto($file)
{
    $body = '';
    foreach(file($file.'.proto') as $line)
    {
	$line = preg_replace('/\/\/ .*$/','',$line);
	$line = preg_replace('/ [ ]+/',' ',$line);
	$line = trim($line);
	
	$body.= $line;
    }

    $proto = array();
    if(preg_match_all('/(?P<type>[a-z]+) (?P<name>[a-z]+) {(?P<content>[^}]+)}/i',$body,$matches))
    {
	foreach($matches['name'] as $id => $name)
	{
	    $type=$matches['type'][$id];
	    $items = array();
	    foreach(explode(';',trim($matches['content'][$id])) as $item)
		if(($item = trim($item))>'')
		{
		    if(preg_match('/(?P<mode>[a-z]+) (?P<type>[a-z]+[0-9]*) (?P<name>[^ ]+) = (?P<id>[0-9]+)/i',$item,$item_matches))
		    {
			foreach($item_matches as $id => $item)
			    if(!is_string($id))
				unset($item_matches[$id]);
			$id = intval($item_matches['id']);
			unset($item_matches['id']);
			$items[$id]=$item_matches;
		    }
		    else
			if(preg_match('/(?P<enum>[a-z0-9_]+) = (?P<id>[0-9]+)/i',$item,$item_matches))
			    $items[$item_matches['id']]=$item_matches['enum'];
			else
			    if(preg_match('/(?P<mode>extensions) (?P<start>[0-9]+) to (?P<end>[0-9]+)/i',$item,$item_matches))
			    {
				foreach($item_matches as $id => $item)
				    if(!is_string($id))
					unset($item_matches[$id]);
				$items[0]=$item_matches;
			    }
			    else
				halt("ERROR: $item\n");
		}
	    ksort($items);
	    $proto[$type][$name]=$items;
	}
    }
    ksort($proto);
    return $proto;
}

function dump_enum($type, $data)
{
    echo "\n// enumeration type $type declaration\n";
    echo "typedef enum {\n";
    foreach($data as $id => $name)
	echo "  $name = $id,\n";
    echo "} $type;\n";
}

function dump_message($type, $data)
{
    $define = strtoupper($type).'_';
    echo "\n// $type - field number declaration\n";
    foreach ($data as $id => $item)
	if($id>0)
	    echo '#define '.$define.strtoupper($item['name'])." $id\n";

    echo "\n// $type - struct declaration\n";
    echo "typedef struct {\n";
    echo "  uint32_t __active_fields;\n";
    foreach ($data as $id => $item)
	if($id>0)
	{
	    $type = $item['type'];

	    switch($mode=$item['mode'])
	    {
		case 'optional':
		    echo "  $type"."_t $item[name];\n";
		    break;
	    }
	}
    echo "} $type;\n";
}

$class = 'openbeacon';
$proto = read_proto($class);

print_r($proto);

$define = '__'.strtoupper($class).'_H__';
echo "#ifndef $define\n";
echo "#define $define\n";

if(isset($proto['enum']))
    foreach($proto['enum'] as $type => $data)
	dump_enum($type,$data);

if(isset($proto['message']))
    foreach($proto['message'] as $type => $data)
	dump_message($type,$data);

echo "\n#endif/*$define*/\n"

?>