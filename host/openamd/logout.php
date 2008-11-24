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

// Log user out
if (isset($_SESSION['handle'])) {
    unset($_SESSION['handle']);
    unset($_SESSION['id']);

    if (isset($_SESSION['rfid'])) {
        unset($_SESSION['rfid']);
    }
    
    unset($_SESSION['phone']);
    unset($_SESSION['provider']);
    unset($_SESSION['sex']);
    unset($_SESSION['age']);
    unset($_SESSION['reminder']);
    unset($_SESSION['location']);
    unset($_SESSION['country']);
    unset($_SESSION['admin']);
    set_error("You have been logged out.<br>","index.php");
}
else {
    foreach($var as $_SESSION) { echo $var; }
    unset($_SESSION['handle']);
    unset($_SESSION['id']);
    set_error("You are not logged in...","index.php");
}


?>
