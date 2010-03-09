<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    if(isset($_GET['floor']))
    {
	$floor=intval($_GET['floor']);
	if($floor<1)
	    $floor=1;
	else
	    if($floor>3)
		$floor=3;
    }
    else
	$floor=1;

    $time_start = microtime(TRUE);

    $im = imagecreatefrompng($floor.'og768x546.png')
	or die('Cannot Initialize new GD image stream');

    $im_width = imagesx($im);
    $im_height = imagesy($im);

    $readers = array(
	1 => array (
	    array(208, 230, 172),
	    array(209, 375,  13),
	    array(210, 475,  13),
	    array(211, 505,  13),
	    array(212, 630,  13),
	    array(213, 685, 150),
	    array(214, 685, 290),
	    array(236, 230, 390),
	    array(241, 620, 290)
	),
	2 => array (
	    array(215, 730, 485),
	    array(216, 600, 150),
	    array(217, 365, 150),
	    array(218, 140, 100),
	    array(219, 140, 405),
	    array(220, 365, 410),
	    array(221, 600, 410)
	),
	3 => array (
	    array(222,  20, 208),
	    array(223, 750, 233),
	    array(225, 620, 145),
	    array(226, 390, 145),
	    array(227, 110, 142),
	    array(228, 111, 410),
	    array(229, 390, 410),
	    array(230, 620, 410)
	)
    );

    $background_color = imagecolorallocate($im, 200, 200, 200);
    $wall_color = imagecolorallocate($im, 215, 215, 215);
    $inactive_color = imagecolorallocate($im, 128, 128, 128);
    $text_color = imagecolorallocate($im,  64, 128, 32);
    $reader_color = imagecolorallocate($im, 255, 64, 32);
    $tag_color = imagecolorallocate($im, 32, 64, 255);

    foreach($readers[$floor] as $reader)
    {
	$x=$reader[1];
	$y=$reader[2];

	imagefilledrectangle($im, $x-3, $y-3, $x+3, $y+3, $reader_color);
	imagestring($im, 4, $x, $y+5, $reader[0], $inactive_color);
    }

    // timestamp
    $duration = ' ['.round((microtime(TRUE)-$time_start)*1000,1).'ms]';
    imagestring($im, 4, $im_width-330, $im_height-30, date(DATE_RSS).$duration, $inactive_color);

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
	    SELECT tag_id, proximity_tag
	    FROM proximity
	    WHERE TIMESTAMPDIFF(SECOND,time,NOW())<30 
	');

	while ($row = mysql_fetch_assoc($result))
	    $tag[intval($row['tag_id'])][proximity]=intval($row['proximity_tag']);


	foreach( $tag as $id => $position )
	{
	    $x = $position['x'];
	    $y = $position['y'];
	    imagefilledellipse($im, $x, $y, 6, 6, $tag_color);
	    imagestring($im, 4, $x,$y+6, $id, $text_color);
	    if(isset($position['proximity'])&&(($prox = intval($position['proximity']))!=0))
	    {
		$x2 = $tag[$prox]['x'];
		$y2 = $tag[$prox]['y'];
		imageline($im,$x,$y,$x2,$y2,$inactive_color);
	    }
	}
    }

    header('Content-type: image/png');
    header('Refresh: 1');
    imagepng($im);
    imagedestroy($im);
?>
