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

<script language="javascript" type="text/javascript">

function show(div, data) {
    document.getElementById(div).innerHTML = data;
}

function ajax(to,postData)
    {
    var xmlHttp=window.XMLHttpRequest?new XMLHttpRequest():new ActiveXObject('MSXML2.XMLHTTP.3.0');
    xmlHttp.open(postData?'POST':'GET',to,true);
    xmlHttp.onreadystatechange=function(){
        if(xmlHttp&&xmlHttp.readyState==4){
            show('main', contentxmlHttp.responseText);
    xmlHttp.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
    xmlHttp.setRequestHeader("Content-length", postData.length);
    xmlHttp.send(postData);
    }


</script>

<div style="border-color: darkgreen; border-style: dotted; padding: 10px;">

    <label for="int">Interest:</label>
    <select id="int" name="int">
    </select>

    <label for="loc">Location:</label>
    <select id="loc" name="loc">
    </select>

    <label for="zipcode">Zipcode:</label>
    <select id="zipcode" name="zipcode">
    </select>

    <label for="sex">Sex:</label>
    <select id="sex" name="sex">
    </select>

    <input type="button" value="Search">
</div>

<br/ ><br />

<div id="main" style="border-color: darkgreen; border-style: dotted; padding: 10px;">
    <p>Please select the filters above to narrow your search.</p>
</div>

<?php include 'footer.php'; ?>
