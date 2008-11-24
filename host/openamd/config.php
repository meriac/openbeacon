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


// CHANGE THESE SETTINGS TO MATCH YOUR SYSTEM

// Oracle DB info
$username = '';
$password = '';
$host = ''; 
$webapp = ''; // no trailing slash at the end

// DO NOT EDIT BELOW THIS LINE


$oracleconn = oci_connect($username, $password, $host);
if (!$oracleconn) {
  $e = oci_error();
  die(htmlentities($e['message']));
  exit;
}

function oracle_query($what, $oracleconn, $query, $params = NULL, $commit = 1) {
  //var_dump($what);  var_dump($query);  var_dump($params); var_dump($commit);
  $statement = oci_parse($oracleconn, $query);
  if (!$statement) {
    $e = oci_error($oracleconn);
    die(htmlentities($what . ": failed to parse query: " . $query . ": " . $e['message'])); 
  }
  if (!is_null($params)) {
    foreach ($params as $key=>$value){
      if (!oci_bind_by_name($statement, $key, $value, -1, SQLT_CHR)) {
        $e = oci_error($statement);
        die(htmlentities($what . ": failed to bind value '" . $value . "' to parameter " . $key . " for query: " . $query . ": " . $e['message']));
      }
    }
  }
  if ($commit) {
    $result = oci_execute($statement, OCI_COMMIT_ON_SUCCESS);
  } else {
    $result = oci_execute($statement, OCI_DEFAULT);
  }
  if (!$result) {
    $e = oci_error($statement);
    die(htmlentities($what . ": failed to execute query: " . $query . ": " . $e['message']));
  }
  oci_fetch_all($statement, $allrows, 0, -1, OCI_ASSOC+OCI_FETCHSTATEMENT_BY_ROW);
  oci_free_statement($statement);
  return $allrows;
}

function get_user_location($userid) {
  //return array("location not available");
  $location = split("[|]",file_get_contents($webapp . "/dumplocations?tagid=$userid"));
  if (sizeof($location) > 1) {
    return $location;
  }
  else {
    return array("unknown location");
  }
}

function get_room_users($room) {
  return split("\n",file_get_contents($webapp . "dumplocations?area=$room"));
}

date_default_timezone_set('UTC');

?>
