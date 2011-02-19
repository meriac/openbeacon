<?php
function read_proto($file)
{
    $body = '';
    foreach(file($file) as $line)
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


$proto = read_proto('openbeacon.proto');
print_r($proto);
?>