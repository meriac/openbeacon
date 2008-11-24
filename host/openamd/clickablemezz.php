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

<script type="text/javascript" src="/javascripts/jquery.js"></script>
<script src="clickablemezz.js" type="application/javascript"></script>
<script type="text/javascript" src="/javascripts/jqModal.js"></script>
<script type="text/javascript" src="/javascripts/jqDnR.js"></script>
<script type="text/javascript" src="/javascripts/jquery-easing.js"></script>
<script type="text/javascript">
$().ready(function() {
  $("#tag_details")
    .jqDrag('.jqDrag')
    .jqResize('.jqResize')
    .jqm({
      overlay: 0
     //,onShow: function(h) {
     //     h.w.css('opacity',0.92).slideDown();
     //   }
     //,onHide: function(h) {
     //     h.w.slideUp("slow",function() { if(h.o) h.o.remove(); });
     //   }
      });
});
function showDetails(datasource) {
  var url = "tag_details.php?tagid=" + datasource;
  $("#tag_details_content").load(url, {}, function() { $("#tag_details").jqmShow(); });
}
</script>
<div style="position: absolute; margin: 0px 0 0 100px;">
<div id="tag_details" class="jqmNotice">
  <div id="tag_details_title" class="jqmnTitle jqDrag">Tag Detail</div>
  <div id="tag_details_content" class="jqmnContent">
  </div>
  <img src="notice/close_icon.png" alt="close" class="jqmClose" />
  <!--<img src="dialog/resize.gif" alt="resize" class="jqResize" />-->
</div>
</div>

<style type="text/css">
.flag {
    color: blue;
}
.tag {
    position: absolute;
    cursor: pointer;
}
#map {
    color: #00a300;
    position: relative;
    margin: 0; padding: 0;
    height: 607px;
}
</style>

<a href="mezzanineviz.php">Default version</a><br />
<a href="mezzanineheat.php">Heat map version</a><br />
<br />
<h2>Visualize: Mezzanine (2nd floor)</h2>
<br>
<table><tr><td valign="top" width=110>
<a href="mezzanineviz.php">2nd Floor (Mezzanine)<br> 
<img src="/images/mezzanine_thumb.png" border=0></a>
<br><br>
<a href="18thfloorviz.php">18th Floor<br>
<img src="/images/18thfloor_thumb.png" border=0></a>
<Br><br>
</td><td valign="top">

<div id="map">
<img src="mezzanine.png" />
</div>

</td></tr></table>

<?php include("footer.php"); ?>
