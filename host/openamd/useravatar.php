<?
echo "<script>alert('test');</script>";

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


session_start();

$udir = '/var/www/hopeamd/avatars/';
$ufile = $udir . $_SESSION['id'].'.png';
$ftype = $_FILES['picture']['tmp_name'];

echo "<script>alert('test');</script>";

move_uploaded_file($_FILES['picture']['tmp_name'], $ufile);
//echo "<script>window.parent.document.getElementById('avatar').src = '/avatars/$_SESSION['id'].png';</script>";
/*
if (($ftype == 'images/gif') || ($ftype == 'images/jpeg') || ($ftype == 'images/png') && ($_FILES['picture']['size'] < 10000)) {
    echo "<script>alert('test');</script>";
move_uploaded_file($_FILES['picture']['tmp_name'], $ufile);
    echo "<script>window.parent.document.getElementById('avatar').src = '/avatars/$_SESSION['id'].png';</script>";
} else {
    echo "<script>alert('diff. test');</script>";
    echo 'images must be either a jpeg, gif, or png and be less than 10000 kb';
} or die ('asdfasdf');
*/
?>
