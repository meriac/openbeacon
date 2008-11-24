<html>
    <body>

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
    
    if ($_REQUEST["sent"] == "reset") {
        if (isset($_REQUEST['id']) && preg_match("/^[0-9]{4}$/",$_REQUEST['id'])) {
            if ($_REQUEST['password'] == $_REQUEST['password2']) {
                $id = $_REQUEST['id'];
                $password = hash('sha256',$_REQUEST['password']);
                $query = "update Person set Password = '$password' where ID=$id";
                oracle_query("resetting password",$oracleconn,$query);
                set_error("Password has been changed.","passwordreset.php");
            }
            else {
                set_error("Passwords need to be the same.", "passwordreset.php");
            }
        }
    }
?>
        <div style="width: 300px; margin: auto;">
            <form action="">
                <input type="hidden" value="reset" name="sent">
                <fieldset>
                    <legend>ID:</legend>
                    <input id="id" type="text" name="id">
                </fieldset>
                <fieldset>
                    <legend>Password:</legend>
                    <input id="password" type="password" name="password">
                </fieldset>
                <fieldset>
                    <legend>Password again:</legend>
                    <input id="password2" type="password" name="password2">
                </fieldset>
                <input type="submit" value="Reset" name="Reset">
            </form>
        </div>

    </body>
</html>

<?php

include('footer.php');

?>
