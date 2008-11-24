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


include "header.php" ?>

<script type="text/javascript" src="/javascripts/jquery.js"></script>
<script type="text/javascript">
 var optremoved=false;
 var remval;
 $().ready(function() {
     $("select[name='cdet']").hide();
     $("select[name='cond']").change(function(){
     var txt = $(this).val();
     var etxt = $("select[name='cond'] option:selected").text();
     var url = "stat_load.php?"+txt;
     if(optremoved) {
       $("select[name='stat'] > option[value='"+txt+"']").after(remval);
     }
     else {
       optremoved = true;
     }
     remval = $("select[name='stat'] > option[value='"+txt+"']");
     $("select[name='stat'] > option[value='"+txt+"']").remove();
     if (txt=='tm') {
     } else {
       $("select[name='cdet']").load(url,{},function(){});
       $("select[name='cdet']").show();
     }
       }).trigger('change');
     $("select[name='cdet']").change(function(){
     $("select[name='stat']").removeAttr("disabled");
       });
     $("select[name='stat']").change(function(){
     var cond = $("select[name='cond'] option:selected").val();
     var cdet = $("select[name='cdet'] option:selected").val();
     var stat = $("select[name='stat'] option:selected").val();
     var url = "stat_load.php?cond="+cond+"&cdet='"+cdet+"'&stat="+stat;
     alert(url);
     $("div#result").load(url,{},function(){});
       });
   });
</script>

<h1>Advanced Statistics</h1>

<h3> Conditional Data </h3>
<p>Conditioning Statistic</p>
<select name="cond">
    <option value="loc">Location</option>
    <option value="zone">Zone</option>
    <option value="its">Interest</option>
    <option value="tlk">Attended Talk</option>
    <option value="tm">Time</option>
    <option value="wbs">Websites</option>
</select>
<select name="cdet"></select>
<p>Desired Statistic</p>
<select name="stat" disabled="disabled">
    <option value="loc">Location</option>
    <option value="zone">Zone</option>
    <option value="its">Interest</option>
    <option value="tlk">Attended Talk</option>
    <option value="tm">Time</option>
    <option value="usr">Users</option>
</select>
<div id="result"></div>
<h3> Markov-Chain Prediction </h3>
Given user provides next likely location

<?php include "footer.php" ?>
