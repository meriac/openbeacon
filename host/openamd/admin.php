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


include('header.php');

if (!$_SESSION['admin']) {
  set_error("You fucking fail.<br>","index.php");
  die();
}

?><table><?php
  ?>
  <tr>
    <td>Lookup badge ID:</td>
    <td>
      <form action="pinlookup.php" method="post">
        <input name="tagid" type="text"/>
        <input type="submit" value="Search">
      </form>
    </td>
  </tr>

  <?php
?></table><?php

include('footer.php');
?>
