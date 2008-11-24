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


/* User permissions

We have three modes of security:

noob (0)
User (1)
Admin (2)

noob can do nothing, always gets redirected to the login page.
user can log in, view things, choose things like schedule
admin has god powers

*/

function user_level($id) {
    $query = "select Level from Security where ID='$id'";
    $result = mysql_query($query);
    if (mysql_num_rows($result) == 1) {
        return mysql_result($result, 0, "Level");
    }

}

$userlevel = user_level($id);

?>
