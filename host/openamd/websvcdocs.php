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


include 'header.php' ?>

<div class="docs">
<h2> Accessing Tag Position Data</h2>

<p>The current tag position data can be accessed via a simple web service.  There are two ways to access this service.  The first service sends a dump of the current locations.  The second continues to send new snapshots of positions as long as you stay connected.  Both use the same output format and support the same query parameters.
</p><p>The address of the service is http://beta.openbeacon.de:8181/sputnik-webapp/dynamic/dumplocations.
</p><p>The following query parameters are supported:
</p>
<table border="0">

<tr>
<th>Parameter Name
</th><th>Value
</th></tr>
<tr>
<td>limit

</td><td>The maximum number of tags to be returned in each snapshot.  This can only be specified once.
</td></tr>
<tr>
<td>tagid
</td><td>A comma-separated list of tagids to return.  This can be specified multiple times.  All tagids specified are added to a single set of tags which will be used as a filter.  If you request a tagid which does not currently have a known location, you will not get a result for that tag.
</td></tr>
<tr>
<td>area
</td><td>A comma-separated list of areas to return.  This can be specified multiple times.  All tagids specified are added to a single set of tags which will be used as a filter.  If you request a tagid which does not currently have a known location, you will not get a result for that tag.
</td></tr>
<tr>
<td>json
</td><td>If set, results will be returned as a standard JSON array.
</td></tr>
<tr>
<td>scale
</td><td>A floating-point value by which all returned coordinates will be multiplied.  This may be useful if you have a platform which only supports a particular range
</td></tr>
<tr>
<td>asint
</td><td>Truncates the coordinates' fractional part.  When used in conjunction with scale, this may be useful if you have a platform which isn't happy parsing floating point values.  You can use scale to scale the space up to (for example) -10,000 to +10,000 and truncate the unnecessary fractional part.

</td></tr></table>
<p>Note that if you specify both tagid and area filters, a tag must match both filters in order to be returned.  For example, the query "dumplocations?tagid=2000&amp;area=Engressia" will return the location information of tag 2000 only if it is currently in Engressia.
</p><p>The output format is one line per tag.  Each line consists of several pipe-separated fields.  For the non-JSON format, these fields are:
</p>
<ul><li> Area ID (string)
</li><li> Tag ID (integer)
</li><li> x coordinate (float or int, depending on asint flag)
</li><li> y coordinate (float or int, depending on asint flag)
</li><li> z coordinate (float or int, depending on asint flag)
</li><li> flags (integer - 0 normally, 2 if tag button is being pressed)

</li></ul>
</p><p>Here's a sample use of the system via telnet.  Note that the server supports simple HTTP 0.9 syntax and does not require any HTTP headers to be sent or parsed.  (Though if you make an HTTP 1.1 request, of course, it will respond with HTTP 1.1 headers.)
</p>
<pre>
bash-3.2$ telnet beta.openbeacon.de 8181
Escape character is '^]'.
GET /sputnik-webapp/dynamic/dumplocations?area=Engressia&amp;limit=5
Engressia|2387|46.7369918823242|83.1452331542969|0
Engressia|2377|32.191349029541|50.0000152587891|0
Engressia|2371|48.3091506958008|94.9274520874023|0
Engressia|2344|50.0000076293945|50.0000076293945|0
Engressia|2301|124.9999771118164|50.0000267028809|0
</pre>
<p>Another example:
</p>
<pre>
bash-3.2$ curl 'http://beta.openbeacon.de:8181/sputnik-webapp/dynamic/dumplocations?tagid=2399&amp;scale=100&amp;asint'
Hopper|2399|7500|5000|0|0

</pre>

</div>

<?php include 'footer.php' ?>
