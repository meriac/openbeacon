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


include 'header.php'; ?>

<iframe id='jaxuploader' name='jaxuploader' style="display: none;">
</iframe>

<div style="border-color: darkgreen; border-style: dotted; width: 500px; margin: auto; padding: 20px; text-align: center;">
    <b>Update your Avatar:</b><br />
    <img id='avatar' src="/avatars/<?= $_SESSION['id'] ?>.png"><br />
    <br />
    <form method=post action="useravatar.php" target="jaxuploader" enctype="multipart/form-data">
        <input type=file name=picture><br />
        <br /><input type=submit>
    </form>
</div>

<?php include 'footer.php'; ?>
