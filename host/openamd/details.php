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


// Login page for OpenAMD project

include('header.php');

if ($_POST['login']) { // If user logged in (or tried to)

$name = $_POST['name'];
$pass = $_POST['password'];

?>
<center>You tried to log in with the following credentials:<br><br>
<?

    // It's aliiiiiiiive!

    $query = "select * from Person where ID='$name'";
    $result = mysql_query($query);
    $rows = mysql_num_rows($result);
    
    if ($rows) {
        ?>
        <center>
        <form action="update_details.php" method="post">
        <table>
        <tr>
            <td>Handle (required):</td>
            <td><input type="handle" name="handle" id="handle" value="<?php echo mysql_result($result, 0, 'Handle');?>"></td>
        </tr>
        <tr>
            <td>ID:</td>
            <td><input type="text" name="name" id="name" value="<?php echo mysql_result($result, 0, 'ID');?>" disabled></td></tr>
            <tr>
            <td>Email:</td>
            <td><input type="text" name="email" id="email" value="<?php echo mysql_result($result, 0, 'Email');?>"></td>
        </tr>
        <!-- removed because europeans are silly -->
	<tr>
            <td>Enter your cell <br>if you want to receive <br>texts during the games <br>and for reminders:</td>
            <td><input type="text" name="phone" id="phone" value="<?php echo mysql_result($result, 0, 'Phone');?>"></td>
        </tr>
	<!-- and americans are smart -->
        <tr>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td><b>Your personal data</b></td>
        </tr>
        <tr>
            <td>Gender:</td>
            <td><input type="radio" name="male"> Male &nbsp;&nbsp;<input type="radio" name="female"> Female
        </tr>
        <tr>
            <td>Age:</td>
            <td><input type="textbox" name="age" size=4></td>
        </tr>
        <tr>
            <td>Zip Code, if you're a US citizen:</td>
            <td><input type="textbox" name="zip"></td>
        </tr>
        <tr>
            <td>Home Town:</td>
            <td><input type="text" name="location" id="location" value="<?php echo mysql_result($result, 0, 'Location');?>"></td>
        </tr>
        <tr>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td><b>Your chosen interests</b></td>
        </tr>
        </table>
        </center>
    <?php
    }
    else {
        echo 'No rows returned.<br>' .
             '<a href="login.php">Back to main</a>';
    }

    // Gets entire interests list
    $query = "select * from Interests_List";
    $result = mysql_query($query);
    $rows = mysql_num_rows($result);

    // Gets interests for user
    $user_query = "select Interest from Interests where ID='$name' order by Interest asc";
    $user_result = mysql_query($user_query);
    $user_rows = mysql_num_rows($user_result);

    if ($rows) {
        ?><center><table><?php
        $user_interest_count = 0;
        for ($i = 0; $i < $rows; $i++) {
            if ($i%1) {
                echo '<tr>';
            }
            if (mysql_result($result, $i, 'Interest_Name')) {
                ?><td><input type="checkbox" name="<?php
                echo mysql_result($result, $i, 'Interest_ID');
                ?>" id="<?php echo mysql_result($result, $i, 'Interest_ID');?>" <?php
                if ($user_interest_count < $user_rows) {
                    if (mysql_result($user_result, $user_interest_count, 'Interest') ==
                        mysql_result($result, $i, 'Interest_ID')) {
                         echo ' checked';
                        $user_interest_count++;
                    }
                }
            ?>></td><td><?php echo mysql_result($result, $i, 'Interest_Name');?></td>
            <?php
            }
            if ($i%2) {
                echo '</tr>';
            }
        }
        ?></table></center>
        <input type="submit" value="Update Personal Data">
        </form>
<?php
    }
    else {
        echo 'User has no redeeming qualities.<br>';
    }
}

else { // Default page to display, no login
?>
<center>
<h1>OpenAMD Project v 0.1</h1>
<form action="details.php" method="post">
<input type="hidden" name="login" id="login" value="1">
<table>
    <tr>
        <td>Name/ID:</td>
        <td><input type="text" name="name" id="name"></td>
    </tr>
    <tr>
        <td>Password:</td>
        <td><input type="password" name="password" id="password"></td>
    </tr>
    <tr>
        <td><input type="submit" value="Log In"></td>
    </tr>
</table>
</center>

<?php
}
?>
