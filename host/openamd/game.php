<?php

/***************************************************************
 *
 * amd.hope.net - index.php
 *
 * Copyright 2008 The OpenAMD Project <contribute@openamd.org>
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


include("header.php");
if (!isset($_SESSION['rfid'])) {
    set_error("You must have an RFID badge to access this page.","index.php");
}
?>
<h2>Play: <?php echo $_SESSION['handle']; ?></h2>
<br>
<!--
<table>
<tr><td>
<a href="lifeclock.php"><img src="lifeclock_banner.png" border=0></a>
</td></tr><tr>
<td>Race against the clock to gain points and win prizes!<br><br><Br><br>
</td></tr>
<tr><td><a href="herd.php"><img src="herd.png" border=0></a></td></tr><tr><td>Follow the crowd in a timed riddle game; team up with others to solve the puzzle. <br><br></td></tr></table>
-->

<p>Sorry, but the game is not yet ready.  It should be working soon!</p>

<?php include("footer.php"); ?>
