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


include('config.php');

function make_dialog($query,$field) {
 global $oracleconn;
  $result = oracle_query("getting query result", $oracleconn, $query);
  echo "<option>Choose Detail-</option>\n";
  if (sizeof($result) > 0) {
      foreach($result as $value) {
    echo "<option value=\"" . $value[$field] . "\">" . $value[$field] . "</option>\n";
    }
  }
}

if (isset($_REQUEST['loc'])) {
  $countries =
    "select distinct c.COUNTRY ".
    "from hopeamd.person p, Countries c ".
    "where c.ID = p.country";
  make_dialog($countries, "COUNTRY");

} 
else if (isset($_REQUEST['zone'])) {
  $zones = "select distinct area_id from snapshot_summary";
  make_dialog($zones,"AREA_ID");

} 
else if (isset($_REQUEST['its'])) {
  make_dialog("select interest_name from hopeamd.interests_list","INTEREST_NAME");

} 
else if (isset($_REQUEST['tlk'])) {
  make_dialog("select talk_title from hopeamd.talks","TALK_TITLE");
}

if(isset($_REQUEST['cond']) &&
   isset($_REQUEST['cdet']) && 
   isset($_REQUEST['stat'])) {
  if ($_REQUEST['stat']=='loc') {
    $pre = "select distinct c.country ";
    $tbls = "from Countries c";
  }
  if ($_REQUEST['stat']=='zone') {
    $pre = "select z.area_id , sum(z.normalized_time_in_area) ";
    $tbls = "from snapshot_summary z";
    $coda = "group by z.area_id order by 2 desc";
  }
  if ($_REQUEST['stat']=='its') {
    $pre = "select i.interest_name, count(i.interest_id) ";
    $tbls = "from interests_list i";
    $coda = "group by i.interest_name order by 2 desc";
  }
  if ($_REQUEST['stat']=='tlk') {
    $pre = "select t.talk_title,  count(distinct p.person_id) ";
    $tbls = "from talk_presence p, talks t";
    $coda = "group by t.talk_title order by 2 desc";
  }
  if ($_REQUEST['stat']=='usr') {
    $pre = "select u.handle from person n";
  }

  $tbls = $tbls.",person u";
  if ($_REQUEST['cond']=='loc') {
     $body = "where u.location = ".$_REQUEST['cdet'];
  }
  if ($_REQUEST['cond']=='zone') {
     $tbls = $tbls.",snapshot_summary z";
     $body = "where z.area_id = ".$_REQUEST['cdet'];
  }
  if ($_REQUEST['cond']=='its') {
     $tbls = $tbls.",interests_list i,interests ill";
     $body = "where u.id = ill.id and ill.interest = i.interest_id and i.interest_name = ".$_REQUEST['cdet'];
  }
  if ($_REQUEST['cond']=='tlk') {
     $tbls = $tbls.",talks t";
     $body = "where t.talk_title = ".$_REQUEST['cdet'];
  }
  echo "$query\n";
  $query = $pre." ".$tbls." ".$body." ".$coda;
  $result = oracle_query("getting query result", $oracleconn, $query);
    if (sizeof($result) > 0) {
      foreach($result as $value) {
        echo "<p>". $value["TALK_TITLE"] . "</p>\n";
    }
  }
}

?>
