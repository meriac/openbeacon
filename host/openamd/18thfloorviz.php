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


include("header.php"); ?>

<script type="text/javascript" src="javascripts/jquery.js"></script>
<script type="text/javascript" src="javascripts/jqModal.js"></script>
<script type="text/javascript" src="javascripts/jqDnR.js"></script>
<script type="text/javascript" src="javascripts/jquery-easing.js"></script>

<script type="text/javascript">
<!--
function MM_swapImgRestore() { //v3.0
  var i,x,a=document.MM_sr; for(i=0;a&&i<a.length&&(x=a[i])&&x.oSrc;i++) x.src=x.oSrc;
}
function MM_preloadImages() { //v3.0
  var d=document; if(d.images){ if(!d.MM_p) d.MM_p=new Array();
    var i,j=d.MM_p.length,a=MM_preloadImages.arguments; for(i=0; i<a.length; i++)
    if (a[i].indexOf("#")!=0){ d.MM_p[j]=new Image; d.MM_p[j++].src=a[i];}}
}

function MM_findObj(n, d) { //v4.01
  var p,i,x;  if(!d) d=document; if((p=n.indexOf("?"))>0&&parent.frames.length) {
    d=parent.frames[n.substring(p+1)].document; n=n.substring(0,p);}
  if(!(x=d[n])&&d.all) x=d.all[n]; for (i=0;!x&&i<d.forms.length;i++) x=d.forms[i][n];
  for(i=0;!x&&d.layers&&i<d.layers.length;i++) x=MM_findObj(n,d.layers[i].document);
  if(!x && d.getElementById) x=d.getElementById(n); return x;
}

function MM_swapImage() { //v3.0
  var i,j=0,x,a=MM_swapImage.arguments; document.MM_sr=new Array; for(i=0;i<(a.length-2);i+=3)
   if ((x=MM_findObj(a[i]))!=null){document.MM_sr[j++]=x; if(!x.oSrc) x.oSrc=x.src; x.src=a[i+2];}
}

$().ready(function() {
    $("#room_details")
    .jqDrag('.jqDrag')
    .jqResize('.jqResize')
    .jqm({
        overlay: 0
    });
});
function showDetails(datasource) {
    var url="room_details.php?room=" + datasource;
    $("#room_details_content").load(url, {}, function() {
        $("#room_details").jqmShow();
    });
}

//-->
</script>

<div style="position: absolute; margin: 0px 0 0 100px;">
<div id="room_details" class="jqmNotice">
    <div id="room_details_title" class="jqmnTitle jqDrag"><b>Room Detail</b></div>
    <div id="room_details_content" class="jqmnContent" style="overflow: scroll; height: 400px">
    </div>
    <img src="notice/close_icon.png" alt="close" class="jqmClose" />
</div>
</div>

<h2>Visualize: 18th Floor </h2>
<br>
<table><tr><td valign="top" width=110>
<a href="mezzanineviz.php">2nd Floor (Mezzanine)<br>
<img src="/images/mezzanine_thumb.png" border=0></a>
<br><br>
<a href="18thfloorviz.php">18th Floor<br>
<img src="/images/18thfloor_thumb.png" border=0></a>
<Br><br>
</td><td>
<p>Click on a room below to see the current talk and which users with RFID badges are currently present.</p>
<br>
<table id="Table_01" width="648" height="601" border="0" cellpadding="0" cellspacing="0">
    <tr>
        <td colspan="2" rowspan="3">
    <a href="#" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('engressia','','images/engressia_over.png',1)" onClick="showDetails('Engressia')"><img src="images/engressia.png" alt="Engressia" name="engressia" width="163" height="153" border="0"></a></td>
  <td colspan="6">
            <img src="images/18thfloor_02.png" width="484" height="28" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="1" height="28" alt=""></td>
    </tr>
    <tr>
        <td colspan="3" rowspan="3">
            <img src="images/18thfloor_03.png" width="254" height="251" alt=""></td>
        <td colspan="2">
            <a href="#" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('Zuse','','images/zuse_over.png',1)" onClick="showDetails('Zuse')"><img src="images/zuse.png" alt="Zuse" name="Zuse" width="198" height="114" border="0"></a></td>
  <td rowspan="5">
            <img src="images/18thfloor_05.png" width="32" height="572" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="1" height="114" alt=""></td>
    </tr>
    <tr>
        <td colspan="2" rowspan="3">
            <img src="images/18thfloor_06.png" width="198" height="182" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="1" height="11" alt=""></td>
    </tr>
    <tr>
        <td colspan="2">
            <img src="images/18thfloor_07.png" width="163" height="126" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="1" height="126" alt=""></td>
    </tr>
    <tr>
        <td rowspan="2">
            <img src="images/18thfloor_08.png" width="1" height="321" alt=""></td>
        <td colspan="2" rowspan="2">
            <a href="#" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('Hopper','','images/hopper_over.png',1)"  onClick="showDetails('Hopper')"><img src="images/hopper.png" alt="Hopper" name="Hopper" width="163" height="321" border="0"></a></td>
  <td colspan="2">
            <img src="images/18thfloor_10.png" width="253" height="45" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="1" height="45" alt=""></td>
    </tr>
    <tr>
        <td>
            <img src="images/18thfloor_11.png" width="115" height="276" alt=""></td>
        <td colspan="2">
    <a href="#" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('Turing','','images/turing_over.png',1)"  onClick="showDetails('Turing')"><img src="images/turing.png" alt="Turing" name="Turing" width="179" height="276" border="0"></a></td>
  <td>
            <img src="images/18thfloor_13.png" width="157" height="276" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="1" height="276" alt=""></td>
    </tr>
    <tr>
        <td>
            <img src="images/spacer.gif" width="1" height="1" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="162" height="1" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="1" height="1" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="115" height="1" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="138" height="1" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="41" height="1" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="157" height="1" alt=""></td>
        <td>
            <img src="images/spacer.gif" width="32" height="1" alt=""></td>
        <td></td>
    </tr>
</table>
<!-- End ImageReady Slices -->
<br>

</td></tr></table>


<?php include("footer.php"); ?>
