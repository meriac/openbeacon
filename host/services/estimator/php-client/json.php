<?php
    $link = mysql_connect('localhost', 'opentracker', 'HYVbrwTP6JMuBL2w');
    if ($link && mysql_select_db('opentracker', $link))
    {	
	$result = mysql_query(' 
	    SELECT tag_id, x, y
    	    FROM distance
    	    WHERE TIMESTAMPDIFF(SECOND,last_seen,NOW())<5 
	');
	$tag = array();	    
	while ($row = mysql_fetch_assoc($result))
	    $tag[intval($row['tag_id'])]=array( 'x'=>$row['x'], 'y'=>$row['y'] );
	    
	$result = mysql_query(' 
	    SELECT tag_id, proximity_tag, tag_flags
    	    FROM proximity
    	    WHERE TIMESTAMPDIFF(SECOND,time,NOW())<30 
	');
	while ($row = mysql_fetch_assoc($result))
	    $tag[intval($row['tag_id'])]['proximity']=intval($row['proximity_tag']);
	
        asort($tag);	
	header('Content-type: text/plain');
//	header('Refresh: 1');
	echo '{"Tracker":'.json_encode($tag).'}';	
    }

?>
    