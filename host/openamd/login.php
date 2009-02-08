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

if (isset($_REQUEST['create'])) {
    if ($_REQUEST['create'] == '1') { // yes I know there are better ways but this works
        set_error("","create.php");
    }
    else {
        set_error("","login.php");
    }
}

if (isset($_SESSION['handle'])) {
    ?><h2>Profile: <?php echo $_SESSION['handle']; ?></h2>
    <a href="pingpong.php">View Ping Log</a>
<?php
    $edited = false;
    $query = "select person.*, providers.provider providername from Person left join providers on (person.provider = providers.id) where Handle='" . $_SESSION['handle'] ."'";

    $allrows = oracle_query("fetching person", $oracleconn, $query);
    $row = $allrows[0];
    $rows = sizeof($allrows);

    if ($rows) {
        if ($row["EDITED"]) {
            $edited = true;
        }
        ?>
        <center>
        <?php if (!$edited) { ?>
        <h3 style="color:#ff0000;"><u>Be careful and accurate:<br>you can only enter your interests once.</u></h3><br><?php
        } ?>
        <form action="update_details.php" method="post">
<?//        <table style="background-color:#111111; border: thin dotted #999999;"><tr> ?>
        <table>
        <!--<tr>
            <td>Change Avatar:</td>
            <td><a href="avatar-up.php"><img src="/avatars/<?= $_SESSION['id'] ?>.png"></a></td>
        </tr>-->
        <tr>
            <td><a href="change_password.php">Change Password</a></td>
            <td></td>
        </tr>
        <tr>
            <td>Handle (required):</td>
            <td><?= $_SESSION['handle']; ?></td>
        </tr>
        <tr>
            <td>ID:</td>
            <td><?= $_SESSION['id'];?></td></tr>
            <tr>
            <td>Email:</td>
            <td><?= $row["EMAIL"];?></td>
        </tr>
<!-- removing because germans are silly
       <tr>
            <td>Cell number (all numbers):</td>
            <td><?php if ($edited) {
                if ($row["PHONE"] != "0") { ?>
We got owned by the (rhymes-with-unease) and didn't even get a lessons learned|malicious software,network security
                    <input type="text" name="phone" onblur="this.style.borderColor='';" onfocus="this.style.borderColor='#3080f0';" id="phone" class="formz" value="<?php
                    echo ereg_replace("[^0-9A-Z]","",mcrypt_cbc(MCRYPT_BLOWFISH,
                    $_SESSION['id'], base64_decode($row["PHONE"]), MCRYPT_DECRYPT, '12345678'));
                    ?>"><?
                }
            }
            else if (isset($_SESSION['phone'])) { ?>
                <input type="text" name="phone" id="phone" value="<?= $_SESSION['phone'];?>"></td><?php
            }
            else { ?>
                <input type="text" name="phone" id="phone"><?php
            }?></td>
        </tr>
        <tr>
            <td>Provider:</td>
            <td><?php
            if ($edited) { ?>
                <select name="provider">
                            <?php
                $provquery = "select * from Providers";
                $provrows = oracle_query("fetching providers", $oracleconn, $provquery);
                foreach ($provrows as $provrow) {
                    ?><option value="<?= $provrow["ID"]; ?>"<?php
                    if ($provrow["ID"] == $row["PROVIDER"]) {
                        echo " selected";
                    }
                    echo ">" . $provrow["PROVIDER"] . "</option>\n";
                }
                 ?></select><?php
            }
            else { ?>
            <select name="provider">
            <?php
            $provquery = "select * from Providers";
            $provrows = oracle_query("fetching providers", $oracleconn, $provquery);
            foreach ($provrows as $provrow) {
            ?><option value="<?php echo $provrow["ID"]; ?>"<?php
                if (isset($_SESSION['provider']) && ($provrow["ID"] == $_SESSION['provider'])) {
                    echo " selected";
                }
                echo ">" . $provrow["PROVIDER"] . "</option>\n";
            }
            ?></select><?php
            }
            ?>
        </tr>
 and americans are smart -->
        <tr>
            <td>Reminders:</td>
            <td><input type="checkbox" name="reminder_email" id="reminder_email" class="formz"<?php
                if($row["REMINDER"] == 1|| $row["REMINDER"] == 3) {
                    echo " checked";
                }
                if (isset($_SESSION['reminder'])) {
                    if ($_SESSION['reminder'] == 1 || $_SESSION['reminder'] == 3) {
                        echo " checked";
                    }
                }
                ?>>Email<!--&nbsp;<input type="checkbox" name="reminder_phone" id="reminder_phone" class="formz"<?php
                if($row["REMINDER"] == 2|| $row["REMINDER"] == 3) {
                    echo " checked";
                }
                if (isset($_SESSION['reminder'])) {
                    if ($_SESSION['reminder'] == 2 || $_SESSION['reminder'] == 3) {
                        echo " checked";
                    }
                }?>>SMS--></td>
        </tr>
        <tr>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td><b>Your personal data</b></td>
        </tr>
        <tr>
            <td>Gender:</td>
            <td><?php if ($edited) {
                if ($row["SEX"] != "0") { ?>
                    <input type="radio" name="gender" value="1"<?php if ($row["SEX"] == 1) { echo " checked"; } ?>> Male &nbsp;&nbsp;<input type="radio" name="gender" value="2"<?php if($row["SEX"] == 2) { echo " checked"; } ?>> Female<?php
                }
            }
            else if (isset($_SESSION['gender'])) { ?>
            <input type="radio" name="gender" value="1"<?php if ($_SESSION['gender'] == '1') { echo " checked"; }?>> Male &nbsp;&nbsp;<input type="radio" name="gender" value="2"<?php if ($_SESSION['gender'] == '2') { echo " checked"; }?>> Female<?php
            }
            else { ?>
            <input type="radio" name="gender" value="1"> Male &nbsp;&nbsp;<input type="radio" name="gender" value="2"> Female<?php
            }?>
        </tr>
        <tr>
            <td>Age:</td>
            <td><?php if ($edited) {
                if ($row["AGE"] != "0") { ?>
                    <input type="text" name="age" onblur="this.style.borderColor='';" onfocus="this.style.borderColor='#3080f0';" size=4 value="<?php echo $row["AGE"]; ?>"><?php
                }
            }
            else if (isset($_SESSION['age'])) { ?>
                <input type="text" name="age" onblur="this.style.borderColor='';" onfocus="this.style.borderColor='#3080f0';" size=4 value="<?php echo $_SESSION['age']; ?>"><?php
            }
            else {?>
                <input type="text" name="age" onblur="this.style.borderColor='';" onfocus="this.style.borderColor='#3080f0';" size=4><?php
            }?></td>
        </tr>
        <tr>
            <td>Home Town (postal code):</td>
            <td><?php if ($edited) { ?>
                <input type="text" size="7" name="location" onblur="this.style.borderColor='';" onfocus="this.style.borderColor='#3080f0';" id="location" value="<?php echo $row["LOCATION"]; ?>"><?php
            }
            else if (isset($_SESSION['location'])) { ?>
                <input type="text" size="7" name="location" onblur="this.style.borderColor='';" onfocus="this.style.borderColor='#3080f0';" id="location" value="<?php $_SESSION['location']; ?>"><?php
            }
            else {?>
                <input type="text" size="7" onblur="this.style.borderColor='';" onfocus="this.style.borderColor='#3080f0';" name="location"><?php
            }?></td>
        </tr>
        <tr>
            <td>Country:</td>
            <td><select name="country">
                <?php
                $countries = "select id, country from Countries order by country asc";
                $countrieslist = oracle_query("get countries", $oracleconn, $countries);
                foreach($countrieslist as $country) { ?>
                    <option value="<?php echo $country["ID"]; ?>"<?php
                        if ($edited) {
                            if ($row["COUNTRY"] == $country["ID"]) {
                                echo " selected>";
                            }
                            else {
                                echo ">";
                            }
                        }
                        elseif (isset($_SESSION['country'])) {
                            if ($_SESSION['country'] == $country["ID"]) {
                                echo " selected>";
                            }
                            else {
                                echo ">";
                            }
                        }
                        else {
                            echo ">";
                        }
                        echo $country["COUNTRY"];
                    ?></option>
                <?php }
            ?></td>
        </tr>
        <tr>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td><b>Interests (choose up to 5):</b></td>
        </tr>

<?php
// Gets entire interests list
    $query = "select * from Interests_List";
    $allrows = oracle_query("fetching interests", $oracleconn, $query);
    $rows = sizeof($allrows);

    // Gets interests for user
    $user_query = "select Interest from Interests where ID='" . $_SESSION['id'] . "' order by Interest asc";
    $user_rows = oracle_query("fetching user interests", $oracleconn, $user_query);
    $num_user_rows = sizeof($user_rows);
    if ($rows) {
        $user_interest_count = 0;
        for ($i = 0; $i < $rows; $i++) {
            if (($i+1)%2) {
                echo '<tr>';
            }
            if ($allrows[$i]["INTEREST_NAME"]) {
                ?><td><input type="checkbox" name="<?php
                echo $allrows[$i]["INTEREST_ID"];
                ?>" id="<?php echo $allrows[$i]["INTEREST_ID"];?>" <?php
                if ($user_interest_count < $num_user_rows) {
                    if ($user_rows[$user_interest_count]["INTEREST"] ==
                        $allrows[$i]["INTEREST_ID"]) {
                         echo 'checked';
                        $user_interest_count++;
                    }
                }
                if ($edited && sizeof($user_rows)>0) {
                    echo " disabled";
                }
            ?>>&nbsp;&nbsp;<?php echo $allrows[$i]["INTEREST_NAME"];?></td>
            <?php
            }
            if ($i%2) {
                echo '</tr>';
            }
        }
    ?><tr>
    <td></td>
    <td><input type="submit" onmouseover="this.style.backgroundColor='3080f0';" onmouseout="this.style.backgroundColor = '';"></td></tr><?php
    }
    else {
        echo 'No rows returned.<br>' .
             '<a href="login.php">Back to main</a>';
    }
        ?></table>

<?
}
}

else {
?>

<div class="body-header">Log into 25c3 OpenAMD</div><br>
<h3>Have an account?<br><a href="create.php">If not, click here to create one.</a></h3><br>
<table>
    <form id="login" action="auth.php" method="post">
    <tr><td>Username:</td><td><input class="formz" type="text" name="user" length="40" onblur="this.style.borderColor='';" onfocus="this.style.borderColor='#3080f0';"></td></tr>
    <tr><td>Password:</td><td><input class="formz" type="password" name="password" length="40" onblur="this.style.borderColor='';" onfocus="this.style.borderColor='#3080f0';"></td></tr>
    <tr><td colspan=2 align=left><input type="submit" value="Login"  onmouseover="this.style.backgroundColor='#3080f0';" onmouseout="this.style.backgroundColor='';" class="formz" text="submit"></td></tr>
    </form>
</table>
<a href="forgot_password.php"><font color="red">Forgot your password?</font></a>
<?php 

}

include("footer.php"); ?>
